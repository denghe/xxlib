print = lua_release_print

local scene = cc_scene_create()
cc_director_runWithScene(scene)

local spr = cc_sprite_create()
spr:setTexture("HelloWorld.png")
spr:setPositionXY(300, 300)
scene:addChild(spr)

local c3 = spr:getColor()
c3[1] = 0
spr:setColor(c3)

local r, g, b = spr:getColorRGB()
g = 0;
spr:setColorRGB(r, g, b)

spr:schedule(function(delta)
    local v2 = spr:getPosition()
    v2[2] = v2[2] + delta * 20    -- change y
    spr:setPosition( v2 )
end)

cc_setFrameUpdate(function(delta)
    local x, y = spr:getPositionXY()
    spr:setPositionXY( x + delta * 20, y )    -- change x
end)

print("end")
