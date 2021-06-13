SRC = src
PACKAGE = demo
CXXMAIN = main
JAVAMAIN = Executor
JAVAINIT = InitializeServer

port = "8888"
zkaddr = "localhost:2181"

LOGFILE = log-$(port).txt

CXXFLAGS = -O0 -Wall -std=c++11

LDFLAGS = -lpthread

.PHONY: all build clean init run

all: build

build: clean
	g++ $(CXXFLAGS) -o $(SRC)/$(CXXMAIN) $(SRC)/$(CXXMAIN).cc $(LDFLAGS)
	javac $(PACKAGE)/$(JAVAMAIN).java
	javac $(PACKAGE)/$(JAVAINIT).java

clean:
	rm -f $(SRC)/$(CXXMAIN)
	rm -f $(PACKAGE)/$(JAVAMAIN)*.class $(PACKAGE)/$(JAVAINIT)*.class
	rm -f log-*.txt

init:
	java $(PACKAGE)/$(JAVAINIT) $(zkaddr)

run:
	sleep 3 && java $(PACKAGE)/$(JAVAMAIN) $(port) $(zkaddr) > $(LOGFILE) 2>&1 &
	$(SRC)/$(CXXMAIN) $(port)