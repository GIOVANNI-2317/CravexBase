# Cravex External Executor Base

Welcome to the **Cravex External Executor Base**, a clean, simple, and open-source foundation for building external executors for Roblox Player on Windows. 

This project is tailored for developers who want a bare-bones, highly readable, and extensible C++ codebase without wading through messy, monolithic designs. It acts as a perfect canvas to build out your own external UI and exploit logic cleanly.

It will need some dependencies of vcpkg. Install vcpkg if you don't have and run this following command:
```cmd
vcpkg install openssl xxhash zstd nlohmann-json cpp-httplib lz4:x64-windows
```

## 🛠️ Modifying the Executor Details

To change the executor's name and version returned in Roblox environment checks (like `identifyexecutor` or `getexecutorname`), simply open the payload in **`data/init.lua`**.

Find the following lines and adjust them to your executor's name:

```lua
env.identifyexecutor = function() return "YourExecutorName", "1.0.0" end
env.getexecutorname = env.identifyexecutor
```

---

## 📚 Adding Functions (Extensibility)

Cravex is designed to easily mimic or implement the full spectrum of standard executor APIs (such as the sUNC specification). Adding new functions to your environment is incredibly straightforward.

1. **Navigate to the Library Folder**: Open `Executor/env/libs/`.
2. **Find or Create a Header**: Pick the relevant header for your function category (e.g., `network.hpp`, `drawing.hpp`, or `misc.hpp`).
3. **Write the C++ Logic**: Define a new `handleFunc` and implement its logic.
4. **Register the Function**: In the same file's `init()` function, register it to the bridge using `registerBridgeMethod("yourluafunc", handleFunc)`.
5. **Add to Lua Payload**: Expose the function in `data/init.lua` so the Roblox environment can call it over the bridge!

*   **`misc.hpp`** - Miscellaneous environment functions (compilation, bytecode manipulation).
*   **`utils.hpp`** - General utility functions and string manipulations.
*   **`cache.hpp`** - Implementation of custom caching functions (`cloneref`, `invalidate`, `iscached`).
*   **`closure.hpp`** - Hooking, newcclosure, checkcaller implementations.
*   **`crypt.hpp`** - Cryptography and hashing systems (Base64, SHA integrations).
*   **`debug.hpp`** - Debug library extensions for upvalues and constants.
*   **`drawing.hpp`** - External rendering and overlay API bounds.
*   **`files.hpp`** - Sandboxed file system tools (`readfile`, `writefile`, `listfiles`).
*   **`input.hpp`** - Virtual inputs, mouse movements, and keystrokes.
*   **`instances.hpp`** - Fireclickdetector, getinstances, rendering pipelines.
*   **`metatable.hpp`** - Metamethod hooking and rawset/rawget bypasses.
*   **`network.hpp`** - Advanced HTTP implementations, WebSocket integrations, and request spoofing.
*   **`scripts.hpp`** - Script environment retrieval (`getgenv`, `getrenv`, `getsenv`).

To implement a new library category, simply create the header piece (e.g., `src/env/libs/network.hpp`), write your C++ handler functions, and add a quick initialization block called by `registerAllEnvFunctions()` in `env.hpp`.

---

## 🛠️ Building the Project

1. Open `Cravex.sln` using Visual Studio 2022.
2. Build the `Cravex` project (the C++ DLL) and the `CravexUI` project (the C# UI tester).
3. The project is pre-configured to build using **C++23**. Ensure you are targeting **Release | x64**.

---

## 📜 Credits and Usage

This project is completely free and open-source for the entire community to use, learn from, and expand. If you utilize Cravex as the base for your own executor, you **must provide explicit credit** back to the original source. 

---

## ⚖️ Disclaimer

**For Educational and Research Purposes Only.**

> **To GitHub Trust & Safety Staff reviewing this project:** This software repository is released strictly as a research project to exhibit functional, clean demonstrations of inter-process communication (IPC) concepts via HTTP bridging. **It does not contain, distribute, or install any pre-packaged harmful logic or malicious binaries.** Its sole purpose is to serve as an architectural study of modular external C++ development. Use of this codebase for malicious interference with software is not endorsed by the author.
