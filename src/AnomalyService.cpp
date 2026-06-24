#include "AnomalyService.h"

#include <cmath>
#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <vector>

struct TemperaturePoint {
    int observationId;
    std::string observationDate;
    double temperature;
};

AnomalyService::AnomalyService(Database& db) : db(db) {}

void AnomalyService::clearOldAnomalies(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate
) {
    pqxx::work tx(db.getConnection());

    tx.exec_params(
        "DELETE FROM anomalies a "
        "USING weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE a.observation_id = o.id "
        "AND c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "AND a.parameter_name = 'temperature'",
        cityName,
        startDate,
        endDate
    );

    tx.commit();
}

void AnomalyService::findTemperatureAnomaliesByRegression(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate
) {
    clearOldAnomalies(cityName, startDate, endDate);

    std::vector<TemperaturePoint> points;

    {
        pqxx::work tx(db.getConnection());

        pqxx::result res = tx.exec_params(
            "SELECT "
            "MIN(o.id) AS observation_id, "
            "o.observation_time::date AS observation_date, "
            "AVG(o.temperature) AS avg_temperature "
            "FROM weather_observations o "
            "JOIN weather_stations s ON s.id = o.station_id "
            "JOIN cities c ON c.id = s.city_id "
            "WHERE c.name = $1 "
            "AND o.observation_time::date BETWEEN $2 AND $3 "
            "AND o.temperature IS NOT NULL "
            "GROUP BY o.observation_time::date "
            "ORDER BY o.observation_time::date",
            cityName,
            startDate,
            endDate
        );

        tx.commit();

        for (const auto& row : res) {
            TemperaturePoint point;

            point.observationId = row["observation_id"].as<int>();
            point.observationDate = row["observation_date"].as<std::string>();
            point.temperature = row["avg_temperature"].as<double>();

            points.push_back(point);
        }
    }

    if (points.size() < 3) {
        std::cout << "\nNot enough data for anomaly search." << std::endl;
        return;
    }

    double sumX = 0.0;
    double sumY = 0.0;
    double sumXY = 0.0;
    double sumXX = 0.0;

    for (size_t i = 0; i < points.size(); i++) {
        double x = static_cast<double>(i);
        double y = points[i].temperature;

        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }

    double n = static_cast<double>(points.size());
    double denominator = n * sumXX - sumX * sumX;

    if (denominator == 0.0) {
        std::cout << "\nRegression error." << std::endl;
        return;
    }

    double a = (n * sumXY - sumX * sumY) / denominator;
    double b = (sumY - a * sumX) / n;

    std::vector<double> predictedValues;
    std::vector<double> deviations;

    double sumSquaredDeviation = 0.0;

    for (size_t i = 0; i < points.size(); i++) {
        double x = static_cast<double>(i);
        double predicted = a * x + b;
        double deviation = points[i].temperature - predicted;

        predictedValues.push_back(predicted);
        deviations.push_back(deviation);

        sumSquaredDeviation += deviation * deviation;
    }

    double standardDeviation = std::sqrt(sumSquaredDeviation / n);
    double limit = 2.5 * standardDeviation;

    int anomalyCount = 0;

    pqxx::work tx(db.getConnection());

    for (size_t i = 0; i < points.size(); i++) {
        double deviation = deviations[i];

        if (std::abs(deviation) > limit) {
            std::string anomalyType;

            if (deviation > 0) {
                anomalyType = "HIGH_TEMPERATURE_ANOMALY";
            } else {
                anomalyType = "LOW_TEMPERATURE_ANOMALY";
            }

            tx.exec_params(
                "INSERT INTO anomalies ("
                "observation_id, anomaly_type, parameter_name, "
                "actual_value, expected_value, deviation, description"
                ") "
                "VALUES ($1, $2, $3, $4, $5, $6, $7)",
                points[i].observationId,
                anomalyType,
                "temperature",
                points[i].temperature,
                predictedValues[i],
                deviation,
                "Daily average temperature differs strongly from regression prediction"
            );

            anomalyCount++;
        }
    }

    tx.commit();

    std::cout << "\nAnomaly search finished." << std::endl;
    std::cout << "City: " << cityName << std::endl;
    std::cout << "Period: " << startDate << " - " << endDate << std::endl;
    std::cout << "Anomalous days found: "
              << anomalyCount
              << std::endl;
}

void AnomalyService::showAnomalies(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT "
        "o.observation_time::date AS observation_date, "
        "a.anomaly_type, "
        "a.actual_value, "
        "a.expected_value, "
        "a.deviation "
        "FROM anomalies a "
        "JOIN weather_observations o ON o.id = a.observation_id "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "AND a.parameter_name = 'temperature' "
        "ORDER BY o.observation_time::date, a.anomaly_type",
        cityName,
        startDate,
        endDate
    );

    tx.commit();

    std::cout << "\nAnomalous days for "
              << cityName
              << " from "
              << startDate
              << " to "
              << endDate
              << std::endl;

    if (res.empty()) {
        std::cout << "No anomalous days found." << std::endl;
        return;
    }

    for (const auto& row : res) {
        std::string type = row["anomaly_type"].as<std::string>();

        if (type == "HIGH_TEMPERATURE_ANOMALY") {
            type = "high temperature anomaly";
        } else if (type == "LOW_TEMPERATURE_ANOMALY") {
            type = "low temperature anomaly";
        }

        std::cout << row["observation_date"].as<std::string>()
                  << " | "
                  << type
                  << " | average temperature: "
                  << row["actual_value"].as<double>()
                  << " C"
                  << " | expected: "
                  << row["expected_value"].as<double>()
                  << " C"
                  << " | deviation: "
                  << row["deviation"].as<double>()
                  << " C"
                  << std::endl;
    }
}