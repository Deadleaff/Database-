#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
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

// ============================================================
// ГЛОБАЛЬНЫЙ ФЛАГ ДЛЯ ЗАВЕРШЕНИЯ РАБОТЫ
// ============================================================
std::atomic<bool> running(true);

// ============================================================
// ОБРАБОТЧИК СИГНАЛОВ (Ctrl+C, kill)
// ============================================================
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\nПолучен сигнал " << sig << ". Завершение работы..." << std::endl;
        running = false;
    }
}

// ============================================================
// ИГНОРИРОВАНИЕ SIGPIPE (чтобы не падать при разрыве соединения)
// ============================================================
void ignore_sigpipe() {
    signal(SIGPIPE, SIG_IGN);
}

// ============================================================
// Получение IP-адреса из sockaddr (поддерживает IPv4 и IPv6)
// ============================================================
void* get_in_addr(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// ============================================================
// Создание и настройка серверного сокета
// ============================================================
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

// ============================================================
// ОТПРАВКА ДАННЫХ КЛИЕНТУ (полная отправка)
// ============================================================
void send_all(int client_fd, const std::string& data) {
    const char* buf = data.c_str();
    size_t remaining = data.size();
    
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

// ============================================================
// ПОЛУЧЕНИЕ ДАННЫХ ОТ КЛИЕНТА
// ============================================================
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
        
        // Если получили меньше, чем буфер, значит сообщение закончилось
        if (received < (ssize_t)(sizeof(buffer) - 1)) {
            break;
        }
    }
    
    // Удаляем пробелы и символы новой строки в конце
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
        result.pop_back();
    }
    
    return result;
}

// ============================================================
// ОБРАБОТКА ЗАПРОСА КЛИЕНТА
// ============================================================
void handle_client_query(int client_fd, DataManager& dm, const std::string& query) {
    std::cout << "Запрос от клиента " << client_fd << ": " << query << std::endl;
    
    Result result = dm.execute_request(query);
    std::string response = result.serialize();
    
    send_all(client_fd, response);
}

// ============================================================
// ВЫВОД ИНФОРМАЦИИ О ПОДКЛЮЧИВШЕМСЯ КЛИЕНТЕ
// ============================================================
void print_client_info(struct sockaddr_storage& their_addr) {
    char s[INET6_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr*)&their_addr),
              s, sizeof(s));
    std::cout << "Новое подключение от " << s << std::endl;
}

// ============================================================
// ОБНОВЛЕНИЕ MAX_FD ПОСЛЕ УДАЛЕНИЯ КЛИЕНТА
// ============================================================
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

// ============================================================
// ГЛАВНАЯ ФУНКЦИЯ СЕРВЕРА
// ============================================================
int main() {
    try {
        // ===== 1. НАСТРОЙКА ОБРАБОТЧИКОВ СИГНАЛОВ =====
        ignore_sigpipe();
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        // ===== 2. ИНИЦИАЛИЗАЦИЯ БАЗЫ ДАННЫХ =====
        GroupHashTable list_table;
        TreeHashTable tree_table;
        DataManager dm(list_table, tree_table);
        
        dm.load("dbfile.txt");
        std::cout << "База данных загружена" << std::endl;
        
        // ===== 3. СОЗДАНИЕ СЕРВЕРНОГО СОКЕТА =====
        int server_fd = create_server_socket();
        std::cout << "Сервер запущен на порту " << PORT << std::endl;
        
        // ===== 4. НАСТРОЙКА SELECT =====
        fd_set master_fds;
        FD_ZERO(&master_fds);
        FD_SET(server_fd, &master_fds);
        int max_fd = server_fd;
        
        // Буферы для частичных данных от клиентов
        std::map<int, std::string> client_buffers;
        
        std::cout << "Ожидание подключений..." << std::endl;
        
        // ===== 5. ОСНОВНОЙ ЦИКЛ =====
        while (running) {
            fd_set read_fds = master_fds;
            
            // Таймаут 1 секунда для проверки флага running
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
                continue;  // Таймаут, проверяем running
            }
            
            // Обрабатываем активные сокеты
            for (int i = 0; i <= max_fd && running; i++) {
                if (!FD_ISSET(i, &read_fds)) continue;
                
                // === НОВОЕ ПОДКЛЮЧЕНИЕ ===
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
                }
                // === ДАННЫЕ ОТ КЛИЕНТА ===
                else {
                    char buffer[4096];
                    ssize_t bytes = recv(i, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes <= 0) {
                        // Клиент отключился или ошибка
                        if (bytes == 0) {
                            std::cout << "Клиент " << i << " отключился" << std::endl;
                        } else {
                            std::cerr << "recv error от клиента " << i << ": " << strerror(errno) << std::endl;
                        }
                        
                        close(i);
                        FD_CLR(i, &master_fds);
                        client_buffers.erase(i);
                        update_max_fd(&master_fds, max_fd);
                    } else {
                        buffer[bytes] = '\0';
                        client_buffers[i] += buffer;
                        
                        // Проверяем, есть ли полное сообщение
                        // Ищем символ новой строки как признак конца сообщения
                        size_t newline_pos = client_buffers[i].find('\n');
                        if (newline_pos != std::string::npos) {
                            std::string query = client_buffers[i].substr(0, newline_pos);
                            
                            // Остаток сохраняем для следующего раза
                            client_buffers[i] = client_buffers[i].substr(newline_pos + 1);
                            
                            // Удаляем пробелы в конце
                            while (!query.empty() && (query.back() == '\n' || query.back() == '\r' || query.back() == ' ')) {
                                query.pop_back();
                            }
                            
                            if (!query.empty()) {
                                handle_client_query(i, dm, query);
                            }
                        }
                    }
                }
            }
        }
        
        // ===== 6. ЗАВЕРШЕНИЕ РАБОТЫ (ОЧИСТКА РЕСУРСОВ) =====
        std::cout << "Очистка ресурсов..." << std::endl;
        
        // Закрываем все клиентские сокеты
        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &master_fds) && i != server_fd) {
                close(i);
            }
        }
        
        // Закрываем серверный сокет
        close(server_fd);
        
        // Сохраняем базу данных
        dm.rewriteFile("dbfile.txt");
        std::cout << "База данных сохранена" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Сервер завершил работу" << std::endl;
    return 0;
}
