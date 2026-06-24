#pragma once

#include "Database.h"

#include <string>

class ImportLogRepository {
private:
    Database& db;

public:
    explicit ImportLogRepository(Database& db);

    int start(int sourceId, const std::string& sourceName);

    void finish(
        int logId,
        const std::string& status,
        int totalRows,
        int loadedRows,
        int skippedRows,
        const std::string& errorText
    );
};