function npcmove(player)
    print(player["name"] .. ": x = " .. player["x"] .. " y = " .. player["y"] .. " handle = " .. player["handle"]);
    if (movenpc ~= nil) then
       local x = player["x"];
       local y = player["y"];
       local tmp;
       if (math.random == nil) then
       	  print("random = nil"); 
       end
       tmp = math.random(1, 10);
       x = x + tmp;
       tmp = math.random(1, 10);
       y = y + tmp;
       if (x <= 0) then
           x = 0;
       end
       if (y <= 0) then
           y = 0;
       end
       if (x >= 100) then
           x = 100;
       end
       if (y >= 100) then
           y = 100;
       end
       print(player["handle"], x, y);
       local ret = movenpc(player["handle"], x, y);
       print("movenpc return " .. ret);
    end
--    print(player["name"], "x = ", player["x"], "y = ", player["y"]);
end

domove = "run";
while domove == "run" do
if (getnpc ~= nil) then
   local ret = getnpc(npcmove);
   print("getnpc return " .. ret);
end
sleep(1);
domove = stopmove();
end

print("lua end");