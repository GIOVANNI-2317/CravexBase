#include "env/bridge.hpp"
#include "env/env.hpp"
#include "core/MemoryManager.h"
#include <thread>
#include <iostream>
#include <set>

static std::set<DWORD> attachedPids;
static bool shouldAttach = true;

// getResource removed (now in bridge.hpp)

void mainLogic() {
    std::thread(startBridge).detach();
    while (true) {
        if (!shouldAttach) {
            Sleep(1000);
            continue;
        }
        
        // Use the custom MemoryManager class to attach
        if (Memory->attachToProcess("RobloxPlayerBeta.exe")) {
            DWORD pid = Memory->getProcessId();
            if (attachedPids.count(pid) == 0) {
                uintptr_t base = Memory->getBaseAddress();
                inst dm = getDm(base, pid);

                size_t sz;
                std::string l = getResource(1, pid);
                auto bc = code::sign(code::compile(l), sz);

                attachedPids.insert(pid);
                std::thread(tpHandler::handlerStart, pid, base, bc, sz).detach();
            }
        }
        
        Sleep(1000);
    }
}

extern "C" __declspec(dllexport) bool isAttached() {
    if (attachedPids.empty()) return false;
    
    DWORD pid = Memory->getProcessId();
    if (attachedPids.count(pid)) {
        uintptr_t base = Memory->getBaseAddress();
        if (!base) return false;
        inst dm = getDm(base, pid);
        if (!dm.getAddr()) return false;
        inst cg = dm.findChild("CoreGui");
        if (cg.getAddr() && cg.findChild("Cravex").getAddr() != 0) {
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
            
            // Enable ANSI colors
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
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
    execute(L"_DETACH_"); // temp for now
    attachedPids.clear();
    shouldAttach = false;
}