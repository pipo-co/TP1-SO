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

#define NUM_CHILD 5
#define PIPE_WRITE 1
#define PIPE_READ 0
#define CHILD_PATH "childCode.out"
#define BUFFER_SIZE 10000

int main(int argc, char const *argv[])
{
    int forkId;
    int fdChildInput[NUM_CHILD];
    int fdChildOutput[NUM_CHILD];
    int fdPipeInput[2];
    int fdPipeOutput[2];
    size_t childCount;
    //int childPID[NUM_CHILD];

    for (size_t i = 0; i < argc - 1 && i < NUM_CHILD; i++){
        childCount = i + 1;

        if(pipe(fdPipeInput)){
            perror("PIPE INPUT ERROR:");
            exit(EXIT_FAILURE);
        }

        fdChildInput[i] = fdPipeInput[PIPE_WRITE];

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
            close(fdPipeInput[PIPE_WRITE]);
            close(fdPipeOutput[PIPE_READ]);

            dup2(fdPipeInput[PIPE_READ], STDIN_FILENO);
            //dup2(fdPipeOutput[PIPE_WRITE], STDOUT_FILENO);

            close(fdPipeInput[PIPE_READ]);
            close(fdPipeOutput[PIPE_WRITE]);
            

            execl(CHILD_PATH, CHILD_PATH, (char*)NULL);
            perror("EXEC OF CHILD FAILED");
        }

        close(fdPipeInput[PIPE_READ]);
        close(fdPipeOutput[PIPE_WRITE]);

        if(write(fdChildInput[i], argv[i + 1], strlen(argv[i + 1]) + 1) == -1 )
            perror("ERROR ON WRITE");
    }
    

    fd_set fdSet;
    char buff[BUFFER_SIZE];
    size_t aux;
    for (size_t i = 0; i < childCount;){
        FD_ZERO(&fdSet);
        for (size_t i = 0; i < childCount; i++)
            FD_SET(fdChildOutput[i], &fdSet);

        select(fdChildOutput[childCount - 1] + 1, &fdSet, NULL, NULL, NULL);

        for (size_t j = 0; j < childCount; j++){
            if(FD_ISSET(fdChildOutput[j], &fdSet)){
                if((aux = read(fdChildOutput[j], buff, BUFFER_SIZE)) == -1)
                    perror("ERROR AL LEER");
                if(aux){
                    if(aux > 0 && aux < BUFFER_SIZE)
                        buff[aux] = 0;

                    printf("Soy el padre %s\n", buff);
                    i++;
                }
            }
        } 
    }
    

    for (size_t i = 0; i < childCount; i++){
        if(wait(NULL) == -1)
            perror("ERROR DE WAIT");
    }
    
}
