main: main.o json/json.o http/http.o server/server.o http_server/http_server.o ac_sys/ac_sys.o fsm/fsm.o
	gcc $? -lpthread -lssl -lcrypto -o main

clean: 
	rm main main.o */*.o