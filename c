if not getgenv then getgenv = function() return _G end end
local env = getgenv()

if not env.DragonFarmConfig then
    env.DragonFarmConfig = {
        SELL_THRESHOLD = 10000,
        MOVEMENT_SPEED = 0.8,
        ATTACK_DELAY = 0.02,
        TWEEN_SPEED = 250,
        
        FPS_BOOST_ENABLED = true,
        TARGET_FPS = 60,
        
        SHOW_FRUIT_GUI = true,
        SHOW_COIN_GUI = true,
        GUI_UPDATE_RATE = 0.3,
        
        SMART_PATHFINDING = true,
        AUTO_RETRY = true,
        MAX_RETRY = 3
    }
end

local CONFIG = env.DragonFarmConfig

--// 📦 SERVICES
local Players = game:GetService("Players")
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local TweenService = game:GetService("TweenService")
local RunService = game:GetService("RunService")

local player = Players.LocalPlayer
local character = player.Character or player.CharacterAdded:Wait()
local hrp = character:WaitForChild("HumanoidRootPart")
local humanoid = character:WaitForChild("Humanoid")

--// 🎯 OPTIMIZED VARIABLES
local activeTween = nil
local isAttacking = false
local lastNodePosition = nil
local farmingActive = true

--// 🔍 SMART FOLDER DETECTION
local function findFoodFolder()
    local paths = {
        {"Interactions", "Nodes", "Food"},
        {"Interactions", "Nodes", "LargeFoodNode"},
        {"Interactions", "Nodes", "Foods"},
        {"Interactions", "FoodNodes"},
        {"World", "Nodes", "Food"}
    }
    
    for _, path in ipairs(paths) do
        local current = workspace
        for _, name in ipairs(path) do
            current = current:FindFirstChild(name)
            if not current then break end
        end
        if current then return current end
    end
    
    local interactions = workspace:FindFirstChild("Interactions")
    if interactions then
        local nodes = interactions:FindFirstChild("Nodes")
        if nodes then
            return nodes:FindFirstChildWhichIsA("Folder")
        end
    end
    
    return nil
end

local FoodFolder = findFoodFolder()

if not FoodFolder then
    warn("⚠️ ไม่พบโฟลเดอร์ Food → จะลองค้นหาอัตโนมัติ")
end

local CollectableNames = {"EdamameFoodModel", "KajiFruitFoodModel", "MistSudachiFoodModel"}

local FARM_WORLD = 125804922932357
local SELL_WORLD = 3475397644
local RANDOM_WORLDS = {
    3475419198, 3475422608, 3487210751,
    3623549100, 3737848045, 3752680052,
    4174118306, 4728805070
}

local SMART_PATROL_ZONES = {
    {pos = Vector3.new(-266, 467, -394), radius = 150},
    {pos = Vector3.new(1757, 783, 672), radius = 200},
    {pos = Vector3.new(1420, 319, -1409), radius = 180},
    {pos = Vector3.new(1922, 620, -2566), radius = 150},
    {pos = Vector3.new(2226, 585, -4128), radius = 220},
    {pos = Vector3.new(330, 901, -4304), radius = 170},
    {pos = Vector3.new(-1433, 779, -4717), radius = 190},
    {pos = Vector3.new(-3031, 780, -4211), radius = 200},
    {pos = Vector3.new(-2643, 547, -2250), radius = 160},
    {pos = Vector3.new(-857, 401, -2132), radius = 140}
}

