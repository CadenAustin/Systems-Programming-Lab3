#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct{
    int free;
    int pipeFDS[2];
} PipeDictionary;

void print_usage(){
    fprintf(stdout, "Usage: runpar NUMCORES COMMAND... _files_ FILE...\n");
    return;
}


void run_command_in_subprocess(char *file, int writeFD){
    printf("Hello, %s %d\n", file, writeFD);
    return;
}

void printout_terminated_subprocess(int status){
    
}

int get_free_pipe(PipeDictionary *pipeManagment, int size){
    for (int x = 0; x < size; x++){
        if (pipeManagment[x].free == 0){
            return x;
        }
    }
    return -1;
}

int main(int argc, char *argv[]){
    char *command[argc];
    int commandIndex = 0, argIndex = 1, numberOfCores = 0;

    if (argc <= 4){
        print_usage();
        exit(EXIT_FAILURE);
    }
    numberOfCores = atoi(argv[1]);
    if (numberOfCores <= 0){
        print_usage();
        exit(EXIT_FAILURE);
    }

    while(strncmp(argv[++argIndex], "_files_", 7)){
        if (argc-1 <= argIndex){
            print_usage();
            exit(EXIT_FAILURE);
        }
        command[commandIndex] = argv[argIndex];
        commandIndex++;
    }
     
    command[commandIndex] = "\0";
    
    PipeDictionary pipeManagment[numberOfCores];
    for (int x = 0; x < numberOfCores; x++){
        pipeManagment[x].free = 0;
         
    }


    for (int fileIndex = argIndex+1; fileIndex < argc; fileIndex++){
        
        printf("Testing"); 
        int pipeIndex = get_free_pipe(pipeManagment, numberOfCores);
    
        if (pipeIndex == -1){
            printf("waiting");
            exit(EXIT_SUCCESS);
        }
        
        printf("%d", pipeIndex);
        pipe(pipeManagment[pipeIndex].pipeFDS);
        pipeManagment[pipeIndex].free = 1;
        
        run_command_in_subprocess(argv[fileIndex], pipeManagment[pipeIndex].pipeFDS[1]);
    }
    
}
