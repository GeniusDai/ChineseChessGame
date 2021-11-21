SRC = src
BIN = bin
SERVER = StartServer
CLIENT = StartClient

CXXFLAGS = -O0 -Wall -Wno-sign-compare -std=c++14 -g -I include

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