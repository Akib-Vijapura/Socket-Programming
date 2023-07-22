#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <map>

class TCPServer
{
public:
    TCPServer(int port) : port_(port) {}

    void Start()
    {
        if (!CreateSocket())
        {
            std::cerr << "Failed to create socket." << std::endl;
            return;
        }

        if (!BindSocket())
        {
            std::cerr << "Failed to bind socket." << std::endl;
            return;
        }

        if (!ListenForConnections())
        {
            std::cerr << "Failed to listen for connections." << std::endl;
            return;
        }

        std::cout << "Server listening for incoming connections..." << std::endl;

        while (true)
        {
            AcceptClientConnection();
        }
    }

private:
    int serverSocket_;
    int port_;
    int clientCount_ = 0;

    std::map<std::string, int> clientConnections_; // Map to track client connections.
    bool CreateSocket()
    {
        serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        return serverSocket_ != -1;
    }

    bool BindSocket()
    {
        serverAddress_.sin_family = AF_INET;
        serverAddress_.sin_port = htons(port_);
        serverAddress_.sin_addr.s_addr = INADDR_ANY;

        return bind(serverSocket_, (struct sockaddr *)&serverAddress_, sizeof(serverAddress_)) != -1;
    }

    bool ListenForConnections()
    {
        return listen(serverSocket_, 5) != -1;
    }

    void AcceptClientConnection()
    {
        socklen_t clientAddressLength = sizeof(clientAddress_);

        int clientSocket = accept(serverSocket_, (struct sockaddr *)&clientAddress_, &clientAddressLength);
        if (clientSocket == -1)
        {
            std::cerr << "Failed to accept client connection." << std::endl;
            return;
        }

        char clientAddressStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress_.sin_addr), clientAddressStr, INET_ADDRSTRLEN);

        if (IsRateLimited(clientAddressStr))
        {
            std::cerr << "Rate limited connection from client: " << clientAddressStr << std::endl;
            close(clientSocket);
            return;
        }

        // Increment the connection count for this client IP.
        clientConnections_[clientAddressStr]++;

        ++clientCount_;
        std::cout << "New client connected. Client count : " << clientCount_ << std::endl;

        std::thread clientThread(&TCPServer::HandleClientConnection, this, clientSocket);
        clientThread.detach();
    }

    void HandleClientConnection(int clientSocket)
    {
        char clientAddressStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress_.sin_addr), clientAddressStr, INET_ADDRSTRLEN);
        std::cout << "Client connected: " << clientAddressStr << std::endl;

        const char *message = "Hello, client! Welcome to the chat. Type 'bye' to disconnect.\n";
        std::string clientNumber = std::to_string(clientCount_);
        std::string fullMessage = message + clientNumber;

        if (send(clientSocket, fullMessage.c_str(), std::strlen(fullMessage.c_str()), 0) == -1)
        {
            std::cerr << "Failed to send message to client." << std::endl;
            close(clientSocket);
            return;
        }

        char buffer[1024];
        std::memset(buffer, 0, sizeof(buffer));

        while (true)
        {
            if (recv(clientSocket, buffer, sizeof(buffer) - 1, 0) == -1)
            {
                std::cerr << "Failed to receive message from client." << std::endl;
                break;
            }

            std::cout << "Client " << clientNumber << " says: " << buffer << std::endl;

            if (std::strcmp(buffer, "bye") == 0)
            {
                std::cout << "Client " << clientNumber << " initiated disconnect." << std::endl;
                send(clientSocket, buffer, std::strlen(buffer), 0);
                break;
            }

            std::memset(buffer, 0, sizeof(buffer));

            std::cout << "Server says: ";
            std::cin.getline(buffer, sizeof(buffer));

            if (send(clientSocket, buffer, std::strlen(buffer), 0) == -1)
            {
                std::cerr << "Failed to send message to client." << std::endl;
                break;
            }
        }

        close(clientSocket);
        std::cout << "Client " << clientNumber << " disconnected." << std::endl;
        --clientCount_;
    }

    bool IsRateLimited(const std::string &clientAddressStr)
    {
        const int maxConnectionsPerIP = 5;  // Maximum allowed connections per IP.
        const int timeWindowInSeconds = 60; // Time window to count connections (in seconds).

        // Check if the IP exists in the map.
        if (clientConnections_.find(clientAddressStr) != clientConnections_.end())
        {
            // Get the last connection time for the IP.
            time_t lastConnectionTime = clientConnections_[clientAddressStr];

            // Check if the time since the last connection exceeds the time window.
            time_t currentTime = time(nullptr);
            if (currentTime - lastConnectionTime < timeWindowInSeconds)
            {
                // If the IP has exceeded the maximum connections, return true (rate limited).
                if (clientConnections_[clientAddressStr] >= maxConnectionsPerIP)
                    return true;
            }
        }

        // If the IP is not in the map or has not exceeded the rate limit, update the last connection time.
        clientConnections_[clientAddressStr] = time(nullptr);
        return false;
    }

    void CloseSocket()
    {
        close(serverSocket_);
    }

    sockaddr_in serverAddress_;
    sockaddr_in clientAddress_;
};

int main()
{
    try
    {
        TCPServer server(12345);
        server.Start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
