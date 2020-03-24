//include
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <dirent.h>
    #include <string.h>
    #include <stdlib.h>
    #include <errno.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <sys/wait.h>
    #include <sys/select.h>
    #include <fcntl.h> 
    #include <semaphore.h> 
//end include

#define NUM_CHILD 3
#define PIPE_WRITE 1
#define PIPE_READ 0
#define BUFFER_SIZE 10000
#define CHILD_PATH "childCode.out"
#define FIFO_PATH "./fifo1"
#define SEM_NAME "mainSem"


int checkAndPrintAns(size_t childCount, char * buff, size_t size, int * fdChildOutput);
int main(int argc, char const *argv[]){

    if( argc <= 1)
        return 1;

    int programCounter = argc;
    int forkId;
    int fdChildOutput[NUM_CHILD];
    int writingFd;
    int fdPipeOutput[2];
    size_t childCount;

    char buff[BUFFER_SIZE];

    if(mkfifo(FIFO_PATH,(mode_t)0666) == -1)
        perror("fallo makefifo");
    
    sem_t * semAdress;
    sem_unlink(SEM_NAME);
    if((semAdress = sem_open( SEM_NAME, O_CREAT | O_EXCL, (mode_t) 0666, 1)) == SEM_FAILED){
        perror("Error creating semaphore");
    }
    
    for (size_t i = 0; i < argc - 1 && i < NUM_CHILD; i++){
        
        childCount = i + 1;

        if(pipe(fdPipeOutput)){
            perror("PIPE OUTPUT ERROR:");
            exit(EXIT_FAILURE);
        }

        fdChildOutput[i] = fdPipeOutput[PIPE_READ];

        if((forkId = fork()) == -1){
            perror("FORK ERROR");
            exit(EXIT_FAILURE);
        }

        if(!forkId){ //child

            close(fdPipeOutput[PIPE_READ]);

            dup2(fdPipeOutput[PIPE_WRITE], STDOUT_FILENO);

            close(fdPipeOutput[PIPE_WRITE]);
            execl(CHILD_PATH, CHILD_PATH, FIFO_PATH, SEM_NAME ,argv[programCounter - 1],(char*)NULL);
            perror("EXEC OF CHILD FAILED");
            exit(EXIT_FAILURE);
        }
        programCounter--;
        close(fdPipeOutput[PIPE_WRITE]);
    }
    
    for (size_t i = 0; i < argc - 1;){

       i += checkAndPrintAns(childCount, buff, BUFFER_SIZE, fdChildOutput);
        
        if((writingFd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK)) != -1){
            if(programCounter > 0){
                if( write(writingFd, argv[programCounter-1], strlen(argv[programCounter-1])) == -1)
                    perror("Error writing");
                close(writingFd);
                sem_wait(semAdress);
                programCounter--;
            }
        }
    }
    

    for (size_t i = 0; i < childCount; i++){
        if(wait(NULL) == -1)
            perror("ERROR DE WAIT");
    }
    
}

int checkAndPrintAns(size_t childCount, char * buff, size_t size, int * fdChildOutput){
    
    fd_set rfds;
    int aux, ansRecieved = 0;
    
    FD_ZERO(&rfds);
    for (size_t k = 0; k < childCount; k++){
        FD_SET(fdChildOutput[k], &rfds);
        //printf("%d",fdChildOutput[k]);
    }
    putchar('\n');

    select(fdChildOutput[childCount - 1] + 1, &rfds, NULL, NULL, NULL);

    for (size_t j = 0; j < childCount; j++){
        if(FD_ISSET(fdChildOutput[j], &rfds)){
            if((aux = read(fdChildOutput[j], buff, BUFFER_SIZE)) == -1){
                perror("ERROR AL LEER");
                exit(EXIT_FAILURE);
            }
            if(aux){
                if(aux > 0 && aux < BUFFER_SIZE)
                    buff[aux] = 0;

                printf("\n%s\n", buff);
                ansRecieved++;
            }
        }
    } 
    return ansRecieved;
}
