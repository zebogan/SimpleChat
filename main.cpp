#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <thread>
#include <string>
#include <algorithm>

/*
 * SimpleChat (formerly booger)
 * 
 * its a garbage chat program so i can learn c++ networking and networking in general
 * only supports 2 users for now
 * probably a memory leak somewhere bc i have no idea what im doing
 * 
 * no comments for now thats a problem for future me
 * 
 * https://github.com/zebogan/SimpleChat
 * 
 * thanks to https://beej.us/guide/bgnet/html/split-wide/system-calls-or-bust.html
 * also chatgpt and random stack overflow posts
 * professional programmer™
 */

void recieveStuff(int new_sock);
void sendStuff(int new_sock);

// "global variables are bad coding practice" -🤓
bool going = true;

int main() {
    //variable declaration 🤯
    int sock, new_fd;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo userInfo, *serverInfo;
    memset(&userInfo, 0, sizeof userInfo);
    userInfo.ai_family = AF_UNSPEC;
    userInfo.ai_socktype = SOCK_STREAM;
    userInfo.ai_flags = AI_PASSIVE;
    std::cout << "vars done\n";

    int status = getaddrinfo(NULL, "8088", &userInfo, &serverInfo);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return 1;
    }
    std::cout << "getaddrinfo done\n";

    sock = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (sock == -1) {
        perror("socket");
        return 1;
    }
    std::cout << "sock done\n";
    if (bind(sock, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
        perror("bind");
        return 1;
    }
    std::cout << "bind done\n";
    if (listen(sock, 5) == -1) {
        perror("listen");
        return 1;
    }
    std::cout << "listen done\n";

    addr_size = sizeof their_addr;
    new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size);
    if (new_fd == -1) {
        perror("accept");
        return 1;
    }
    std::cout << "accept done\n";

    std::thread recvThread(recieveStuff, new_fd);
    std::cout << "made recieve thread\n";

    std::thread sendThread(sendStuff, new_fd);
    std::cout << "made send thread\n";

    sendThread.join();
    recvThread.join();

    freeaddrinfo(serverInfo);
    close(new_fd);
    close(sock);
    
    return 0;
}

void sendStuff(int new_sock) {
    std::string userInput;
    int bytes_sent;

    while (going) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        int ready = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);

        if (ready > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
            std::getline(std::cin, userInput);
            userInput += '\n';
            const char* msg = userInput.c_str();
            int len = strlen(msg);
            bytes_sent = send(new_sock, msg, len, 0);
            if (bytes_sent == -1) {
                std::cout << strerror(errno);
                return;
            }
        }
        if (userInput.find("exit") != std::string::npos) {
            going = false;
        }
    }
}

void recieveStuff(int new_sock) {
    char buffer[1024];
    while (going) {
        int bytesReceived = recv(new_sock, buffer, sizeof(buffer), 0);
        if (bytesReceived == -1) {
            perror("recv");
            return;
        }
        if (bytesReceived == 0) {
            std::cout << "client exit\n";
            return;
        }
        buffer[bytesReceived] = '\0';

        std::cout << buffer;

        if (strstr(buffer, "exit") != NULL) {
            going = false;
        }
    }
}