#pragma once
#include <env/bridge.hpp>
#include <winhttp.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>

#pragma comment(lib, "winhttp.lib")

namespace ws {
    struct WebSocketClient {
        HINTERNET hSession = NULL;
        HINTERNET hConnect = NULL;
        HINTERNET hRequest = NULL;
        HINTERNET hWebSocket = NULL;
        std::vector<std::string> messages;
        std::mutex mtx;
        bool closed = false;

        ~WebSocketClient() {
            if (hWebSocket) WinHttpCloseHandle(hWebSocket);
            if (hRequest) WinHttpCloseHandle(hRequest);
            if (hConnect) WinHttpCloseHandle(hConnect);
            if (hSession) WinHttpCloseHandle(hSession);
        }
    };

    inline std::unordered_map<std::string, std::shared_ptr<WebSocketClient>> clients;
    inline std::mutex clientsMtx;

    inline std::string handleConnect(const std::string& data, const json& settings, DWORD pid) {
        std::string url = settings.value("url", "");
        if (url.empty()) return "";

        // Trim whitespace from URL
        url.erase(0, url.find_first_not_of(" \n\r\t"));
        url.erase(url.find_last_not_of(" \n\r\t") + 1);

        auto client = std::make_shared<WebSocketClient>();
        
        // Improved URL parsing
        bool isSecure = url.find("wss://") == 0;
        std::string remaining = url.substr(isSecure ? 6 : 5);
        
        size_t pathPos = remaining.find('/');
        std::string hostPort = (pathPos == std::string::npos) ? remaining : remaining.substr(0, pathPos);
        std::string path = (pathPos == std::string::npos) ? "/" : remaining.substr(pathPos);

        // Trim whitespace from hostPort
        hostPort.erase(0, hostPort.find_first_not_of(" \n\r\t"));
        hostPort.erase(hostPort.find_last_not_of(" \n\r\t") + 1);
        
        size_t portPos = hostPort.find(':');
        std::string host = (portPos == std::string::npos) ? hostPort : hostPort.substr(0, portPos);
        int port = (portPos == std::string::npos) ? (isSecure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT) : std::stoi(hostPort.substr(portPos + 1));

        client->hSession = WinHttpOpen(L"CravexBase/0.1.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!client->hSession) return "Error: WinHttpOpen failed (" + std::to_string(GetLastError()) + ")";

        std::wstring wHost(host.begin(), host.end());
        client->hConnect = WinHttpConnect(client->hSession, wHost.c_str(), (INTERNET_PORT)port, 0);
        if (!client->hConnect) return "Error: WinHttpConnect failed (" + std::to_string(GetLastError()) + ")";

        std::wstring wPath(path.begin(), path.end());
        client->hRequest = WinHttpOpenRequest(client->hConnect, L"GET", wPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, isSecure ? WINHTTP_FLAG_SECURE : 0);
        if (!client->hRequest) return "Error: WinHttpOpenRequest failed (" + std::to_string(GetLastError()) + ")";

        // SSL compatibility
        if (isSecure) {
            DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | 
                            SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE | 
                            SECURITY_FLAG_IGNORE_CERT_CN_INVALID | 
                            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
            WinHttpSetOption(client->hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
        }

        if (!WinHttpSetOption(client->hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0)) return "Error: WinHttpSetOption failed (" + std::to_string(GetLastError()) + ")";

        if (!WinHttpSendRequest(client->hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) return "Error: WinHttpSendRequest failed (" + std::to_string(GetLastError()) + ")";
        if (!WinHttpReceiveResponse(client->hRequest, NULL)) return "Error: WinHttpReceiveResponse failed (" + std::to_string(GetLastError()) + ")";

        client->hWebSocket = WinHttpWebSocketCompleteUpgrade(client->hRequest, NULL);
        if (!client->hWebSocket) return "Error: WinHttpWebSocketCompleteUpgrade failed (" + std::to_string(GetLastError()) + ")";

        std::string handle = std::to_string(reinterpret_cast<uintptr_t>(client.get()));
        {
            std::lock_guard<std::mutex> lock(clientsMtx);
            clients[handle] = client;
        }

        // Background receive loop
        std::thread([client]() {
            while (!client->closed) {
                BYTE buffer[4096];
                DWORD dwBytesRead = 0;
                WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;
                DWORD dwError = WinHttpWebSocketReceive(client->hWebSocket, buffer, sizeof(buffer), &dwBytesRead, &bufferType);
                
                if (dwError == ERROR_SUCCESS) {
                    if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE || bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) {
                        std::lock_guard<std::mutex> lock(client->mtx);
                        client->messages.push_back(std::string((char*)buffer, dwBytesRead));
                    } else if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                        client->closed = true;
                    }
                } else {
                    client->closed = true;
                }
            }
        }).detach();

        return handle;
    }

    inline std::string handleSend(const std::string& data, const json& settings, DWORD pid) {
        std::string handle = settings.value("h", "");
        std::shared_ptr<WebSocketClient> client;
        {
            std::lock_guard<std::mutex> lock(clientsMtx);
            if (clients.count(handle)) client = clients[handle];
        }
        if (!client || client->closed) return "false";

        DWORD dwError = WinHttpWebSocketSend(client->hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (PVOID)data.c_str(), (DWORD)data.size());
        return dwError == ERROR_SUCCESS ? "true" : "false";
    }

    inline std::string handleClose(const std::string& data, const json& settings, DWORD pid) {
        std::string handle = settings.value("h", "");
        std::shared_ptr<WebSocketClient> client;
        {
            std::lock_guard<std::mutex> lock(clientsMtx);
            if (clients.count(handle)) {
                client = clients[handle];
                clients.erase(handle);
            }
        }
        if (client) {
            client->closed = true;
            WinHttpWebSocketClose(client->hWebSocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
        }
        return "true";
    }

    inline std::string handlePoll(const std::string& data, const json& settings, DWORD pid) {
        std::string handle = settings.value("h", "");
        std::shared_ptr<WebSocketClient> client;
        {
            std::lock_guard<std::mutex> lock(clientsMtx);
            if (clients.count(handle)) client = clients[handle];
        }
        if (!client) return "{\"closed\":true}";

        json res;
        {
            std::lock_guard<std::mutex> lock(client->mtx);
            res["messages"] = client->messages;
            client->messages.clear();
        }
        res["closed"] = client->closed;
        return res.dump();
    }

    inline void init() {
        registerBridgeMethod("websocket_connect", handleConnect);
        registerBridgeMethod("websocket_send", handleSend);
        registerBridgeMethod("websocket_close", handleClose);
        registerBridgeMethod("websocket_poll", handlePoll);
    }
}
