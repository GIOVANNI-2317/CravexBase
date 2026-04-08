if _G._cravex_init then return {HideTemp = function() end} end


local coreGui, httpSvc, players = game:GetService("CoreGui"), game:GetService("HttpService"), game:GetService("Players")
local realType, realTypeof, realTostring = type, typeof, tostring
local oldInstNew = Instance.new

local safeWait = task and task.wait or wait
local safeSpawn = task and task.spawn or function(f, ...) coroutine.wrap(f)(...) end
local clockFunc = os and os.clock or tick

local baseDir = Instance.new("Folder", coreGui); baseDir.Name = "Cravex"
local pointDir = Instance.new("Folder", baseDir); pointDir.Name = "Pointer"

local lvlBindthing = Instance.new("BindableEvent")
lvlBindthing.Event:Connect(function() end)
local idPtr = Instance.new("ObjectValue")
idPtr.Name = "lvlBindthing"
idPtr.Value = lvlBindthing
idPtr.Parent = pointDir

local apiAddr = "http://localhost:6767"
local procId = "-$-crvx-procid-$-"

-- environment tables
local pM = setmetatable({}, { __mode = "k" }) -- proxy map
local oM = setmetatable({}, { __mode = "k" }) -- object map

-- Proxy Service (pSv)
local pObj -- forward declare

local function toP(...)
    local args = table.pack(...)
    for i = 1, args.n do
        local v = args[i]
        if realTypeof(v) == "Instance" then
            args[i] = oM[v] and oM[v].p or pObj(v)
        end
    end
    return table.unpack(args, 1, args.n)
end

local function toO(...)
    local args = table.pack(...)
    for i = 1, args.n do
        local v = args[i]
        if realType(v) == "userdata" and pM[v] then
            args[i] = pM[v].o
        end
    end
    return table.unpack(args, 1, args.n)
end

local function genErr(obj)
    local _, err = xpcall(function() obj:__namecall() end, function() return debug.info(2, "f") end)
    return err
end

local tA, tB, mCache = genErr(OverlapParams.new()), genErr(Color3.new()), {}
local function getnamecallmethod()
    local _, e = pcall(tA)
    local m = if type(e) == "string" then e:match("^(.+) is not a valid member of %w+$") else nil
    if not m then
        _, e = pcall(tB)
        m = if type(e) == "string" then e:match("^(.+) is not a valid member of %w+$") else nil
    end
    if not m or m == "__namecall" then return mCache[coroutine.running()] end
    mCache[coroutine.running()] = m
    return m
end

local function idx(t, k)
    local d = pM[t]
    if not d then return t[k] end
    local o, c = d.o, d.c
    if c and c[k] then return c[k] end
    local v = o[k]
    if type(v) == "function" then
        return function(self, ...)
            return toP(v(toO(self, ...)))
        end
    end
    return toP(v)
end

local function nc(t, ...)
    local d = pM[t]
    if not d then return t(...) end
    local o, c = d.o, d.c
    local m = getnamecallmethod()
    if c and c[m] then return c[m](t, ...) end
    local f = o[m]
    return toP(f(toO(t, ...)))
end

local function ni(t, k, v)
    local d = pM[t]
    local o = d.o
    o[k] = toO(v)
end

function pObj(o, c)
    if not o or oM[o] then return oM[o] and oM[o].p or o end
    local p = newproxy(true)
    local m = getmetatable(p)
    m.__index = idx
    m.__namecall = nc
    m.__newindex = ni
    m.__tostring = function() return realTostring(o) end
    m.__metatable = "The metatable is locked"
    
    local d = { o = o, p = p, c = c }
    pM[p] = d
    oM[o] = d
    return p
end

local function clean(t)
    if type(t) ~= "table" then
        if type(t) == "userdata" and pM[t] then return clean(pM[t].o) end
        if type(t) == "userdata" or type(t) == "table" or type(t) == "function" then
            return tostring(t)
        end
        return t
    end
    local res = {}
    for k, v in pairs(t) do
        local nk = type(k) == "table" and tostring(k) or k
        res[nk] = clean(v)
    end
    return res
end



local bridgeQueue = {}
local bridgeResults = {}

safeSpawn(function()
    while true do
        for reqId, req in pairs(bridgeQueue) do
            bridgeQueue[reqId] = nil
            safeSpawn(function()
                local response, startClock = nil, clockFunc()
                local reqData = req.typ .. "\n" .. procId .. "\n" .. httpSvc:JSONEncode(req.settings or {}) .. "\n" .. (req.data or "")
                
                pcall(function()
                    httpSvc:RequestInternal({
                        Url = apiAddr .. "/handle",
                        Method = "POST",
                        Body = reqData,
                        Headers = {["Content-Type"] = "text/plain"}
                    }):Start(function(success, body) 
                        response = {Success = success, Body = body.Body}
                    end)
                end)
                
                while not response and clockFunc() - startClock < 5 do safeWait() end
                bridgeResults[reqId] = response and response.Body or ""
            end)
        end
        safeWait()
    end
end)

local function bridgeReq(typ, data, settings)
    local reqId = httpSvc:GenerateGUID(false)
    bridgeQueue[reqId] = {typ = typ, data = data, settings = clean(settings or {})}
    while bridgeResults[reqId] == nil do safeWait() end
    local res = bridgeResults[reqId]
    bridgeResults[reqId] = nil
    return res
