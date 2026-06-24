#include "OpenMeteoClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>

using json = nlohmann::json;

static size_t writeData(
    void* buffer,
    size_t size,
    size_t nmemb,
    void* userp
) {
    size_t realSize = size * nmemb;
    std::string* text = static_cast<std::string*>(userp);

    text->append(static_cast<char*>(buffer), realSize);

    return realSize;
}

static std::optional<double> getNumber(const json& arr, size_t i) {
    if (i >= arr.size()) {
        return std::nullopt;
    }

    if (arr[i].is_null()) {
        return std::nullopt;
    }

    return arr[i].get<double>();
}

std::vector<WeatherObservation> OpenMeteoClient::loadWeather(
    const City& city,
    int sourceId,
    const std::string& startDate,
    const std::string& endDate
) {
    std::string url = makeUrl(city, startDate, endDate);

    std::cout << "Loading weather for " << city.name << std::endl;

    std::string answer = getRequest(url);

    if (answer.empty()) {
        std::cerr << "Empty response from Open-Meteo" << std::endl;
        return {};
    }

    return parseJson(answer, city, sourceId);
}

std::string OpenMeteoClient::makeUrl(
    const City& city,
    const std::string& startDate,
    const std::string& endDate
) {
    std::ostringstream url;

    url << "https://archive-api.open-meteo.com/v1/archive"
        << "?latitude=" << city.latitude
        << "&longitude=" << city.longitude
        << "&start_date=" << startDate
        << "&end_date=" << endDate
        << "&hourly="
        << "temperature_2m,"
        << "relative_humidity_2m,"
        << "surface_pressure,"
        << "wind_speed_10m,"
        << "wind_direction_10m,"
        << "precipitation,"
        << "cloud_cover"
        << "&timezone=Europe/Moscow";

    return url.str();
}

std::string OpenMeteoClient::getRequest(const std::string& url) {
    CURL* curl = curl_easy_init();

    if (curl == nullptr) {
        std::cerr << "CURL init error" << std::endl;
        return "";
    }

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode code = curl_easy_perform(curl);

    if (code != CURLE_OK) {
        std::cerr << "CURL error: "
                  << curl_easy_strerror(code)
                  << std::endl;

        response.clear();
    }

    curl_easy_cleanup(curl);

    return response;
}

std::vector<WeatherObservation> OpenMeteoClient::parseJson(
    const std::string& text,
    const City& city,
    int sourceId
) {
    std::vector<WeatherObservation> result;

    try {
        json root = json::parse(text);

        if (!root.contains("hourly")) {
            std::cerr << "No hourly field in JSON" << std::endl;
            return result;
        }

        json hourly = root["hourly"];

        json time = hourly["time"];
        json temperature = hourly["temperature_2m"];
        json humidity = hourly["relative_humidity_2m"];
        json pressure = hourly["surface_pressure"];
        json windSpeed = hourly["wind_speed_10m"];
        json windDirection = hourly["wind_direction_10m"];
        json precipitation = hourly["precipitation"];
        json cloudiness = hourly["cloud_cover"];

        for (size_t i = 0; i < time.size(); i++) {
            WeatherObservation obs;

            obs.cityName = city.name;
            obs.stationCode = city.stationCode;
            obs.sourceId = sourceId;

            std::string dateTime = time[i].get<std::string>();

            for (char& ch : dateTime) {
                if (ch == 'T') {
                    ch = ' ';
                }
            }

            obs.observationTime = dateTime + ":00";

            obs.temperature = getNumber(temperature, i);
            obs.humidity = getNumber(humidity, i);
            obs.pressure = getNumber(pressure, i);
            obs.windSpeed = getNumber(windSpeed, i);
            obs.windDirection = getNumber(windDirection, i);
            obs.precipitation = getNumber(precipitation, i);
            obs.cloudiness = getNumber(cloudiness, i);

            result.push_back(obs);
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    return result;
}