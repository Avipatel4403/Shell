#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include "arraylist.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef BUFSIZE
#define BUFSIZE 512
#endif

typedef struct Exec {
    char *path;
    char *input;
    char *output;
    char **args;
} Exec;

//Input loop
char *lineBuffer;
int linePos, lineSize,totalChar;

//Tokenization
char **tokens;
int *sizeOfToken;
int numOfTokens;

//Parsing
Exec **execs;
int numOfExecs;


void processLine();
void createExecutions();
void tokenize(void);
void addToken(char tok[],int size);
void freeToken();
void initToken();
void append(char *, int);




int main(int argc, char **argv) {
    
	totalChar = 0;

    int fin, bytes, pos, lstart;
    char buffer[BUFSIZE];

	initToken();

    // open specified file or read from stdin
    if (argc > 1) {
	fin = open(argv[1], O_RDONLY);
	if (fin == -1) {
	    perror(argv[1]);
	    exit(EXIT_FAILURE);
	}
    } else { fin = 0; }

    if (isatty(fin)) {
        // remind user if they are running in interactive mode
	fputs("[Reading from terminal]\n", stderr);
    }

    // set up storage for the current line
    lineBuffer = malloc(BUFSIZE);
    lineSize = BUFSIZE;
    linePos = 0;

    // read input
    while ((bytes = read(fin, buffer, BUFSIZE)) > 0) {

        totalChar = totalChar + bytes;

        if (DEBUG) fprintf(stderr, "read %d bytes\n", bytes);

        lstart = 0;

        // search for newlines
        for (pos = 0; pos < bytes; ++pos) {

            if (buffer[pos] == '\n') {
                int thisLen = pos - lstart + 1;
                if (DEBUG) fprintf(stderr, "finished line %d+%d bytes\n", linePos, thisLen);

                append(buffer + lstart, thisLen);
                processLine();
                linePos = 0;
                lstart = pos + 1;
            }
        }

        if (lstart < bytes) {
            // partial line at the end of the buffer
            int thisLen = pos - lstart;
            if (DEBUG) fprintf(stderr, "partial line %d+%d bytes\n", linePos, thisLen);
            append(buffer + lstart, thisLen);
        }
    }

    //handles if in batch does not have a new line
    if (linePos > 0) {
        // file ended with partial line
        append("\n", 1);
        processLine();
    }

	printf("\nTotal Char: %d \n",totalChar);

    free(lineBuffer);
	freeToken();
    close(fin);

    return EXIT_SUCCESS;
}

void processLine() {

    tokenize();

    //print each token
    printf("Num of Tokens: %d\n", numOfTokens);
	printf("Tokens: \n");
	for(int i = 0;i < numOfTokens;i++){
        if(strcmp(tokens[i], "\n")) {
            printf("NEWLINE\n");
        } else { printf("%s\n",tokens[i]); }
	}
    printf("Done printing tokens\n");

    createExecutions();

    printf("%p\n", execs[0]);

}

void createExecutions() {

    execs = malloc(sizeof(Exec *));

    Exec *cur = malloc(sizeof(Exec));
    cur->path = NULL;

    int numOfArgs = 0;
    int maxExecs = 1;
    int maxArgs = 2;


    printf("%s\n", tokens[1]);
    for(int i = 0; i < numOfTokens; i++) {

        if(!cur->path) {
            cur->path = tokens[i];
            cur->args = malloc(sizeof(char *));
            addExec(cur);
        }

        // else if (strcmp(tokens[i], "<") == 0) {
        //     if(strcmp(tokens[i+1], "\n") != 0) {
        //         cur->input = tokens[i+1];
        //         i++;
        //     }
        // }

        // else if (strcmp(!tokens[i], ">") == 0) {
        //     if(strcmp(tokens[i+1], "\n") != 0) {
        //         cur->output = tokens[i+1];
        //         i++;
        //     }
        // } 
        
        else if (strcmp(tokens[i], "\n") == 0) {
            execs = &cur;
        } 
        
        else {
            if(numOfArgs == maxArgs) {
                maxArgs *= 2;
                realloc(cur->args, maxArgs);
            }
            cur->args[numOfArgs] = tokens[1]
            printf("Added Arg\n");
        }
        
    }
}

