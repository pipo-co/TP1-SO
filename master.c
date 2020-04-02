// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
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
#define MIN_CHILD_COUNT 2
#define MIN_INIT_CHILD_TASK_COUNT 1
#define CHILD_TASK_PER_CYCLE 1
#define CHILD_PATH "childCode.out"
#define FILE_NAME "output.txt"
#define SHM_NAME "/shm"
#define COMS_NAME "/coms_sem"
#define OUTPUT_MAX_SIZE (INPUT_MAX_SIZE + 50)
#define MAX_INIT_ARGS 50
#define INPUT_MAX_SIZE 200

#define HANDLE_ERROR(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)

typedef struct childStruct{
    pid_t pid;
    int fdInput;
    int fdOutput;
    size_t tasksPending; 
}childStruct;

//Returns total tasks delivered to children on init.
size_t prepareChildren(childStruct childArray[], char const *taskArray[], size_t childCount, size_t initChildTaskCount, size_t* taskCounter);
void outputInfo(char output[], FILE* file, size_t* mapCounter, char* map, sem_t* coms_sem, size_t tasksRecivedCounter);
void * initializeSHMSpace(const char * name, int oFlags, mode_t mode, int prot, size_t size);
void prepareChildOutputFdSet(childStruct * childArray,  size_t childCount, fd_set * rfds);
void freeResources(sem_t* coms_sem, char* map, size_t totalTasks, FILE * outputFile);
size_t assignTasks(int fd, char const *taskArray[], size_t tasksToAssign);
void colectChildren(childStruct childArray[], size_t childCount);
int minimum(int n1, int n2);

int main(int argc, char const *argv[]){
    if(argc == 1)
        return 0;

    const char ** taskArray = argv + 1;
    childStruct childArray[MAX_NUM_CHILD];
    size_t childCount, initChildTaskCount, childTasksPerCycle;
    size_t taskCounter = 0;
    size_t totalTasks = argc - 1;
    size_t generalTasksPending = 0;
    size_t mapCounter = 0;
    
    if(setvbuf(stdout, NULL, _IONBF, 0) != 0)
        HANDLE_ERROR("Master - Setvbuf");

    //Seting Configuration Variables and consecuent validation
        childCount = (MIN_CHILD_COUNT >= totalTasks/100) ? MIN_CHILD_COUNT : totalTasks/100 ; //Menos que MAX_NUM_CHILD
        initChildTaskCount = (MIN_INIT_CHILD_TASK_COUNT >= totalTasks/100) ? MIN_INIT_CHILD_TASK_COUNT : totalTasks/100 ; //Menos que MAX_INIT_ARGS. No puede ser 0.
        childTasksPerCycle = CHILD_TASK_PER_CYCLE; //La consigna dice que tiene que ser 1. La variable existe para la abstraccion.

        if(childCount > MAX_NUM_CHILD)
            childCount = MAX_NUM_CHILD;

        if(initChildTaskCount > MAX_INIT_ARGS)
            initChildTaskCount = MAX_INIT_ARGS;

        if(childCount*initChildTaskCount >= totalTasks)
            initChildTaskCount = 1;
    //End Configuration
    
    //Shared Memory, Sempahores and Output File Initialization
        char * map = initializeSHMSpace(SHM_NAME, O_EXCL | O_CREAT | O_RDWR, S_IRWXU, PROT_WRITE,  totalTasks * OUTPUT_MAX_SIZE * sizeof(char));

        sem_t * coms_sem = sem_open(COMS_NAME, O_EXCL | O_CREAT, O_RDWR, 0);
        if(coms_sem == SEM_FAILED)
            HANDLE_ERROR("Master - Semaphore open coms");
    
        FILE * fileOutput = fopen(FILE_NAME, "w");
        if(fileOutput == NULL)
            HANDLE_ERROR("Master - Opening output file");
    //End Initialization

    
    printf("%zu\n", totalTasks); //Le pasamos a View la cantidad de files
    sleep(2); //La consigna pide que master dure mas de 2 segundos

    generalTasksPending += prepareChildren(childArray, taskArray, childCount, initChildTaskCount, &taskCounter);
        
    //Main Logic of Master
    fd_set fdSet;
    int fdAvailable; //Si bien a esta variable se le podria reducir el scope, nos parece que de esta forma queda mas claro el codigo
    int readAux;
    char output[OUTPUT_MAX_SIZE];
    size_t tasksRecivedCount, taskAsigned;
    
    while(taskCounter < totalTasks || generalTasksPending > 0){
        
        prepareChildOutputFdSet(childArray, childCount, &fdSet);

        if((fdAvailable = select(childArray[childCount - 1].fdOutput + 1, &fdSet, NULL, NULL, NULL)) == -1)
            HANDLE_ERROR("Master - Select");

        for(size_t i = 0; fdAvailable > 0 && i < childCount && generalTasksPending > 0; i++){
            if(FD_ISSET(childArray[i].fdOutput, &fdSet)){
                if((readAux = read(childArray[i].fdOutput, output, OUTPUT_MAX_SIZE - 1)) == -1)
                    HANDLE_ERROR("Master - Read Output from child");
                    
                if(readAux){ //Lee EOF como available
                    output[readAux] = 0;

                    tasksRecivedCount = 0;                  
                    for( char* auxPtr = output; (auxPtr = strchr(auxPtr, '\n')) != NULL; auxPtr++)
                        tasksRecivedCount++;

                    outputInfo(output, fileOutput, &mapCounter, map, coms_sem, tasksRecivedCount); //Imprimir en archivo y en memoria compartida

                    childArray[i].tasksPending -= tasksRecivedCount;
                    generalTasksPending -= tasksRecivedCount;

                    if(childArray[i].tasksPending <= 0){
                        //Le mando tareas al escalvo correspondiente
                        taskAsigned = assignTasks(childArray[i].fdInput, taskArray + taskCounter, minimum(totalTasks - taskCounter, childTasksPerCycle));
                        generalTasksPending += taskAsigned;
                        taskCounter += taskAsigned;
                        childArray[i].tasksPending += taskAsigned;
                    }
                }
                fdAvailable--;
            }
        } 
    }
    
    //Collect children and Free Resources
        colectChildren(childArray, childCount);
        freeResources(coms_sem, map, totalTasks, fileOutput);

    return 0;
}

