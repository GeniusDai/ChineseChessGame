SRC = src
BIN = bin
SERVER = StartServer
CLIENT = StartClient

CXXFLAGS = -O0 -Wall -std=c++11 -g

LDFLAGS = -lpthread

.PHONY: all build clean run-server run-client

all: build

build: clean
	mkdir $(BIN)
	g++ $(CXXFLAGS) -o $(BIN)/$(SERVER) $(SRC)/$(SERVER).cc $(LDFLAGS)
	g++ $(CXXFLAGS) -o $(BIN)/$(CLIENT) $(SRC)/$(CLIENT).cc $(LDFLAGS)

clean:
	rm -rf $(BIN)

run-server:
	$(BIN)/StartServer

run-client:
	$(BIN)/StartClient