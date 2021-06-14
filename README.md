# Chinese Chess Game

About
-----

Implemented by *Java/C++/ZooKeeper*...

* Java-client deal with the ZooKeeper-Server

* CPP-client manage the chess board

* Use TCP/IP for IPC

Pre-requirement
---------------

* ZooKeeper >= 3.6.3

* g++ >= 4.8.5

* make >= 3.82

* Unix OS: Linux, OSX

Build and Run
-------------

To build the project:

    % make build

To initialize the ZooKeeper-Server:

    % make init

To start the Client:

    % make run port=<ipc-tcp-port>

TODO
----

* ZooKeeper-Client shall deal with network problems

* Validity of chess move shall be accessed

* C++ source code could be complied to dynamic libs

* IPC could be encrypted