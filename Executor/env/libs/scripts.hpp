#pragma once
#include <env/bridge.hpp>
#include "../../core/inst.hpp"
#include "../../core/code.hpp"

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

        // Bytecode string object is at bcPtr + 0x10
        uintptr_t strObj = bcPtr + 0x10;
        size_t sz = mem::read<size_t>(strObj + 0x10, pid); // length at +0x10
        size_t cap = mem::read<size_t>(strObj + 0x18, pid); // capacity at +0x18

        uintptr_t dataPtr = (cap > 15) ? mem::read<uintptr_t>(strObj, pid) : strObj;

        if (!dataPtr || !sz) return "null:no_data_ptr:" + className;

        std::string buffer;
        buffer.resize(sz);
        mem::readBytes(dataPtr, buffer.data(), sz, pid);

        // Decompress
        return code::decompress(buffer);
    }

    inline void init() {
        registerBridgeMethod("compile", handleCompile);
        registerBridgeMethod("setscriptbytecode", handleSetScriptBytecode);
        registerBridgeMethod("getscriptbytecode", handleGetScriptBytecode);
    }
}