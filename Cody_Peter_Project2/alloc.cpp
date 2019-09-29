/*
 * created by Cody Peter
 * for the purpose of allocating resources from the available map.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>

#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "string.h"

using namespace std;

void readAllocMap(char* map, int* allocArr);
void writeAllocMap(char* map, int* allocArr);


#define KEY 0xAA11

struct sembuf p = { 0, -1, SEM_UNDO}; // semwait
struct sembuf v = { 0, +1, SEM_UNDO}; // semsignal
int semid;

union semun {
  int              val;
  struct semid_ds *buf;
  unsigned short  *array;
  struct seminfo  *__buf;
};

int main(int argc, char* argv[]) {
  int res, quant;
    struct stat sb;
    
    semid = semget(KEY, 1, 0666 | IPC_CREAT);
    if(semid == -1) {
        perror("semget"); exit(1);
    }
    union semun u;
    u.val = 1;
    if(semctl(semid, 0, SETVAL, u) < 0) {
        perror("semctl"); exit(1);
    }
    
    int resFile = open("res.txt", O_RDWR);
    if (fstat(resFile, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    
//    printf("File size: %lld bytes\n",
//           (long long) sb.st_size);
//
//    if (lseek(resFile, 40, 0) == -1) {
//        perror("seek");
//        exit(EXIT_FAILURE);
//    }
    
    char* map = (char*)mmap(0, 20, PROT_READ | PROT_WRITE, MAP_SHARED, resFile, 0);
    assert(map != MAP_FAILED);
    close(resFile);

    int *allocArr = (int*)malloc(5 * sizeof(int));
    for (int k = 0; k < 5; k++) {
        allocArr[k] = 0;
    }
    
    while (true) {
        cout << "enter resource type and amount needed: ";
        cin >> res >> quant;
        if (res >= 0 && res < 5) {
            //Wait
            if(semop(semid, &p, 1) < 0)
            {
                perror("semop p"); exit(13);
            }
            readAllocMap(map, allocArr);
            if (allocArr[res] - quant >= 0) {
                allocArr[res] -= quant;
                writeAllocMap(map, allocArr);
                readAllocMap(map, allocArr);
            } else {
                cout << "Insufficient amount of resource: " << res << "\n";
                res = -1;
            }
            //Signal
            if(semop(semid, &v, 1) < 0)
            {
                perror("semop p"); exit(14);
            }
        } else {
            break;
        }
    }
    if (munmap(map, sb.st_size) == -1) {
        perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
    }
    exit(0);
}

/*
 * Read from #input(map) and write values to #output(allocArr)
 */
void readAllocMap(char* map, int* allocArr) {
    char c;
    bool index = true;
    int i, val;
    size_t iSize;
    for (iSize = 0, i = 0; iSize < 20; iSize++) {
        c = map[iSize];
        if (c != ' ' && c != '\n') {
            if (index) {
                i = c - '0';
                index = false;
            } else {
                val = c - '0';
                allocArr[i] = val;
                index = true;
            }
        }
    }
}

/*
 * Write from #input(allocArr) and write values to #output(map)
 */
void writeAllocMap(char* map, int* allocArr) {
    size_t i;
    int index = 0;
    
    for (index = 0, i = 0; index < 5; index++) {
        map[i++] = index + '0';
        map[i++] = ' ';
        map[i++] = allocArr[index] + '0';
        map[i++] = '\n';
    }
    if (msync((void *)map, 20, MS_SYNC) == -1) {
        perror("Could not sync to disk");
    }
}
