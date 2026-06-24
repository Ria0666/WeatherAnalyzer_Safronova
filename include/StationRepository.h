#pragma once

#include "Database.h"
#include "Models.h"

class StationRepository {
private:
    Database& db;

public:
    explicit StationRepository(Database& db);

    int getOrCreateStation(const City& city);
};