CFLAGS = -o2 -pthread -lpthread
httpserver: main.cpp httpserver.h threadpool.h httpserver.cpp
	  g++ $(CFLAGS) httpserver.h threadpool.h httpserver.cpp  main.cpp -o httpserver

clean:
	  -rm -f httpserver