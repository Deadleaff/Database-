#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <string>

#define PORT "3490"
#define MAX_REQUEST_SIZE 4096

void* get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];
    char req[MAX_REQUEST_SIZE];
    bool running = true;
    uint32_t net_msg_size;
    uint32_t msg_size;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo);
    if (status != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return 1;
    }
    
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            std::cerr << "socket error: " << strerror(errno) << std::endl;
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            std::cerr << "connect error: " << strerror(errno) << std::endl;
            continue;
        }
        break;
    }
    
    if (p == NULL) {
        std::cerr << "client: failed to connect" << std::endl;
        return 2;
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));
    std::cout << "client: connected to " << s << std::endl;
    freeaddrinfo(servinfo);
    
    while (running) {
        std::cout << "> ";
        std::cin.getline(req, MAX_REQUEST_SIZE);
        
        if (std::string(req) == "q") {
            running = false;
            break;
        }
        
        if (strlen(req) == 0) {
            continue;
        }
        
        std::string query = req;
        query += "\n";
        
        if (send(sockfd, query.c_str(), query.size(), 0) == -1) {
            perror("send");
            break;
        }
        
        if (recv(sockfd, &net_msg_size, sizeof(uint32_t), 0) == -1) {
            perror("recv size");
            break;
        }
        
        msg_size = ntohl(net_msg_size);
        
        if (msg_size == 0 || msg_size > 10 * 1024 * 1024) {
            std::cerr << "Invalid message size: " << msg_size << std::endl;
            char trash[4096];
            while (recv(sockfd, trash, sizeof(trash), 0) > 0) {}
            continue;
        }
        
        std::vector<char> buf(msg_size + 1);
        size_t received = 0;
        while (received < msg_size) {
            ssize_t n = recv(sockfd, buf.data() + received, msg_size - received, 0);
            if (n <= 0) {
                perror("recv data");
                break;
            }
            received += n;
        }
        
        if (received == msg_size) {
            buf[msg_size] = '\0';
            std::cout << buf.data() << std::endl;
        }
    }
    
    close(sockfd);
    return 0;
}
