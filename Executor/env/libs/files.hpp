#pragma once
#include <env/bridge.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace files {

    inline fs::path getWorkspace() {
        fs::path ws = fs::current_path() / "workspace";
        if (!fs::exists(ws)) {
            fs::create_directory(ws);
        }
        return ws;
    }

    inline fs::path resolvePath(const std::string& input) {
        fs::path ws = getWorkspace();
        fs::path target = ws / input;
        
        // Simple sandbox check
        auto targetStr = target.lexically_normal().string();
        auto wsStr = ws.lexically_normal().string();
        if (targetStr.find(wsStr) != 0) {
            return ""; // Traversal attempt outside workspace
        }
        return target;
    }

    inline std::string handleReadFile(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty() || !fs::exists(target) || !fs::is_regular_file(target)) return "";
        
        std::ifstream file(target, std::ios::binary);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    inline std::string handleWriteFile(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty()) return "false";
        
        std::ofstream file(target, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return "false";
        file << data;
        return "true";
    }

    inline std::string handleAppendFile(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty()) return "false";
        
        std::ofstream file(target, std::ios::binary | std::ios::app);
        if (!file.is_open()) return "false";
        file << data;
        return "true";
    }

    inline std::string handleIsFile(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        return (target.empty() == false && fs::exists(target) && fs::is_regular_file(target)) ? "true" : "false";
    }

    inline std::string handleIsFolder(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        return (target.empty() == false && fs::exists(target) && fs::is_directory(target)) ? "true" : "false";
    }

    inline std::string handleMakeFolder(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty()) return "false";
        return fs::create_directories(target) ? "true" : "false";
    }

    inline std::string handleDelFile(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty() || !fs::is_regular_file(target)) return "false";
        return fs::remove(target) ? "true" : "false";
    }

    inline std::string handleDelFolder(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty() || !fs::is_directory(target)) return "false";
        return fs::remove_all(target) > 0 ? "true" : "false";
    }

    inline std::string handleListFiles(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty() || !fs::is_directory(target)) return "[]";
        
        json result = json::array();
        for (const auto& entry : fs::directory_iterator(target)) {
            // Return path relative to workspace to match sUNC
            fs::path rel = fs::relative(entry.path(), getWorkspace());
            std::string pathStr = rel.string();
            // Convert backslashes to forward slashes
            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
            result.push_back(pathStr);
        }
        return result.dump();
    }

    inline void init() {
        registerBridgeMethod("readfile", handleReadFile);
        registerBridgeMethod("writefile", handleWriteFile);
        registerBridgeMethod("appendfile", handleAppendFile);
        registerBridgeMethod("isfile", handleIsFile);
        registerBridgeMethod("isfolder", handleIsFolder);
        registerBridgeMethod("makefolder", handleMakeFolder);
        registerBridgeMethod("delfile", handleDelFile);
        registerBridgeMethod("delfolder", handleDelFolder);
        registerBridgeMethod("listfiles", handleListFiles);
    }
}
