#pragma once
#include <env/bridge.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <windows.h>

namespace random {
    inline std::string handlePrintinfo(const std::string& data, const json& settings, DWORD pid) {
        // 1. Find the directory of the currently running DLL (cravex.dll)
        char dllPath[MAX_PATH];
        HMODULE hMod = GetModuleHandleA("cravex.dll");
        if (!hMod) {
            // Fallback if handle not found by name
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&handlePrintinfo, &hMod);
        }
        
        GetModuleFileNameA(hMod, dllPath, MAX_PATH);
        std::filesystem::path p(dllPath);
        std::filesystem::path dir = p.parent_path();
        
        // 2. Build the path to Caller.exe in the SAME directory
        std::filesystem::path callerPath = dir / "Caller.exe";

        if (!std::filesystem::exists(callerPath)) {
             std::cout << " \033[1;31m[!]\033[0m Error: Caller.exe not found at: " << callerPath.string() << std::endl;
             return "false";
        }

        // 3. Construct the command for system()
        // We use "start /B" to run in background without creating a visible window
        // Quotes are tricky with system(), so we wrap the entire command.
        std::string command = "start /B \"\" \"" + callerPath.string() + "\" print info \"" + data + "\"";

        // 4. Execute using system()
        system(command.c_str());
        
        return "true";
    }

    inline void init() {
        registerBridgeMethod("printinfo", handlePrintinfo);
    }
}