if _G._cravex_init then return {HideTemp = function() end} end


local coreGui, httpSvc, players = game:GetService("CoreGui"), game:GetService("HttpService"), game:GetService("Players")

local safeWait = task and task.wait or wait
local safeSpawn = task and task.spawn or function(f, ...) coroutine.wrap(f)(...) end
local clockFunc = os and os.clock or tick

local baseDir = Instance.new("Folder", coreGui); baseDir.Name = "CravexBase"
local pointDir = Instance.new("Folder", baseDir); pointDir.Name = "Pointer"

local lvlBindthing = Instance.new("BindableEvent")
lvlBindthing.Event:Connect(function() end)
local idPtr = Instance.new("ObjectValue")
idPtr.Name = "lvlBindthing"
idPtr.Value = lvlBindthing
idPtr.Parent = pointDir

local apiAddr = "http://localhost:6767"
local procId = "-$-crvx-procid-$-"

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
    bridgeQueue[reqId] = {typ = typ, data = data, settings = settings}
    while bridgeResults[reqId] == nil do safeWait() end
    local res = bridgeResults[reqId]
    bridgeResults[reqId] = nil
    return res
end

local env = getfenv(function() end)
env.getgenv = function() return env end

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
    if bc:sub(1, 5) == "null:" then
        warn("[Cravex] getscriptbytecode failed: " .. bc:sub(6))
        return nil
    elseif bc == "null" or bc == "" then
        return nil
    end
    return bc
end

env.getnilinstances = function()
    local cached = env.getinstances()
    local res = {}
    for i = 1, #cached do
        local obj = cached[i]
        if not obj.Parent then
            table.insert(res, obj)
        end
    end
    return res
end

env.getscripts = function()
    local res = {}
    local cg = game:GetService("CoreGui")
    local cp = game:GetService("CorePackages")
    for _, s in ipairs(env.getinstances()) do
        if (s:IsA("LocalScript") or s:IsA("ModuleScript") or s:IsA("Script")) and not s:IsA("CoreScript") then
            if not s:IsDescendantOf(cg) and not s:IsDescendantOf(cp) then
                table.insert(res, s)
            end
        end
    end
    return res
end

local scriptTarget = coreGui:FindFirstChild("RobloxGui").Modules.Common:FindFirstChild("Constants") or coreGui:FindFirstChild("RobloxGui").Modules.Common:FindFirstChild("CommonUtil")
env.loadstring = function(src, chunk)
    if type(src) ~= "string" then return nil, "Expected string" end
    chunk = chunk or "loadstring"

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
    local bc = env.getscriptbytecode(scr)
    if not bc then return nil end
    local func, err = env.loadstring(bc)
    return func
end

env.getscripthash = function(scr)
    local bc = env.getscriptbytecode(scr)
    if not bc then return nil end
    return typeof(env.crypt) == "table" and type(env.crypt.hash) == "function" and env.crypt.hash(bc, "sha384") or "mock_hash"
end

env.getrenv = function() return _G end
env.getreg = function() return debug.getregistry() end -- not working btw

_G.loadstring = env.loadstring
getfenv(0).loadstring = env.loadstring

env.getinstances = function()
    local insts = {}
    local reg = env.getreg()
    for _, v in pairs(reg) do
        if typeof(v) == "Instance" then
            table.insert(insts, v)
        end
    end
    return insts
end



-- Request & Crypt
env.request = function(options)
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

env.HttpGet = function(...)
    local args = {...}
    local url = type(args[1]) == "string" and args[1] or args[2]
    if type(url) ~= "string" then return "" end
    local resp = env.request({Url = url})
    if not resp.Success then
        warn("[Cravex] HttpGet failed for " .. url .. ": " .. tostring(resp.Body))
        return ""
    end
    return resp.Body or ""
end
env.HttpPost = function(...)
    local args = {...}
    local url = type(args[1]) == "string" and args[1] or args[2]
    local body = type(args[1]) == "string" and args[2] or args[3]
    if type(url) ~= "string" then return "" end
    local resp = env.request({Url = url, Method = "POST", Body = type(body) == "string" and body or ""})
    return resp and resp.Body or ""
end

env.GetObjects = function(asset)
    return { game:GetService("InsertService"):LoadLocalAsset(asset) }
end

-- game proxy
local realGame = game
env.game = newproxy(true)
local gameMeta = getmetatable(env.game)

local function getReal(v) return v == env.game and realGame or v end

gameMeta.__index = function(self, key)
    if key == "HttpGet" or key == "HttpGetAsync" then return function(_, ...) return env.HttpGet(...) end end
    if key == "HttpPost" or key == "HttpPostAsync" then return function(_, ...) return env.HttpPost(...) end end
    
    if key == "GetService" or key == "getService" or key == "service" or key == "FindService" or key == "findService" then
        return function(_, serviceName)
            local s = realGame:GetService(serviceName)
            if s == realGame then return env.game end
            return s
        end
    end

    local val = realGame[key]
    if type(val) == "function" then
        return function(s, ...)
            return val(getReal(s), ...)
        end
    end
    return val
end

gameMeta.__newindex = function(_, k, v) realGame[k] = v end
gameMeta.__tostring = function() return "game" end
gameMeta.__metatable = "The metatable is locked"

