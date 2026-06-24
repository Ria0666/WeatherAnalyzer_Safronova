#pragma once

#include "Database.h"

#include <string>

class AnomalyService {
private:
    Database& db;

    void clearOldAnomalies(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate
    );

public:
    explicit AnomalyService(Database& db);

    void findTemperatureAnomaliesByRegression(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate
    );

    void showAnomalies(
        const std::string& cityName,
        const std::string& startDate,
        const std::string& endDate
    );
};