end

local env = getfenv(0)
local cravex = {}
env.getgenv = function() return env end
cravex.assert = function(cond, msg) 
    if not cond then error(msg, 2) end
end

local uiHolder = Instance.new("ScreenGui")
uiHolder.Name = httpSvc:GenerateGUID(false)
uiHolder.IgnoreGuiInset = true
uiHolder.DisplayOrder = 2147483647
pcall(function() uiHolder.Parent = coreGui end)
env.gethui = function() return uiHolder end

-- input
env.isrbxactive = function() return bridgeReq("isrbxactive") == "true" end
env.isgameactive = env.isrbxactive
env.iswindowactive = env.isrbxactive

env.mouse1click = function() bridgeReq("mouse1click") end
env.mouse1press = function() bridgeReq("mouse1press") end
env.mouse1release = function() bridgeReq("mouse1release") end
env.mouse2click = function() bridgeReq("mouse2click") end
env.mouse2press = function() bridgeReq("mouse2press") end
env.mouse2release = function() bridgeReq("mouse2release") end
env.mousemoveabs = function(x, y) bridgeReq("mousemoveabs", "", {x=tostring(x), y=tostring(y)}) end
env.mousemoverel = function(x, y) bridgeReq("mousemoverel", "", {dx=tostring(x), dy=tostring(y)}) end
env.mousescroll = function(v) bridgeReq("mousescroll", tostring(v)) end

env.keypress = function(k) bridgeReq("keypress", tostring(k)) end
env.keyrelease = function(k) bridgeReq("keyrelease", tostring(k)) end
env.keytap = function(k) bridgeReq("keytap", tostring(k)) end
env.keyclick = env.keypress

env.messagebox = function(m, t, ty) bridgeReq("messagebox", tostring(m), {t=tostring(t), type=tostring(ty)}) end

--[[
-- WebSocket signals
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

-- env
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

-- WS Client
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
]]

env.identifyexecutor = function() return "Cravex", "1.1.0" end
env.getexecutorname = env.identifyexecutor

env.compile = function(src, enc)
    return bridgeReq("compile", src, {enc = tostring(enc == nil or enc)})
end

env.setscriptbytecode = function(scr, bc)
    assert(type(scr) == "userdata", "invalid argument #1 to 'setscriptbytecode' (Instance expected, got " .. typeof(scr) .. ")", 2)
    assert(type(bc) == "string", "invalid argument #2 to 'setscriptbytecode' (string expected, got " .. type(bc) .. ")", 2)
    local val = Instance.new("ObjectValue", toO(pointDir))
    val.Name = httpSvc:GenerateGUID(false); val.Value = toO(scr)
    bridgeReq("setscriptbytecode", bc, {cn = val.Name})
    val:Destroy()
end

env.getscriptbytecode = function(scr)
    assert(type(scr) == "userdata", "invalid argument #1 to 'getscriptbytecode' (Instance expected, got " .. typeof(scr) .. ")", 2)
    local val = Instance.new("ObjectValue", toO(pointDir))
    val.Name = httpSvc:GenerateGUID(false); val.Value = toO(scr)
    local bc = bridgeReq("getscriptbytecode", "", {cn = val.Name})
    val:Destroy()
    if bc:sub(1, 4) == "null" or bc == "" then
        error(bc:sub(6) or "Script contains no bytecode (uncompiled)", 2)
    end
    return bc
end

env.getinitbytecode = function()
    return bridgeReq("getinitbytecode")
end


env.getscripts = function()
	local res = {}
    local cg, cp = game:GetService("CoreGui"), game:GetService("CorePackages")
	for i, v in pairs(oM) do
        local scr = v.p
		if scr:IsA("LocalScript") or scr:IsA("ModuleScript") or scr:IsA("Script") then
			local p = scr.Parent
            local is_c = false
            while p do
                if p == cg or p == cp then
                    is_c = true
                    break
                end
                p = p.Parent
            end
            if not is_c then
                table.insert(res, scr)
            end
		end
	end
	return res
end

local scriptTarget = coreGui:FindFirstChild("RobloxGui").Modules.Common:FindFirstChild("Constants") or coreGui:FindFirstChild("RobloxGui").Modules.Common:FindFirstChild("CommonUtil")
env.loadstring = function(src, chunk)
    assert(type(src) == "string", "invalid argument #1 to 'loadstring' (string expected, got " .. type(src) .. ")", 2)
    chunk = chunk or "loadstring"
    assert(type(chunk) == "string", "invalid argument #2 to 'loadstring' (string expected, got " .. type(chunk) .. ")", 2)

    local bc = env.compile("return function(...) " .. src .. "\nend", true)
    if type(bc) ~= "string" or #bc < 1 then 
        warn("[Cravex] loadstring compilation failed for " .. chunk .. ": " .. tostring(bc))
        return nil, "Compile error: " .. tostring(bc) 
    end
    
    local firstByte = string.byte(bc, 1)
    if not firstByte or firstByte > 15 then
        warn("[Cravex] loadstring check failed: " .. tostring(bc))
        return nil, bc
    end
    
    env.setscriptbytecode(scriptTarget, bc)
    
    local success, res = pcall(function() return debug.loadmodule(scriptTarget) end)
    if not success or type(res) ~= "function" then 
        warn("[Cravex] loadstring LoadModule failed: " .. tostring(res))
        return nil, "LoadModule failed: " .. tostring(res) 
    end
    
    local success2, res2 = pcall(function() return res() end)
    if success2 and type(res2) == "function" then
        return setfenv(res2, env)
    end
    warn("[Cravex] loadstring execution failed: " .. tostring(res2))
    return nil, "Chunk execution failed: " .. tostring(res2)