void addExec(Exec *cur) {

}

// print the contents of crntLine in reverse order
// requires: 
// - linePos is the length of the line in lineBuffer
// - linePos is at least 1
// - final character of current line is '\n'
void tokenize(void){
    int l = 0, r = linePos;

    assert(lineBuffer[linePos-1] == '\n');

	//keeps track of if we are in a token or not
	int tokenFound = 0;
	//size of token
	int size = 0;
	//keeps track of the index of the tok
	int tokIndex = 0;
	//token
	char startOfToken[BUFSIZE];

    //iterates through line
    while (l < r) {
		//token
		if(lineBuffer[l] != ' '){
			//if it is start of token3
			if(tokenFound == 0){
				tokenFound = 1;
				size++;
				tokIndex = 0;
				startOfToken[tokIndex] = lineBuffer[l];
				tokIndex++;
			}
			//if we are adding a redirection or pipe that is next to space
			else if((lineBuffer[l] == '<' || lineBuffer[l] == '>' || lineBuffer[l] == '|') && !tokenFound){
				startOfToken[0] = lineBuffer[l];
				startOfToken[1] = '\0';
				addToken(startOfToken,2);
				memset(startOfToken, 0, sizeof(startOfToken));
			}
			//if we are adding a redirection or pipe that is not next to space
			else if((lineBuffer[l] == '<' || lineBuffer[l] == '>' || lineBuffer[l] == '|') && tokenFound){
				//place token before the special character
				size++;
				startOfToken[tokIndex + 1] = '\0';
				addToken(startOfToken,size + 1);
				memset(startOfToken, 0, sizeof(startOfToken));
				size = 0;
				tokenFound = 0;
				//place the special character
				startOfToken[0] = lineBuffer[l];
				startOfToken[1] = '\0';
				addToken(startOfToken,2);
				memset(startOfToken, 0, sizeof(startOfToken));
			}
			//else updating the size of token
			else{
				startOfToken[tokIndex] = lineBuffer[l];
				tokIndex++;
				size++;
			}
		}
		//going to be a space and reset
		else if(lineBuffer[l] == ' '){
			if(size > 0){
				size++;
				startOfToken[tokIndex+1] = '\0';
				addToken(startOfToken,size + 1);
				memset(startOfToken, 0, sizeof(startOfToken));
				tokIndex = 0;
				size = 0;
				tokenFound = 0;
			}
		}

		l++;
		if(l == r && size > 0){
				size++;
				startOfToken[tokIndex+1] = '\0';
				addToken(startOfToken,size + 1);
				memset(startOfToken, 0, sizeof(startOfToken));
				tokIndex = 0;
				size = 0;
				tokenFound = 0;
		}
	}
}

void addToken(char tok[],int size){
	numOfTokens++;
	if(tokens == NULL){
		tokens = (char**)malloc(numOfTokens * sizeof(char*));
		sizeOfToken = (int*)malloc(numOfTokens * sizeof(int));
		tokens[0] = malloc(size *sizeof(char));
		strcpy(tokens[0],tok);
		sizeOfToken[0] = size;
	}
	else{
		tokens = realloc(tokens,numOfTokens * sizeof(char*));
		sizeOfToken = realloc(sizeOfToken,numOfTokens * sizeof(int));

		tokens[numOfTokens - 1] = malloc(size * sizeof(char));
		strcpy(tokens[numOfTokens - 1],tok);
		sizeOfToken[numOfTokens - 1] = size;
	}
}


void freeToken(){
	for(int i = 0; i < numOfTokens;i++){
			free(tokens[i]);
	}
	free(tokens);
	free(sizeOfToken);
}

void initToken(){
	numOfTokens = 0;
	tokens = NULL;
	sizeOfToken = NULL;
	
}

// add specified text the line buffer, expanding as necessary
// assumes we are adding at least one byte
void append(char *buf, int len)
{
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