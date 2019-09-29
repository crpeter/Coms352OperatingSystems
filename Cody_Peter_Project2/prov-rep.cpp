/*
 * created by Cody Peter
 * for the purpose of providing resources and reporting on available resources.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>

#include "string.h"


using namespace std;


void readAllocMap(char* map, int* allocArr);
void writeAllocMap(char* map, int* allocArr);
void reportLoop(char* map, int* allocArr, size_t fileSize);
void provideLoop(char* map, int* allocArr);
void report(char* map, int* allocArr, size_t fileSize);

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
    
    char* map = (char*)mmap(0, 20, PROT_READ | PROT_WRITE, MAP_SHARED, resFile, 0);
    assert(map != MAP_FAILED);
    close(resFile);
    
    int *allocArr = (int*)malloc(5 * sizeof(int));
    for (int k = 0; k < 5; k++) {
        allocArr[k] = 0;
    }
    readAllocMap(map, allocArr);
//    writeAllocMap(map, allocArr);
//    exit(0);
    pid_t childpid;
    
    if ((childpid = fork()) == -1) {
        perror("Couldn't grab the fork");
        exit(0);
    }
    if (childpid == 0) {
        reportLoop(map, allocArr, sb.st_size);
    } else {
        provideLoop(map, allocArr);
    }
    //readAllocMap(map, allocArr);
}

/*
 * Run a loop that accepts input on what and how many resources to allocat.
 */
void provideLoop(char* map, int* allocArr) {
    char input;
    int res, quant;
    while (true) {
        cout << "Provide more resources(y/n)?";
        cin >> input;
        if (input != 'y') {
            break;
        }
        cout << "Enter the resource number and number of additional resources to be provided: ";
        cin >> res >> quant;
        if (res >= 0 && res < 5) {
            //Wait
            if(semop(semid, &p, 1) < 0)
            {
                perror("semop p"); exit(13);
            }
            readAllocMap(map, allocArr);
            if (allocArr[res] + quant <= 9) {
                allocArr[res] += quant;
                writeAllocMap(map, allocArr);
            } else {
                cout << "Too many for resource: " << res << "][" << allocArr[res] + quant << "\n";
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
}

/*
 * Run a loop that sleeps 10 sec and then reports on state of resources
 */
void reportLoop(char* map, int* allocArr, size_t fileSize) {
    while (true) {
        sleep(10);
        //Wait
        if(semop(semid, &p, 1) < 0)
        {
            perror("semop p"); exit(13);
        }
        readAllocMap(map, allocArr);
        report(map, allocArr, fileSize);
        //Signal
        if(semop(semid, &v, 1) < 0)
        {
            perror("semop p"); exit(14);
        }
        //exit(0);
    }
}

/*
 * Report on state of resources
 */
void report(char* map, int* allocArr, size_t fileSize) {
    int i, pSize, vecSize;
    unsigned char *vec;

    cout << "\n\nReport:\n";
    cout << "Page size is: " << (pSize = getpagesize());
    cout << "\nNumber of pages needed: " << (1 + (fileSize / pSize));
    cout << "\nCurrent State of Resources:\n";
    for (i = 0; i < 5; i++) {
        printf("%d  %d\n", i, allocArr[i]);
    }
    cout << "Status of Pages in Main Memory:\n";
    vecSize = (fileSize + pSize - 1) / pSize;
    vec = (unsigned char*) calloc(1, vecSize);
    if (mincore((void*)map, fileSize, vec) != 0) {
        perror("Couldn't mincore");
        return;
    } else {
        for (i = 0; i < (1 + (fileSize / pSize)); i++) {
            cout << "Page " << i << ": " << (vec[i] & 1 ? "Resident" : "Non-Resident");
            cout << "\n\n";
        }
    }
}

/*
 * Read from #input(map) and write values to #output(allocArr)
 */
void readAllocMap(char* map, int* allocArr) {
    char c;
    bool index = true;
    int i, val;
    size_t iSize;
    
    for (int k = 0; k < 5; k++) {
        allocArr[k] = 0;
    }
    for (iSize = 0, i = 0; iSize < 20; iSize++) {
        c = map[iSize];
        if (c != ' ' && c != '\n') {
            if (index) {
                //i = c - '0';
                index = false;
            } else {
                val = c - '0';
                allocArr[i++] = val;
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
