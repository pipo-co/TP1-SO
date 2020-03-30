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

#define SHM_NAME "/shm"
#define SM_NAME "/sm_sem"
#define COMS_NAME "/coms_sem"
#define OUTPUT_MAX_SIZE 250
#define MAX_INT_LEN 10

#define HANDLE_ERROR(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int viewProcess(char ** map, sem_t * sh_sem, sem_t * coms_sem);
void freeResources(sem_t* sm_sem, sem_t* coms_sem, char* map, size_t totalTasks);

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

        if(ftruncate(shm, totalTasks * OUTPUT_MAX_SIZE) == -1)
            HANDLE_ERROR("View - Ftruncate");

        char * map = mmap(NULL, totalTasks * OUTPUT_MAX_SIZE, PROT_WRITE, MAP_SHARED, shm, 0);
        if(map == MAP_FAILED)
            HANDLE_ERROR("View - Mmap");

        if(close(shm) == -1)
            perror("View - Close Shm");

        sem_t * sm_sem = sem_open(SM_NAME, O_CREAT,O_RDWR,1);
        if(sm_sem == SEM_FAILED)
            HANDLE_ERROR("View - Open Semaphore sm_sem");

        sem_t * coms_sem = sem_open(COMS_NAME, O_CREAT, O_RDWR, 0);
        if(coms_sem == SEM_FAILED)
            HANDLE_ERROR("View - Open Semaphore coms_sem");

    //Main process
    while(viewProcess(&map, sm_sem, coms_sem));


    freeResources(sm_sem, coms_sem, map, totalTasks);
    return 0;
}

int viewProcess(char ** map, sem_t *sm_sem, sem_t * coms_sem){

    if(sem_wait(coms_sem) == -1)
        HANDLE_ERROR("View - Wait Sempahore coms_sem");


    if(sem_wait(sm_sem) == -1)
        HANDLE_ERROR("View - Wait Sempahore sm_sem");

    if(**map != 0){

        printf("%s", *map);
        *map += strlen(*map);

        if(sem_post(sm_sem) == -1)
            HANDLE_ERROR("View - Post Sempahore sm_sem");

        return 1;
    }
    return 0;         
}

void freeResources(sem_t* sm_sem, sem_t* coms_sem, char* map, size_t totalTasks){
    if(sem_close(sm_sem) == -1)
            perror("View - Close Semaphore sm_sem");

        if(sem_unlink(SM_NAME) == -1)
            if(errno != ENOENT)
                perror("Master - Unlink Semaphore sm_sem");

        if(sem_close(coms_sem) == -1)
            perror("View - Close Semaphore coms_sem");

        if(sem_unlink(COMS_NAME) == -1)
            if(errno != ENOENT)
                perror("Master - Unlink Semaphore coms_sem");

        if(munmap(map, totalTasks * OUTPUT_MAX_SIZE) == -1)
           perror("View - Munmap");

        if(shm_unlink(SHM_NAME) == -1)
            if(errno != ENOENT)
                perror("Master - Unlink Shared Memory");
}