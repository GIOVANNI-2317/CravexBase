#pragma once
#include <env/bridge.hpp>
#include <Windows.h>
#include "../../core/sys.hpp"

namespace input {

    inline std::string handleIsRbxActive(const std::string& data, const json& settings, DWORD pid) {
        HWND robloxHwnd = proc::getHwnd(pid);
        return (GetForegroundWindow() == robloxHwnd) ? "true" : "false";
    }

    // --- Mouse APIs ---
    inline std::string handleMouse1Click(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true")
            mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        return "";
    }
    inline std::string handleMouse1Press(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        return "";
    }
    inline std::string handleMouse1Release(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        return "";
    }
    inline std::string handleMouse2Click(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true")
            mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        return "";
    }
    inline std::string handleMouse2Press(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
        return "";
    }
    inline std::string handleMouse2Release(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        return "";
    }
    inline std::string handleMouseMoveAbs(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") {
            try {
                int px = std::stoi(settings.value("x", "0"));
                int py = std::stoi(settings.value("y", "0"));
                int width = GetSystemMetrics(SM_CXSCREEN) - 1;
                int height = GetSystemMetrics(SM_CYSCREEN) - 1;
                
                HWND hwnd = proc::getHwnd(pid);
                RECT c_rect; GetClientRect(hwnd, &c_rect);
                POINT pt{ c_rect.left, c_rect.top };
                ClientToScreen(hwnd, &pt);

                int sx = (px + pt.x) * (65535 / width);
                int sy = (py + pt.y) * (65535 / height);
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, sx, sy, 0, 0);
            } catch(...) {}
        }
        return "";
    }
    inline std::string handleMouseMoveRel(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") {
            try {
                int dx = std::stoi(settings.value("dx", "0"));
                int dy = std::stoi(settings.value("dy", "0"));
                mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
            } catch(...) {}
        }
        return "";
    }
    inline std::string handleMouseScroll(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") {
            try {
                int scroll = std::stoi(data);
                mouse_event(MOUSEEVENTF_WHEEL, 0, 0, scroll, 0);
            } catch(...) {}
        }
        return "";
    }

    // --- Keyboard APIs ---
    inline std::string handleKeyPress(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") {
            try {
                UINT key = std::stoul(data);
                keybd_event(0, (BYTE)MapVirtualKeyA(key, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE, 0);
            } catch(...) {}
        }
        return "";
    }
    inline std::string handleKeyRelease(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") {
            try {
                UINT key = std::stoul(data);
                keybd_event(0, (BYTE)MapVirtualKeyA(key, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0);
            } catch(...) {}
        }
        return "";
    }
    inline std::string handleKeyTap(const std::string& data, const json& settings, DWORD pid) {
        if (handleIsRbxActive("", settings, pid) == "true") {
            try {
                UINT key = std::stoul(data);
                keybd_event(0, (BYTE)MapVirtualKeyA(key, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE, 0);
                keybd_event(0, (BYTE)MapVirtualKeyA(key, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0);
            } catch(...) {}
        }
        return "";
    }
    inline std::string handleMessageBox(const std::string& data, const json& settings, DWORD pid) {
        std::string title = settings.value("t", "Message");
        int type = std::stoi(settings.value("type", "0"));
        MessageBoxA(NULL, data.c_str(), title.c_str(), type);
        return "";
    }

    inline void init() {
        registerBridgeMethod("isrbxactive", handleIsRbxActive);
        
        // Input APIs
        registerBridgeMethod("mouse1click", handleMouse1Click);
        registerBridgeMethod("mouse1press", handleMouse1Press);
        registerBridgeMethod("mouse1release", handleMouse1Release);
        registerBridgeMethod("mouse2click", handleMouse2Click);
        registerBridgeMethod("mouse2press", handleMouse2Press);
        registerBridgeMethod("mouse2release", handleMouse2Release);
        registerBridgeMethod("mousemoveabs", handleMouseMoveAbs);
        registerBridgeMethod("mousemoverel", handleMouseMoveRel);
        registerBridgeMethod("mousescroll", handleMouseScroll);

        registerBridgeMethod("keypress", handleKeyPress);
        registerBridgeMethod("keyrelease", handleKeyRelease);
        registerBridgeMethod("keytap", handleKeyTap);
        registerBridgeMethod("messagebox", handleMessageBox);
    }
}