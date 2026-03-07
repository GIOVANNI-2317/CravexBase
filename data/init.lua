if _G._cravex_init then return {HideTemp = function() end} end


local coreGui, httpSvc, players = game:GetService("CoreGui"), game:GetService("HttpService"), game:GetService("Players")

local safeWait = task and task.wait or wait
local safeSpawn = task and task.spawn or function(f, ...) coroutine.wrap(f)(...) end
local clockFunc = os and os.clock or tick

local baseDir = Instance.new("Folder", coreGui); baseDir.Name = "CravexBase"
local pointDir = Instance.new("Folder", baseDir); pointDir.Name = "Pointer"
local apiAddr = "http://localhost:6767"
local procId = "-$-crvx-procid-$-"

local function bridgeReq(typ, data, settings)
    local response, startClock = nil, clockFunc()
    httpSvc:RequestInternal({
        Url = apiAddr .. "/handle",
        Method = "POST",
        Body = typ .. "\n" .. procId .. "\n" .. httpSvc:JSONEncode(settings or {}) .. "\n" .. (data or ""),
        Headers = {["Content-Type"] = "text/plain"}
    }):Start(function(success, body) response = body; response.Success = success end)
    while not response and clockFunc() - startClock < 5 do safeWait() end
    return response and response.Body or ""
end

local env = getfenv(function() end)
env.getgenv = function() return env end

-- Signal implementation for WebSockets
local Signal = {}
Signal.__index = Signal
function Signal.new() return setmetatable({ _fns = {} }, Signal) end
function Signal:Connect(fn)
    table.insert(self._fns, fn)
    return { Disconnect = function()
        for i, v in ipairs(self._fns) do if v == fn then table.remove(self._fns, i); break end end
    end }
end
function Signal:Fire(...) for _, fn in ipairs(self._fns) do safeSpawn(fn, ...) end end

-- Restore UNC Essentials
env.getfenv = function(lvl)
    if lvl == 0 or lvl == nil then return env end
    if type(lvl) == "number" then return getfenv(lvl + 1) end
    return getfenv(lvl)
end

local original_defer = task and task.defer or function(f, ...) safeSpawn(f, ...) end
env.task = {
    wait = safeWait,
    spawn = safeSpawn,
    defer = function(f, ...)
        local args = {...}
        safeSpawn(function()
            safeWait(1.5)
            original_defer(f, unpack(args))
        end)
    end
}
if type(task) == "table" then
    for k, v in pairs(task) do if not env.task[k] then env.task[k] = v end end
end

-- WebSocket Class
local WebSocket = {}
WebSocket.__index = WebSocket
function WebSocket.connect(url)
    local handle = bridgeReq("websocket_connect", "", {url = url})
    if handle == "" or handle:sub(1, 6) == "Error:" then 
        if handle ~= "" then warn("WebSocket Error: " .. handle) end
        return nil 
    end
    local self = setmetatable({ _handle = handle, OnMessage = Signal.new(), OnClose = Signal.new(), _closed = false }, WebSocket)
    safeSpawn(function()
        while not self._closed do
            local raw = bridgeReq("websocket_poll", "", {h = self._handle})
            if raw ~= "" then
                local success, data = pcall(function() return httpSvc:JSONDecode(raw) end)
                if success and data then
                    for _, msg in ipairs(data.messages or {}) do self.OnMessage:Fire(msg) end
                    if data.closed then self._closed = true; self.OnClose:Fire(); break end
                end
            end
            safeWait(0.1)
        end
    end)
    return self
end
function WebSocket:Send(msg) if not self._closed then bridgeReq("websocket_send", tostring(msg), {h = self._handle}) end end
function WebSocket:Close()
    if self._closed then return end
    self._closed = true
    bridgeReq("websocket_close", "", {h = self._handle})
    self.OnClose:Fire()
end
env.WebSocket = WebSocket

env.identifyexecutor = function() return "Cravex", "1.1.0" end
env.getexecutorname = env.identifyexecutor

env.compile = function(src, enc)
    return bridgeReq("compile", src, {enc = tostring(enc == nil or enc)})
end

env.setscriptbytecode = function(scr, bc)
    local val = Instance.new("ObjectValue", pointDir)
    val.Name = httpSvc:GenerateGUID(false); val.Value = scr
    bridgeReq("setscriptbytecode", bc, {cn = val.Name})
    val:Destroy()
end

env.getscriptbytecode = function(scr)
    local val = Instance.new("ObjectValue", pointDir)
    val.Name = httpSvc:GenerateGUID(false); val.Value = scr
    local bc = bridgeReq("getscriptbytecode", "", {cn = val.Name})
    val:Destroy()
    return bc
end

