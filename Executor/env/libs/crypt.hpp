#pragma once
#include <lz4.h>
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <string>
#include "../../deps/server/base64.hpp"

using json = nlohmann::json;

namespace crypt {
    inline std::string handleBase64Encode(const std::string& data, const json& settings, DWORD pid) {
        return base64::to_base64(data);
    }

    inline std::string handleBase64Decode(const std::string& data, const json& settings, DWORD pid) {
        try {
            return base64::from_base64(data);
        }
        catch (...) {
            return "";
        }
    }

    inline std::string handleLz4Compress(const std::string& data, const json& settings, DWORD pid) {
        if (data.empty()) return "";
        
        int inputSize = (int)data.size();
        
        // LZ4_compressBound gives the maximum possible size for an incompressible block
        int maxCompressedSize = LZ4_compressBound(inputSize);
        std::string compressed;
        compressed.resize(maxCompressedSize);

        // Perform real LZ4 compression
        int compressedSize = LZ4_compress_default(data.data(), compressed.data(), inputSize, maxCompressedSize);
        if (compressedSize <= 0) return "";
        
        // Header: [4 bytes original size] + [compressed data]
        // This is standard for raw block decompression
        std::string result;
        result.resize(4 + compressedSize);
        uint32_t sz = (uint32_t)inputSize;
        memcpy(result.data(), &sz, 4);
        memcpy(result.data() + 4, compressed.data(), compressedSize);

        // Return as base64 to ensure binary safety (no null byte truncation)
        return base64::to_base64(result);
    }

    inline std::string handleLz4Decompress(const std::string& data, const json& settings, DWORD pid) {
        // Data comes in as Base64 from the bridge
        std::string decoded;
        try {
            decoded = base64::from_base64(data);
        } catch(...) { return ""; }

        if (decoded.size() < 4) return "";

        // Read the 4-byte original size header
        uint32_t originalSize = 0;
        memcpy(&originalSize, decoded.data(), 4);
        
        if (originalSize == 0 || originalSize > 100 * 1024 * 1024) return ""; // 100MB limit for safety

        std::string out;
        out.resize(originalSize);

        // Decompress the raw LZ4 block starting after the 4-byte header
        int decompressedSize = LZ4_decompress_safe(decoded.data() + 4, out.data(), (int)decoded.size() - 4, (int)originalSize);
        if (decompressedSize < 0) return "";

        // Return the final result as Base64 so Lua side can reliably decode it
        return base64::to_base64(out);
    }

    inline void init() {
        registerBridgeMethod("crypt.base64encode", handleBase64Encode);
        registerBridgeMethod("crypt.base64decode", handleBase64Decode);
        registerBridgeMethod("crypt.lz4compress", handleLz4Compress);
        registerBridgeMethod("crypt.lz4decompress", handleLz4Decompress);
    }
}