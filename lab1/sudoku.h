#ifndef SUDOKU_H
#define SUDOKU_H

const bool DEBUG_MODE = false;
enum { ROW=9, COL=9, N = 81, NEIGHBOR = 20 };
const int NUM = 9;


extern int board[N];
extern int spaces[N];
extern int nspaces;
//extern int (*chess)[COL];


void input(const char in[N]);


//bool available(int guess, int cell);




bool solve_sudoku_dancing_links(int unused);
bool solved();
#endif
