#include "CityRepository.h"

#include <pqxx/pqxx>

CityRepository::CityRepository(Database& db) : db(db) {}

std::vector<City> CityRepository::getCities() {
    std::vector<City> cities;

    pqxx::work tx(db.getConnection());

    pqxx::result rows = tx.exec(
        "SELECT id, name, country, region, latitude, longitude "
        "FROM cities "
        "ORDER BY id"
    );

    tx.commit();

    for (const auto& row : rows) {
        City city;

        city.id = row["id"].as<int>();
        city.name = row["name"].as<std::string>();
        city.country = row["country"].as<std::string>();

        if (!row["region"].is_null()) {
            city.region = row["region"].as<std::string>();
        }

        if (!row["latitude"].is_null()) {
            city.latitude = row["latitude"].as<double>();
        }

        if (!row["longitude"].is_null()) {
            city.longitude = row["longitude"].as<double>();
        }

        city.stationName = city.name + " weather station";
        city.stationCode = city.name;

        for (char& ch : city.stationCode) {
            if (ch == ' ') {
                ch = '_';
            }
        }

        cities.push_back(city);
    }

    return cities;
}