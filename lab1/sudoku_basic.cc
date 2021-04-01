#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>
#include "sudoku.h"
using namespace std;
__thread int board[N];
__thread int write_num;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex_write = PTHREAD_MUTEX_INITIALIZER;

map<int, vector<int> >puzzleSet;
map<int, vector<int> >::iterator it;
map<int, vector<int> >::iterator it_write;
int (*chess)[COL] = (int (*)[COL])board;

/*static void find_spaces()
{
  nspaces = 0;
  for (int cell = 0; cell < N; ++cell) {
    if (board[cell] == 0)
      spaces[nspaces++] = cell;
  }
}*/

void solveSudoku()
{
    bool (*solve)(int) = solve_sudoku_dancing_links;
    pthread_mutex_lock(&queue_mutex);
    if (it != puzzleSet.end())
    {
        vector<int> tmp = it->second;
        copy(tmp.begin(), tmp.end(), board);
        write_num = it->first;
        it++;
    }else {
        pthread_mutex_unlock(&queue_mutex);
        return;
    }
    pthread_mutex_unlock(&queue_mutex);
    if (solve(0))
    {
        
        vector<int> tmp(board, board + 81);
        pthread_mutex_lock(&queue_mutex_write);
        puzzleSet[write_num] = tmp;
        total_solved++;
        cout<<"solved::"<<total_solved<<endl;
        pthread_mutex_unlock(&queue_mutex_write);
        

    }
    return;
}
void output()
{
    it = puzzleSet.begin();
    while (it != puzzleSet.end())
    {
        vector<int> tmp = it->second;
        for (int i = 0; i < 81; i++)
            cout << tmp[i];
        cout << endl;
        it++;
    }
    
}
/*void output(int num)
{   
    for (int i = 0; i < 81; i++)
        cout << puzzleSet[num][i];
    cout << endl;         
}*/
void input(const char in[N], int num)
{
    vector<int> tmp;
    for (int cell = 0; cell < N; ++cell) {
        tmp.push_back(in[cell] - '0');
        //board[cell] = in[cell] - '0';
        assert(0 <= tmp[cell] && tmp[cell] <= NUM);
    }
    puzzleSet.insert(pair<int, vector<int> >(num, tmp));
}

/*bool available(int guess, int cell)
{
  for (int i = 0; i < NEIGHBOR; ++i) {
    int neighbor = neighbors[cell][i];
    if (board[neighbor] == guess) {
      return false;
    }
  }
  return true;
}*/

/*bool solve_sudoku_basic(int which_space)
{
  if (which_space >= nspaces) {
    return true;
  }

  // find_min_arity(which_space);
  int cell = spaces[which_space];

  for (int guess = 1; guess <= NUM; ++guess) {
    if (available(guess, cell)) {
      // hold
      assert(board[cell] == 0);
      board[cell] = guess;

      // try
      if (solve_sudoku_basic(which_space+1)) {
        return true;
      }

      // unhold
      assert(board[cell] == guess);
      board[cell] = 0;
    }
  }
  return false;
}*/
