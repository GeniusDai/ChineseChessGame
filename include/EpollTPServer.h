#ifndef __EPOLL_TP_SERVER_H
#define __EPOLL_TP_SERVER_H

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <memory>

#include "EpollTP.h"

using namespace std;

template<
    template<typename ThreadShare> class Handler,
    typename ThreadShare
>
class EpollTPServer {
public:
    int _thrNum;
    int _sock;
    int _port;
    unique_ptr<EpollTP<Handler<ThreadShare>, ThreadShare> > _tp;
    ThreadShare *_ts;

    EpollTPServer(int thrNum, int port, ThreadShare *ts) : _thrNum(thrNum), _port(port), _ts(ts) {
        _sock = _listen();
        _tp = make_unique<EpollTP<Handler<ThreadShare>, ThreadShare> >(_sock, _thrNum, _ts);
    }

    int _listen() {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw FatalException("socket error");
        }
        struct sockaddr_in addr;
        ::bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(this->_port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (::bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0) {
            throw FatalException("bind error");
        }
        ::listen(sock, 2);
        cout << "listening in port " << this->_port << endl;
        return sock;
    }

    void start() {
        _tp->start();
    }
};

#endif