#include "ForecastService.h"

#include <iostream>
#include <pqxx/pqxx>

ForecastService::ForecastService(Database& db) : db(db) {}

int ForecastService::getCityId(const std::string& cityName) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT id "
        "FROM cities "
        "WHERE name = $1 "
        "LIMIT 1",
        cityName
    );

    tx.commit();

    if (res.empty()) {
        return 0;
    }

    return res[0]["id"].as<int>();
}

void ForecastService::buildTemperatureForecast(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate,
    int windowDays
) {
    int cityId = getCityId(cityName);

    if (cityId == 0) {
        std::cout << "\nCity not found." << std::endl;
        return;
    }

    if (windowDays <= 0) {
        std::cout << "\nWindow size must be positive." << std::endl;
        return;
    }

    pqxx::work tx(db.getConnection());

    tx.exec_params(
        "DELETE FROM forecast_results "
        "WHERE city_id = $1 "
        "AND parameter_name = 'temperature' "
        "AND model_name = 'Moving average' "
        "AND forecast_date BETWEEN $2 AND $3",
        cityId,
        startDate,
        endDate
    );

    pqxx::result res = tx.exec_params(
        "WITH forecast_dates AS ( "
        "    SELECT generate_series($2::date, $3::date, INTERVAL '1 day')::date AS forecast_date "
        "), "
        "daily_temperature AS ( "
        "    SELECT "
        "        o.observation_time::date AS day_date, "
        "        AVG(o.temperature) AS avg_temp "
        "    FROM weather_observations o "
        "    JOIN weather_stations s ON s.id = o.station_id "
        "    JOIN cities c ON c.id = s.city_id "
        "    WHERE c.id = $1 "
        "    AND o.temperature IS NOT NULL "
        "    GROUP BY o.observation_time::date "
        ") "
        "SELECT "
        "    f.forecast_date, "
        "    AVG(d.avg_temp) AS predicted_temp, "
        "    COUNT(d.avg_temp) AS used_days "
        "FROM forecast_dates f "
        "LEFT JOIN daily_temperature d "
        "    ON d.day_date >= f.forecast_date - ($4::int * INTERVAL '1 day') "
        "    AND d.day_date < f.forecast_date "
        "GROUP BY f.forecast_date "
        "ORDER BY f.forecast_date",
        cityId,
        startDate,
        endDate,
        windowDays
    );

    int savedCount = 0;

    for (const auto& row : res) {
        if (row["predicted_temp"].is_null()) {
            continue;
        }

        int usedDays = row["used_days"].as<int>();

        if (usedDays == 0) {
            continue;
        }

        std::string forecastDate = row["forecast_date"].as<std::string>();
        double predictedTemp = row["predicted_temp"].as<double>();

        tx.exec_params(
            "INSERT INTO forecast_results ("
            "city_id, parameter_name, forecast_date, predicted_value, model_name"
            ") "
            "VALUES ($1, $2, $3, $4, $5)",
            cityId,
            "temperature",
            forecastDate,
            predictedTemp,
            "Moving average"
        );

        savedCount++;
    }

    tx.commit();

    std::cout << "\nTemperature forecast finished." << std::endl;
    std::cout << "City: " << cityName << std::endl;
    std::cout << "Period: " << startDate << " - " << endDate << std::endl;
    std::cout << "Model: Moving average" << std::endl;
    std::cout << "Window size: " << windowDays << " days" << std::endl;
    std::cout << "Saved forecast rows: " << savedCount << std::endl;

    if (savedCount == 0) {
        std::cout << "No forecast was saved. Probably there is no previous data for this period." << std::endl;
    }
}

void ForecastService::showTemperatureForecast(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT "
        "    f.forecast_date, "
        "    f.predicted_value, "
        "    f.model_name "
        "FROM forecast_results f "
        "JOIN cities c ON c.id = f.city_id "
        "WHERE c.name = $1 "
        "AND f.parameter_name = 'temperature' "
        "AND f.forecast_date BETWEEN $2 AND $3 "
        "ORDER BY f.forecast_date",
        cityName,
        startDate,
        endDate
    );

    tx.commit();

    std::cout << "\nTemperature forecast for "
              << cityName
              << " from "
              << startDate
              << " to "
              << endDate
              << std::endl;

    if (res.empty()) {
        std::cout << "No forecast found." << std::endl;
        return;
    }

    for (const auto& row : res) {
        std::cout << row["forecast_date"].as<std::string>()
                  << " | predicted temperature: "
                  << row["predicted_value"].as<double>()
                  << " C"
                  << " | model: "
                  << row["model_name"].as<std::string>()
                  << std::endl;
    }
}

