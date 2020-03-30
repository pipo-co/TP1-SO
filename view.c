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
#define SM_NAME "/sem"
#define COMS_NAME "/sem2"
#define OUTPUT_SIZE 10000
#define MAX_INIT_ARGS 20
#define INPUT_MAX_SIZE 15000
#define MAX_TOTAL_TASKS 10

int viewProcess(char * map, sem_t * sh_sem, sem_t * coms_sem);

int main(int argc, char const *argv[])
{
     printf("hola1\n");
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
    
    size_t tTasks = atoi(totalTasks);
    if(tTasks==5)
        printf("view %ld\n",tTasks);
    if(ftruncate(shm, tTasks*(INPUT_MAX_SIZE)) == -1){
        perror("Ftruncate");
        exit(EXIT_FAILURE);
    }
  
    char * map = mmap(NULL,tTasks*(INPUT_MAX_SIZE), PROT_WRITE, MAP_SHARED, shm, 0);
    
    close(shm);
    if(map == MAP_FAILED){
        perror("Mmap");
        exit(EXIT_FAILURE);
    }

    sem_t * sm_sem = sem_open(SM_NAME, O_CREAT,O_RDWR,1);
    if(sm_sem == SEM_FAILED){
        perror("Semaphore");
        exit(EXIT_FAILURE);
    }
    sem_t * coms_sem = sem_open(COMS_NAME, O_CREAT, O_RDWR, 0);
    if(coms_sem == SEM_FAILED){
        perror("Semaphore2");
        exit(EXIT_FAILURE);
    }
   
    

    while(viewProcess(map, sm_sem, coms_sem));
   

    sem_close(sm_sem);
    sem_unlink(SM_NAME);
    sem_close(coms_sem);
    sem_unlink(COMS_NAME);
    munmap(map,tTasks*(INPUT_MAX_SIZE));
    shm_unlink(SHM_NAME);
    return 0;
}

int viewProcess(char * map, sem_t *sm_sem, sem_t * coms_sem){
    //printf("chaucha\n");
    if(sem_wait(coms_sem)==-1){
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
    //printf("chau\n");
    if(sem_wait(sm_sem)==-1){
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
    //printf("chau2\n");
    if(*map != 0){
        printf("%s", map);
        map+=strlen(map)+1;
       // printf("chau3\n");
        if(sem_post(sm_sem)==-1){
            perror("sem_post");
            exit(EXIT_FAILURE);
        } 
       // printf("chau4\n");
        return 1;
    }
    printf("ESTE ES EL MEJOR PRONTF");
    return 0;
            
}