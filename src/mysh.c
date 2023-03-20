#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_SIZE 1024

//command line
typedef struct Command{
    char **line;
    int size;
}Command;

//holds the lines for batchmode
Command* commands;

//intialize the original 
void intializeLines(){
    commands = malloc(sizeof(Command));
    commands->size = 0;
    commands->line = NULL;
}


//optional for now(used to for both)
typedef struct token{
    char* data;
    struct token* next;
}token;


void insertLine(char* line){
    //there was no line 
    if(commands->size == 0){
        commands->size ++;
        commands->line = (char**)malloc(commands->size *sizeof(char*));
        commands->line[0] = (char*)malloc((strlen(line)+1) * sizeof(char));
        strcpy(commands->line[0], line);
    }
    else{
        commands->size++;
        commands->line = (char**)realloc(commands->line,commands->size *sizeof(char*));
        commands->line[commands->size - 1] = (char*)malloc((strlen(line)+1) * sizeof(char));
        strcpy(commands->line[commands->size - 1], line);
    }
}

void freeCommands(){
    for(int i = 0;i < commands->size;i++){
        free(commands->line[i]);
    }
    free(commands->line);
}


//takes line or something else and tokenizes each of the strings into something readable
void tokenize(char* line){

}


void execute(token* com){
    
}

int main(int argc, char **argv){
    //batch mode
    if(argc == 2){

        intializeLines();

        FILE *f = fopen(argv[1],"r");
        
        //checks in the file did not open correcly
        if(f == NULL){
            printf("File does not exist\n");
            return EXIT_FAILURE;
        }
        
        //grabs each line from file
        char line[MAX_INPUT_SIZE];

        //get command
        while(fgets(line,MAX_INPUT_SIZE,f)){
            //inserts each line into a linked list of characters 
            insertLine(line);
        }

        fclose(f);

    }

    //interactive mode
    else{
        char *line;
        printf("Welcome to our shell:\n");
        do{
            printf("\n>");
            
            scanf("%s",line);

        }while(!strcmp(line,"exit"));


    }
    freeCommands();

    return EXIT_SUCCESS;
}