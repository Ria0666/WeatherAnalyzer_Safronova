#pragma once

#include "Database.h"

#include <string>

class ForecastService {
private:
    Database& db;

    int getCityId(const std::string& cityName);

public:
    explicit ForecastService(Database& db);

    void buildTemperatureForecast(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate,
        int windowDays
    );

    void showTemperatureForecast(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate
    );

    void buildPrecipitationProbabilityForecast(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate,
        int windowDays
    );

    void showPrecipitationProbabilityForecast(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate
    );
};