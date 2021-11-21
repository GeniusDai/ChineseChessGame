#include "RAMIOHandler.h"
#include "EpollTPServer.h"

int main(int argc, char **argv) {
    SharedData data;
    EpollTPServer<RAMIOHandler, SharedData > server{8, 8889, &data};
    server.start();
    return 0;
}