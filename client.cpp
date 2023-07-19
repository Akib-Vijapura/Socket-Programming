#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

class Client
{
public:
    Client() : clientSocket(-1) {}

    bool connectToServer(const char *serverAddress, int port)
    {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1)
        {
            std::cerr << "Failed to create socket." << std::endl;
            return false;
        }

        serverAddress_.sin_family = AF_INET;
        serverAddress_.sin_port = htons(port);
        if (inet_pton(AF_INET, serverAddress, &(serverAddress_.sin_addr)) <= 0)
        {
            std::cerr << "Invalid server address." << std::endl;
            return false;
        }

        if (connect(clientSocket, (struct sockaddr *)&serverAddress_, sizeof(serverAddress_)) == -1)
        {
            std::cerr << "Failed to connect to server." << std::endl;
            return false;
        }

        return true;
    }

    bool sendMessage(const char *message)
    {
        if (send(clientSocket, message, std::strlen(message), 0) == -1)
        {
            std::cerr << "Failed to send message to server." << std::endl;
            return false;
        }
        return true;
    }

    bool receiveMessage(char *buffer, int bufferSize)
    {
        std::memset(buffer, 0, bufferSize);
        if (recv(clientSocket, buffer, bufferSize - 1, 0) == -1)
        {
            std::cerr << "Failed to receive message from server." << std::endl;
            return false;
        }
        return true;
    }

    void closeConnection()
    {
        close(clientSocket);
        clientSocket = -1;
    }

private:
    int clientSocket;
    sockaddr_in serverAddress_;
};

int main()
{
    Client client;

    if (!client.connectToServer("127.0.0.1", 12345))
    {
        return 1;
    }

    char buffer[1024];

    if (!client.receiveMessage(buffer, sizeof(buffer)))
    {
        client.closeConnection();
        return 1;
    }

    std::cout << "Server says: " << buffer << std::endl;

    while (true)
    {
        std::cout << "Client says: ";
        std::cin.getline(buffer, sizeof(buffer));

        if (!client.sendMessage(buffer))
        {
            break;
        }

        if (!client.receiveMessage(buffer, sizeof(buffer)))
        {
            break;
        }

        std::cout << "Server says: " << buffer << std::endl;

        if (std::strcmp(buffer, "bye") == 0)
        {
            std::cout << "Client disconnect done." << std::endl;
            break;
        }
    }

    client.closeConnection();

    return 0;
}
