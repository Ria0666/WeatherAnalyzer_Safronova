#include "ImportService.h"

#include <iostream>

ImportService::ImportService(
    OpenMeteoClient& client,
    CityRepository& cityRepo,
    StationRepository& stationRepo,
    ObservationRepository& obsRepo,
    ImportLogRepository& logRepo
)
    : client(client),
      cityRepo(cityRepo),
      stationRepo(stationRepo),
      obsRepo(obsRepo),
      logRepo(logRepo) {}

bool ImportService::isValid(const WeatherObservation& obs) {
    if (obs.observationTime.empty()) {
        return false;
    }

    if (!obs.temperature.has_value()) {
        return false;
    }

    double temp = obs.temperature.value();

    if (temp < -90.0 || temp > 60.0) {
        return false;
    }

    if (obs.humidity.has_value()) {
        double h = obs.humidity.value();

        if (h < 0.0 || h > 100.0) {
            return false;
        }
    }

    if (obs.windSpeed.has_value() && obs.windSpeed.value() < 0.0) {
        return false;
    }

    if (obs.precipitation.has_value() && obs.precipitation.value() < 0.0) {
        return false;
    }

    return true;
}

ImportResult ImportService::importWeather(
    int sourceId,
    const std::string& startDate,
    const std::string& endDate
) {
    ImportResult result;

    int logId = logRepo.start(
        sourceId,
        "Open-Meteo Historical Weather API"
    );

    try {
        std::vector<City> cities = cityRepo.getCities();

        for (const City& city : cities) {
            int stationId = stationRepo.getOrCreateStation(city);

            std::vector<WeatherObservation> observations =
                client.loadWeather(city, sourceId, startDate, endDate);

            result.totalRows += static_cast<int>(observations.size());

            for (WeatherObservation obs : observations) {
                obs.stationId = stationId;
                obs.sourceId = sourceId;

                if (!isValid(obs)) {
                    result.skippedRows++;
                    continue;
                }

                if (obsRepo.save(obs)) {
                    result.loadedRows++;
                } else {
                    result.skippedRows++;
                }
            }
        }

        logRepo.finish(
            logId,
            "SUCCESS",
            result.totalRows,
            result.loadedRows,
            result.skippedRows,
            ""
        );
    } catch (const std::exception& e) {
        logRepo.finish(
            logId,
            "FAILED",
            result.totalRows,
            result.loadedRows,
            result.skippedRows,
            e.what()
        );

        std::cerr << "Import failed: " << e.what() << std::endl;
    }

    return result;
}