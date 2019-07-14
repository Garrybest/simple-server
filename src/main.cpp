#include <iostream>
#include <string.h>
#include "Server.hpp"

int main(int argc, char *argv[])
{
    const char *ip = argc >= 3 ? argv[1] : "127.0.0.1";
    const int port = argc >= 4 ? atoi(argv[2]) : 9527;
    Server server(ip, port);
    server.start();
    return 0;
}