#ifndef _RAM_HANDLER_H_
#define _RAM_HANDLER_H_

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

#include "IOHandler.h"

using namespace std;

template<typename ThreadShare>
class RAMHandler : public IOHandler {
    static const int _msgBufferSize = 100;
    const char *initMsg = "0 0 0 0\n";
    ThreadShare *_ts;
public:
    RAMHandler(int sock, mutex *mptr, ThreadShare *ts) : IOHandler(sock, mptr), _ts(ts) {}

    void onConnect(int conn) {
        _ts->message[conn] = make_pair(unique_ptr<char []>(new char[_msgBufferSize]), 0);
        if (_ts->single.empty()) {
            _ts->single.insert(conn);
            cout << "single client " << conn << endl;
        } else {
            auto iter = _ts->single.begin();
            int oppo = *iter;
            _ts->single.erase(iter);
            _ts->match[oppo] = conn;
            _ts->match[conn] = oppo;
            write(oppo, initMsg, strlen(initMsg));
            cout << "match client " << conn << "&" << oppo << endl;
        }
        RegisterFd(conn, EPOLLIN);
    }

    void onReadable(int conn) {
        char buf[100];
        int len = read(conn, buf, 100);
        if (len == 0) {
            cout << "client closed the socket, will close " << conn << endl;
            ::close(conn);
            RemoveFd(conn);
            _ts->message.erase(conn);
            if (_ts->match.find(conn) != _ts->match.cend()) {
                cout << "find oppo, will close socket " << _ts->match[conn] << endl;
                ::close(_ts->match[conn]);
                RemoveFd(_ts->match[conn]);
                _ts->message.erase(_ts->match[conn]);
                _ts->match.erase(_ts->match[conn]);
            } else {
                _ts->single.erase(conn);
            }
            _ts->match.erase(conn);
            return;
        }

        _appendOrSendMessage(conn, buf, len);
    }

    void _appendOrSendMessage(int conn, char *buf, int len) {
        int curr = _ts->message[conn].second;
        if (len + curr >= _msgBufferSize) {
            throw "buffer overflow error";
        }
        for (int i = 0; i < len; ++i) {
            _ts->message[conn].first[curr+i] = buf[i];
        }
        _ts->message[conn].second = curr + len;
        if (_ts->message[conn].first[curr+len-1] == '\n') {
            cout << "receive message from " << conn << endl;
            ::write(_ts->match[conn], _ts->message[conn].first.get(), _ts->message[conn].second);
            _ts->message[conn].second = 0;
        } else {
            cout << "receive partial message from " << conn << endl;
        }
    }

    void onWritable(int conn) {

    }

    void onPassivelyClose(int conn) {

    }
};

class SharedData {
public:
    unordered_map<int, int> match;
    unordered_set<int> single;
    unordered_map<int, pair<unique_ptr<char []>, int> > message;
};

#endif  // RAMHandler.h
