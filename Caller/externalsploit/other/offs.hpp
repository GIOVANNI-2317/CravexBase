#pragma once
#include <Windows.h>

namespace globals {
    inline uintptr_t base = 0x0;
}

#define rebase(addr) (globals::base + addr)

namespace offs {
	inline constexpr uintptr_t print = 0x1D1FF80;

    // Advanced Luau Offsets for cloneref/hookfunction
    inline constexpr uintptr_t lua_gettop = 0x41A9900;
    inline constexpr uintptr_t lua_settop = 0x41AA070;
    inline constexpr uintptr_t luaL_typeerrorL = 0x27263D0;
    inline constexpr uintptr_t LuaL_checktype = 0x3DC78D0;
    inline constexpr uintptr_t luaT_objtypename = 0x42129D0;
    inline constexpr uintptr_t luaA_toobject = 0x41A9B10;
    inline constexpr uintptr_t InstancePush = 0x1C58210;
}