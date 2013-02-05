listen_port = 12345
max_client = 8192
map_length = 200

userid_start = 0

mapserver_config = {
	 {"127.0.0.1", 12346},
--	 {"127.0.0.1", 12347},
};

mapserver_count = table.getn(mapserver_config);
--print(mapserver_count);



