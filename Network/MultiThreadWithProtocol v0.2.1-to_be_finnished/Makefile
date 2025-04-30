CXX = g++
CXXFLAGS = -std=c++17
LDFLAGS = -lws2_32 -lz

SRC_SERVER = MultiThreadServerDemo.cpp
SRC_CLIENT = ClientDemo.cpp
SRC_COMMON = Protocol.cpp Logger.cpp Globals.cpp

all: Server.exe Client.exe

Server.exe: $(SRC_SERVER) $(SRC_COMMON)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

Client.exe: $(SRC_CLIENT) $(SRC_COMMON)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.exe *.o