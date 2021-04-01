#ifndef SUDOKU_H
#define SUDOKU_H
#include <map>
#include <vector>
using namespace std;
#define THREADNUM 2

const bool DEBUG_MODE = false;
enum { ROW=9, COL=9, N = 81, NEIGHBOR = 20 };
const int NUM = 9;


extern __thread  int board[N];
extern int total_solved;
extern map<int, vector<int> > puzzleSet;
extern map<int, vector<int> >::iterator it;
extern int (*chess)[COL];
//extern int (*chess)[COL];
void init_neighbors();
void solveSudoku();
void input(const char in[N], int num);
void output();
bool solved();
//bool available(int guess, int cell);




bool solve_sudoku_dancing_links(int unused);
bool solved();
#endif
