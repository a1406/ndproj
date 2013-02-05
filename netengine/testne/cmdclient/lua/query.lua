function printnpc(player)
    print(player["name"] .. ": x = " .. player["x"] .. " y = " .. player["y"] .. " handle = " .. player["handle"]);
--    print(player["name"], "x = ", player["x"], "y = ", player["y"]);
end

if (getnpc ~= nil) then
   local ret = getnpc(printnpc);
   print("getnpc return " .. ret);
end

print("lua end");