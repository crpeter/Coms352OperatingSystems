#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "math.h"


void *row_thread( void *ptr );
void *col_thread( void *ptr );
void bubble_sort(int line, int type);
void swap(int *xp, int *yp);
void print_a();

//#define ROW_COUNT 4
//#define COL_COUNT 4

sem_t mutex;

//int a[ ROW_COUNT ][ COL_COUNT ];

// 4x4 array can be chanded for other values
int a[4][4];// = {{3, 11, 6, 16}, {8, 1, 5, 10}, {14, 7, 12, 2}, {4, 13, 9, 15}};

// Global counters for thread methods.
int i = 0, j = 0, k = 0;
int ROW_COUNT, COL_COUNT;

// Main method accepts an input of n to represent nxn matrix. and reads from input.txt
int main(int argc, char* argv[]) {
  FILE *ptr_file;
  char buf[1000];
  int help[1000];
  int index, count = 0;
  
  if (argc < 2) {
    printf("Usage: ./out \n (nxn, total matrix values)\n");
    return -1;
  }
  count = strtol(argv[1], NULL, 10);
  printf("count: %d", count);
  ROW_COUNT = count;
  COL_COUNT = count;
  ptr_file = fopen("input.txt", "r");
  if (!ptr_file)
    return 1;
  
  for (i = 0; i < count; i++)
  {
    for (j = 0; j < count; j++) {
      fscanf(ptr_file, "%d", &a[i][j]);
    }
  }
  if (ptr_file !=stdin) fclose(ptr_file);
  
  pthread_t thread1, thread2;
  pthread_t threadList[ROW_COUNT];

  sem_init(&mutex, 0, 1);
  
  //print_a();
  // Alternate sending row and column threads
  int k;
  for (k = 0; k < ROW_COUNT + 1; k++) {
    if (k % 2 == 0) {
      if (pthread_create( &threadList[k], NULL, row_thread, &k))
        printf("thread failed to create\n");
    } else {
      if (pthread_create( &threadList[k], NULL, col_thread, &k))
        printf("thread failed to create\n");
    }
    //print_a();
  }
  sem_destroy(&mutex);
  
  sleep(2);
  print_a();
  exit(0);
  
  return 0;
}

// Thread that sorts rows
void *row_thread( void *row )
{
  sem_wait(&mutex);
  for (i = 0; i < ROW_COUNT; i++) {
    bubble_sort(i, 0);
  }
  printf("k: %d\n", k++);
  print_a();
  sem_post(&mutex);
}

// Thread that sorts columns
void *col_thread( void *col )
{
  sem_wait(&mutex);
  for (i = 0; i < COL_COUNT; i++) {
    bubble_sort(i, 1);
  }
  printf("k: %d\n", k++);
  print_a();
  sem_post(&mutex);
}

// Bubble sort helper method takes a line either row or column and type (indicationg whether to sort vertically or horizontally)
void bubble_sort(int line, int type) {
  int i, j;
  int n = ROW_COUNT;
  if (type == 0) {
    for (i = 0; i < n-1; i++) {
      for (j = 0; j < n-i-1; j++) {
        if (line % 2 == 0) {
          if (a[line][j] > a[line][j+1])
            swap(&a[line][j], &a[line][j+1]);
        } else {
          if (a[line][j] < a[line][j+1])
            swap(&a[line][j], &a[line][j+1]);
        }
      }
    }
  } else {
    for (i = 0; i < n-1; i++) {
      for (j = 0; j < n-i-1; j++) {
        if (a[j][line] > a[j+1][line])
          swap(&a[j][line], &a[j+1][line]);
      }
    }
  }
}


// Swap elements of an array helper method.
void swap(int *fst, int *snd)
{
  int temp = *fst;
  *fst = *snd;
  *snd = temp;
}

// Print array a helper method.
void print_a() {
  for (i = 0; i < ROW_COUNT; i++) {
    for (j = 0; j < COL_COUNT; j++) {
      printf("%3d", a[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}
