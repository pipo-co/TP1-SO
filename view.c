#define _POSIX_C_SOURCE 200112L


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


#define MAX_NUM_CHILD 5
#define PIPE_WRITE 1
#define PIPE_READ 0
#define CHILD_PATH "childCode.out"
#define FILE_NAME "answer.txt"
#define SHM_NAME "/shm"
#define SEM_NAME "/sem"
#define OUTPUT_SIZE 10000
#define MAX_INIT_ARGS 20
#define INPUT_MAX_SIZE 1000
#define MAX_TOTAL_TASKS 10
int main(int argc, char const *argv[])
{
    char totalTasks[MAX_TOTAL_TASKS];
    
    int shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRWXU);
    if(shm == -1){
        perror("Shared memory");
        exit(EXIT_FAILURE);
    }
    
    
    if(read(STDIN_FILENO,totalTasks,strlen(totalTasks))==-1){
        perror("Error while reading");
        exit(EXIT_FAILURE);
    }
    
    int tTasks = atoi(totalTasks);
    
    if(ftruncate(shm, tTasks*(40 + INPUT_MAX_SIZE)) == -1){
        perror("Ftruncate");
        exit(EXIT_FAILURE);
    }
    
    char * map = mmap(NULL,tTasks*(40 + INPUT_MAX_SIZE), PROT_WRITE, MAP_SHARED, shm, 0);
    
    if(map== MAP_FAILED){
        perror("Mmap");
        exit(EXIT_FAILURE);
    }

    sem_t * sem = sem_open(SEM_NAME, 0);
    if(sem == SEM_FAILED){
        perror("Semaphore");
        exit(EXIT_FAILURE);
    }

    while(1){
        if(*map != 0){
            sem_wait(sem);
            printf("%s\n", map);
            map+=strlen(map);
            sem_post(sem);
        }
    }
    
    close(shm); 
    return 0;
}