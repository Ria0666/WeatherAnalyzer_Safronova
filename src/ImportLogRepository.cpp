#include "ImportLogRepository.h"

#include <pqxx/pqxx>

ImportLogRepository::ImportLogRepository(Database& db) : db(db) {}

int ImportLogRepository::start(
    int sourceId,
    const std::string& sourceName
) {
    pqxx::work tx(db.getConnection());

    pqxx::result res = tx.exec_params(
        "INSERT INTO import_logs "
        "(file_name, status, source_id) "
        "VALUES ($1, $2, $3) "
        "RETURNING id",
        sourceName,
        "STARTED",
        sourceId
    );

    int id = res[0]["id"].as<int>();

    tx.commit();

    return id;
}

void ImportLogRepository::finish(
    int logId,
    const std::string& status,
    int totalRows,
    int loadedRows,
    int skippedRows,
    const std::string& errorText
) {
    pqxx::work tx(db.getConnection());

    tx.exec_params(
        "UPDATE import_logs "
        "SET finished_at = CURRENT_TIMESTAMP, "
        "status = $1, "
        "total_rows = $2, "
        "loaded_rows = $3, "
        "skipped_rows = $4, "
        "error_message = $5 "
        "WHERE id = $6",
        status,
        totalRows,
        loadedRows,
        skippedRows,
        errorText,
        logId
    );

    tx.commit();
}