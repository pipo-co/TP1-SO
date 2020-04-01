// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>

#define SHM_NAME "/shm"
#define COMS_NAME "/coms_sem"
#define OUTPUT_MAX_SIZE 250
#define MAX_INT_LEN 10

#define HANDLE_ERROR(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

void viewProcess(char ** mapIter, sem_t * coms_sem);
void freeResources(sem_t* coms_sem, char* map, size_t totalTasks);

int main(int argc, char const *argv[]){

    char totalTasksBuff[MAX_INT_LEN];
    size_t totalTasks;

    if(argc <= 1){ //Recibe info por stdin
        if(read(STDIN_FILENO, totalTasksBuff, MAX_INT_LEN) == -1)
            HANDLE_ERROR("View - Error Reading Stdin");
        totalTasks = atoi(totalTasksBuff);
    } else
        totalTasks = atoi(argv[1]);

    //Shared Memory and Sempahores Initialization
        int shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRWXU);
        if(shm == -1)
            HANDLE_ERROR("View - Open Shared Memory");

        char * map = mmap(NULL, totalTasks * OUTPUT_MAX_SIZE, PROT_WRITE, MAP_SHARED, shm, 0);
        if(map == MAP_FAILED)
            HANDLE_ERROR("View - Mmap");

        if(close(shm) == -1)
            perror("View - Close Shm");

        if(shm_unlink(SHM_NAME) == -1)
            perror("Master - Unlink Shared Memory");

        sem_t * coms_sem = sem_open(COMS_NAME, O_CREAT, O_RDWR, 0);
        if(coms_sem == SEM_FAILED)
            HANDLE_ERROR("View - Open Semaphore coms_sem");
    
    char* mapIter = map;

    //Main process
    for (size_t tasksDone = 0; tasksDone < totalTasks; tasksDone++)
        viewProcess(&mapIter, coms_sem);
    
    freeResources(coms_sem, map, totalTasks);
    return 0;
}

void viewProcess(char ** mapIter, sem_t * coms_sem){
    char * printPtr = *mapIter;
    
    if(sem_wait(coms_sem) == -1)
        HANDLE_ERROR("View - Wait Sempahore coms_sem");

    if((*mapIter = strchr(*mapIter, '\n')) == NULL)
        perror("View - Tried to output an incomplete task");

    **mapIter = '\0';
    (*mapIter)++;  

    printf("%s\n", printPtr);    
}

void freeResources( sem_t* coms_sem, char* map, size_t totalTasks){
    
    if(sem_close(coms_sem) == -1)
        perror("View - Close Semaphore coms_sem");

    if(sem_unlink(COMS_NAME) == -1)
        if(errno != ENOENT)
            perror("Master - Unlink Semaphore coms_sem");

    if(munmap(map, totalTasks * OUTPUT_MAX_SIZE) == -1)
        perror("View - Munmap");
}
