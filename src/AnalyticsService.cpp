#include "AnalyticsService.h"

#include <iostream>
#include <pqxx/pqxx>
#include <string>

std::string getMonthName(int month) {
    if (month == 1) return "January";
    if (month == 2) return "February";
    if (month == 3) return "March";
    if (month == 4) return "April";
    if (month == 5) return "May";
    if (month == 6) return "June";
    if (month == 7) return "July";
    if (month == 8) return "August";
    if (month == 9) return "September";
    if (month == 10) return "October";
    if (month == 11) return "November";
    if (month == 12) return "December";

    return "Unknown";
}

AnalyticsService::AnalyticsService(Database& db) : db(db) {}

void AnalyticsService::showTemperatureStats(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT "
        "AVG(o.temperature) AS avg_temp, "
        "MIN(o.temperature) AS min_temp, "
        "MAX(o.temperature) AS max_temp "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3",
        cityName,
        startDate,
        endDate
    );

    tx.commit();

    std::cout << "\nTemperature statistics for " << cityName << std::endl;

    if (res.empty() || res[0]["avg_temp"].is_null()) {
        std::cout << "No data for selected period." << std::endl;
        return;
    }

    std::cout << "Average temperature: "
              << res[0]["avg_temp"].as<double>()
              << " C"
              << std::endl;

    std::cout << "Minimum temperature: "
              << res[0]["min_temp"].as<double>()
              << " C"
              << std::endl;

    std::cout << "Maximum temperature: "
              << res[0]["max_temp"].as<double>()
              << " C"
              << std::endl;
}

void AnalyticsService::showMonthlyTemperature(
    const std::string& cityName,
    int year
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT "
        "EXTRACT(MONTH FROM o.observation_time) AS month, "
        "AVG(o.temperature) AS avg_temp "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND EXTRACT(YEAR FROM o.observation_time) = $2 "
        "GROUP BY month "
        "ORDER BY month",
        cityName,
        year
    );

    tx.commit();

    std::cout << "\nAverage temperature by months for "
              << cityName
              << ", "
              << year
              << std::endl;

    if (res.empty()) {
        std::cout << "No data." << std::endl;
        return;
    }

    for (const auto& row : res) {
        int month = row["month"].as<int>();

        std::cout << getMonthName(month)
                  << ": "
                  << row["avg_temp"].as<double>()
                  << " C"
                  << std::endl;
    }
}

void AnalyticsService::showMonthlyPrecipitation(
    const std::string& cityName,
    int year
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT "
        "EXTRACT(MONTH FROM o.observation_time) AS month, "
        "SUM(o.precipitation) AS total_precipitation "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND EXTRACT(YEAR FROM o.observation_time) = $2 "
        "GROUP BY month "
        "ORDER BY month",
        cityName,
        year
    );

    tx.commit();

    std::cout << "\nPrecipitation by months for "
              << cityName
              << ", "
              << year
              << std::endl;

    if (res.empty()) {
        std::cout << "No data." << std::endl;
        return;
    }

    for (const auto& row : res) {
        int month = row["month"].as<int>();

        if (row["total_precipitation"].is_null()) {
            std::cout << getMonthName(month)
                      << ": 0 mm"
                      << std::endl;
        } else {
            std::cout << getMonthName(month)
                      << ": "
                      << row["total_precipitation"].as<double>()
                      << " mm"
                      << std::endl;
        }
    }
}

void AnalyticsService::showRainyDays(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate,
    double precipitationLimit
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT "
        "COUNT(DISTINCT o.observation_time::date) AS days_count "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE c.name = $1 "
        "AND o.observation_time::date BETWEEN $2 AND $3 "
        "AND o.precipitation > $4",
        cityName,
        startDate,
        endDate,
        precipitationLimit
    );

    tx.commit();

    std::cout << "\nRainy days in "
              << cityName
              << " from "
              << startDate
              << " to "
              << endDate
              << ": ";

    if (res.empty() || res[0]["days_count"].is_null()) {
        std::cout << 0 << std::endl;
        return;
    }

    std::cout << res[0]["days_count"].as<int>() << std::endl;
}

void AnalyticsService::compareCitiesTemperature(
    const std::string& startDate,
    const std::string& endDate
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT "
        "c.name AS city_name, "
        "AVG(o.temperature) AS avg_temp "
        "FROM weather_observations o "
        "JOIN weather_stations s ON s.id = o.station_id "
        "JOIN cities c ON c.id = s.city_id "
        "WHERE o.observation_time::date BETWEEN $1 AND $2 "
        "GROUP BY c.name "
        "ORDER BY avg_temp DESC",
        startDate,
        endDate
    );

    tx.commit();

    std::cout << "\nCity temperature comparison from "
              << startDate
              << " to "
              << endDate
              << std::endl;

    if (res.empty()) {
        std::cout << "No data." << std::endl;
        return;
    }

    for (const auto& row : res) {
        if (row["avg_temp"].is_null()) {
            std::cout << row["city_name"].as<std::string>()
                      << ": no temperature data"
                      << std::endl;
        } else {
            std::cout << row["city_name"].as<std::string>()
                      << ": "
                      << row["avg_temp"].as<double>()
                      << " C"
                      << std::endl;
        }
    }
}