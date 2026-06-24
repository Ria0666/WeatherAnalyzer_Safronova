#pragma once

#include <optional>
#include <string>

struct City {
    int id = 0;

    std::string name;
    std::string country;
    std::string region;

    double latitude = 0.0;
    double longitude = 0.0;

    std::string stationName;
    std::string stationCode;
};

struct WeatherObservation {
    int stationId = 0;
    int sourceId = 0;

    std::string cityName;
    std::string stationCode;
    std::string observationTime;

    std::optional<double> temperature;
    std::optional<double> humidity;
    std::optional<double> pressure;
    std::optional<double> windSpeed;
    std::optional<double> windDirection;
    std::optional<double> precipitation;
    std::optional<double> cloudiness;
};

struct ImportResult {
    int totalRows = 0;
    int loadedRows = 0;
    int skippedRows = 0;
};