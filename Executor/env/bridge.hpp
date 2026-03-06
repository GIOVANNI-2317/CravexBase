#pragma once
void registerAllEnvFunctions();
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../deps/server/httplib.h"
#include "../deps/server/nlohmann/json.hpp"
#include "../core/sys.hpp"
#include "../core/inst.hpp"
#include "../core/code.hpp"
#include <unordered_map>
#include <sstream>
#include <regex>
#include <functional>

using json = nlohmann::json;
using namespace httplib;

inline std::string globalSrc = "";
inline uintptr_t globalOrd = 0;
inline std::unordered_map<DWORD, uintptr_t> processOrders;

inline inst getPtr(std::string name, DWORD pid) {
    uintptr_t base = proc::getBase(pid);
    inst dm = getDm(base, pid);
    inst cg = dm.findChild("CoreGui");
    inst ex = cg.findChild("CravexBase");
    inst pt = ex.findChild("Pointer");
    inst p = pt.findChild(name);
    uintptr_t t = mem::read<uintptr_t>(p.getAddr() + offs::value, pid);
    return inst(t, pid);
}

// Define the handler type for clean registration
using BridgeHandler = std::function<std::string(const std::string& data, const json& settings, DWORD pid)>;
inline std::unordered_map<std::string, BridgeHandler> bridgeHandlers;

// Utility function to make adding new functions very easy
inline void registerBridgeMethod(const std::string& methodName, BridgeHandler handler) {
    bridgeHandlers[methodName] = handler;
}

inline std::string handleRequest(const std::string& bodyData) {
    std::stringstream stream(bodyData);
    std::string reqType, pidString, settingsString, textData, line;
    
    std::getline(stream, reqType); 
    std::getline(stream, pidString); 
    std::getline(stream, settingsString);
    
    while (std::getline(stream, line)) {
        textData += line + "\n";
    }
    if (!textData.empty()) {
        textData.pop_back();
    }
    
    DWORD procId = std::stoul(pidString);
    json parsedSettings = json::parse(settingsString);
    
    if (bridgeHandlers.find(reqType) != bridgeHandlers.end()) {
        return bridgeHandlers[reqType](textData, parsedSettings, procId);
    }
    
    return "";
}

inline void startBridge() {
    registerAllEnvFunctions();
    Server srv;
    srv.Post("/handle", [](const Request& req, Response& res) {
        res.set_content(handleRequest(req.body), "text/plain");
    });
    srv.listen("localhost", 6767);
}

inline void executeScript(std::string src) {
    globalSrc = src;
    globalOrd++;
}
