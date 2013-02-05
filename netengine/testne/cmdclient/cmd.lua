function printnpc(player)
    print(player["name"], "x = ", player["x"], "y = ", player["y"]);
end

function callback(func)
    local tang = {};
    tang["name"] = "tangpeilei";
    func(tang);
end


print("call lua_addnpc");

--[[
if (addnpc ~= nil) then
   ret = addnpc("tang", 2.3334, 56.785);
   print("addnpc return " .. ret);
end
if (reload ~= nil) then
   reload();
end
--]]

if (getnpc ~= nil) then
    ret = getnpc(printnpc);
    print(ret);

--   print(npc["11a"]);
end

-- callback(printnpc);

print("lua end");