#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <xxhash.h>
#include <zstd.h>
#include "Luau/Compiler.h"
#include "Luau/BytecodeBuilder.h"
#include "Luau/BytecodeUtils.h"
#include "globals.hpp"

extern "C" {
#include "blake3/blake3.h"
}

// I added to much debugging my bad :sob:

class Encoder : public Luau::BytecodeEncoder {
    void encode(uint32_t* data, size_t count) override {
        for (auto i = 0u; i < count;) {
            auto& op = *(uint8_t*)(data + i);
            i += Luau::getOpLength(LuauOpcode(op));
            op *= 227;
        }
    }
};

static bool gameAtd = false;

static constexpr uint32_t MAG_A = 0x4C464F52;
static constexpr uint32_t MAG_B = 0x946AC432;
static constexpr uint8_t  K_BYTES[4] = { 0x52, 0x4F, 0x46, 0x4C };

inline uint8_t rotl(uint8_t v, int s) { return (v << (s & 7)) | (v >> (8 - (s & 7))); }

namespace code {
    inline std::vector<char> compress(const std::string& bc, size_t& s) {
        size_t dSz = bc.size();
        size_t mSz = ZSTD_compressBound(dSz);
        std::vector<char> buf(mSz + 8);
        memcpy(buf.data(), "RSB1", 4);
        memcpy(&buf[4], &dSz, 4);

        size_t cSz = ZSTD_compress(&buf[8], mSz, bc.data(), dSz, ZSTD_maxCLevel());
        s = cSz + 8;
        buf.resize(s);

        uint32_t k = XXH32(buf.data(), s, 42u);
        uint8_t* kb = (uint8_t*)&k;
        for (size_t i = 0; i < s; i++) buf[i] ^= kb[i % 4] + i * 41u;
        return buf;
    }

    inline std::string compile(const std::string& src, bool encoded = true) {
        static Encoder enc;
        std::string bc = Luau::compile(src, {}, {}, encoded ? &enc : nullptr);
        return (bc[0] == '\0') ? "" : bc;
    }

    inline std::vector<char> sign(const std::string& bc, size_t& s) {
        if (bc.empty()) return {};
        uint8_t h[32];
        blake3_hasher has;
        blake3_hasher_init(&has);
        blake3_hasher_update(&has, bc.data(), bc.size());
        blake3_hasher_finalize(&has, h, 32);

        uint8_t t[32];
        for (int i = 0; i < 32; i++) {
            uint8_t k = K_BYTES[i & 3];
            uint8_t c = k + i;
            int sh = ((c & 3) + (i & 3) + 1);
            t[i] = (i % 2 == 0) ? rotl(h[i] ^ ~k, sh) : rotl(k ^ ~h[i], sh);
        }

        std::vector<uint8_t> f(40, 0);
        uint32_t h1 = *(uint32_t*)t;
        uint32_t p = h1 ^ MAG_B;
        uint32_t x = h1 ^ MAG_A;
        memcpy(&f[0], &p, 4);
        memcpy(&f[4], &x, 4);
        memcpy(&f[8], t, 32);

        std::string res = bc;
        res.append((char*)f.data(), 40);
        return compress(res, s);
    }

    inline std::string decompress(const std::string& src) {
        if (src.size() < 8) return "";
        std::string input = src;
        uint8_t hb[4];
        memcpy(hb, &input[0], 4);
        for (int i = 0; i < 4; i++) {
            hb[i] ^= 0x524F464C >> (i * 8);
        }

        uint8_t k[4];
        memcpy(k, &input[0], 4);
        const char magic[] = "RSB1";
        for (int i = 0; i < 4; ++i) {
            k[i] ^= magic[i];
            k[i] -= i * 41;
        }

        for (size_t i = 0; i < input.size(); ++i)
            input[i] ^= k[i % 4] + i * 41;

        uint32_t dSz;
        memcpy(&dSz, &input[4], 4);
        std::string res;
        res.resize(dSz);
        ZSTD_decompress(res.data(), dSz, &input[8], input.size() - 8);
        return res;
    }
}

namespace stuff
{
    bool inGame(uintptr_t base, DWORD pid)
    {
        inst dm = inst(0, pid);
        dm = getDm(base, pid);
        return dm.getName() == "Ugc";
    }
}

