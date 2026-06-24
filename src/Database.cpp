#include "Database.h"

#include <iostream>
#include <stdexcept>

Database::Database(const std::string& connectionString) {
    connection = std::make_unique<pqxx::connection>(connectionString);

    if (!connection->is_open()) {
        throw std::runtime_error("Database connection failed");
    }

    std::cout << "Connected to database: "
              << connection->dbname()
              << std::endl;
}

pqxx::connection& Database::getConnection() {
    return *connection;
}