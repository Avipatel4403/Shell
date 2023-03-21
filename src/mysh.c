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

char *lineBuffer;
int linePos, lineSize;

// add specified text the line buffer, expanding as necessary
// assumes we are adding at least one byte
void append(char *buf, int len){
    int newPos = linePos + len;
    if (newPos > lineSize) {
        lineSize *= 2;
        if (DEBUG) fprintf(stderr, "expanding line buffer to %d\n", lineSize);
        assert(lineSize >= newPos);
        lineBuffer = realloc(lineBuffer, lineSize);
        if (lineBuffer == NULL) {
            perror("line buffer");
            exit(EXIT_FAILURE);
        }
    }
    memcpy(lineBuffer + linePos, buf, len);
    linePos = newPos;
}

//optional for now(used to for both)
typedef struct token{
    char** data;
    int size;
}token;

//
token *tokens;

void intializeTokens(){
    tokens = malloc(sizeof(token));
    tokens->data = NULL;
    tokens->size = 0;

}

//takes line or something else and tokenizes each of the strings into something readable
void tokenize(){
    int sizeOfToken = 0;
    int last = 0;

    intializeTokens();

    //traverse through the line of code
    for(int i = 0;i < BUFSIZE;i++){

        //if we hit a space
        if(lineBuffer[i] == ' '){

            if(tokens->size == 0){
                tokens->size++;
                tokens->data = (char **)malloc(tokens->size * sizeof(char *));
            }
            else{

            }

        }
        else if(lineBuffer[i] == '|' || lineBuffer[i] == '<' || lineBuffer[i] == '>'){

        }
        else{
            sizeOfToken++;
        }

    }

}

void execute(char* line){
  
}

int main(int argc, char **argv){
    int fin, bytes, pos, lstart;
    char buffer[BUFSIZE];
    //checks if arg has a batchmode
    if (argc > 1) {
        fin = open(argv[1], O_RDONLY);
        if (fin == -1) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
    }

    //goes into interactive mode
    } else {
        fin = 0;
    }

    printf("Welcome to my Shell\n");
    // remind user if they are running in interactive mode
    if (isatty(fin)) {
        fputs("[Reading from terminal]\n", stderr);
    }

    // set up storage for the current line
    lineBuffer = malloc(BUFSIZE);
    lineSize = BUFSIZE;
    linePos = 0;

    // read input
    while ((bytes = read(fin, buffer, BUFSIZE)) > 0) {
        if (DEBUG) {
            fprintf(stderr, "read %d bytes\n", bytes);
        }
        // search for newlines
        lstart = 0;
        for (pos = 0; pos < bytes; ++pos) {
            if (buffer[pos] == '\n') {
                int thisLen = pos - lstart + 1;
                if (DEBUG) fprintf(stderr, "finished line %d+%d bytes\n", linePos,thisLen);
                append(buffer + lstart, thisLen);
                linePos = 0;
                lstart = pos + 1;
            }
        }
        if (lstart < bytes) {
            // partial line at the end of the buffer
            int thisLen = pos - lstart;
            if (DEBUG) fprintf(stderr, "partial line %d+%d bytes\n", linePos,thisLen);
            append(buffer + lstart, thisLen);
        }

        //prints character
        for(int i = 0; i < BUFSIZE;i++){
            printf("%c",buffer[i]);
            
        }
        printf("\n");
        tokenize();
    }


    free(lineBuffer);
    close(fin);
    return EXIT_SUCCESS;
}