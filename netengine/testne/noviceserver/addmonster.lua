if (addmonster ~= nil) then
   local ret;
   local i;
   for i = 3.3, 193.3, 10 do
       ret = addmonster(16, i, 3,3);       
       print("addnpc return " .. ret);
   end
end

print("lua end");