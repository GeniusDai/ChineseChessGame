#include <sys/epoll.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <assert.h>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

using namespace std;

#ifndef _SERVER_
#define _SERVER_

class Server {
public:
    static const int _port = 8889;
    static const int _maxClients = 500;
    static const int _msgBufferSize = 100;
    const char *initMsg = "0 0 0 0\n";
    int _sock;
    int _epfd;
    struct epoll_event _events[_maxClients];

    unordered_map<int, int> match;
    unordered_set<int> single;
    unordered_map<int, pair<unique_ptr<char []>, int> > message;

    Server() = default;
    Server(Server &s) = delete;

    void initServer() {
        _epfd = ::epoll_create1(0);
        if (_epfd == -1) {
            throw "create epoll failed";
        }
        initListenSocket();
        registerListenSocket(_epfd, _sock);
    }

    void initListenSocket() {
        _sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (_sock < 0) {
            throw "socket error";
        }
        struct sockaddr_in addr;
        ::bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(this->_port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (::bind(_sock, (sockaddr *)&addr, sizeof(addr)) < 0) {
            throw "bind error";
        }
        ::listen(_sock, 2);
        cout << "listening in port " << this->_port << endl;
    }

    void registerListenSocket(int epfd, int sock) {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sock;
        if (::epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
            throw "register listen socket fail";
        }
    }

    void listenAndServe() {
        initServer();
        while(true) {
            int num = epoll_wait(_epfd, this->_events, _maxClients, -1);
            if (num == -1) {
                throw "epoll_wait error";
            }

            for (int i = 0; i < num; ++i) {
                if (_events[i].data.fd == _sock) {
                    int conn = ::accept4(_sock, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
                    if (conn == -1) {
                        throw "accept4 error";
                    }
                    cout << "accept conn: " << conn << endl;
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = conn;
                    if (::epoll_ctl(_epfd, EPOLL_CTL_ADD, conn, &ev) == -1) {
                        throw "epoll_ctl add connect socket error";
                    }

                    message[conn] = make_pair(unique_ptr<char []>(new char[_msgBufferSize]), 0);

                    if (single.empty()) {
                        single.insert(conn);
                        cout << "single client " << conn << endl;
                    } else {
                        auto iter = single.begin();
                        int oppo = *iter;
                        single.erase(iter);
                        match[oppo] = conn;
                        match[conn] = oppo;
                        write(oppo, initMsg, strlen(initMsg));
                        cout << "match client " << conn << "&" << oppo << endl;
                    }
                } else {
                    processClient(_events[i].data.fd);
                }
            }
        }
    }

    void processClient(int conn) {
        char buf[100];
        int len = read(conn, buf, 100);
        if (len == 0) {
            cout << "client closed the socket, will close " << conn << endl;
            ::close(conn);
            ::epoll_ctl(_epfd, EPOLL_CTL_DEL, conn, NULL);
            message.erase(conn);
            if (match.find(conn) != match.cend()) {
                cout << "find oppo, will close socket " << match[conn] << endl;
                ::close(match[conn]);
                ::epoll_ctl(_epfd, EPOLL_CTL_DEL, match[conn], NULL);
                message.erase(match[conn]);
                match.erase(match[conn]);
            } else {
                single.erase(conn);
            }
            match.erase(conn);
            return;
        }

        appendOrSendMessage(conn, buf, len);
    }

    void appendOrSendMessage(int conn, char *buf, int len) {
        int curr = message[conn].second;
        if (len + curr >= _msgBufferSize) {
            throw "buffer overflow error";
        }
        for (int i = 0; i < len; ++i) {
            message[conn].first[curr+i] = buf[i];
        }
        message[conn].second = curr + len;
        if (message[conn].first[curr+len-1] == '\n') {
            cout << "receive message from " << conn << endl;
            ::write(match[conn], message[conn].first.get(), message[conn].second);
            message[conn].second = 0;
        } else {
            cout << "receive partial message from " << conn << endl;
        }
    }
};

#endif  // Server.h
