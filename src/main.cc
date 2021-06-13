#include "PlayChessGame.h"
#include <string>

using namespace std;

int main(int argc, char **argv) {
    assert(argc == 2);
    int port = PlayChessGame::parsePort(argv[1]);
    PlayChessGame::start(port);
    return 0;
}