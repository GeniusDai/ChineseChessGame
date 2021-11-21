#ifndef __EPOLL_TP_H_
#define __EPOLL_TP_H_

#include <mutex>
#include <memory>
#include <vector>

#include "IOHandler.h"

template <typename Handler, typename ThreadShare>
class EpollTP final {
    vector<shared_ptr<Handler> > _handlers;
    int _sock;
    mutex _m;
    int _thrNum;
    ThreadShare *_ts;
public:
    EpollTP(int sock, int thrNum, ThreadShare *ts) : _sock(sock), _thrNum(thrNum), _ts(ts) {
        mutex *mptr = &_m;
        if (_sock == 0) {
            mptr = nullptr;
        }
        for (int i = 0; i < thrNum; ++i) {
            _handlers.emplace_back(make_shared<Handler>(_sock, mptr, ts));
        }
    }

    void start() {
        for (auto h : _handlers) {
            h->start();
        }

        for (auto h : _handlers) {
            h->join();
        }
    }
};

#endif