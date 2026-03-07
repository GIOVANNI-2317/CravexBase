#pragma once
#include <Windows.h>
#include <functional>
#include <string>
#include <vector>
#include "sys.hpp"
#include "../offs.hpp"

class inst {
private:
    uintptr_t addr;
    DWORD pid;
public:
    inst(uintptr_t a, DWORD p) : addr(a), pid(p) {}

    uintptr_t getAddr() { return addr; }

    std::string getName() {
        uintptr_t nPtr = mem::read<uintptr_t>(addr + offs::name, pid);
        size_t sz = mem::read<size_t>(nPtr + 0x10, pid);
        if (sz >= 16) nPtr = mem::read<uintptr_t>(nPtr, pid);
        std::string s;
        char c = 0;
        for (int i = 0; (c = mem::read<char>(nPtr + i, pid)) != 0; i++) s += c;
        return s;
    }

    inst findChild(std::string name) {
        uintptr_t cPtr = mem::read<uintptr_t>(addr + offs::children, pid);
        if (!cPtr) return inst(0, pid);
        uintptr_t start = mem::read<uintptr_t>(cPtr, pid);
        uintptr_t end = mem::read<uintptr_t>(cPtr + offs::childrenEnd, pid);
        for (uintptr_t ch = start; ch < end; ch += 0x10) {
            uintptr_t p = mem::read<uintptr_t>(ch, pid);
            if (p) {
                inst i(p, pid);
                if (i.getName() == name) return i;
            }
        }
        return inst(0, pid);
    }

    inst waitChild(std::string name) {
        inst i = findChild(name);
        while (!i.getAddr()) { Sleep(10); i = findChild(name); }
        return i;
    }

    std::string getClass() {
        uintptr_t dPtr = mem::read<uintptr_t>(addr + offs::classDesc, pid);
        uintptr_t nPtr = mem::read<uintptr_t>(dPtr + offs::classDescToName, pid);
        size_t sz = mem::read<size_t>(nPtr + 0x10, pid);
        if (sz >= 16) nPtr = mem::read<uintptr_t>(nPtr, pid);
        std::string s;
        char c = 0;
        for (int i = 0; (c = mem::read<char>(nPtr + i, pid)) != 0; i++) s += c;
        return s;
    }

    std::function<void()> setCode(const std::vector<char>& bc, size_t sz) {
        uintptr_t o = (getClass() == "LocalScript") ? offs::lscriptBc : offs::mscriptBc;
        uintptr_t emb = mem::read<uintptr_t>(addr + o, pid);
        uintptr_t oldPtr = mem::read<uintptr_t>(emb + 0x10, pid);
        uint64_t oldSz = mem::read<uint64_t>(emb + 0x20, pid);

        HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        void* nMem = VirtualAllocEx(h, nullptr, sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!nMem) { CloseHandle(h); return []() {}; }

        mem::writeBytes((uintptr_t)nMem, bc.data(), bc.size(), pid);
        mem::write<uintptr_t>(emb + 0x10, (uintptr_t)nMem, pid);
        mem::write<uint64_t>(emb + 0x20, (uint64_t)sz, pid);
        CloseHandle(h);

        return [=]() {
            mem::write<uintptr_t>(emb + 0x10, oldPtr, pid);
            mem::write<uint64_t>(emb + 0x20, oldSz, pid);
            HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
            VirtualFreeEx(h, nMem, 0, MEM_RELEASE);
            CloseHandle(h);
            };
    }
};

inline inst getDm(uintptr_t base, DWORD pid) {
    uintptr_t f = mem::read<uintptr_t>(base + offs::fakeDataModelPtr, pid);
    uintptr_t r = mem::read<uintptr_t>(f + offs::fakeDataModelToDataModel, pid);
    return inst(r, pid);
}