size_t prepareChildren(childStruct childArray[], char const *taskArray[], size_t childCount, size_t initChildTaskCount, size_t* taskCounter){

    int fdPipeInput[2];
    int fdPipeOutput[2];
    char * initArgsArray[MAX_INIT_ARGS + 2];
    int totalTasksDelivered = 0;

    for (size_t i = 0; i < childCount; i++){
        pid_t forkId;

        if(pipe(fdPipeInput))
            HANDLE_ERROR("Master - Creating Child Input Pipe");

        childArray[i].fdInput = fdPipeInput[PIPE_WRITE];

        if(pipe(fdPipeOutput))
            HANDLE_ERROR("Master - Creating Child Output Pipe");

        childArray[i].fdOutput = fdPipeOutput[PIPE_READ];

        if((forkId = fork()) == -1)
            HANDLE_ERROR("Master - Forking to Create Child");

        childArray[i].pid = forkId;

        if(!forkId){ //child
            if(close(fdPipeInput[PIPE_WRITE]) == -1)
                perror("Master - Closing Pipe Input");

            if(close(fdPipeOutput[PIPE_READ]) == -1)
                perror("Master - Closing Pipe Output");

            if(dup2(fdPipeInput[PIPE_READ], STDIN_FILENO) == -1)
                HANDLE_ERROR("Master - Dup2 Setting Child Stdin");

            if(dup2(fdPipeOutput[PIPE_WRITE], STDOUT_FILENO) == -1)
                HANDLE_ERROR("Master - Dup2 Setting Child Stdout");

            if(close(fdPipeInput[PIPE_READ]) == -1)
                perror("Master - Closing Child Pipe Input");
            
            if(close(fdPipeOutput[PIPE_WRITE]) == -1)
                perror("Master - Closing Child Pipe Output");

            initArgsArray[0] = CHILD_PATH;
            for(size_t i = 1; i <= initChildTaskCount; i++)
                initArgsArray[i] = taskArray[(*taskCounter)++];
            initArgsArray[initChildTaskCount + 1] = NULL;

            execv(CHILD_PATH,  initArgsArray);
            HANDLE_ERROR("Master - Exec");
        }

        childArray[i].tasksPending = initChildTaskCount;
        totalTasksDelivered += initChildTaskCount;
        *taskCounter += initChildTaskCount;

        if(close(fdPipeInput[PIPE_READ]) == -1)
            perror("Master - Closing Parent Pipe Input");

        if(close(fdPipeOutput[PIPE_WRITE]) == -1)
            perror("Master - Closing Parent Pipe Output");
    }
    return totalTasksDelivered;
}