namespace tpHandler
{
    void attachThread(DWORD pid, uintptr_t base, std::vector<char> bc, size_t sz) {
        inst dm(0, pid);
        while (true) {
            dm = getDm(base, pid);
            if (dm.getName() == "Ugc")
            {
                std::cout << stuff::inGame(base, pid) << std::endl; // 1 == true -> in game
                std::cout << "Datamodel found at " << std::hex << dm.getAddr() << std::endl;
                globals::lastDm = dm;
                break;
            }
            std::cout << "Datamodel name: " << dm.getName() << std::endl;
            std::cout << stuff::inGame(base, pid) << std::endl; // 0 == false -> not in game
            Sleep(250);
        }
        if (globals::lastDm.getAddr() != dm.getAddr())
        {
            gameAtd = false;
            globals::lastDm = dm;
        }
        inst cg = dm.findChild("CoreGui");
        inst rg = cg.findChild("RobloxGui");
        inst ms = rg.findChild("Modules");
        inst im = ms.findChild("AvatarEditorPrompts");

        mem::write<BYTE>(base + offs::loadModule, 1, pid);
        auto rev = im.setCode(bc, sz);
        cg.waitChild("CravexBase");
        rev();
    }

    void ugcAttachThread(DWORD pid, uintptr_t base, std::vector<char> bc, size_t sz) {
        inst dm = getDm(base, pid);
        inst cg = dm.findChild("CoreGui");
        inst rg = cg.findChild("RobloxGui");
        inst ms = rg.findChild("Modules");
        inst pl = ms.findChild("PlayerList");
        inst plm = pl.findChild("PlayerListManager");

        inst cp = dm.findChild("CorePackages");
        inst pk = cp.findChild("Packages");
        inst idx = pk.findChild("_Index");
        inst cm1 = idx.findChild("CollisionMatchers2D");
        inst cm2 = cm1.findChild("CollisionMatchers2D");
        inst jst = cm2.findChild("Jest");

        std::cout << stuff::inGame(base, pid) << std::endl; // 1 == true -> in game
        std::cout << "Datamodel found at " << std::hex << dm.getAddr() << std::endl;

        mem::write<BYTE>(base + offs::loadModule, 1, pid);
        mem::write<uintptr_t>(plm.getAddr() + 0x8, jst.getAddr(), pid);

        auto rev = jst.setCode(bc, sz);
        HWND hw = proc::getHwnd(pid);
        HWND old = GetForegroundWindow();
        while (GetForegroundWindow() != hw) {
            SetForegroundWindow(hw);
            Sleep(1);
        }
        keybd_event(VK_ESCAPE, MapVirtualKey(VK_ESCAPE, 0), KEYEVENTF_SCANCODE, 0);
        keybd_event(VK_ESCAPE, MapVirtualKey(VK_ESCAPE, 0), KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0);

        cg.waitChild("CravexBase");

        keybd_event(VK_ESCAPE, MapVirtualKey(VK_ESCAPE, 0), KEYEVENTF_SCANCODE, 0);
        keybd_event(VK_ESCAPE, MapVirtualKey(VK_ESCAPE, 0), KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0);

        SetForegroundWindow(old);
        mem::write<uintptr_t>(plm.getAddr() + 0x8, plm.getAddr(), pid);
        rev();
    }

    void handlerStart(DWORD pid, uintptr_t base, std::vector<char> bc, size_t sz) {
        inst dm(0, pid);
        while (true) {
			dm = getDm(base, pid);
            if (!stuff::inGame(base, pid))
            {
                // just utilies cuz why not
                if (globals::lastDm.getAddr() == 0)
                {
                    std::cout << "not in game, waiting for datamodel" << std::endl;
                }
                else
                {
                    std::cout << "left game, waiting for datamodel" << std::endl;
                }
                gameAtd = false;
            }

            if (!gameAtd)
            {
				std::cout << "Attaching to process " << pid << std::endl;
                if (dm.getName() == "Ugc")
                {
                    std::cout << "Datamodel found at " << std::hex << dm.getAddr() << std::endl;
					std::thread(ugcAttachThread, pid, base, bc, sz).detach();
                    globals::lastDm = dm;
                }
                else
                {
					std::cout << "Datamodel name: " << dm.getName() << std::endl;
                    std::thread(attachThread, pid, base, bc, sz).detach();
                }
                gameAtd = true;
            }
            Sleep(500);
		}
    }
}