#pragma once

#include "Database.h"
#include "Models.h"

#include <optional>
#include <string>

class ObservationRepository {
private:
    Database& db;

    std::string numberOrNull(
        const std::optional<double>& value,
        pqxx::work& tx
    );

public:
    explicit ObservationRepository(Database& db);

    bool save(const WeatherObservation& obs);
};