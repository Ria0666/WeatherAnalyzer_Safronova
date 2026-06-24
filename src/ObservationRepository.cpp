#include "ObservationRepository.h"

#include <iostream>
#include <pqxx/pqxx>
#include <sstream>

ObservationRepository::ObservationRepository(Database& db) : db(db) {}

std::string ObservationRepository::numberOrNull(
    const std::optional<double>& value,
    pqxx::work& tx
) {
    if (!value.has_value()) {
        return "NULL";
    }

    return tx.quote(value.value());
}

bool ObservationRepository::save(const WeatherObservation& obs) {
    try {
        pqxx::work tx(db.getConnection());

        std::ostringstream sql;

        sql << "INSERT INTO weather_observations ("
            << "station_id, source_id, observation_time, "
            << "temperature, humidity, pressure, wind_speed, "
            << "wind_direction, precipitation, cloudiness"
            << ") VALUES ("
            << tx.quote(obs.stationId) << ", "
            << tx.quote(obs.sourceId) << ", "
            << tx.quote(obs.observationTime) << ", "
            << numberOrNull(obs.temperature, tx) << ", "
            << numberOrNull(obs.humidity, tx) << ", "
            << numberOrNull(obs.pressure, tx) << ", "
            << numberOrNull(obs.windSpeed, tx) << ", "
            << numberOrNull(obs.windDirection, tx) << ", "
            << numberOrNull(obs.precipitation, tx) << ", "
            << numberOrNull(obs.cloudiness, tx)
            << ") "
            << "ON CONFLICT (station_id, observation_time) DO UPDATE SET "
            << "source_id = EXCLUDED.source_id, "
            << "temperature = EXCLUDED.temperature, "
            << "humidity = EXCLUDED.humidity, "
            << "pressure = EXCLUDED.pressure, "
            << "wind_speed = EXCLUDED.wind_speed, "
            << "wind_direction = EXCLUDED.wind_direction, "
            << "precipitation = EXCLUDED.precipitation, "
            << "cloudiness = EXCLUDED.cloudiness";

        tx.exec(sql.str());
        tx.commit();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Save observation error: "
                  << e.what()
                  << std::endl;

        return false;
    }
}