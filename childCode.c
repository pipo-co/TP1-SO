#define _DEFAULT_SOURCE

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

#define INPUT_MAX_SIZE 1000
#define COMMAND_MAX_SIZE 1000
#define PIPE_WRITE 1
#define PIPE_READ 0
enum args{NAME = 0, FIFO,SEM, FIRST_TASK};

int processRequest(const char * input, char * buffer, size_t size);

int main(int argc, char const *argv[]){

    setvbuf(stdout, NULL, _IONBF, 0);

    int fd, aux;
    char input[INPUT_MAX_SIZE];
    char command[COMMAND_MAX_SIZE];

    processRequest(argv[FIRST_TASK],command,COMMAND_MAX_SIZE);
    
    sem_t *semAdress;
    if((semAdress = sem_open(argv[SEM],O_RDWR)) == SEM_FAILED){
        perror("Error creating semaphore");
    }

    while(1){
        if( (fd = open(argv[FIFO], O_RDONLY)) == -1 ){
            perror("Error open hijo");
            exit(EXIT_FAILURE);
        }

        if((aux=read(fd, input, INPUT_MAX_SIZE)) == -1){
            perror("FALLO EL READ");
            exit(EXIT_FAILURE);
        }
        else if (aux){
                if(aux > 0 && aux < INPUT_MAX_SIZE)
                    input[aux] = 0;
        }
        sem_post(semAdress);
        close(fd);
        
        processRequest(input,command,COMMAND_MAX_SIZE);
    }   
    return 0;
}

int processRequest(const char * input, char * buffer, size_t size){

    sprintf(buffer, "minisat %s | grep -o -e \"Number of .*[0-9]\\+\" -e \"CPU time.*\" -e \".*SATISFIABLE\"", input);

    FILE* fp = popen(buffer, "r"); //Validar

    size_t commandLen = fread(buffer, sizeof(char), size, fp); //Validar que sea EOF o error.
    buffer[commandLen] = 0;

    printf("File name:  %s\n%sID of slave who processed it:  %d", input, buffer, getpid());

    return 0;
}
