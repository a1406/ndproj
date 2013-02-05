print(collectgarbage("count"));
print("hello world");
print(collectgarbage("count"));
collectgarbage("collect");
print(collectgarbage("count"));
