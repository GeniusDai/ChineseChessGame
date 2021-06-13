#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <exception>
#include <cstring>
#include <unordered_map>
#include <assert.h>

using namespace std;

#ifndef _TCPListener_
#define _TCPListener_

class TCPListener {
public:
    int port_;
    int connSock_;

    static const char END = '\n';
    static const char SEP = '#';

    TCPListener(int port) : port_(port) {}

    void startAccept() {
        int listenSock;
        if ((listenSock = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            throw "socket error";
        }
        struct sockaddr_in addr;
        ::bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (::bind(listenSock, (sockaddr *)&addr, sizeof(addr)) < 0) {
            throw "bind error";
        }
        ::listen(listenSock, 2);
        cout << "listening in port " << port_ << endl;
        connSock_ = ::accept(listenSock, NULL, NULL);
        ::close(listenSock);
        cout << "Connect established, listen socket closed..." << endl;
    }

    void writeConn(string data) {
        char buf[data.size() + 1];
        buf[data.size()] = this->END;
        for(int i = 0; i < data.size(); ++i) {
            buf[i] = data[i];
        }
        ::write(this->connSock_, buf, data.size() + 1);
    }

    string _readConn() {
        string data;
        char buf[1];
        while(true) {
            ::read(this->connSock_, buf, 1);
            if (buf[0] != this->END) {
                data += buf[0];
            } else {
                break;
            }
        }
        return data;
    }

    unordered_map<string, string>
    readParseConn() {
        string data = _readConn();
        int sep = -1;
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == SEP) {
                sep = i;
                break;
            }
        }
        assert(sep > 0);
        return {
            {"player", data.substr(0, sep)},
            {"move", data.substr(sep + 1, data.size() - sep - 1)}
        };
    }

    void closeConn() {
        ::close(connSock_);
    }

    ~TCPListener() {
        closeConn();
    }
};

#endif // TCPListener.h