CXXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: sudoku
.PHONY: clean

sudoku: main.cc neighbor.cc sudoku_basic.cc  sudoku_dancing_links.cc sudoku.h ThreadPool.h
	g++ -O2 -pthread -o sudoku_solve main.cc neighbor.cc sudoku_basic.cc  sudoku_dancing_links.cc


clean:
	rm demo sudoku
