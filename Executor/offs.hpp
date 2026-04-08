#pragma once
#include <Windows.h>

namespace offs {
    inline constexpr uintptr_t loadModule = 0x7CD0BA8;

    inline constexpr uintptr_t fakeDataModelPtr = 0x834A988;
    inline constexpr uintptr_t fakeDataModelToDataModel = 0x1C0;

    inline constexpr uintptr_t children = 0x78;
    inline constexpr uintptr_t childrenEnd = 0x8;
    inline constexpr uintptr_t name = 0xB0;
    inline constexpr uintptr_t value = 0xD0;

    inline constexpr uintptr_t classDesc = 0x18;
    inline constexpr uintptr_t classDescToName = 0x8;

    inline constexpr uintptr_t lscriptBc = 0x1A8;
    inline constexpr uintptr_t mscriptBc = 0x150;
    inline constexpr uintptr_t iscoreScript = 0x180; // will need for future updates maybe, but not yet
}