--// 🚀 ULTRA FPS BOOSTER
local function applyUltraOptimization()
    if not CONFIG.FPS_BOOST_ENABLED then return end
    
    print("🚀 กำลังเปิด Ultra Performance Mode...")
    
    pcall(function()
        settings().Rendering.QualityLevel = Enum.QualityLevel.Level01
        settings().Rendering.MeshPartDetailLevel = Enum.MeshPartDetailLevel.Level01
        
        local lighting = game:GetService("Lighting")
        for _, effect in ipairs(lighting:GetChildren()) do
            if effect:IsA("PostEffect") then
                effect.Enabled = false
            end
        end
        
        lighting.GlobalShadows = false
        lighting.FogEnd = 1e9
        lighting.Brightness = 0
        
        local terrain = workspace:FindFirstChildOfClass("Terrain")
        if terrain then
            terrain.WaterWaveSize = 0
            terrain.WaterWaveSpeed = 0
            terrain.WaterReflectance = 0
            terrain.WaterTransparency = 1
        end
        
        workspace.FallenPartsDestroyHeight = -5000
        
        if setfpscap then
            setfpscap(CONFIG.TARGET_FPS)
        end
    end)
    
    task.spawn(function()
        while CONFIG.FPS_BOOST_ENABLED do
            for _, obj in ipairs(workspace:GetDescendants()) do
                if obj:IsA("ParticleEmitter") or obj:IsA("Trail") or 
                   obj:IsA("Beam") or obj:IsA("Fire") or obj:IsA("Smoke") then
                    pcall(function() obj.Enabled = false end)
                elseif obj:IsA("Light") then
                    pcall(function() obj.Enabled = false end)
                elseif obj:IsA("Explosion") then
                    pcall(function() obj:Destroy() end)
                end
            end
            task.wait(10)
        end
    end)
    
    local function cleanupDecor()
        local patterns = {"tree", "leaf", "plant", "grass", "bush", "flower", 
                         "mushroom", "deco", "rock", "stone", "prop"}
        for _, obj in ipairs(workspace:GetDescendants()) do
            local lowerName = string.lower(obj.Name)
            for _, pattern in ipairs(patterns) do
                if lowerName:find(pattern) and obj:IsA("Model") then
                    pcall(function() obj:Destroy() end)
                    break
                end
            end
        end
    end
    
    task.spawn(function()
        cleanupDecor()
        task.wait(30)
        cleanupDecor()
    end)
    
    pcall(function()
        local playerGui = player:WaitForChild("PlayerGui")
        local workspaceGui = playerGui:FindFirstChild("WorkspaceGui")
        if workspaceGui then
            workspaceGui:Destroy()
        end
    end)
    
    print("✅ Ultra Performance Mode เปิดใช้งานสำเร็จ")
end

--// 🛡️ SMART MOB DAMAGE REMOVER
local function setupMobDamageProtection()
    local RS = game:GetService("ReplicatedStorage")
    local RemotesFolder = RS:WaitForChild("Remotes", 5)
    if not RemotesFolder then return end
    
    local function removeAllMobDamage()
        local count = 0
        for _, obj in ipairs(RemotesFolder:GetDescendants()) do
            if obj.Name == "MobDamageRemote" then
                pcall(function() 
                    obj:Destroy() 
                    count += 1
                end)
            end
        end
        return count
    end
    
    task.spawn(function()
        for i = 1, 10 do
            local removed = removeAllMobDamage()
            if removed > 0 then
                print(("🛡️ ลบ MobDamageRemote จำนวน %d"):format(removed))
            end
            task.wait(1)
        end
        
        RemotesFolder.DescendantAdded:Connect(function(obj)
            if obj.Name == "MobDamageRemote" then
                task.defer(function()
                    pcall(function() obj:Destroy() end)
                end)
            end
        end)
    end)
end

--// 🎯 SMART MOVEMENT SYSTEM
local function stopCurrentTween()
    if activeTween then
        activeTween:Cancel()
        activeTween = nil
    end
end

local function smoothTweenTo(targetCFrame, speed)
    if not targetCFrame then return false end
    
    stopCurrentTween()
    
    local distance = (hrp.Position - targetCFrame.Position).Magnitude
    local tweenTime = distance / (speed or CONFIG.TWEEN_SPEED)
    
    local tweenInfo = TweenInfo.new(
        tweenTime,
        Enum.EasingStyle.Quad,
        Enum.EasingDirection.Out
    )
    
    activeTween = TweenService:Create(hrp, tweenInfo, {CFrame = targetCFrame})
    activeTween:Play()
    
    return true
end

--// 🐉 OPTIMIZED DRAGON ATTACK
local function getDragonRemote()
    local dragonsFolder = character:FindFirstChild("Dragons")
    if not dragonsFolder then return nil end
    
    for _, dragon in ipairs(dragonsFolder:GetChildren()) do
        local remotes = dragon:FindFirstChild("Remotes")
        if remotes then
            local remote = remotes:FindFirstChild("PlaySoundRemote")
            if remote then return remote end
        end
    end
    return nil
