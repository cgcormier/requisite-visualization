#include "api/ApiHandlers.h"
#include "api/CsvCatalog.h"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

volatile std::sig_atomic_t keepRunning = 1;

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle InvalidSocket = INVALID_SOCKET;

void closeSocket(SocketHandle socketHandle) {
    closesocket(socketHandle);
}
#else
using SocketHandle = int;
constexpr SocketHandle InvalidSocket = -1;

void closeSocket(SocketHandle socketHandle) {
    close(socketHandle);
}
#endif

void handleSignal(int) {
    keepRunning = 0;
}

std::string reasonPhrase(int status) {
    switch (status) {
        case 200:
            return "OK";
        case 204:
            return "No Content";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 500:
            return "Internal Server Error";
        default:
            return "OK";
    }
}

bool parseRequestLine(const std::string& request, std::string& method, std::string& target) {
    const std::size_t lineEnd = request.find("\r\n");
    const std::string line = request.substr(0, lineEnd == std::string::npos ? request.size() : lineEnd);

    std::istringstream stream(line);
    std::string version;
    stream >> method >> target >> version;
    return !method.empty() && !target.empty() && !version.empty();
}

std::string buildHttpResponse(const api::ApiResponse& response) {
    std::ostringstream out;
    out << "HTTP/1.1 " << response.status << " " << reasonPhrase(response.status) << "\r\n"
        << "Content-Type: " << response.contentType << "\r\n"
        << "Content-Length: " << response.body.size() << "\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
        << "Access-Control-Allow-Headers: Content-Type\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << response.body;
    return out.str();
}

bool sendAll(SocketHandle socketHandle, const std::string& payload) {
    std::size_t totalSent = 0;
    while (totalSent < payload.size()) {
#ifdef _WIN32
        const int sent = send(
            socketHandle,
            payload.data() + totalSent,
            static_cast<int>(payload.size() - totalSent),
            0
        );
#else
        const ssize_t sent = send(
            socketHandle,
            payload.data() + totalSent,
            payload.size() - totalSent,
            0
        );
#endif
        if (sent <= 0) {
            return false;
        }
        totalSent += static_cast<std::size_t>(sent);
    }
    return true;
}

std::string readRequest(SocketHandle socketHandle) {
    std::string request;
    char buffer[4096];

    while (request.find("\r\n\r\n") == std::string::npos && request.size() < 65536) {
#ifdef _WIN32
        const int received = recv(socketHandle, buffer, static_cast<int>(sizeof(buffer)), 0);
#else
        const ssize_t received = recv(socketHandle, buffer, sizeof(buffer), 0);
#endif
        if (received <= 0) {
            break;
        }
        request.append(buffer, buffer + received);
    }

    return request;
}

int parsePort() {
    const char* rawPort = std::getenv("API_PORT");
    if (rawPort == nullptr || rawPort[0] == '\0') {
        return 8080;
    }

    char* end = nullptr;
    const long parsed = std::strtol(rawPort, &end, 10);
    if (end == rawPort || *end != '\0' || parsed <= 0 || parsed > 65535) {
        return 8080;
    }

    return static_cast<int>(parsed);
}

void handleClient(SocketHandle clientSocket, const api::ApiHandlers& handlers) {
    const std::string request = readRequest(clientSocket);
    std::string method;
    std::string target;

    api::ApiResponse response;
    if (!parseRequestLine(request, method, target)) {
        response.status = 400;
        response.body = "{\"error\":{\"code\":\"bad_request\",\"message\":\"Invalid HTTP request.\"}}";
    } else {
        response = handlers.handleRequest(method, target);
    }

    const std::string payload = buildHttpResponse(response);
    sendAll(clientSocket, payload);
    closeSocket(clientSocket);
}

} // namespace

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize WinSock." << std::endl;
        return 1;
    }
#endif

    api::CsvCatalog catalog;
    std::string loadError;
    if (!catalog.load(&loadError)) {
        std::cerr << "Failed to load course catalog: " << loadError << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    const int port = parsePort();
    SocketHandle serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == InvalidSocket) {
        std::cerr << "Failed to create server socket." << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int reuse = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        std::cerr << "Failed to bind API server to 127.0.0.1:" << port << "." << std::endl;
        closeSocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(serverSocket, 16) != 0) {
        std::cerr << "Failed to listen for API requests." << std::endl;
        closeSocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    api::ApiHandlers handlers(catalog);
    std::cout << "API server listening on http://127.0.0.1:" << port
              << " with " << catalog.size() << " courses." << std::endl;

    while (keepRunning) {
        sockaddr_in clientAddress{};
#ifdef _WIN32
        int clientAddressLength = sizeof(clientAddress);
#else
        socklen_t clientAddressLength = sizeof(clientAddress);
#endif
        SocketHandle clientSocket = accept(
            serverSocket,
            reinterpret_cast<sockaddr*>(&clientAddress),
            &clientAddressLength
        );

        if (clientSocket == InvalidSocket) {
            if (!keepRunning) {
                break;
            }
            continue;
        }

        handleClient(clientSocket, handlers);
    }

    closeSocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
