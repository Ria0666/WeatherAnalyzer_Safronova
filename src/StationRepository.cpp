#include "StationRepository.h"

#include <pqxx/pqxx>

StationRepository::StationRepository(Database& db) : db(db) {}

int StationRepository::getOrCreateStation(const City& city) {
    pqxx::work tx(db.getConnection());

    pqxx::result found = tx.exec_params(
        "SELECT id "
        "FROM weather_stations "
        "WHERE city_id = $1 AND station_code = $2",
        city.id,
        city.stationCode
    );

    if (!found.empty()) {
        int id = found[0]["id"].as<int>();
        tx.commit();
        return id;
    }

    pqxx::result inserted = tx.exec_params(
        "INSERT INTO weather_stations "
        "(city_id, name, station_code, latitude, longitude, elevation) "
        "VALUES ($1, $2, $3, $4, $5, $6) "
        "RETURNING id",
        city.id,
        city.stationName,
        city.stationCode,
        city.latitude,
        city.longitude,
        0.0
    );

    int id = inserted[0]["id"].as<int>();

    tx.commit();

    return id;
}