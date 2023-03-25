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


char *lineBuffer;
int linePos, lineSize,totalChar;

char** tokens;
int *sizeOfToken;
int numOfTokens;

//for token
void append(char *, int);
void initToken();
void Tokenize(void);
void addToken(char tok[],int size);
void freeToken();

//parse


//execute
typedef struct Exec {
	char *path;
	char *input;
	char *output;
	char **args;
} Exec;

//functions
Exec* cd;
Exec* pwd;
Exec* echo;
//&&
Exec* twoAND;
//||
Exec* twoVert;
// |
Exec* PushingP;



int main(int argc, char **argv){
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
    } else {
	fin = 0;
    }
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
		totalChar = totalChar + bytes;
	if (DEBUG) fprintf(stderr, "read %d bytes\n", bytes);

	// search for newlines
	lstart = 0;
	for (pos = 0; pos < bytes; ++pos) {
	    if (buffer[pos] == '\n') {
		int thisLen = pos - lstart + 1;
		if (DEBUG) fprintf(stderr, "finished line %d+%d bytes\n", linePos, thisLen);

		append(buffer + lstart, thisLen);
		Tokenize();
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
	Tokenize();
    }

	//print each token
	// printf("Tokens: \n");
	// for(int i = 0;i < numOfTokens;i++){
	// 	printf("%s",tokens[i]);
	// }

	for(int i = 0;i < numOfTokens;i++){
        if(strcmp(tokens[i], "\n") == 0) {
            printf("NEWLINE\n");
        } else { printf("%s\n",tokens[i]); }
    }



    free(lineBuffer);
	freeToken();
    close(fin);

    return EXIT_SUCCESS;
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

// print the contents of crntLine in reverse order
// requires: 
// - linePos is the length of the line in lineBuffer
// - linePos is at least 1
// - final character of current line is '\n'

void initToken(){
	numOfTokens = 0;
	tokens = NULL;
	sizeOfToken = NULL;
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



void Tokenize(void){
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
			if(tokenFound == 0 && !(lineBuffer[l] == '<' || lineBuffer[l] == '>' || lineBuffer[l] == '|')){
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

				//add new line
				startOfToken[0] = '\n';
				startOfToken[1] = '\0';
				addToken(startOfToken,2);
				memset(startOfToken, 0, sizeof(startOfToken));
		}
		else if(l == r && size == 0){
				startOfToken[0] = '\n';
				startOfToken[1] = '\0';
				addToken(startOfToken,2);
				memset(startOfToken, 0, sizeof(startOfToken));
		}
	}
}

void freeToken(){
	for(int i = 0; i < numOfTokens;i++){
		free(tokens[i]);
	}
	free(tokens);
	free(sizeOfToken);
}