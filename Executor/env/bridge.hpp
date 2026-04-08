#pragma once
void registerAllEnvFunctions();
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../deps/server/httplib.h"
#include "../deps/server/nlohmann/json.hpp"
#include "../core/sys.hpp"
#include "../core/inst.hpp"
#include "../core/code.hpp"
#include "../core/code.hpp"
#include <unordered_map>
#include <sstream>
#include "util.hpp"
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
    inst ex = cg.findChild("Cravex");
    inst pt = ex.findChild("Pointer");
    inst p = pt.findChild(name);
    uintptr_t t = mem::read<uintptr_t>(p.getAddr() + offs::value, pid);
    return inst(t, pid);
}

// Handler type
using BridgeHandler = std::function<std::string(const std::string& data, const json& settings, DWORD pid)>;
inline std::unordered_map<std::string, BridgeHandler> bridgeHandlers;

// Register new methods
inline void registerBridgeMethod(const std::string& methodName, BridgeHandler handler) {
    bridgeHandlers[methodName] = handler;
}

inline std::string getResource(int id, DWORD pid) {
    HMODULE h = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&getResource, &h);
    HRSRC res = FindResourceA(h, MAKEINTRESOURCEA(id), MAKEINTRESOURCEA(10));
    if (!res) return "";
    HGLOBAL load = LoadResource(h, res);
    if (!load) return "";
    void* data = LockResource(load);
    if (!data) return "";
    DWORD sz = SizeofResource(h, res);
    std::string s((char*)data, sz);
    size_t p = s.find("-$-crvx-procid-$-");
    if (p != std::string::npos) s.replace(p, 17, std::to_string(pid));
    return s;
}

inline std::string handleRequest(const std::string& bodyData) {
    std::stringstream stream(bodyData);
    std::string reqType, pidString, settingsString;

    std::getline(stream, reqType);
    std::getline(stream, pidString);
    std::getline(stream, settingsString);

    std::string textData((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

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