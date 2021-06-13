#include "ChessGame.h"
#include "TCPListener.h"

#include <math.h>

using namespace std;

#ifndef _PlayChessGame_
#define _PlayChessGame_

class PlayChessGame {
public:
    static const string invalid;

    static int parsePort(char *port) {
        string temp;
        for (int i = 0; ; ++i) {
            if (port[i] != '\0') temp += port[i];
            else break;
        }

        return stoi(temp);
    }

    static void start(int port) {
        TCPListener listener(port);
        ChessGame game;
        listener.startAccept();
        game.showGameBoard();
        game.welcome();
        while (true) {
            // The read system call really wasting CPU in OSX test
            unordered_map<string, string> msg = listener.readParseConn();

            game.showGameBoard(msg["player"]);
            if (msg["move"] != invalid) {
                game.moveChess(msg["move"]);
                game.showGameBoard(msg["player"]);
            }
            string move = game.askMove();
            game.moveChess(move);
            game.showGameBoard(msg["player"]);
            listener.writeConn(move);
        }
    }
};

const string PlayChessGame::invalid = "0 0 0 0";

#endif // PlayChessGame.h