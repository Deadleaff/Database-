#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <vector>
#include <map>
#include <atomic>

#include "header2.h"

#define PORT "3490"
#define BACKLOG 10

std::atomic<bool> running(true);

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\nПолучен сигнал " << sig << ". Завершение работы..." << std::endl;
        running = false;
    }
}

void ignore_sigpipe() {
    signal(SIGPIPE, SIG_IGN);
}

void* get_in_addr(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int create_server_socket() {
    struct addrinfo hints, *servinfo, *p;
    int sockfd = -1;
    int yes = 1;
    int rv;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    rv = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (rv != 0) {
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(rv));
    }
    
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            std::cerr << "socket error: " << strerror(errno) << std::endl;
            continue;
        }
        
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            std::cerr << "setsockopt error: " << strerror(errno) << std::endl;
            close(sockfd);
            continue;
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            std::cerr << "bind error: " << strerror(errno) << std::endl;
            close(sockfd);
            continue;
        }
        
        break;
    }
    
    freeaddrinfo(servinfo);
    
    if (p == NULL) {
        throw std::runtime_error("server: failed to bind");
    }
    
    if (listen(sockfd, BACKLOG) == -1) {
        close(sockfd);
        throw std::runtime_error(std::string("listen error: ") + strerror(errno));
    }
    
    return sockfd;
}

void send_all(int client_fd, const std::string& data) {
    const char* buf = data.c_str();
    uint32_t remaining = data.size();
    uint32_t netsize = htonl(remaining);
    send(client_fd, &netsize, sizeof(uint32_t), MSG_NOSIGNAL);
    while (remaining > 0) {
        ssize_t sent = send(client_fd, buf, remaining, MSG_NOSIGNAL);
        if (sent == -1) {
            if (errno == EINTR) continue;
            std::cerr << "send error: " << strerror(errno) << std::endl;
            break;
        }
        buf += sent;
        remaining -= sent;
    }
}

std::string recv_all(int client_fd) {
    char buffer[4096];
    std::string result;
    
    while (true) {
        ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            if (received == 0) {
                std::cout << "Клиент " << client_fd << " отключился" << std::endl;
            } else if (errno != EINTR) {
                std::cerr << "recv error: " << strerror(errno) << std::endl;
            }
            return "";
        }
        
        buffer[received] = '\0';
        result += buffer;
        
        if (received < (ssize_t)(sizeof(buffer) - 1)) {
            break;
        }
    }
    
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
        result.pop_back();
    }
    
    return result;
}

void print_client_info(struct sockaddr_storage& their_addr) {
    char s[INET6_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr*)&their_addr),
              s, sizeof(s));
    std::cout << "Новое подключение от " << s << std::endl;
}

void update_max_fd(fd_set* master_fds, int& max_fd) {
    if (max_fd > 0 && !FD_ISSET(max_fd, master_fds)) {
        for (int i = max_fd - 1; i >= 0; i--) {
            if (FD_ISSET(i, master_fds)) {
                max_fd = i;
                break;
            }
        }
    }
}

int main() {
    try {
        ignore_sigpipe();
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        GroupHashTable list_table;
        TreeHashTable tree_table;
        DataManager dm(list_table, tree_table);
        
        // Карта сессий: fd -> ClientSession
        std::map<int, ClientSession*> sessions;
        
        dm.load("dbfile.txt");
        std::cout << "База данных загружена" << std::endl;
        
        int server_fd = create_server_socket();
        std::cout << "Сервер запущен на порту " << PORT << std::endl;
        
        fd_set master_fds;
        FD_ZERO(&master_fds);
        FD_SET(server_fd, &master_fds);
        int max_fd = server_fd;
        
        std::map<int, std::string> client_buffers;
        
        std::cout << "Ожидание подключений..." << std::endl;
        
        while (running) {
            fd_set read_fds = master_fds;
            
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
            
            if (activity == -1) {
                if (errno == EINTR) continue;
                std::cerr << "select error: " << strerror(errno) << std::endl;
                break;
            }
            
            if (activity == 0) {
                continue;
            }
            
            for (int i = 0; i <= max_fd && running; i++) {
                if (!FD_ISSET(i, &read_fds)) continue;
                
                if (i == server_fd) {
                    struct sockaddr_storage their_addr;
                    socklen_t sin_size = sizeof(their_addr);
                    
                    int client_fd = accept(server_fd, (struct sockaddr*)&their_addr, &sin_size);
                    if (client_fd == -1) {
                        std::cerr << "accept error: " << strerror(errno) << std::endl;
                        continue;
                    }
                    
                    print_client_info(their_addr);
                    
                    FD_SET(client_fd, &master_fds);
                    if (client_fd > max_fd) max_fd = client_fd;
                    client_buffers[client_fd] = "";
                    
                    // Создаём сессию для клиента
                    sessions[client_fd] = new ClientSession(client_fd);
                }
                else {
                    char buffer[4096];
                    ssize_t bytes = recv(i, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes <= 0) {
                        if (bytes == 0) {
                            std::cout << "Клиент " << i << " отключился" << std::endl;
                        } else {
                            std::cerr << "recv error от клиента " << i << ": " << strerror(errno) << std::endl;
                        }
                        
                        close(i);
                        FD_CLR(i, &master_fds);
                        client_buffers.erase(i);
                        
                        // Удаляем сессию клиента
                        auto it = sessions.find(i);
                        if (it != sessions.end()) {
                            delete it->second;
                            sessions.erase(it);
                        }
                        
                        update_max_fd(&master_fds, max_fd);
                    } else {
                        buffer[bytes] = '\0';
                        client_buffers[i] += buffer;
                        
                        size_t newline_pos = client_buffers[i].find('\n');
                        if (newline_pos != std::string::npos) {
                            std::string query = client_buffers[i].substr(0, newline_pos);
                            client_buffers[i] = client_buffers[i].substr(newline_pos + 1);
                            
                            while (!query.empty() && (query.back() == '\n' || query.back() == '\r' || query.back() == ' ')) {
                                query.pop_back();
                            }
                            
                            if (!query.empty()) {
                                std::cout << "Запрос от клиента " << i << ": " << query << std::endl;
                                
                                ClientSession* session = sessions[i];
                                Result result = dm.execute_request(query, session);
                                
                                std::string response;
                                if (query.find("PRINT") == 0 || query.find("print") == 0) {
                                    // Для PRINT используем форматирование в таблицу
                                    response = result.format_as_table(result.columns);
                                } else {
                                    response = result.serialize();
                                }
                                
                                send_all(i, response);
                            }
                        }
                    }
                }
            }
        }
        
        std::cout << "Очистка ресурсов..." << std::endl;
        
        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &master_fds) && i != server_fd) {
                close(i);
            }
        }
        
        // Удаляем все сессии
        for (auto& pair : sessions) {
            delete pair.second;
        }
        sessions.clear();
        
        close(server_fd);
        dm.rewriteFile("dbfile.txt");
        std::cout << "База данных сохранена" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Сервер завершил работу" << std::endl;
    return 0;
}
