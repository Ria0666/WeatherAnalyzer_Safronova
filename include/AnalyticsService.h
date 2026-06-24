#pragma once

#include "Database.h"

#include <string>

class AnalyticsService {
private:
    Database& db;

public:
    explicit AnalyticsService(Database& db);

    void showTemperatureStats(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate
    );

    void showMonthlyTemperature(
        const std::string& cityName,
        int year
    );

    void showMonthlyPrecipitation(
        const std::string& cityName,
        int year
    );

    void showRainyDays(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate,
        double precipitationLimit
    );

    void compareCitiesTemperature(
        const std::string& startDate,
        const std::string& endDate
    );
};