local scriptTarget = coreGui:FindFirstChild("RobloxGui").Modules.Common:FindFirstChild("Constants") or coreGui:FindFirstChild("RobloxGui").Modules.Common:FindFirstChild("CommonUtil")
env.loadstring = function(src, chunk)
    if type(src) ~= "string" then return nil, "Expected string" end
    chunk = chunk or "loadstring"
    local bc = env.compile("return{[ [["..chunk.."]] ]=function(...) " .. src .. "\nend}", true)
    if type(bc) ~= "string" or #bc < 1 then return nil, "Compile error" end
    
    local firstByte = string.byte(bc, 1)
    if not firstByte or firstByte > 15 then -- Bytecode magic version is low ASCII.
        return nil, bc
    end
    
    env.setscriptbytecode(scriptTarget, bc)
    local success, res = pcall(function() return debug.loadmodule(scriptTarget) end)
    if not success or type(res) ~= "function" then return nil, bc end
    
    local success2, res2 = pcall(function() return res() end)
    if success2 and typeof(res2) == "table" and res2[chunk] then
        return setfenv(res2[chunk], env)
    end
    return nil, "Load failed"
end

-- Request & Crypt
env.request = function(options)
    local response = bridgeReq("request", "", options)
    if response == "" or response == "{}" then return nil end
    return httpSvc:JSONDecode(response)
end

env.http = { request = env.request }
env.http_request = env.request

env.crypt = {}
env.crypt.base64encode = function(data) return bridgeReq("crypt.base64encode", data) end
env.crypt.base64decode = function(data) return bridgeReq("crypt.base64decode", data) end
env.crypt.base64 = {
    encode = env.crypt.base64encode,
    decode = env.crypt.base64decode
}

env.HttpGet = function(url) 
    local resp = env.request({Url = url})
    return resp and resp.Body or ""
end
env.HttpPost = function(url, body) return env.request({Url = url, Method = "POST", Body = body}) end

-- Filesystem api
env.readfile = function(path) return bridgeReq("readfile", "", {f = path}) end
env.writefile = function(path, data) bridgeReq("writefile", data, {f = path}) end
env.appendfile = function(path, data) bridgeReq("appendfile", data, {f = path}) end
env.isfile = function(path) return bridgeReq("isfile", "", {f = path}) == "true" end
env.isfolder = function(path) return bridgeReq("isfolder", "", {f = path}) == "true" end
env.makefolder = function(path) bridgeReq("makefolder", "", {f = path}) end
env.delfile = function(path) bridgeReq("delfile", "", {f = path}) end
env.delfolder = function(path) bridgeReq("delfolder", "", {f = path}) end
env.listfiles = function(path)
    local raw = bridgeReq("listfiles", "", {f = path})
    if raw == "" or raw == "[]" then return {} end
    return httpSvc:JSONDecode(raw)
end
env.loadfile = function(path)
    if not env.isfile(path) then return nil, "File not found" end
    return env.loadstring(env.readfile(path), path)
end

env.isrbxactive = function()
    return bridgeReq("isrbxactive") == "true"
end
env.isgameactive = env.isrbxactive

env.dumpstring = env.getscriptbytecode

env.getcallbackvalue = function(obj, prop) return nil end

-- Closures & State
env.checkcaller = function() return true end
env.iscclosure = function(f) return debug.info(f, "s") == "[C]" or debug.info(f, "s") == "=[C]" end
env.islclosure = function(f) return not env.iscclosure(f) end
env.isexecutorclosure = function(f) return true end
env.newcclosure = function(f) return f end
env.clonefunction = function(f) return function(...) return f(...) end end
env.hookfunction = function(f, h) return f end -- Placeholder
env.getscriptclosure = function(s) return function() return require(s) end end
env.getscriptfunction = env.getscriptclosure

-- Console
env.rconsoleprint = function(m) print(m) end
env.rconsoleclear = function() end
env.rconsolename = function(t) end

-- Drawing
env.isrenderobj = function(o) return type(o) == "table" and o.__type == "Drawing" end
env.cleardrawcache = function() end

local clonerefs = {}
env.cloneref = function(obj)
    local proxy = newproxy(true)
    local meta = getmetatable(proxy)
    meta.__index = function(t, n)
        local v = obj[n]
        if typeof(v) == "function" then
            return function(self, ...)
                if self == t then self = obj end
                return v(self, ...)
            end
        else
            return v
        end
    end
    meta.__newindex = function(_, n, v) obj[n] = v end
    meta.__tostring = function() return tostring(obj) end
    meta.__metatable = getmetatable(obj)
    clonerefs[proxy] = obj
    return proxy
end

env.compareinstances = function(p1, p2)
    return (clonerefs[p1] or p1) == (clonerefs[p2] or p2) end

-- listener loop
bridgeReq("listen")
safeSpawn(function()
    while true do
        local res = bridgeReq("listen")
        if res and #res > 1 then
            if res == "_DETACH_" then
                if baseDir then baseDir:Destroy() end
                _G._cravex_init = false
                break
            end
            safeSpawn(function()
                local func, err = env.loadstring(res)
                if func then pcall(func) else warn(err) end
            end)
        end
        safeWait()
    end
end)

print("Attached!")
_G._cravex_init = true

pcall(function()
    game:GetService("StarterGui"):SetCore("SendNotification", {
        Title = "Cravex Base",
        Text = "Successfully Attached to Roblox!",
        Duration = 5
    })
end)

return {
    HideTemp = function() end,
    GetIsModal = function() return false end,
    ToggleVisibility = function() end
}