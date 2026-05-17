#pragma once

#include <string>

struct DatabaseConfig {
    std::string databaseUrl;
    std::string host;
    int port;
    std::string name;
    std::string user;
    std::string password;

    static DatabaseConfig fromEnvironment();

    bool hasDatabaseUrl() const;
    std::string safeConnectionUri() const;
    std::string libpqConnectionString(bool includePassword = false) const;
};
