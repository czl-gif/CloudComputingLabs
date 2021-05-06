CFLAGS = -o1 -pthread -lpthread
httpserver: main.cpp httpserver.h threadpool.h httpserver.cpp proxyserver.h proxyserver.cpp
	  g++ $(CFLAGS) httpserver.h threadpool.h httpserver.cpp proxyserver.h proxyserver.cpp main.cpp -o httpserver

clean:
	  -rm -f httpserver