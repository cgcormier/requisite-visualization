#include "DatabaseConfig.h"

#include <cstdlib>
#include <sstream>
#include <string>

namespace {
std::string envOrDefault(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }

    return value;
}

int envIntOrDefault(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

std::string redactedUrl(const std::string& url) {
    const std::size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
        return url;
    }

    const std::size_t userInfoStart = schemeEnd + 3;
    const std::size_t at = url.find('@', userInfoStart);
    if (at == std::string::npos) {
        return url;
    }

    const std::size_t passwordSeparator = url.find(':', userInfoStart);
    if (passwordSeparator == std::string::npos || passwordSeparator > at) {
        return url;
    }

    return url.substr(0, passwordSeparator + 1) + "****" + url.substr(at);
}
}

DatabaseConfig DatabaseConfig::fromEnvironment() {
    DatabaseConfig config;
    config.databaseUrl = envOrDefault("DATABASE_URL", "");
    config.host = envOrDefault("DB_HOST", "localhost");
    config.port = envIntOrDefault("DB_PORT", 5432);
    config.name = envOrDefault("DB_NAME", "requisite_visualization");
    config.user = envOrDefault("DB_USER", "requisite_user");
    config.password = envOrDefault("DB_PASSWORD", "");
    return config;
}

bool DatabaseConfig::hasDatabaseUrl() const {
    return !databaseUrl.empty();
}

std::string DatabaseConfig::safeConnectionUri() const {
    if (hasDatabaseUrl()) {
        return redactedUrl(databaseUrl);
    }

    std::ostringstream uri;
    uri << "postgresql://";
    if (!user.empty()) {
        uri << user;
        if (!password.empty()) {
            uri << ":****";
        }
        uri << "@";
    }
    uri << host << ":" << port << "/" << name;
    return uri.str();
}

std::string DatabaseConfig::libpqConnectionString(bool includePassword) const {
    std::ostringstream connection;
    connection << "host=" << host
               << " port=" << port
               << " dbname=" << name
               << " user=" << user;

    if (!password.empty()) {
        connection << " password=" << (includePassword ? password : "****");
    }

    return connection.str();
}
