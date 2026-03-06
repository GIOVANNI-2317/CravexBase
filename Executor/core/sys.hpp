#pragma once
#include <vector>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>

#define NT_SUCCESS(x) (((NTSTATUS)(x)) >= 0)

typedef NTSTATUS(NTAPI* pNtRVM)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS(NTAPI* pNtWVM)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);

inline pNtRVM fnRead = nullptr;
inline pNtWVM fnWrite = nullptr;

inline void setupSystemFunctions() {
    if (!fnRead) {
        if (HMODULE mod = GetModuleHandleW(L"ntdll.dll")) {
            fnRead = reinterpret_cast<pNtRVM>(GetProcAddress(mod, "NtReadVirtualMemory"));
            fnWrite = reinterpret_cast<pNtWVM>(GetProcAddress(mod, "NtWriteVirtualMemory"));
        }
    }
}

namespace mem {
    inline void readBytes(uintptr_t target, void* buffer, size_t len, DWORD procId) {
        setupSystemFunctions();
        if (!fnRead) return;
        if (HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId)) {
            SIZE_T outRead = 0;
            fnRead(procHandle, reinterpret_cast<PVOID>(target), buffer, len, &outRead);
            CloseHandle(procHandle);
        }
    }

    inline void writeBytes(uintptr_t target, const void* buffer, size_t len, DWORD procId) {
        setupSystemFunctions();
        if (!fnWrite) return;
        if (HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId)) {
            SIZE_T outWritten = 0;
            fnWrite(procHandle, reinterpret_cast<PVOID>(target), const_cast<PVOID>(buffer), len, &outWritten);
            CloseHandle(procHandle);
        }
    }

    template <typename T>
    inline T read(uintptr_t target, DWORD procId) {
        T result{};
        readBytes(target, &result, sizeof(T), procId);
        return result;
    }

    template <typename T>
    inline void write(uintptr_t target, T value, DWORD procId) {
        writeBytes(target, &value, sizeof(T), procId);
    }
}

namespace proc {
    inline std::vector<DWORD> getPids() {
        std::vector<DWORD> results;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W procEntry;
            procEntry.dwSize = sizeof(PROCESSENTRY32W);
            if (Process32FirstW(snapshot, &procEntry)) {
                do {
                    if (lstrcmpiW(L"RobloxPlayerBeta.exe", procEntry.szExeFile) == 0) {
                        results.push_back(procEntry.th32ProcessID);
                    }
                } while (Process32NextW(snapshot, &procEntry));
            }
            CloseHandle(snapshot);
        }
        return results;
    }

    inline uintptr_t getBase(DWORD procId) {
        uintptr_t resultBase = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
        if (snapshot != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W modEntry;
            modEntry.dwSize = sizeof(MODULEENTRY32W);
            if (Module32FirstW(snapshot, &modEntry)) {
                do {
                    if (lstrcmpiW(modEntry.szModule, L"RobloxPlayerBeta.exe") == 0) {
                        resultBase = reinterpret_cast<uintptr_t>(modEntry.modBaseAddr);
                        break;
                    }
                } while (Module32NextW(snapshot, &modEntry));
            }
            CloseHandle(snapshot);
        }
        return resultBase;
    }

    struct WindowFinderInfo {
        DWORD targetPid;
        HWND foundHwnd;
    };

    inline HWND getHwnd(DWORD procId) {
        WindowFinderInfo info = { procId, nullptr };
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto finder = reinterpret_cast<WindowFinderInfo*>(lParam);
            DWORD curPid = 0;
            GetWindowThreadProcessId(hwnd, &curPid);
            if (curPid == finder->targetPid) {
                finder->foundHwnd = hwnd;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&info));
        return info.foundHwnd;
    }
}
