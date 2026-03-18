#include "env/bridge.hpp"
#include "env/env.hpp"
#include <thread>
#include <iostream>
#include <set>

static std::set<DWORD> attachedPids;
static bool shouldAttach = true;

std::string getResource(int id, DWORD pid) {
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

void mainLogic() {
    std::thread(startBridge).detach();
    while (true) {
        if (!shouldAttach) {
            Sleep(1000);
            continue;
        }
        for (DWORD pid : proc::getPids()) {
            if (attachedPids.count(pid)) continue;
            uintptr_t base = proc::getBase(pid);
            inst dm = getDm(base, pid);

            size_t sz;
            std::string l = getResource(1, pid);
            auto bc = code::sign(code::compile(l), sz);

            attachedPids.insert(pid);
			std::thread(tpHandler::handlerStart, pid, base, bc, sz).detach();
        }
        Sleep(1000);
    }
}

extern "C" __declspec(dllexport) bool isAttached() {
    if (attachedPids.empty()) return false;
    for (DWORD pid : attachedPids) {
        uintptr_t base = proc::getBase(pid);
        if (!base) continue;
        inst dm = getDm(base, pid);
        if (!dm.getAddr()) continue;
        inst cg = dm.findChild("CoreGui");
        if (cg.getAddr() && cg.findChild("CravexBase").getAddr() != 0) {
            return true;
        }
    }
    return false;
}

extern "C" __declspec(dllexport) void attach(bool debug) {
    shouldAttach = true;
    static bool init = false;
    if (!init) {
        init = true;
        if (debug) {
            AllocConsole();
            FILE* f;
            freopen_s(&f, "CONOUT$", "w", stdout);
        }
        std::thread(mainLogic).detach();
    }
}

extern "C" __declspec(dllexport) void execute(const wchar_t* input) {
    int sz = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
    std::string s(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, input, -1, &s[0], sz, NULL, NULL);
    if (!s.empty() && s.back() == '\0') {
        s.pop_back();
    }
    if (s.length() > 0) executeScript(s);
}

extern "C" __declspec(dllexport) void detach() {
    execute(L"_DETACH_");
    attachedPids.clear();
    shouldAttach = false;
}