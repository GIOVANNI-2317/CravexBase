#pragma once
#include <env/bridge.hpp>
#include "../../core/inst.hpp"
#include "../../core/code.hpp"
#include "decompiler.hpp"

namespace scripts {

    inline std::string handleCompile(const std::string& data, const json& settings, DWORD pid) {
        return code::compile(data, settings.value("enc", "true") == "true");
    }

    inline std::string handleSetScriptBytecode(const std::string& data, const json& settings, DWORD pid) {
        size_t outSize;
        std::vector<char> signedCode = code::sign(data, outSize);
        inst targetScript = getPtr(settings.value("cn", ""), pid);
        targetScript.setCode(signedCode, outSize);
        return std::string("");
    }

    inline std::string handleGetScriptBytecode(const std::string& data, const json& settings, DWORD pid) {
        inst targetScript = getPtr(settings.value("cn", ""), pid);
        if (!targetScript.getAddr()) return "null:invalid_ptr";

        uintptr_t bcPtr = 0;
        std::string className = targetScript.getClass();

        if (className == "LocalScript" || className == "Script") {
            bcPtr = mem::read<uintptr_t>(targetScript.getAddr() + offs::lscriptBc, pid);
        }
        else if (className == "ModuleScript") {
            bcPtr = mem::read<uintptr_t>(targetScript.getAddr() + offs::mscriptBc, pid);
        }
        else {
            return "null:unsupported_class:" + className;
        }

        if (!bcPtr) return "null:no_bc_ptr:" + className;

        // Try reading as a string object first (offset 0x10 for MSVC std::string)
        uintptr_t strObj = bcPtr + 0x10;
        size_t sz = mem::read<size_t>(strObj + 0x10, pid); 
        size_t cap = mem::read<size_t>(strObj + 0x18, pid);

        uintptr_t dataPtr = 0;

        // Validation: if sz or cap are unrealistically large or zero, it might not be a string object
        if (sz > 0 && sz < 10000000 && cap >= sz && cap < 20000000) {
            dataPtr = (cap > 15) ? mem::read<uintptr_t>(strObj, pid) : strObj;
        } else {
            // Fallback: It might be a direct pointer + size structure (Often seen in some Roblox versions)
            // Some versions have Pointer at bcPtr + 0x0 and Size at bcPtr + 0x8
            dataPtr = mem::read<uintptr_t>(bcPtr, pid);
            sz = mem::read<size_t>(bcPtr + 0x8, pid);
            
            if (!dataPtr || sz == 0 || sz > 10000000) return "null:no_data_ptr:" + className;
        }

        std::string buffer;
        buffer.resize(sz);
        mem::readBytes(dataPtr, buffer.data(), sz, pid);

        // Decompress
        return code::decompress(buffer);
    }

    inline std::string handleDecompile(const std::string& data, const json& settings, DWORD pid) {
        static decompiler::LuauDecompiler ldec;
        return ldec.decompile(data, false);
    }

    inline std::string handleDisassemble(const std::string& data, const json& settings, DWORD pid) {
        static decompiler::LuauDecompiler ldec;
        return ldec.decompile(data, true);
    }

    inline std::string handleGetInitBytecode(const std::string& data, const json& settings, DWORD pid) {
        std::string src = getResource(1, pid);
        std::string bc = code::compile(src);
        return util::base64_encode(bc);
    }

    inline void init() {
        registerBridgeMethod("compile", handleCompile);
        registerBridgeMethod("setscriptbytecode", handleSetScriptBytecode);
        registerBridgeMethod("getscriptbytecode", handleGetScriptBytecode);
        registerBridgeMethod("decompile", handleDecompile);
        registerBridgeMethod("disassemble", handleDisassemble);
        registerBridgeMethod("getinitbytecode", handleGetInitBytecode);
    }
}