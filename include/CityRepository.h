#pragma once

#include "Database.h"
#include "Models.h"

#include <vector>

class CityRepository {
private:
    Database& db;

public:
    explicit CityRepository(Database& db);

    std::vector<City> getCities();
};