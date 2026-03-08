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
        if (!targetScript.getAddr()) return "null:invalid_ptr";

        uintptr_t bcPtr = 0;
        std::string className = targetScript.getClass();

        if (className == "LocalScript" || className == "Script") {
            bcPtr = mem::read<uintptr_t>(targetScript.getAddr() + offs::lscriptBc, pid);
        }
        else if (className == "ModuleScript") {
            bcPtr = mem::read<uintptr_t>(targetScript.getAddr() + offs::mscriptBc, pid);
        }
        else {
            return "null:unsupported_class:" + className;
        }

        if (!bcPtr) return "null:no_bc_ptr:" + className;

        // Bytecode string object is at bcPtr + 0x10
        uintptr_t strObj = bcPtr + 0x10;
        size_t sz = mem::read<size_t>(strObj + 0x10, pid); // length at +0x10
        size_t cap = mem::read<size_t>(strObj + 0x18, pid); // capacity at +0x18

        uintptr_t dataPtr = (cap > 15) ? mem::read<uintptr_t>(strObj, pid) : strObj;

        if (!dataPtr || !sz) return "null:no_data_ptr:" + className;

        std::string buffer;
        buffer.resize(sz);
        mem::readBytes(dataPtr, buffer.data(), sz, pid);

        // Decompress
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
        std::string fullUrl = settings.value("Url", "");
        if (fullUrl.empty()) return "{\"Success\":false, \"StatusCode\":400, \"Body\":\"Empty URL\"}";

        std::string method = settings.value("Method", "GET");
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        std::string body = settings.value("Body", "");

        // Simple URL Parsing: scheme://host[:port][/path]
        std::string scheme, host, path = "/";
        size_t schemeEnd = fullUrl.find("://");
        if (schemeEnd != std::string::npos) {
            scheme = fullUrl.substr(0, schemeEnd + 3);
            std::string afterScheme = fullUrl.substr(schemeEnd + 3);
            size_t pathStart = afterScheme.find("/");
            if (pathStart != std::string::npos) {
                host = afterScheme.substr(0, pathStart);
                path = afterScheme.substr(pathStart);
            } else {
                host = afterScheme;
            }
        } else {
            return "{\"Success\":false, \"StatusCode\":400, \"Body\":\"Invalid URL Scheme\"}";
        }

        auto cli = std::make_unique<httplib::Client>(scheme + host);
        cli->set_follow_location(true);
        cli->set_connection_timeout(10);
        cli->set_read_timeout(15);

        httplib::Headers headers;
        if (settings.contains("Headers")) {
            for (auto& [k, v] : settings["Headers"].items()) {
                if (v.is_string()) headers.insert({ k, v.get<std::string>() });
            }
        }
        
        if (headers.find("User-Agent") == headers.end()) {
            headers.insert({ "User-Agent", "CravexBase/0.1.1" });
        }

        httplib::Result res;
        if (method == "GET") {
            res = cli->Get(path.c_str(), headers);
        } else if (method == "POST") {
            res = cli->Post(path.c_str(), headers, body, settings.value("ContentType", "text/plain"));
        } else if (method == "PUT") {
            res = cli->Put(path.c_str(), headers, body, settings.value("ContentType", "text/plain"));
        } else if (method == "PATCH") {
            res = cli->Patch(path.c_str(), headers, body, settings.value("ContentType", "text/plain"));
        } else if (method == "DELETE") {
            res = cli->Delete(path.c_str(), headers);
        } else {
            return "{\"Success\":false, \"StatusCode\":405, \"Body\":\"Unsupported Method\"}";
        }

        if (res) {
            json out;
            out["StatusCode"] = res->status;
            out["Body"] = res->body;
            out["Success"] = (res->status >= 200 && res->status < 300);
            
            json h = json::object();
            for (auto& it : res->headers) {
                h[it.first] = it.second;
            }
            out["Headers"] = h;
            return out.dump();
        } else {
            auto err = res.error();
            std::string errMsg = "Unknown Error";
            if (err == httplib::Error::Connection) errMsg = "Connection Error";
            else if (err == httplib::Error::SSLServerVerification) errMsg = "SSL Verification Error";
            else if (err == httplib::Error::Read) errMsg = "Read Timeout";

            return "{\"Success\":false, \"StatusCode\":500, \"Body\":\"Request Failed: " + errMsg + "\"}";
        }
    }

    inline std::string handleIsRbxActive(const std::string& data, const json& settings, DWORD pid) {
        HWND robloxHwnd = proc::getHwnd(pid);
        return (GetForegroundWindow() == robloxHwnd) ? "true" : "false";
    }

    inline std::string handleSetClipboard(const std::string& data, const json& settings, DWORD pid) {
        if (!OpenClipboard(NULL)) return "false";
        EmptyClipboard();
        HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, data.size() + 1);
        if (!hg) {
            CloseClipboard();
            return "false";
        }
        memcpy(GlobalLock(hg), data.c_str(), data.size() + 1);
        GlobalUnlock(hg);
        SetClipboardData(CF_TEXT, hg);
        CloseClipboard();
        return "true";
    }

    inline std::string handleGetClipboard(const std::string& data, const json& settings, DWORD pid) {
        if (!OpenClipboard(NULL)) return "";
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (!hData) {
            CloseClipboard();
            return "";
        }
        char* pszText = static_cast<char*>(GlobalLock(hData));
        std::string text(pszText);
        GlobalUnlock(hData);
        CloseClipboard();
        return text;
    }

    inline void init() {
        registerBridgeMethod("compile", handleCompile);
        registerBridgeMethod("setscriptbytecode", handleSetScriptBytecode);
        registerBridgeMethod("getscriptbytecode", handleGetScriptBytecode);
        registerBridgeMethod("listen", handleListen);
        registerBridgeMethod("request", handleRequest);
        registerBridgeMethod("isrbxactive", handleIsRbxActive);
        registerBridgeMethod("setclipboard", handleSetClipboard);
        registerBridgeMethod("getclipboard", handleGetClipboard);
    }
}