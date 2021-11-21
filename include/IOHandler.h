#ifndef __IOHANDLER_H
#define __IOHANDLER_H

#include <sys/epoll.h>
#include <sys/socket.h>

#include <cstring>
#include <cstdio>
#include <mutex>
#include <memory>
#include <thread>
#include <sstream>

#include "Exception.h"

using namespace std;

const int MAX_SIZE = 1024;

// IOHandler --> thread

class IOHandler {
public:
    int _epfd = ::epoll_create(1);
    struct epoll_event _evs[MAX_SIZE];
    shared_ptr<thread> _t;

    mutex *_mptr = nullptr;
    int _listenfd = 0;

    // For client
    IOHandler() = default;

    // For Server
    IOHandler(int listenfd, mutex *mptr) {
        _mptr = mptr;
        _listenfd = listenfd;
    }

    void _start() {
        cout << "start thread " << this_thread::get_id() << endl;
        while (true) {
            int timeout = 500;

            if (_listenfd && _mptr->try_lock()) {
                timeout = -1;
                cout << "thread " << this_thread::get_id() << " get lock" << endl;
                RegisterFd(_listenfd, EPOLLIN);
            }

            int num = epoll_wait(_epfd, _evs, MAX_SIZE, timeout);

            for (int i = 0; i < num; ++i) {
                if (_listenfd && _evs[i].data.fd == _listenfd) {
                    int conn = ::accept4(_listenfd, NULL, NULL, SOCK_NONBLOCK);
                    cout << "new connection " << conn << " accepted" << endl;
                    RemoveFd(_listenfd);
                    _mptr->unlock();
                    cout << "thread " << this_thread::get_id() << " release lock" << endl;
                    onConnect(conn);
                } else {
                    if (_evs[i].events & EPOLLIN) {
                        cout << "conn " << _evs[i].data.fd << " readable" << endl;
                        onReadable(_evs[i].data.fd);
                    }
                    if (_evs[i].events & EPOLLRDHUP) {
                        cout << "conn " << _evs[i].data.fd << " closed" << endl;
                        onPassivelyClose(_evs[i].data.fd);
                    }
                    if (_evs[i].events & EPOLLOUT) {
                        cout << "conn " << _evs[i].data.fd << " writable" << endl;
                        onWritable(_evs[i].data.fd);
                    }
                }
            }
        }
    }

    void start() {
        _t = make_shared<thread>(&IOHandler::_start, this);
    }

    void join() {
        _t->join();
    }

    void RegisterFd(int fd, uint32_t events) {
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = events;
        if (::epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            stringstream ss;
            ss << "register fd " << fd << " error";
            ::perror(ss.str().c_str());
            throw NonFatalException(ss.str().c_str());
        }
    }

    void RemoveFd(int fd) {
        if (::epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
            stringstream ss;
            ss << "remove fd " << fd << " error";
            ::perror(ss.str().c_str());
            throw NonFatalException(ss.str().c_str());
        }
    }

    virtual void onConnect(int conn) = 0;

    virtual void onReadable(int conn) = 0;

    virtual void onWritable(int conn) = 0;

    virtual void onPassivelyClose(int conn) = 0;
};

#endif