#pragma once
#include <env/bridge.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "../deps/server/base64.hpp"

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
        std::string out = buffer.str();
        if (settings.value("b64", false)) {
            return base64::to_base64(out);
        }
        return out;
    }

    inline std::string handleWriteFile(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty()) return "false";

        std::string pathStr = target.string();
        pathStr.erase(pathStr.find_last_not_of(" \n\r\t\0") + 1);
        std::string lowerPath = pathStr;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

        const std::vector<std::string> blocked = { ".7z", ".ade", ".adp", ".apk", ".appx", ".appxbundle", ".application", ".bat", ".cab", ".chm", ".cmd", ".com", ".cpl", ".csh", ".dll", ".dmg", ".docm", ".drv", ".exe", ".gadget", ".gz", ".hta", ".img", ".inf", ".ins", ".isp", ".iso", ".jar", ".js", ".jse", ".ksh", ".lib", ".lnk", ".mde", ".msc", ".msh", ".msh1", ".msh2", ".mshxml", ".msi", ".msp", ".mst", ".nsh", ".ocx", ".php", ".pif", ".pl", ".pptm", ".ps", ".ps1", ".ps1xml", ".ps2", ".ps2xml", ".psc1", ".psc2", ".psd1", ".psm1", ".py", ".pyw", ".rar", ".rb", ".rbw", ".reg", ".scf", ".scr", ".sct", ".sh", ".shb", ".sys", ".tar", ".url", ".vb", ".vbe", ".vbs", ".vxd", ".ws", ".wsc", ".wsf", ".wsh", ".xlsm", ".xml", ".zip" };
        for (const auto& bExt : blocked) {
            if (lowerPath.size() >= bExt.size() && lowerPath.compare(lowerPath.size() - bExt.size(), bExt.size(), bExt) == 0) return "false";
        }

        if (target.has_parent_path()) {
            fs::create_directories(target.parent_path());
        }

        std::ofstream file(target, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return "false";
        
        std::string writeData = data;
        if (settings.value("b64", false)) {
            try { writeData = base64::from_base64(data); } catch(...) {}
        }
        file.write(writeData.data(), writeData.size());
        return "true";
    }

    inline std::string handleAppendFile(const std::string& data, const json& settings, DWORD pid) {
        fs::path target = resolvePath(settings.value("f", ""));
        if (target.empty()) return "false";

        std::string pathStr = target.string();
        pathStr.erase(pathStr.find_last_not_of(" \n\r\t\0") + 1);
        std::string lowerPath = pathStr;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

        const std::vector<std::string> blocked = { ".7z", ".ade", ".adp", ".apk", ".appx", ".appxbundle", ".application", ".bat", ".cab", ".chm", ".cmd", ".com", ".cpl", ".csh", ".dll", ".dmg", ".docm", ".drv", ".exe", ".gadget", ".gz", ".hta", ".img", ".inf", ".ins", ".isp", ".iso", ".jar", ".js", ".jse", ".ksh", ".lib", ".lnk", ".mde", ".msc", ".msh", ".msh1", ".msh2", ".mshxml", ".msi", ".msp", ".mst", ".nsh", ".ocx", ".php", ".pif", ".pl", ".pptm", ".ps", ".ps1", ".ps1xml", ".ps2", ".ps2xml", ".psc1", ".psc2", ".psd1", ".psm1", ".py", ".pyw", ".rar", ".rb", ".rbw", ".reg", ".scf", ".scr", ".sct", ".sh", ".shb", ".sys", ".tar", ".url", ".vb", ".vbe", ".vbs", ".vxd", ".ws", ".wsc", ".wsf", ".wsh", ".xlsm", ".xml", ".zip" };
        for (const auto& bExt : blocked) {
            if (lowerPath.size() >= bExt.size() && lowerPath.compare(lowerPath.size() - bExt.size(), bExt.size(), bExt) == 0) return "false";
        }

        if (target.has_parent_path()) {
            fs::create_directories(target.parent_path());
        }

        std::ofstream file(target, std::ios::binary | std::ios::app);
        if (!file.is_open()) return "false";
        
        std::string writeData = data;
        if (settings.value("b64", false)) {
            try { writeData = base64::from_base64(data); } catch(...) {}
        }
        file.write(writeData.data(), writeData.size());
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