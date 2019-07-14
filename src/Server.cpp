#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include "Server.hpp"

#define LISTENQ 5
#define MAXSIZE 87380
#define EPOLLEVENTS 100

void Server::start()
{
    int listenfd;
    listenfd = socket_bind(ip, port);
    listen(listenfd, LISTENQ);
    do_epoll(listenfd);
}

int Server::socket_bind(const char *ip, int port)
{
    int listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        perror("socket error: ");
        exit(1);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    return listenfd;
}

void Server::do_epoll(int listenfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ready_cnt;
    epollfd = epoll_create1(0);
    add_event(epollfd, listenfd, EPOLLIN);
    while (true)
    {
        ready_cnt = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        handle_events(epollfd, events, ready_cnt, listenfd);
    }
    close(epollfd);
}

void Server::add_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        std::cout << "Add event failed!" << std::endl;
    }
}

void Server::delete_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev) < 0)
    {
        std::cout << "Delete event failed!" << std::endl;
    }
}

void Server::modify_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) < 0)
    {
        std::cout << "Modify event failed!" << std::endl;
    }
}

void Server::handle_events(int epollfd, struct epoll_event *events,
                           int num, int listenfd)
{
    int i;
    int fd;

    for (i = 0; i < num; i++)
    {
        fd = events[i].data.fd;
        if ((fd == listenfd) && (events[i].events & EPOLLIN))
            handle_accpet(epollfd, listenfd);
        else if (events[i].events & EPOLLIN)
            do_read(epollfd, fd);
        else if (events[i].events & EPOLLOUT)
            do_write(epollfd, fd);
    }
}

void Server::handle_accpet(int epollfd, int listenfd)
{
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;

    clifd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    if (clifd == -1)
        perror("Accpet error: ");
    else
    {
        std::cout << "Accept a new client: "
                  << inet_ntoa(cliaddr.sin_addr)
                  << ":" << cliaddr.sin_port << std::endl;
        add_event(epollfd, clifd, EPOLLIN);
    }
}

void Server::do_read(int epollfd, int fd)
{
    int nread;
    char buf[MAXSIZE];
    nread = read(fd, buf, MAXSIZE);
    if (nread == -1)
    {
        perror("Read error: ");
        delete_event(epollfd, fd, EPOLLIN);
        close(fd);
        bufmap.erase(fd);
    }
    else if (nread == 0)
    {
        fprintf(stderr, "Client closed.\n");
        delete_event(epollfd, fd, EPOLLIN);
        close(fd);
        bufmap.erase(fd);
    }
    else
    {
        bufmap[fd] = std::string(buf, nread);
        std::cout << "Read message is: " << bufmap[fd] << std::endl;
        modify_event(epollfd, fd, EPOLLOUT);
    }
}

void Server::do_write(int epollfd, int fd)
{
    int nwrite;
    std::string &buf = bufmap[fd];

    nwrite = write(fd, buf.c_str(), buf.size());
    if (nwrite == -1)
    {
        perror("Write error: ");
        delete_event(epollfd, fd, EPOLLOUT);
        close(fd);
        bufmap.erase(fd);
    }
    else
        modify_event(epollfd, fd, EPOLLIN);

    buf.clear();
}