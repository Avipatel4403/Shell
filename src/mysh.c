#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#ifndef DEBUG
#define DEBUG 0
#endif
#ifndef BUFSIZE
#define BUFSIZE 512
#endif


typedef struct Token{
    char* token;
    int size;
    struct Token* next;
}Token;


Token *head;

void initToken(){
    head = NULL;
}


void addToken(char* tok, int size){
    if(head == NULL){
        head = malloc(sizeof(Token));
        //needs to change so we malloc head.token for the size given and copy
        head->token = tok;
        head->size = size;
        head->next = NULL;
    }
    else{
        Token* ptr = head;
        while(ptr != NULL){
            ptr = ptr->next;
        }
        ptr = malloc(sizeof(Token));


        ptr->token = tok;
        ptr->size = size;
        ptr->next = NULL;
    }
}


void freeToken(){
    while(head != NULL){
        Token *ptr = head;
        head = head->next;
        free(ptr->token);
        free(ptr);
    }
}


void execute(char* line){
  
}

int main(int argc, char **argv){
    int fin, bytes, lstart;
    char buffer[BUFSIZE];

    // open specified file or read from stdin
    if (argc > 1){
        fin = open(argv[1], O_RDONLY);
        if (fin == -1) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
    } else {
        fin = 0;
    }

    // remind user if they are running in interactive mode
    if (isatty(fin)) {
        fputs("WELCOME TO OUR SHELL\n", stderr);
    }

    /*
    if space create a new pointer
    if \n add as a new token to signify as new line
    every | < > will result in its own token
    
    */


   initToken();

        // read input
        while ((bytes = read(fin, buffer, BUFSIZE)) > 0) {
            //start of token
            int start = 0;
            //end of token
            int last = 0;
            //size of token
            int size = 0;
            //will send token into addToken()
            char newToken[BUFSIZE];
            
            //goes through what has been read in(stored in buffer)
            for (int i = 0; i < bytes; i++) {
                //if space and size > 0 add
                if(buffer[i] == ' ' && size > 0){

                }
                //if new line insert new line as a token
                else if(buffer[i] == '\n'){

                }
                //if | < > then make sure its not in between two spaces
                else if(buffer[i] == '<' || buffer[i] == '>' || buffer[i] == '<'){
                    //check if it was not connected to any other characters
                    //just insert the single < > | 
                    if(size == 0){

                    }
                    //we need to split the string that is an object like foo and add to token along with the special character
                    else{

                    }

                }
                //else just increment size and insert into new token
                else{
                    size++;
                    newToken[size - 1] = buffer[i];
                }

            }

        }



    freeToken();
    close(fin);
    return EXIT_SUCCESS;
}