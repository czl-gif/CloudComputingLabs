#include<iostream>
#include<sys/time.h>
#include<stdio.h>
#include<cstring>


using namespace std;
#include "sudoku.h"

int64_t  now()
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main()
{
    //string filename;
    char filename[128];

    //fstream inputfile;
    char puzzle[128];
    int total_solved = 0;
    int total = 0;
    double sec = 0;
    int64_t start = now();
    while(scanf("%s", filename) != 0) {
        if(strlen(filename)== 0)
     		break;
        if(filename[0] == '0')
            break;
        FILE* fp = fopen(filename, "r");
        //inputfile.open(filename, ios::in);
        start = now();
        while(fgets(puzzle, sizeof puzzle, fp) != NULL) {
            if(strlen(puzzle) >= N) {
                ++total;
                input(puzzle);
                if(solve_sudoku_dancing_links(0))
                    ++total_solved;
                else {
                    printf("No: %s", puzzle);
                }
            }
        }
        int64_t end = now();
        sec += (end-start)/1000000.0;

   }
    printf("%f sec %f ms each %d\n", sec, 1000*sec/total, total_solved);


   return 0;
}