void outputInfo(char output[], FILE* file, size_t* mapCounter, char* map, sem_t* coms_sem, size_t tasksRecivedCounter){

    if(fwrite(output, strlen(output), sizeof(output[0]), file) == 0)
        HANDLE_ERROR("Master - Fwrite to output file");

    size_t outputSize = strlen(output);
    memcpy(map + *mapCounter, output, outputSize);
    *mapCounter += outputSize;

    for(size_t i = 0; i < tasksRecivedCounter; i++)
        if(sem_post(coms_sem) == -1)
            HANDLE_ERROR("Master - Post Semaphore coms_sem");
}

void prepareChildOutputFdSet(childStruct * childArray,  size_t childCount, fd_set * rfds){

    FD_ZERO(rfds); 

    for (size_t i = 0; i < childCount; i++)
        FD_SET(childArray[i].fdOutput, rfds);  
}

size_t assignTasks(int fd, char const *taskArray[], size_t tasksToAssign){

    char inputBuff[INPUT_MAX_SIZE];
    
    for (size_t i = 0; i < tasksToAssign; i++){
        sprintf(inputBuff, "%s\n", taskArray[i]);
        
        if(write(fd, inputBuff, strlen(inputBuff)) == -1)
            HANDLE_ERROR("Master - Write to send file path to child");
    }
    return tasksToAssign;
}

void *initializeSHMSpace(const char * name, int oFlags, mode_t mode, int prot, size_t size){

    int shm = shm_open(name,  oFlags, mode);
    if(shm == -1)
        HANDLE_ERROR("Master - shm_open");
        
    if(ftruncate(shm, size) == -1)
        HANDLE_ERROR("Master - Ftruncate");
        
    void * map = mmap(NULL, size, prot, MAP_SHARED, shm, 0);
    if(map == MAP_FAILED)
        HANDLE_ERROR("Master - Mmap");
        
    if(close(shm) == -1)
        perror("Master - Close Shm");

    return map;
}

void colectChildren(childStruct childArray[], size_t childCount){
   
    for (size_t i = 0; i < childCount; i++){
        if(close(childArray[i].fdInput) == -1)
            HANDLE_ERROR("Master - Closing childs' Input ");

        if(close(childArray[i].fdOutput) == -1)
            HANDLE_ERROR("Master - Closing childs' Output ");
    }

    for (size_t i = 0; i < childCount; i++){
        if(wait(NULL) == -1)
            HANDLE_ERROR("Master - Waiting for children");
    }
}

void freeResources( sem_t* coms_sem, char* map, size_t totalTasks, FILE * outputFile){
    
    if(fclose(outputFile) == EOF)
        perror("Master - Close output FILE");
    

    if(sem_close(coms_sem) == -1)
        perror("Master - Close Semaphore coms_sem");

    if(sem_unlink(COMS_NAME) == -1)
        if(errno != ENOENT)
            perror("Master - Unlink Semaphore coms_sem");
    
    if(shm_unlink(SHM_NAME) == -1)
        if(errno != ENOENT)
            perror("Master - Unlink Shared Memory");

    if(munmap(map, totalTasks * OUTPUT_MAX_SIZE) == -1)
        perror("Master - Munmap");
}

int minimum(int n1, int n2){
    if(n1 <= n2)
        return n1;
    return n2;
}