end

local function attackNode(targetNode)
    if isAttacking then return false end
    if not targetNode or not targetNode.Parent then return false end
    
    local health = targetNode:FindFirstChild("Health")
    if not health or health.Value <= 0 then return false end
    
    isAttacking = true
    local remote = getDragonRemote()
    
    if not remote then 
        isAttacking = false
        return false 
    end
    
    local attackCount = 0
    local maxAttacks = 500
    
    while health and health.Parent and health.Value > 0 and attackCount < maxAttacks do
        if not targetNode.Parent then break end
        
        pcall(function()
            remote:FireServer("Breath", "Destructibles", targetNode)
        end)
        
        attackCount += 1
        task.wait(CONFIG.ATTACK_DELAY)
    end
    
    isAttacking = false
    return true
end

--// 🌳 SMART NODE DETECTION
local function getValidNodes()
    if not FoodFolder then 
        FoodFolder = findFoodFolder()
        if not FoodFolder then return {} end
    end
    
    local validNodes = {}
    
    for _, folder in ipairs(FoodFolder:GetChildren()) do
        for _, node in ipairs(folder:GetDescendants()) do
            local health = node:FindFirstChild("Health")
            if health and health.Value > 0 and node:IsA("BasePart") then
                table.insert(validNodes, {
                    node = node,
                    distance = (hrp.Position - node.Position).Magnitude,
                    health = health.Value
                })
            end
        end
    end
    
    table.sort(validNodes, function(a, b)
        return a.distance < b.distance
    end)
    
    return validNodes
end

local function findBestNode()
    local nodes = getValidNodes()
    
    if #nodes == 0 then return nil end
    
    if lastNodePosition then
        for _, data in ipairs(nodes) do
            local distToLast = (data.node.Position - lastNodePosition).Magnitude
            if distToLast < 100 then
                return data.node
            end
        end
    end
    
    return nodes[1].node
end

--// 🍎 OPTIMIZED COLLECTION
local function collectNearbyItems()
    local collected = 0
    for _, obj in ipairs(workspace.Camera:GetChildren()) do
        for _, name in ipairs(CollectableNames) do
            if obj.Name == name then
                for _, part in ipairs(obj:GetDescendants()) do
                    if part:IsA("BasePart") then
                        pcall(function()
                            part.CFrame = hrp.CFrame
                            collected += 1
                        end)
                    end
                end
            end
        end
    end
    return collected
end

--// 🚜 PROFESSIONAL FARMING SYSTEM
local function executeFarmingCycle()
    local retryCount = 0
    
    while farmingActive do
        local targetNode = findBestNode()
        
        if targetNode and targetNode:FindFirstChild("Health") then
            retryCount = 0
            
            local targetPos = targetNode.CFrame * CFrame.new(0, 8, 0)
            smoothTweenTo(targetPos, CONFIG.TWEEN_SPEED)
            
            task.wait(0.5)
            
            local success = attackNode(targetNode)
            if success then
                lastNodePosition = targetNode.Position
                print("✅ ทำลายต้นไม้สำเร็จ")
            end
            
            collectNearbyItems()
            
        else
            retryCount += 1
            
            if retryCount >= CONFIG.MAX_RETRY then
                warn("⚠️ ไม่พบต้นไม้ → เริ่ม Smart Patrol")
                
                for _, zone in ipairs(SMART_PATROL_ZONES) do
                    smoothTweenTo(CFrame.new(zone.pos), CONFIG.TWEEN_SPEED)
                    task.wait(1.5)
                    
                    local nearbyNode = findBestNode()
                    if nearbyNode then
                        print("✅ พบต้นไม้ใหม่ใน Patrol Zone")
                        break
                    end
                end
                
                retryCount = 0
            else
                task.wait(2)
            end
        end
        
        task.wait(0.5)
    end
end

local function startAutoFarm()
    task.spawn(function()
        executeFarmingCycle()
    end)
end

local function startAutoCollect()
    task.spawn(function()
        while true do
            task.wait(1)
            collectNearbyItems()
        end
    end)
end

