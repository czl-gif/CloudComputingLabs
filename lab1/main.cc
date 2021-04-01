#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include<iostream>
#include<sys/time.h>
#include<stdio.h>
#include<cstring>


using namespace std;
#include "sudoku.h"
#include "ThreadPool.h"

const int THREADNUM0 = thread::hardware_concurrency();
int64_t  now()
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000000 + tv.tv_usec;
}

int total_solved = 0;

int main()
{
    init_neighbors();
    ThreadPool threadpool(THREADNUM0 - 1);
    //string filename;
    char filename[128];

    //fstream inputfile;
    char puzzle[128];
    
    int total = 0;
    int output_num = 0;
    double sec = 0;
    while(fgets(filename, 128, stdin)) {
        if(filename[0]=='\n') {
			printf("stop reading the file,please wait\n");
			break;
		}
        if(strlen(filename)== 0)
     		break;
        if(filename[0] == '0')
            break;
        if(filename[strlen(filename) - 1] == '\n')
            filename[strlen(filename) - 1] = '\0';
        FILE* fp = fopen(filename, "r");

        if(fp == NULL) {
            printf("%s 文件里没有数据\n", filename);
            continue;
        }
        //inputfile.open(filename, ios::in);
        //start = now();
        while(fgets(puzzle, sizeof puzzle, fp) != NULL) {
            if(strlen(puzzle) >= N) {
                
                input(puzzle, total++);
                /*if(solve_sudoku_dancing_links(0))
                    ++total_solved;
                else {
                    printf("No: %s", puzzle);
                }*/
            }
        }
        

   }
   it = puzzleSet.begin();
   int len = puzzleSet.size();
   int64_t start = now();
   int m_len = len / 12;
   //cout << "len::" << len <<endl;
   while (len > m_len)
   {
       len --;
       threadpool.enqueue(solveSudoku);
   }
   //usleep(1000);
   while(m_len --)
   {
       solveSudoku();
   }
   while ( total_solved < total)
   {
       usleep(10);
       if (total_solved >= total)
            break;
       continue;
   }
   int64_t end = now();
    sec += (end-start)/1000000.0;
   cout<<"sol::"<<total_solved<<' '<<total<<endl;
   output();
    printf("%f sec %f ms each %d\n", sec, 1000*sec/total, total_solved);


   return 0;
}
