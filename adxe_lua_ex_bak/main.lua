print = release_print

local scene = cc_Scene_create()
cc_runWithScene(scene)

local spr = cc_Sprite_create()
spr:setTexture("HelloWorld.png")
scene:addChild(spr)

local for10000000 = function()
    for i = 1, 10000000 do
    end
end
local beginTime = os.clock()
for10000000()
release_print( "for10000000 elapsed time = ", os.clock() - beginTime)

local setGetPos10000000 = function(tar)
    for i = 1, 10000000 do
	    tar:setPosition(tar:getPosition())
    end
end
local beginTime = os.clock()
setGetPos10000000(spr)
print("setGetPos10000000 elapsed time = ", os.clock() - beginTime)

local setColor10000000 = function(tar)
    for i = 1, 10000000 do
	    tar:setColor(255, 255, 255)
    end
end
local beginTime = os.clock()
setColor10000000(spr)
print("setColor10000000 elapsed time = ", os.clock() - beginTime)

print("end")

spr:setPosition(300, 300)

local r, g, b = spr:getColor()
g = 0;
spr:setColor(r, g, b)

cc_setFrameUpdate(function(delta)
    local x, y = spr:getPosition()
    spr:setPosition( x + delta * 20, y )    -- change x
end)