--// 💰 SMART SELLING
local function sellAllResources()
    local resources = player:WaitForChild("Data"):WaitForChild("Resources")
    local items = {"Edamame", "KajiFruit", "MistSudachi"}
    local totalSold = 0
    
    for _, itemName in ipairs(items) do
        local itemValue = resources:FindFirstChild(itemName)
        if itemValue and itemValue.Value > 0 then
            local amount = itemValue.Value
            pcall(function()
                ReplicatedStorage.Remotes.SellItemRemote:FireServer({{
                    ItemName = itemName,
                    Amount = amount
                }})
                totalSold += 1
                print(("💰 ขาย %s จำนวน %d"):format(itemName, amount))
            end)
            task.wait(0.3)
        end
    end
    
    return totalSold
end

local function monitorInventory()
    task.spawn(function()
        while true do
            task.wait(2)
            
            local resources = player:WaitForChild("Data"):WaitForChild("Resources")
            local counts = {}
            
            for _, item in ipairs({"Edamame", "KajiFruit", "MistSudachi"}) do
                local val = resources:FindFirstChild(item)
                counts[item] = val and val.Value or 0
            end
            
            local shouldSell = false
            for _, count in pairs(counts) do
                if count >= CONFIG.SELL_THRESHOLD then
                    shouldSell = true
                    break
                end
            end
            
            if shouldSell then
                print("📦 ถึง Threshold → วาร์ปไปขาย")
                farmingActive = false
                stopCurrentTween()
                
                task.wait(2)
                pcall(function()
                    ReplicatedStorage.Remotes.WorldTeleportRemote:InvokeServer(SELL_WORLD, {})
                end)
                break
            end
        end
    end)
end

