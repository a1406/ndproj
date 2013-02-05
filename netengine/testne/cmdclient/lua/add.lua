if (addnpc ~= nil) then
   for i = 1,10,1 do
       local name = "tttaaa" .. i;
       local ret = addnpc(name);
       print("addnpc return " .. ret);
   end
end

print("lua end");