#pragma once
#include "bridge.hpp"
#include "libs/misc.hpp"
#include "libs/cache.hpp"
#include "libs/closure.hpp"
#include "libs/crypt.hpp"
#include "libs/debug.hpp"
#include "libs/drawing.hpp"
#include "libs/files.hpp"
#include "libs/input.hpp"
#include "libs/instances.hpp"
#include "libs/metatable.hpp"
#include "libs/scripts.hpp"
#include "libs/utils.hpp"
#include "libs/websockets.hpp"

inline void registerAllEnvFunctions() {
    msc::init();
    cache::init();
    closure::init();
    crypt::init();
    dbg::init();
    drawing::init();
    files::init();
    input::init();
    instances::init();
    metatable::init();
    scripts::init();
    utils::init();
    ws::init();
}