end

env.getscriptclosure = function(scr)
    assert(type(scr) == "userdata", "invalid argument #1 to 'getscriptclosure' (Instance expected, got " .. typeof(scr) .. ")", 2)
    local bc = env.getscriptbytecode(scr)
    if not bc then return nil end
    local func, err = env.loadstring(bc)
    return func
end

env.getscripthash = function(scr)
    assert(type(scr) == "userdata", "invalid argument #1 to 'getscripthash' (Instance expected, got " .. typeof(scr) .. ")", 2)
    local bc = env.getscriptbytecode(scr)
    if not bc then return nil end
    return typeof(env.crypt) == "table" and type(env.crypt.hash) == "function" and env.crypt.hash(bc, "sha384") or "mock_hash"
end

env.getrenv = function() return _G end
env.getreg = function() return debug.getregistry() end -- not working btw

_G.loadstring = env.loadstring
getfenv(0).loadstring = env.loadstring

task.spawn(function() 
    game.DescendantAdded:Connect(pObj)
    for _, v in ipairs(game:GetDescendants()) do
        pObj(v)
    end
end)

env.getinstances = function()
	local insts = {}
	for i, v in pairs(oM) do
		table.insert(insts, v.p)
	end
	return insts
end

env.getnilinstances = function()
	local nils = {}
	for i, v in pairs(oM) do
		if v.p.Parent == nil then
			table.insert(nils, v.p)
		end
	end
	return nils
end

-- signals
local function createSignal()
	local handlers = {}
	local signal = {}
	function signal:Connect(handler)
		table.insert(handlers, handler)
		return {
			Disconnect = function()
				for i, h in ipairs(handlers) do
					if h == handler then
						table.remove(handlers, i)
						break
					end
				end
			end
		}
	end
	function signal:Fire(...)
		for _, h in ipairs(handlers) do
			safeSpawn(h, ...)
		end
	end
	return signal
end

