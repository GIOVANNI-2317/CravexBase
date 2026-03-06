#pragma once
#include "../../core/sys.hpp"
#include "../../core/inst.hpp"
#include "../../core/code.hpp"
#include <env/bridge.hpp>
#include <deps/server/nlohmann/json.hpp>

using json = nlohmann::json;

namespace msc {
    inline std::string handleCompile(const std::string& data, const json& settings, DWORD pid) {
        return code::compile(data, settings.value("enc", "true") == "true");
    }

    inline std::string handleSetScriptBytecode(const std::string& data, const json& settings, DWORD pid) {
        size_t outSize;
        std::vector<char> signedCode = code::sign(data, outSize);
        inst targetScript = getPtr(settings.value("cn", ""), pid);
        targetScript.setCode(signedCode, outSize);
        return std::string("");
    }

    inline std::string handleGetScriptBytecode(const std::string& data, const json& settings, DWORD pid) {
        inst targetScript = getPtr(settings.value("cn", ""), pid);
        uintptr_t bcPtr = 0;
        std::string className = targetScript.getClass();
        
        if (className == "LocalScript") {
            bcPtr = mem::read<uintptr_t>(targetScript.getAddr() + offs::lscriptBc, pid);
        } else if (className == "ModuleScript") {
            bcPtr = mem::read<uintptr_t>(targetScript.getAddr() + offs::mscriptBc, pid);
        }

        if (!bcPtr) return "";

        // Bytecode string object is at bcPtr + 0x10
        uintptr_t strObj = bcPtr + 0x10;
        size_t sz = mem::read<size_t>(strObj + 0x10, pid); // length at +0x10
        size_t cap = mem::read<size_t>(strObj + 0x18, pid); // capacity at +0x18
        
        uintptr_t dataPtr = (cap > 15) ? mem::read<uintptr_t>(strObj, pid) : strObj;

        if (!dataPtr || !sz) return "";

        std::string buffer;
        buffer.resize(sz);
        mem::readBytes(dataPtr, buffer.data(), sz, pid);
        
        // Decompress the bytecode as requested
        return code::decompress(buffer);
    }

    inline std::string handleListen(const std::string& data, const json& settings, DWORD pid) {
        if (!processOrders.count(pid) || processOrders[pid] < globalOrd) {
            processOrders[pid] = globalOrd;
            return globalSrc;
        }
        return std::string("");
    }
    
    inline std::string handleRequest(const std::string& data, const json& settings, DWORD pid) {
        std::string targetUrl = settings.value("Url", settings.value("url", ""));
        std::string method = settings.value("Method", settings.value("method", "GET"));
        std::string reqBody = settings.value("Body", settings.value("body", ""));
        
        // Trim whitespace from URL
        targetUrl.erase(0, targetUrl.find_first_not_of(" \n\r\t"));
        targetUrl.erase(targetUrl.find_last_not_of(" \n\r\t") + 1);

        if (targetUrl.empty()) return "{}";

        std::regex urlRegex(R"(^(http[s]?:\/\/)?([^\/]+)(\/.*)?$)");
        std::smatch matchResult;
        
        if (!std::regex_match(targetUrl, matchResult, urlRegex)) return "{}";
        
        std::string hostName = matchResult[2];
        std::string endpointPath = matchResult[3].matched ? matchResult[3].str() : "/";
        
        httplib::Client httpClient(hostName.c_str());
        httpClient.set_follow_location(true);
        httplib::Headers reqHeaders;
        
        // Handle both "Headers" and "headers" (UNC uses "Headers")
        json headers = settings.contains("Headers") ? settings["Headers"] : (settings.contains("headers") ? settings["headers"] : json::object());
        
        bool hasUserAgent = false;
        for (auto& headerItem : headers.items()) {
            std::string key = headerItem.key();
            std::string val = headerItem.value().is_string() ? headerItem.value().get<std::string>() : headerItem.value().dump();
            reqHeaders.insert({key, val});
            
            // Case-insensitive check for User-Agent
            std::string keyLower = key;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
            if (keyLower == "user-agent") hasUserAgent = true;
        }
        
        if (!hasUserAgent) {
            reqHeaders.insert({"User-Agent", "CravexBase/0.1.1"});
        }
        
        httplib::Result httpRes;
        if (method == "GET") {
            httpRes = httpClient.Get(endpointPath, reqHeaders);
        } else if (method == "POST") {
            httpRes = httpClient.Post(endpointPath, reqHeaders, reqBody, "application/json");
        } else if (method == "PUT") {
            httpRes = httpClient.Put(endpointPath, reqHeaders, reqBody, "application/json");
        } else if (method == "DELETE") {
            httpRes = httpClient.Delete(endpointPath, reqHeaders);
        }
        
        if (httpRes) {
            json respJson;
            respJson["Body"] = httpRes->body; 
            respJson["StatusCode"] = httpRes->status;
            for (auto& headerItem : httpRes->headers) {
                respJson["Headers"][headerItem.first] = headerItem.second;
            }
            return respJson.dump();
        }
        return "{}";
    }

    inline std::string handleIsRbxActive(const std::string& data, const json& settings, DWORD pid) {
        HWND robloxHwnd = proc::getHwnd(pid);
        return (GetForegroundWindow() == robloxHwnd) ? "true" : "false";
    }

    inline void init() {
        registerBridgeMethod("compile", handleCompile);
        registerBridgeMethod("setscriptbytecode", handleSetScriptBytecode);
        registerBridgeMethod("getscriptbytecode", handleGetScriptBytecode);
        registerBridgeMethod("listen", handleListen);
        registerBridgeMethod("request", handleRequest);
        registerBridgeMethod("isrbxactive", handleIsRbxActive);
    }
}
