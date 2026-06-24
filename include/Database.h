#pragma once

#include <memory>
#include <pqxx/pqxx>
#include <string>

class Database {
private:
    std::unique_ptr<pqxx::connection> connection;

public:
    explicit Database(const std::string& connectionString);

    pqxx::connection& getConnection();
};