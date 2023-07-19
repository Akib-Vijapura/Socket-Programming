all: server client

server: server.cpp
	@echo "compiling server"
	g++ server.cpp -o server.o

client: client.cpp
	@echo "compiling client"
	g++ client.cpp -o client.o

clean:
	rm -f server.o client.o
