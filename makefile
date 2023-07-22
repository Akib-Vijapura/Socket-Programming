all: server client

server: server.cpp
	@echo "compiling server"
	g++ -std=c++11 server.cpp -o server.o

client: client.cpp
	@echo "compiling client"
	g++ -std=c++11 client.cpp -o client.o

clean:
	rm -f server.o client.o