void ForecastService::buildPrecipitationProbabilityForecast(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate,
    int windowDays
) {
    int cityId = getCityId(cityName);

    if (cityId == 0) {
        std::cout << "\nCity not found." << std::endl;
        return;
    }

    if (windowDays <= 0) {
        std::cout << "\nWindow size must be positive." << std::endl;
        return;
    }

    pqxx::work tx(db.getConnection());

    tx.exec_params(
        "DELETE FROM forecast_results "
        "WHERE city_id = $1 "
        "AND parameter_name = 'precipitation_probability' "
        "AND model_name = 'Moving average' "
        "AND forecast_date BETWEEN $2 AND $3",
        cityId,
        startDate,
        endDate
    );

    pqxx::result res = tx.exec_params(
        "WITH forecast_dates AS ( "
        "    SELECT generate_series($2::date, $3::date, INTERVAL '1 day')::date AS forecast_date "
        "), "
        "daily_precipitation AS ( "
        "    SELECT "
        "        o.observation_time::date AS day_date, "
        "        SUM(COALESCE(o.precipitation, 0)) AS total_precipitation "
        "    FROM weather_observations o "
        "    JOIN weather_stations s ON s.id = o.station_id "
        "    JOIN cities c ON c.id = s.city_id "
        "    WHERE c.id = $1 "
        "    GROUP BY o.observation_time::date "
        "), "
        "rainy_days AS ( "
        "    SELECT "
        "        day_date, "
        "        CASE "
        "            WHEN total_precipitation > 0 THEN 1 "
        "            ELSE 0 "
        "        END AS is_rainy "
        "    FROM daily_precipitation "
        ") "
        "SELECT "
        "    f.forecast_date, "
        "    AVG(r.is_rainy) * 100.0 AS precipitation_probability, "
        "    COUNT(r.is_rainy) AS used_days "
        "FROM forecast_dates f "
        "LEFT JOIN rainy_days r "
        "    ON r.day_date >= f.forecast_date - ($4::int * INTERVAL '1 day') "
        "    AND r.day_date < f.forecast_date "
        "GROUP BY f.forecast_date "
        "ORDER BY f.forecast_date",
        cityId,
        startDate,
        endDate,
        windowDays
    );

    int savedCount = 0;

    for (const auto& row : res) {
        if (row["precipitation_probability"].is_null()) {
            continue;
        }

        int usedDays = row["used_days"].as<int>();

        if (usedDays == 0) {
            continue;
        }

        std::string forecastDate = row["forecast_date"].as<std::string>();
        double probability = row["precipitation_probability"].as<double>();

        tx.exec_params(
            "INSERT INTO forecast_results ("
            "city_id, parameter_name, forecast_date, predicted_value, model_name"
            ") "
            "VALUES ($1, $2, $3, $4, $5)",
            cityId,
            "precipitation_probability",
            forecastDate,
            probability,
            "Moving average"
        );

        savedCount++;
    }

    tx.commit();

    std::cout << "\nPrecipitation probability forecast finished." << std::endl;
    std::cout << "City: " << cityName << std::endl;
    std::cout << "Period: " << startDate << " - " << endDate << std::endl;
    std::cout << "Model: Moving average" << std::endl;
    std::cout << "Window size: " << windowDays << " days" << std::endl;
    std::cout << "Saved forecast rows: " << savedCount << std::endl;

    if (savedCount == 0) {
        std::cout << "No forecast was saved. Probably there is no previous data for this period." << std::endl;
    }
}

void ForecastService::showPrecipitationProbabilityForecast(
    const std::string& cityName,
    const std::string& startDate,
    const std::string& endDate
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "SELECT "
        "    f.forecast_date, "
        "    f.predicted_value, "
        "    f.model_name "
        "FROM forecast_results f "
        "JOIN cities c ON c.id = f.city_id "
        "WHERE c.name = $1 "
        "AND f.parameter_name = 'precipitation_probability' "
        "AND f.forecast_date BETWEEN $2 AND $3 "
        "ORDER BY f.forecast_date",
        cityName,
        startDate,
        endDate
    );

    tx.commit();

    std::cout << "\nPrecipitation probability forecast for "
              << cityName
              << " from "
              << startDate
              << " to "
              << endDate
              << std::endl;

    if (res.empty()) {
        std::cout << "No forecast found." << std::endl;
        return;
    }

    for (const auto& row : res) {
        std::cout << row["forecast_date"].as<std::string>()
                  << " | precipitation probability: "
                  << row["predicted_value"].as<double>()
                  << "%"
                  << " | model: "
                  << row["model_name"].as<std::string>()
                  << std::endl;
    }
}