-- Request & Crypt
env.request = function(options)
    assert(type(options) == "table", "invalid argument #1 to 'request' (table expected, got " .. type(options) .. ")", 2)
    assert(type(options.Url) == "string", "invalid option 'Url' for argument #1 to 'request' (string expected, got " .. type(options.Url) .. ")", 2)
    local response = bridgeReq("request", "", options)
    if response == "" or response == "{}" then return {Success = false, StatusCode = 500, Body = "", Headers = {}} end
    local decoded = httpSvc:JSONDecode(response)
    if type(decoded.Success) ~= "boolean" then
        decoded.Success = (decoded.StatusCode >= 200 and decoded.StatusCode < 300)
    end
    return decoded
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
env.crypt.generatekey = function(len)
    local key = ''
    local x = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
    for i = 1, len or 32 do local n = math.random(1, #x) key = key .. x:sub(n, n) end
    return env.crypt.base64encode(key)
end
env.crypt.generatebytes = env.crypt.generatekey
env.crypt.random = env.crypt.generatekey
env.crypt.encrypt = function(data, key)
    local result = {}
    data, key = tostring(data), tostring(key)
    for i = 1, #data do
        local byte = string.byte(data, i)
        local keyByte = string.byte(key, (i - 1) % #key + 1)
        table.insert(result, string.char(bit32.bxor(byte, keyByte)))
    end
    return table.concat(result), key
end
env.crypt.decrypt = env.crypt.encrypt

env.base64_encode = env.crypt.base64encode
env.base64_decode = env.crypt.base64decode
env.crypt.lz4compress = function(data) 
    local b64 = bridgeReq("crypt.lz4compress", tostring(data))
    return env.crypt.base64decode(b64)
end
env.crypt.lz4decompress = function(data) 
    local b64 = bridgeReq("crypt.lz4decompress", env.crypt.base64encode(tostring(data)))
    return env.crypt.base64decode(b64)
end
env.lz4compress = env.crypt.lz4compress
env.lz4decompress = env.crypt.lz4decompress

env.HttpGet = function(url, returnRaw)
    assert(type(url) == "string", "invalid argument #1 to 'HttpGet' (string expected, got " .. type(url) .. ")", 2)
    local returnRaw = returnRaw or true
    local resp = env.request({Url = url})
    if not resp.Success then
        warn("[Cravex] HttpGet failed for " .. url .. ": " .. tostring(resp.Body))
        return ""
    end
    return resp.Body or ""
end
env.HttpPost = function(url, body, contentType)
    assert(type(url) == "string", "invalid argument #1 to 'HttpPost' (string expected, got " .. type(url) .. ")", 2)
    contentType = contentType or "application/json"
    assert(type(contentType) == "string", "invalid argument #3 to 'HttpPost' (string expected, got " .. type(contentType) .. ")", 2)
    local resp = env.request({
        Url = url,
        Method = "POST",
        Body = type(body) == "string" and body or tostring(body),
        Headers = { ["Content-Type"] = contentType }
    })
    return resp and resp.Body or ""
end

env.GetObjects = function(asset)
    assert(type(asset) == "string", "invalid argument #1 to 'GetObjects' (string expected, got " .. type(asset) .. ")", 2)
    return { game:GetService("InsertService"):LoadLocalAsset(asset) }
end

-- hooks
env.game = pObj(game, {
    HttpGet = function(_, url) return env.HttpGet(url) end,
    HttpGetAsync = function(_, url) return env.HttpGet(url) end,
    HttpPost = function(_, url, data, type) return env.HttpPost(url, data, type) end,
    HttpPostAsync = function(_, url, data, type) return env.HttpPost(url, data, type) end,
    GetObjects = function(_, asset) return env.GetObjects(asset) end,
    GetService = function(s, name)
        local raw = toO(s):GetService(name)
        return toP(raw)
    end,
    getService = function(s, name) return toP(toO(s):GetService(name)) end,
    service = function(s, name) return toP(toO(s):GetService(name)) end,
    FindService = function(s, name) return toP(toO(s):FindService(name)) end,
    findService = function(s, name) return toP(toO(s):FindService(name)) end,
})
env.Game = env.game
env.workspace = pObj(workspace)
env.Workspace = env.workspace
env.script = pObj(script)

env.Instance = {
    new = function(cls, parent)
        return toP(oldInstNew(cls, toO(parent)))
    end
}

env.typeof = function(obj)
    return realTypeof(toO(obj))
end

env.type = function(obj)
    return realType(toO(obj))
end

env.rawequal = function(a, b)
    return toO(a) == toO(b)
end

-- filesystem
local blockedExts = { ".7z", ".ade", ".adp", ".apk", ".appx", ".appxbundle", ".application", ".bat", ".cab", ".chm", ".cmd", ".com", ".cpl", ".csh", ".dll", ".dmg", ".docm", ".drv", ".exe", ".gadget", ".gz", ".hta", ".img", ".inf", ".ins", ".isp", ".iso", ".jar", ".js", ".jse", ".ksh", ".lib", ".lnk", ".mde", ".msc", ".msh", ".msh1", ".msh2", ".mshxml", ".msi", ".msp", ".mst", ".nsh", ".ocx", ".php", ".pif", ".pl", ".pptm", ".ps", ".ps1", ".ps1xml", ".ps2", ".ps2xml", ".psc1", ".psc2", ".psd1", ".psm1", ".py", ".pyw", ".rar", ".rb", ".rbw", ".reg", ".scf", ".scr", ".sct", ".sh", ".shb", ".sys", ".tar", ".url", ".vb", ".vbe", ".vbs", ".vxd", ".ws", ".wsc", ".wsf", ".wsh", ".xlsm", ".xml", ".zip" }
local function hasBlockedExt(p)
    if type(p) ~= "string" then return false end
    p = p:lower():gsub("[%s%z]+$", "")
    for _, ext in ipairs(blockedExts) do
        if #p >= #ext and p:sub(-#ext) == ext then return true end
    end
    return false
end

env.readfile = function(path) 
    if not env.isfile(path) then error("file does not exist", 2) end
    local res = bridgeReq("readfile", "", {f = tostring(path), b64 = true}) 
    if res and res ~= "" then return env.crypt.base64decode(res) end
    return ""
end
env.writefile = function(path, data) 
    if hasBlockedExt(path) then error("attempt to write restricted file format", 2) end
    bridgeReq("writefile", env.crypt.base64encode(type(data) == "string" and data or tostring(data)), {f = tostring(path), b64 = true}) 
end
env.appendfile = function(path, data) 
    if hasBlockedExt(path) then error("attempt to write restricted file format", 2) end
    bridgeReq("appendfile", env.crypt.base64encode(type(data) == "string" and data or tostring(data)), {f = tostring(path), b64 = true}) 
end
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

-- misc
local cclosureRegistry = {}
local realDebug = debug
local oldDebugInfo = realDebug.info
env.debug = setmetatable({}, {
    __index = realDebug
})
env.debug.info = function(f, ...)
    local res = {oldDebugInfo(f, ...)}
    if type(f) == "function" and cclosureRegistry[f] then
        local args = {...}
        local options = args[#args]
        if type(options) == "string" then
            local resultIndex = 0
            for j=1, #options do
                local char = string.sub(options, j, j)
                if char ~= " " then
                    resultIndex = resultIndex + 1
                    if char == "s" then
                        res[resultIndex] = "[C]"
                    end
                end
            end
        end
    end
    return unpack(res)
end

env.checkcaller = function()
    return debug.info(1, 'slnaf') == debug.info(env.getgenv, 'slnaf')
end
env.iscclosure = function(f) return env.debug.info(f, "s") == "[C]" or env.debug.info(f, "s") == "=[C]" end
env.islclosure = function(f) return not env.iscclosure(f) end
env.isexecutorclosure = function(f) return true end
env.newcclosure = function(f) 
    local wrapped = function(...) return f(...) end
    cclosureRegistry[wrapped] = true
    return wrapped
end
env.newlclosure = function(f)
    return function(...) return f(...) end
end
env.clonefunction = function(f) 
    if env.iscclosure(f) then return env.newcclosure(f) end
    return env.newlclosure(f)
end

env.cloneref = function(inst)
    assert(type(inst) == "userdata", "invalid argument #1 to 'cloneref' (Instance expected, got " .. typeof(inst) .. ")", 2)
    local proxy = newproxy(true)
    local proxyMt = getmetatable(proxy)
    
    proxyMt.__index = function(_, key)
        local value = inst[key]
        if type(value) == "function" then
            return function(_, ...) return value(inst, ...) end
        end
        return value
    end

    proxyMt.__newindex = function(_, key, value) inst[key] = value end
    proxyMt.__tostring = function() return inst.Name end
    proxyMt.__type = "Instance"
    proxyMt.__metatable = "The metatable is locked"

    return proxy
end

env.hookfunction = function(f, h) return f end
env.getscriptclosure = function(s) return function() return require(s) end end
env.getscriptfunction = env.getscriptclosure

env.fireclickdetector = function(detector, distance, event)
    if typeof(detector) ~= "Instance" or not detector:IsA("ClickDetector") then return end
    local part = detector.Parent
    if not part or not part:IsA("BasePart") then return end
    
    local cam = workspace.CurrentCamera
    if not cam then return end
    
    local oldCFrame = part.CFrame
    local oldTrans = part.Transparency
    local oldCanCollide = part.CanCollide
    local oldMaxDist = detector.MaxActivationDistance
    
    detector.MaxActivationDistance = math.huge
    part.CFrame = cam.CFrame * CFrame.new(0, 0, -1.5)
    
    local vu = game:GetService("VirtualUser")
    local center = cam.ViewportSize / 2
    
    task.spawn(function()
        task.wait(0.05) -- Wait for physics/render queue to acknowledge the CFrame change
        
        if event == "RightMouseClick" then
            vu:Button2Down(center)
            task.wait()
            vu:Button2Up(center)
        elseif event == "MouseHoverEnter" then
            vu:MoveMouse(center, cam.CFrame)
        elseif event == "MouseHoverLeave" then
            vu:MoveMouse(Vector2.new(0,0), cam.CFrame)
        else
            vu:Button1Down(center)
            task.wait()
            vu:Button1Up(center)
        end
        
        task.wait(0.05)
        
        part.CFrame = oldCFrame
        part.Transparency = oldTrans
        part.CanCollide = oldCanCollide
        detector.MaxActivationDistance = oldMaxDist
    end)
end

-- console
env.rconsoleprint = function(m) print(m) end
env.rconsoleclear = function() end
env.rconsolename = function(t) end

-- drawing
local drawingUI = Instance.new("ScreenGui")
drawingUI.Name = httpSvc:GenerateGUID(false)
drawingUI.IgnoreGuiInset = true
drawingUI.DisplayOrder = 2147483647
local s, e = pcall(function() drawingUI.Parent = coreGui end)
if not s then drawingUI.Parent = players.LocalPlayer:WaitForChild("PlayerGui") end

local drawings = {}
env.cleardrawcache = function()
    for _, v in pairs(drawings) do pcall(function() v:Remove() end) end
    drawings = {}
end

env.isrenderobj = function(o) 
    if type(o) == "table" and o.__type == "Drawing" then return true end
    if type(o) == "userdata" and typeof(o) == "table" and o.__type == "Drawing" then return true end
    return false
end
env.getrenderproperty = function(obj, prop) return obj[prop] end
env.setrenderproperty = function(obj, prop, val) obj[prop] = val end

local function updateDrawing(obj)

    if not obj.Instance then return end
    local inst = obj.Instance
    inst.Visible = obj.Visible
    inst.ZIndex = obj.ZIndex
    if obj.Type == "Line" then
        if not obj.From or not obj.To then return end
        local dist = (obj.To - obj.From).Magnitude
        local center = (obj.To + obj.From) / 2
        local angle = math.deg(math.atan2(obj.To.Y - obj.From.Y, obj.To.X - obj.From.X))
        inst.Position = UDim2.new(0, center.X, 0, center.Y)
        inst.Size = UDim2.new(0, dist, 0, obj.Thickness)
        inst.Rotation = angle
        inst.BackgroundColor3 = obj.Color
        inst.BackgroundTransparency = obj.Transparency
    elseif obj.Type == "Text" then
        inst.Text = obj.Text
        inst.TextColor3 = obj.Color
        inst.TextTransparency = obj.Transparency
        inst.TextSize = obj.Size
        inst.Position = UDim2.new(0, obj.Position.X, 0, obj.Position.Y)
        inst.TextXAlignment = obj.Center and Enum.TextXAlignment.Center or Enum.TextXAlignment.Left
        local stroke = inst:FindFirstChild("UIStroke")
        if obj.Outline then
            if not stroke then stroke = Instance.new("UIStroke", inst) end
            stroke.Color = obj.OutlineColor
            stroke.Transparency = obj.Transparency
        elseif stroke then stroke:Destroy() end
    elseif obj.Type == "Circle" then
        inst.Position = UDim2.new(0, obj.Position.X - obj.Radius, 0, obj.Position.Y - obj.Radius)
        inst.Size = UDim2.new(0, obj.Radius * 2, 0, obj.Radius * 2)
        inst.BackgroundColor3 = obj.Color
        inst.BackgroundTransparency = obj.Filled and obj.Transparency or 1
        local stroke = inst:FindFirstChild("UIStroke")
        if not obj.Filled then
            if not stroke then stroke = Instance.new("UIStroke", inst) end
            stroke.Color = obj.Color
            stroke.Transparency = obj.Transparency
            stroke.Thickness = obj.Thickness
        elseif stroke then stroke:Destroy() end
    elseif obj.Type == "Square" then
        inst.Position = UDim2.new(0, obj.Position.X, 0, obj.Position.Y)
        inst.Size = UDim2.new(0, obj.Size.X, 0, obj.Size.Y)
        inst.BackgroundColor3 = obj.Color
        inst.BackgroundTransparency = obj.Filled and obj.Transparency or 1
        local stroke = inst:FindFirstChild("UIStroke")
        if not obj.Filled then
            if not stroke then stroke = Instance.new("UIStroke", inst) end
            stroke.Color = obj.Color
            stroke.Transparency = obj.Transparency
            stroke.Thickness = obj.Thickness
        elseif stroke then stroke:Destroy() end
    elseif obj.Type == "Quad" or obj.Type == "Triangle" then
        inst.Visible = false
    elseif obj.Type == "Image" then
        inst.Position = UDim2.new(0, obj.Position.X, 0, obj.Position.Y)
        inst.Size = UDim2.new(0, obj.Size.X, 0, obj.Size.Y)
        inst.ImageColor3 = obj.Color
        inst.ImageTransparency = obj.Transparency
    end
end

env.Drawing = {
    new = function(drawType)
        assert(type(drawType) == "string", "arg #1 must be type string", 2)
        local obj = {
            __type = "Drawing", Type = drawType,
            Visible = false, ZIndex = 0, Transparency = 0, Color = Color3.new(1,1,1),
            __OBJECT_EXISTS = true,
            Remove = function(self)
                self.Visible = false
                self.__OBJECT_EXISTS = false
                if self.Instance then self.Instance:Destroy() end
                drawings[self] = nil
            end,
            Destroy = function(self) self:Remove() end
        }
        
        if drawType == "Line" then
            obj.Thickness = 1
            obj.From = Vector2.new()
            obj.To = Vector2.new()
            obj.Instance = Instance.new("Frame")
            obj.Instance.AnchorPoint = Vector2.new(0.5, 0.5)
            obj.Instance.BorderSizePixel = 0
        elseif drawType == "Text" then
            obj.Text = ""
            obj.Size = 16
            obj.Center = false
            obj.Outline = false
            obj.OutlineColor = Color3.new(0,0,0)
            obj.Position = Vector2.new()
            obj.Font = 0
            obj.Instance = Instance.new("TextLabel")
            obj.Instance.BackgroundTransparency = 1
            obj.Instance.Font = Enum.Font.Code
        elseif drawType == "Circle" then
            obj.Thickness = 1
            obj.NumSides = 0
            obj.Radius = 0
            obj.Filled = false
            obj.Position = Vector2.new()
            obj.Instance = Instance.new("Frame")
            obj.Instance.BorderSizePixel = 0
            Instance.new("UICorner", obj.Instance).CornerRadius = UDim.new(1, 0)
        elseif drawType == "Square" then
            obj.Thickness = 1
            obj.Size = Vector2.new()
            obj.Position = Vector2.new()
            obj.Filled = false
            obj.Instance = Instance.new("Frame")
            obj.Instance.BorderSizePixel = 0
        elseif drawType == "Quad" then
            obj.Thickness = 1
            obj.PointA = Vector2.new()
            obj.PointB = Vector2.new()
            obj.PointC = Vector2.new()
            obj.PointD = Vector2.new()
            obj.Filled = false
            obj.Instance = Instance.new("Frame")
            obj.Instance.BackgroundTransparency = 1
        elseif drawType == "Triangle" then
            obj.Thickness = 1
            obj.PointA = Vector2.new()
            obj.PointB = Vector2.new()
            obj.PointC = Vector2.new()
            obj.Filled = false
            obj.Instance = Instance.new("Frame")
            obj.Instance.BackgroundTransparency = 1
        elseif drawType == "Image" then
            obj.Data = ""
            obj.Size = Vector2.new()
            obj.Position = Vector2.new()
            obj.Rounding = 0
            obj.Instance = Instance.new("ImageLabel")
            obj.Instance.BackgroundTransparency = 1
        else
            error("Invalid Drawing type")
        end
        
        obj.Instance.Parent = toO(drawingUI)
        drawings[obj] = true
        
        local proxy = newproxy(true)
        local meta = getmetatable(proxy)
        meta.__index = function(_, k) 
            if k == "TextBounds" and drawType == "Text" then
                return obj.Instance.TextBounds
            end
            if k == "__OBJECT_EXISTS" then
                return obj.__OBJECT_EXISTS
            end
            return obj[k]
        end
        meta.__newindex = function(_, k, v)
            if obj[k] == v then return end
            obj[k] = v
            updateDrawing(obj)
        end
        
        return proxy
    end,
    Fonts = { UI = 0, System = 1, Plex = 2, Monospace = 3 },
    Font = { UI = 0, System = 1, Plex = 2, Monospace = 3 }
}

env.compareinstances = function(p1, p2)
    assert(type(p1) == "userdata", "invalid argument #1 to 'compareinstances' (Instance expected, got " .. typeof(p1) .. ")", 2)
    assert(type(p2) == "userdata", "invalid argument #2 to 'compareinstances' (Instance expected, got " .. typeof(p2) .. ")", 2)
    return toO(p1) == toO(p2)
end

-- clipboard
env.setclipboard = function(data) 
    assert(type(data) == "string", "invalid argument #1 to 'setclipboard' (string expected, got " .. type(data) .. ")", 2)
    bridgeReq("setclipboard", tostring(data)) 
end
env.toclipboard = env.setclipboard

env.getclipboard = function() return bridgeReq("getclipboard") end

-- decompile // ashore.rip API
local ASHORE_API = "https://decompiler.ashore.rip/"

env.decompile = function(scr)
    assert(type(scr) == "userdata", "invalid argument #1 to 'decompile' (Instance expected, got " .. typeof(scr) .. ")", 2)
    local bc = env.getscriptbytecode(scr)
    
    local Output = env.request({
        Url = ASHORE_API .. "decompile";
        Method = "POST";
        Body = bc;
    });
    
    if Output.StatusCode == 200 then
        return Output.Body;
    end;

    return "-- Failed to decompile bytecode\n" .. (Output.Body or tostring(Output.StatusCode))
end

env.disassemble = function(scr)
    assert(type(scr) == "userdata", "invalid argument #1 to 'disassemble' (Instance expected, got " .. typeof(scr) .. ")", 2)
    local bc = env.getscriptbytecode(scr)
    return bridgeReq("disassemble", bc) -- Disassemble stays native for speed
end

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
                if func then 
                    local success, execErr = pcall(func)
                    if not success then warn("Execution Error: " .. tostring(execErr)) end
                else 
                    warn("Compile Error: " .. tostring(err)) 
                end
            end)
        end
        safeWait(0.1)
    end
end)

local fx = {
    collectgarbage = collectgarbage,
    printidentity = printidentity,
    elapsedTime = elapsedTime,
    version = version,
    table = {
        getn = table.getn or function(t) return #t end,
        foreachi = table.foreachi or function(t, f) for i, v in ipairs(t) do f(i, v) end end,
        foreach = table.foreach or function(t, f) for k, v in pairs(t) do f(k, v) end end,
    }
}

local renv = {
    _VERSION = _VERSION,
    UserSettings = UserSettings,
    assert = assert,
    bit32 = {
        arshift = bit32.arshift, band = bit32.band, bnot = bit32.bnot, bor = bit32.bor, btest = bit32.btest,
        extract = bit32.extract, lshift = bit32.lshift, replace = bit32.replace, rshift = bit32.rshift, xor = bit32.xor,
    },
    collectgarbage = fx.collectgarbage,
    coroutine = {
        create = coroutine.create, resume = coroutine.resume, running = coroutine.running,
        status = coroutine.status, wrap = coroutine.wrap, yield = coroutine.yield, isyieldable = coroutine.isyieldable,
    },
    debug = {
        traceback = debug.traceback, profilebegin = debug.profilebegin, profileend = debug.profileend, info = debug.info, dumpcodesize = debug.dumpcodesize, getmemorycategory = debug.getmemorycategory, setmemorycategory = debug.setmemorycategory,
    },
    delay = delay,
    elapsedTime = fx.elapsedTime,
    error = error,
    gcinfo = gcinfo,
    getfenv = getfenv,
    ipairs = ipairs,
    math = {
        abs = math.abs, acos = math.acos, asin = math.asin, atan = math.atan, atan2 = math.atan2, ceil = math.ceil,
        cos = math.cos, cosh = math.cosh, deg = math.deg, exp = math.exp, floor = math.floor, fmod = math.fmod,
        frexp = math.frexp, ldexp = math.ldexp, log = math.log, log10 = math.log10, max = math.max, min = math.min,
        modf = math.modf, pow = math.pow, rad = math.rad, random = math.random, randomseed = math.randomseed,
        sin = math.sin, sinh = math.sinh, sqrt = math.sqrt, tan = math.tan, tanh = math.tanh, pi = math.pi,
    },
    next = next,
    newproxy = newproxy,
    os = {
        clock = os.clock, date = os.date, difftime = os.difftime, time = os.time,
    },
    pairs = pairs,
    print = print,
    printidentity = fx.printidentity,
    rawequal = rawequal,
    rawget = rawget,
    rawlen = rawlen,
    rawset = rawset,
    select = select,
    setfenv = setfenv,
    spawn = spawn,
    string = {
        byte = string.byte, char = string.char, find = string.find, format = string.format, gmatch = string.gmatch,
        gsub = string.gsub, len = string.len, lower = string.lower, match = string.match, pack = string.pack,
        packsize = string.packsize, rep = string.rep, reverse = string.reverse, sub = string.sub,
        unpack = string.unpack, upper = string.upper,
    },
    table = {
        getn = fx.table.getn, foreachi = fx.table.foreachi, foreach = fx.table.foreach, sort = table.sort, unpack = table.unpack, freeze = table.freeze, clear = table.clear, pack = table.pack, move = table.move, insert = table.insert, create = table.create, maxn = table.maxn, isfrozen = table.isfrozen, concat = table.concat, clone = table.clone, find = table.find, remove = table.remove,
    },
    task = {
        defer = task.defer, delay = task.delay, spawn = task.spawn, wait = task.wait,
    },
    tick = tick,
    time = time,
    tonumber = tonumber,
    tostring = tostring,
    type = type,
    utf8 = {
        char = utf8.char, charpattern = utf8.charpattern, codepoint = utf8.codepoint, codes = utf8.codes,
        len = utf8.len, nfdnormalize = utf8.nfdnormalize, nfcnormalize = utf8.nfcnormalize,
    },
    version = fx.version,
    wait = wait,
    xpcall = xpcall,
}

env.setreadonly = function(t, ro)
    assert(type(t) == "table", "invalid argument #1 to 'setreadonly' (table expected, got " .. type(t) .. ")", 2)
    local meta = getmetatable(t) or {}
    if ro then
        meta.__newindex = function(_, k) error("Attempt to modify a read-only table", 2) end
    else
        meta.__newindex = nil
    end
    setmetatable(t, meta)
end
env.make_readonly = env.setreadonly

env.isourclosure = function(func)
    assert(typeof(func) == "function", "Invalid argument #1 to 'isourclosure' (Function expected, got " .. typeof(func) .. ")", 2)
    local our = true
    local function checktable(t)
        for i, v in pairs(t) do
            if not our then return end
            if v == func then
                our = false
                return
            elseif typeof(v) == "table" then
                checktable(v)
            end
        end
    end
    checktable(renv)
    return our
end
env.isexecutorclosure = env.isourclosure
env.checkclosure = env.isourclosure

env.getrenv = function()
    local t = table.clone(renv)
    t.table = env.table
    t.typeof = env.typeof
    t.game = env.game
    t.Game = env.Game
    t.script = env.script
    t.workspace = env.workspace
    t.Workspace = env.Workspace
    t.getmetatable = env.getmetatable
    t.setmetatable = env.setmetatable
    t.require = env.require
    t._G = table.clone(env._G)
    env.setreadonly(t, true)
    return t
end

env.WebSocket = {
	connect = function(url)
		assert(type(url) == "string", "invalid argument #1 to 'connect' (string expected, got " .. type(url) .. ")", 2)
		local handle = bridgeReq("websocket_connect", "", {url = url})
		if handle:sub(1, 6) == "Error:" then error(handle, 2) end
		
		local ws = {
			OnMessage = createSignal(),
			OnClose = createSignal()
		}
		
		function ws:Send(msg)
			assert(type(msg) == "string", "invalid argument #1 to 'Send' (string expected, got " .. type(msg) .. ")", 2)
			bridgeReq("websocket_send", msg, {h = handle})
		end
		
		function ws:Close()
			bridgeReq("websocket_close", "", {h = handle})
		end
		
		safeSpawn(function()
			while true do
				local poll = bridgeReq("websocket_poll", "", {h = handle})
				if poll == "" then break end
				local data = httpSvc:JSONDecode(poll)
				if data.messages then
					for _, msg in ipairs(data.messages) do
						ws.OnMessage:Fire(msg)
					end
				end
				if data.closed then
					ws.OnClose:Fire()
					break
				end
				safeWait(0.1)
			end
		end)
		
		return ws
	end
}
getgenv().WebSocket = env.WebSocket

local b64chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
env.base64_encode = function(data)
    assert(type(data) == "string", "invalid argument #1 to 'base64_encode' (string expected, got " .. typeof(data) .. ")", 2)
    return ((data:gsub('.', function(x) 
        local r,b='',x:byte()
        for i=8,1,-1 do r=r..(b%2^i-b%2^(i-1)>0 and '1' or '0') end
        return r;
    end)..'0000'):gsub('%d%d%d?%d?%d?%d?', function(x)
        if (#x < 6) then return '' end
        local c=0
        for i=1,6 do c=c+(x:sub(i,i)=='1' and 2^(6-i) or 0) end
        return b64chars:sub(c+1,c+1)
    end)..({ '', '==', '=' })[#data%3+1])
end
env.base64encode = env.base64_encode

env.base64_decode = function(data)
    assert(type(data) == "string", "invalid argument #1 to 'base64_decode' (string expected, got " .. typeof(data) .. ")", 2)
    data = string.gsub(data, '[^'..b64chars..'=]', '')
    return (data:gsub('.', function(x)
        if (x == '=') then return '' end
        local r,f='',(b64chars:find(x)-1)
        for i=6,1,-1 do r=r..(f%2^i-f%2^(i-1)>0 and '1' or '0') end
        return r;
    end):gsub('%d%d%d?%d?%d?%d?%d?%d?', function(x)
        if (#x ~= 8) then return '' end
        local c=0
        for i=1,8 do c=c+(x:sub(i,i)=='1' and 2^(8-i) or 0) end
        return string.char(c)
    end))
end
env.base64decode = env.base64_decode

env.crypt = env.crypt or {}
env.crypt.base64encode = env.base64_encode
env.crypt.base64decode = env.base64_decode
env.crypt.base64_encode = env.base64_encode
env.crypt.base64_decode = env.base64_decode

env.printinfo = function(msg) -- random add
    msg = type(msg) == "string" and msg or tostring(msg)
    bridgeReq("printinfo", msg) 
end

print("Ready.")
_G._cravex_init = true

pcall(function()
    game:GetService("StarterGui"):SetCore("SendNotification", {
        Title = "Cravex",
        Text = "Successfully Attached to Roblox!",
        Duration = 5
    })
end)

return {
    HideTemp = function() end,
    GetIsModal = function() return false end,
    ToggleVisibility = function() end
}