--// 👥 SMART PLAYER MONITOR
local function monitorPlayers()
    task.spawn(function()
        while true do
            task.wait(3)
            
            local playerCount = #Players:GetPlayers()
            
            if playerCount > 1 then
                warn("⚠️ ตรวจพบผู้เล่นอื่น → วาร์ปหนี")
                farmingActive = false
                stopCurrentTween()
                
                task.wait(2)
                
                local randomWorld = RANDOM_WORLDS[math.random(1, #RANDOM_WORLDS)]
                pcall(function()
                    ReplicatedStorage.Remotes.WorldTeleportRemote:InvokeServer(randomWorld, {})
                end)
                
                print("🌍 วาร์ปไปโลก:", randomWorld)
                task.wait(10)
                
                farmingActive = true
            end
        end
    end)
end

--// 🎨 PROFESSIONAL GUI
local function createModernGUI()
    local existing = game.CoreGui:FindFirstChild("FarmGUI")
    if existing then existing:Destroy() end
    
    local screenGui = Instance.new("ScreenGui")
    screenGui.Name = "FarmGUI"
    screenGui.ResetOnSpawn = false
    screenGui.IgnoreGuiInset = true
    screenGui.Parent = game.CoreGui
    
    if CONFIG.SHOW_COIN_GUI then
        local coinFrame = Instance.new("Frame")
        coinFrame.Size = UDim2.new(0, 280, 0, 70)
        coinFrame.Position = UDim2.new(0.5, -140, 0.05, 0)
        coinFrame.BackgroundColor3 = Color3.fromRGB(15, 15, 15)
        coinFrame.BackgroundTransparency = 0.2
        coinFrame.BorderSizePixel = 0
        coinFrame.Parent = screenGui
        
        local corner = Instance.new("UICorner")
        corner.CornerRadius = UDim.new(0, 15)
        corner.Parent = coinFrame
        
        local coinLabel = Instance.new("TextLabel")
        coinLabel.Size = UDim2.new(1, -20, 1, -20)
        coinLabel.Position = UDim2.new(0, 10, 0, 10)
        coinLabel.BackgroundTransparency = 1
        coinLabel.TextColor3 = Color3.fromRGB(255, 215, 0)
        coinLabel.TextStrokeTransparency = 0.3
        coinLabel.TextScaled = true
        coinLabel.Font = Enum.Font.GothamBold
        coinLabel.Text = "💰 Loading..."
        coinLabel.Parent = coinFrame
        
        task.spawn(function()
            local coinsValue = player:WaitForChild("Data"):WaitForChild("Currency"):WaitForChild("Coins")
            while task.wait(CONFIG.GUI_UPDATE_RATE) do
                coinLabel.Text = string.format("💰 %s Coins", tostring(coinsValue.Value))
            end
        end)
    end
    
    if CONFIG.SHOW_FRUIT_GUI then
        local fruitFrame = Instance.new("Frame")
        fruitFrame.Size = UDim2.new(0, 300, 0, 170)
        fruitFrame.Position = UDim2.new(0.5, -150, 0.15, 0)
        fruitFrame.BackgroundColor3 = Color3.fromRGB(15, 15, 15)
        fruitFrame.BackgroundTransparency = 0.2
        fruitFrame.BorderSizePixel = 0
        fruitFrame.Parent = screenGui
        
        local frameCorner = Instance.new("UICorner")
        frameCorner.CornerRadius = UDim.new(0, 15)
        frameCorner.Parent = fruitFrame
        
        local fruits = {
            {name = "Edamame", icon = "🫘", color = Color3.fromRGB(144, 238, 144)},
            {name = "KajiFruit", icon = "🍊", color = Color3.fromRGB(255, 165, 0)},
            {name = "MistSudachi", icon = "🍋", color = Color3.fromRGB(255, 255, 102)}
        }
        
        for i, fruit in ipairs(fruits) do
            local label = Instance.new("TextLabel")
            label.Name = fruit.name
            label.Size = UDim2.new(1, -30, 0, 45)
            label.Position = UDim2.new(0, 15, 0, (i-1) * 50 + 15)
            label.BackgroundTransparency = 1
            label.TextColor3 = fruit.color
            label.TextStrokeTransparency = 0.3
            label.TextScaled = true
            label.Font = Enum.Font.GothamBold
            label.TextXAlignment = Enum.TextXAlignment.Left
            label.Text = fruit.icon .. " Loading..."
            label.Parent = fruitFrame
        end
        
        task.spawn(function()
            local resources = player:WaitForChild("Data"):WaitForChild("Resources")
            while task.wait(CONFIG.GUI_UPDATE_RATE) do
                for _, fruit in ipairs(fruits) do
                    local val = resources:FindFirstChild(fruit.name)
                    local label = fruitFrame:FindFirstChild(fruit.name)
                    if val and label then
                        label.Text = string.format("%s %s: %d", fruit.icon, fruit.name, val.Value)
                    end
                end
            end
        end)
    end
end

--// 🔄 ANTI-AFK
local function enableAntiAFK()
    task.spawn(function()
        local VIM = game:GetService("VirtualInputManager")
        while true do
            task.wait(180)
            pcall(function()
                VIM:SendKeyEvent(true, Enum.KeyCode.Space, false, game)
                task.wait(0.1)
                VIM:SendKeyEvent(false, Enum.KeyCode.Space, false, game)
            end)
        end
    end)
end

--// 🚀 MAIN EXECUTION
print("=" :rep(50))
print("🐉 PROFESSIONAL DRAGON FARM SCRIPT")
print("=" :rep(50))

applyUltraOptimization()
createModernGUI()
enableAntiAFK()

local currentPlace = game.PlaceId

if currentPlace == FARM_WORLD then
    print("🚜 โหมดฟาร์ม → เริ่มทำงาน")
    task.wait(2)
    
    setupMobDamageProtection()
    startAutoFarm()
    startAutoCollect()
    monitorInventory()
    monitorPlayers()
    
elseif currentPlace == SELL_WORLD then
    print("💰 โหมดขาย → ขายของแล้วกลับฟาร์ม")
    task.wait(3)
    
    sellAllResources()
    task.wait(2)
    
    pcall(function()
        ReplicatedStorage.Remotes.WorldTeleportRemote:InvokeServer(FARM_WORLD, {})
    end)
    
else
    print("🌍 อยู่นอกโลก → วาร์ปกลับฟาร์ม")
    task.wait(1)
    
    pcall(function()
        ReplicatedStorage.Remotes.WorldTeleportRemote:InvokeServer(FARM_WORLD, {})
    end)
end

print("✅ Script เริ่มทำงานแล้ว")
