#pragma once

#include "Models.h"

#include <string>
#include <vector>

class OpenMeteoClient {
public:
    std::vector<WeatherObservation> loadWeather(
        const City& city,
        int sourceId,
        const std::string& startDate,
        const std::string& endDate
    );

private:
    std::string makeUrl(
        const City& city,
        const std::string& startDate,
        const std::string& endDate
    );

    std::string getRequest(const std::string& url);

    std::vector<WeatherObservation> parseJson(
        const std::string& text,
        const City& city,
        int sourceId
    );
};