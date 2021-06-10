CFLAGS = -o1 -pthread -lpthread
kvstore2pcsystem: main.cpp kvstore2pcsystem.h kvstore2pcsystem.cpp coordinator.h coordinator.cpp participant.h participant.cpp
	  g++ $(CFLAGS) kvstore2pcsystem.h kvstore2pcsystem.cpp main.cpp coordinator.h coordinator.cpp participant.h participant.cpp -o kvstore2pcsystem

clean:
	  -rm -f kvstore2pcsystem