-- hooks
local oldInstanceNew = Instance.new
env.Instance = {
    new = function(cls, parent)
        return oldInstanceNew(cls, getReal(parent))
    end
}

local oldTypeof = typeof
env.typeof = function(obj)
    if obj == env.game then return "Instance" end
    return oldTypeof(obj)
end

local oldType = type
env.type = function(obj)
    if obj == env.game then return "userdata" end
    return oldType(obj)
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

env.checkcaller = function() return true end
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
    new = function(type)
        local obj = {
            __type = "Drawing", Type = type,
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
        
        if type == "Line" then
            obj.Thickness = 1
            obj.From = Vector2.new()
            obj.To = Vector2.new()
            obj.Instance = Instance.new("Frame")
            obj.Instance.AnchorPoint = Vector2.new(0.5, 0.5)
            obj.Instance.BorderSizePixel = 0
        elseif type == "Text" then
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
        elseif type == "Circle" then
            obj.Thickness = 1
            obj.NumSides = 0
            obj.Radius = 0
            obj.Filled = false
            obj.Position = Vector2.new()
            obj.Instance = Instance.new("Frame")
            obj.Instance.BorderSizePixel = 0
            Instance.new("UICorner", obj.Instance).CornerRadius = UDim.new(1, 0)
        elseif type == "Square" then
            obj.Thickness = 1
            obj.Size = Vector2.new()
            obj.Position = Vector2.new()
            obj.Filled = false
            obj.Instance = Instance.new("Frame")
            obj.Instance.BorderSizePixel = 0
        elseif type == "Quad" then
            obj.Thickness = 1
            obj.PointA = Vector2.new()
            obj.PointB = Vector2.new()
            obj.PointC = Vector2.new()
            obj.PointD = Vector2.new()
            obj.Filled = false
            obj.Instance = Instance.new("Frame")
            obj.Instance.BackgroundTransparency = 1
        elseif type == "Triangle" then
            obj.Thickness = 1
            obj.PointA = Vector2.new()
            obj.PointB = Vector2.new()
            obj.PointC = Vector2.new()
            obj.Filled = false
            obj.Instance = Instance.new("Frame")
            obj.Instance.BackgroundTransparency = 1
        elseif type == "Image" then
            obj.Data = ""
            obj.Size = Vector2.new()
            obj.Position = Vector2.new()
            obj.Rounding = 0
            obj.Instance = Instance.new("ImageLabel")
            obj.Instance.BackgroundTransparency = 1
        else
            error("Invalid Drawing type")
        end
        
        obj.Instance.Parent = drawingUI
        drawings[obj] = true
        
        local proxy = newproxy(true)
        local meta = getmetatable(proxy)
        meta.__index = function(_, k) 
            if k == "TextBounds" and type == "Text" then
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

local pMap = setmetatable({}, { __mode = "k" })

env.cloneref = function(obj)
    if typeof(obj) ~= "Instance" then return obj end
    
    local proxy = newproxy(true)
    local m = getmetatable(proxy)
    
    m.__index = function(_, k)
        local val = obj[k]
        if type(val) == "function" then
            return function(s, ...)
                return val(s == proxy and obj or s, ...)
            end
        end
        return val
    end
    
    m.__namecall = function(_, ...)
        local method = env.getnamecallmethod()
        return obj[method](obj, ...)
    end
    
    m.__newindex = function(_, k, v)
        obj[k] = v
    end
    
    m.__tostring = function()
        return tostring(obj)
    end
    
    m.__metatable = "The metatable is locked"
    
    pMap[proxy] = obj
    return proxy
end

env.compareinstances = function(p1, p2)
    local r1 = pMap[p1] or p1
    local r2 = pMap[p2] or p2
    return r1 == r2 
end

-- clipboard
env.setclipboard = function(data) bridgeReq("setclipboard", tostring(data)) end
env.toclipboard = env.setclipboard
env.getclipboard = function() return bridgeReq("getclipboard") end

-- decompile // uses konstant decompiler for this
local API: string = "http://api.plusgiant5.com"

local last_call = 0
local function call(konstantType: string, scriptPath: Script | ModuleScript | LocalScript): string
    local success: boolean, bytecode: string = pcall(getscriptbytecode, scriptPath)

    if (not success) then
        return `-- Failed to get script bytecode, error:\n\n--[[\n{bytecode}\n--]]`
    end

    local time_elapsed = os.clock() - last_call
    if time_elapsed <= .5 then
        task.wait(.5 - time_elapsed)
    end
    local httpResult = request({
        Url = API .. konstantType,
        Body = bytecode,
        Method = "POST",
        Headers = {
            ["Content-Type"] = "text/plain"
        },
    })
    last_call = os.clock()
    
    if (httpResult.StatusCode ~= 200) then
        return `-- Error occured while requesting the API, error:\n\n--[[\n{httpResult.Body}\n--]]`
    else
        return httpResult.Body
    end
end

local function decompile(scriptPath: Script | ModuleScript | LocalScript): string
    return call("/konstant/decompile", scriptPath)
end

local function disassemble(scriptPath: Script | ModuleScript | LocalScript): string
    return call("/konstant/disassemble", scriptPath)
end

env.decompile = decompile
env.disassemble = disassemble

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

print("Ready.")
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