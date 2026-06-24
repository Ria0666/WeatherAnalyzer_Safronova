#pragma once

#include "CityRepository.h"
#include "ImportLogRepository.h"
#include "Models.h"
#include "ObservationRepository.h"
#include "OpenMeteoClient.h"
#include "StationRepository.h"

#include <string>

class ImportService {
private:
    OpenMeteoClient& client;
    CityRepository& cityRepo;
    StationRepository& stationRepo;
    ObservationRepository& obsRepo;
    ImportLogRepository& logRepo;

    bool isValid(const WeatherObservation& obs);

public:
    ImportService(
        OpenMeteoClient& client,
        CityRepository& cityRepo,
        StationRepository& stationRepo,
        ObservationRepository& obsRepo,
        ImportLogRepository& logRepo
    );

    ImportResult importWeather(
        int sourceId,
        const std::string& startDate,
        const std::string& endDate
    );
};