#pragma once
#include <string>
#include <unordered_map>

class Server
{

public:
    Server(const char *ip, const int port) : ip(ip), port(port) {}
    ~Server() {}
    void start();

private:
    const char *ip;
    const int port;
    std::unordered_map<int, std::string> bufmap;

    int socket_bind(const char *ip, int port);
    void do_epoll(int listenfd);
    void add_event(int epollfd, int fd, int state);
    void delete_event(int epollfd, int fd, int state);
    void modify_event(int epollfd, int fd, int state);
    void handle_events(int epollfd, struct epoll_event *events,
                       int num, int listenfd);
    void handle_accpet(int epollfd, int listenfd);
    void do_read(int epollfd, int fd);
    void do_write(int epollfd, int fd);
};