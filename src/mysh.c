#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include "arraylist.h"

#ifndef BUFSIZE
#define BUFSIZE 512
#endif

typedef struct Exec {
    char *path;
    char **args;
    char *input;
    char *output;
    int argsAmnt;
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

//bare names
char bareNames[6][15] = {
     "/usr/local/sbin",
     "/usr/local/bin",
     "/usr/sbin",
     "/usr/bin", 
     "/sbin",
     "/bin"
};

//Tokenizing
void tokenize(void);
void addToken(char tok[],int size);
void freeTokens();
void initToken();

//Parsing and Running Executables
int createExecutables();
void addExec(Exec *cur, int numOfArgs, int *maxExecs);
void freeExecs();

//Other
int processLine();
int executeLine();
void append(char *, int);

//features
int cd(Exec* command);
int pwd(Exec* command);
int echo(Exec* command);
int findBareName(Exec* command);

//returns index of where the file is located if not negative
int findBareName(Exec* command){
    // int found = 0;
    // for(int i = 0; i < 6; i++){
    //     stat(bareNames[i],);
    //     if(found > -1){
    //         return found;
    //     }
    // }
    // return -1;
    return 0;
}

int main(int argc, char **argv) 
{
    
	totalChar = 0;

    int fin, bytes, pos, lstart;
    char buffer[BUFSIZE];

    // open specified file or read from stdin
    if (argc > 1) {
        fin = open(argv[1], O_RDONLY);
        if (fin == -1) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
    } 
    else { fin = 0; }

    // remind user if they are running in interactive mode
    if (isatty(fin)) {
	    fputs("[Reading from terminal]\n", stderr);
        write(0,"mysh> ",7);
    }

    // set up storage for the current line
    lineBuffer = malloc(BUFSIZE);
    lineSize = BUFSIZE;
    linePos = 0;
    int execLineStatus;

    // read input
    while ((bytes = read(fin, buffer, BUFSIZE)) > 0) {

        totalChar = totalChar + bytes;

        lstart = 0;

        // search for newlines
        for (pos = 0; pos < bytes; ++pos) {

            if (buffer[pos] == '\n') {
                int thisLen = pos - lstart + 1;

                append(buffer + lstart, thisLen);
                execLineStatus = processLine();
                if(execLineStatus == EXIT_SUCCESS) {
                    write(0,"mysh> ",7);
                } else {
                    write(0,"!mysh> ",8);
                }
                linePos = 0;
                lstart = pos + 1;
            }
        }

        if (lstart < bytes) {
            // partial line at the end of the buffer
            int thisLen = pos - lstart;
            append(buffer + lstart, thisLen);
        }

    }

    //handles if in batch does not have a new line
    if (linePos > 0) {
        // file ended with partial line
        append("\n", 1);

        execLineStatus = processLine();
        if(execLineStatus == EXIT_SUCCESS) {
            write(0,"mysh> ",7);
        } else {
            write(0,"!mysh> ",8);
        }
        
    }

    free(lineBuffer);
    close(fin);

    return EXIT_SUCCESS;
}

void printExecs() 
{
    printf("------------------------\n");
    printf("Printing Executables\n");
    printf("------------------------\n");
    for(int i = 0; i < numOfExecs; i++) {
        printf("Exec %d\n", i);
        Exec *exec = execs[i];
        printf("Path: %s\n", exec->path);
        printf("Input: %s\n", exec->input);
        printf("Output: %s\n", exec->output);
        printf("Num of Args: %d\n", exec->argsAmnt);
        printf("Args:");
        for(int arg = 0; arg < exec->argsAmnt; arg++) {
            printf(" %s", exec->args[arg]);
        }
        printf("\n------------------------\n");
        
    }
}

void printTokens() 
{
    //print each token
    printf("Num of Tokens: %d\n", numOfTokens);
	printf("Tokens: \n");
    printf("------------------------\n");
	for(int i = 0;i < numOfTokens;i++){
        if(strcmp(tokens[i], "\n") == 0) {
            printf("NEWLINE\n");
        } 
        else { printf("%s\n",tokens[i]); }
	}
    printf("------------------------\n");
    printf("Done printing tokens\n");
    printf("------------------------\n");
}

int processLine() 
{

    tokenize();

    printTokens();

    int createExecExit = createExecutables();
    if(createExecExit == EXIT_FAILURE) {
        printf("ERROR!!!\n");
        return EXIT_FAILURE;
    }
    
    printExecs();

    int execLineExit = executeLine();
    if(execLineExit == EXIT_FAILURE) {
        printf("Invalid Command Line Arguments\n");
        return EXIT_FAILURE;
    }

    freeExecs();
    freeTokens();

    return EXIT_SUCCESS;
}

int executeLine() 
{
    return EXIT_SUCCESS;
}

//Creating Commands

//change directory
int cd(Exec* command)
{
    if(chdir(command->path) == 0){
        return 0;
    }
    else{
        perror("Directory Not Found");
        return 1;
    }
}

// int pwd(Exec* command){
//     char* wd = malloc(BUFSIZ * sizeof(char));
//     int file;
//     getcwd(wd,0);
//     if(command->output != NULL){
//         file =  open(command->output,O_WRONLY);
//         if(file >= 0){
//         write(file, wd,strlen(wd));
//        }
//        else{
//         perror("File does not exist");
//         return 1;
//        }
//     }
//     else{
//         write(0,wd,strlen(wd));
//     }
//     close(file);
//     free(wd);
//     return 0;
// }

int PushingP(Exec* A,Exec* B)
{   
    int fds[2];
    pipe(fds);

    int pid = fork();
    if (pid == 0) {
        // in the child, use dup2 to reset stdout
    
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
    
        // execv();
    }

    close(fds[1]);
    int bytes;
    char buffer[BUFSIZE];

    // read from fds[0] until we reach EOF
    while ((bytes = read(fds[0], buffer, BUFSIZE)) > 0) {
        //...
    }
    return 0;
}

int createExecutables() 
{   
    execs = malloc(sizeof(Exec *));
    numOfExecs = 0;
    
    int numOfArgs = 0;
    int maxExecs = 1;
    int maxArgs = 1;

    Exec *cur = malloc(sizeof(Exec));
    cur->path = NULL;

    for(int i = 0; i < numOfTokens; i++) {
        
        if(strcmp(tokens[i], "|") == 0 || strcmp(tokens[i], "||") == 0 || strcmp(tokens[i], "&&") == 0) 
        {
            if(strcmp(tokens[i+1], "|") == 0 || strcmp(tokens[i+1], "||") == 0 
                || strcmp(tokens[i+1], "&&") == 0 || strcmp(tokens[i+1], "\n") == 0) 
            {
                //Next token is a special-token
                return EXIT_FAILURE;
            }

            if(cur->path) 
            {
                //Store current executable
                addExec(cur, numOfArgs, &maxExecs);
            
                //Create and store new executable for the special-token
                cur = malloc(sizeof(Exec));
            } 
            else 
            {
                //No path before this
                return EXIT_FAILURE;
            }

            //Populate special Token
            cur->path = tokens[i];
            cur->input = NULL;
            cur->output = NULL;
            cur->args = malloc(sizeof(char *));
            numOfArgs = 0;
            printf("Added Special-Token: %s\n", cur->path);
            addExec(cur, numOfArgs, &maxExecs);

            //Make a new current executable
            cur = malloc(sizeof(Exec));
            cur->path = NULL;
            numOfArgs = 0;
        }
        else if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0) 
        {
            //Check if command given
            if(!cur->path) 
            {
                printf("No command given before redirection\n");
                return EXIT_FAILURE;
            }

            if(strcmp(tokens[i + 1], "\n") == 0 || strcmp(tokens[i + 1], "|") == 0 
                || strcmp(tokens[i + 1], "<") == 0 || strcmp(tokens[i + 1], ">") == 0) 
            {
                //Next token is not an argument
                return EXIT_FAILURE;
            } 
            else {
                //Next token is an argument
                if(strcmp(tokens[i], "<") == 0) {
                    cur->input = tokens[i+1];
                    printf("Input set to: %s\n", tokens[i+1]);
                } else {
                    cur->output = tokens[i+1];
                    printf("Output set to: %s\n", tokens[i+1]);
                }
                i++;
            }
        }
        else if (strcmp(tokens[i], "\n") == 0) 
        {
            if(!cur->path) 
            {
                //Current executable has no path, happens right after |, ||, or &&
                printf("Freeeeeing\n");
                free(cur);
            } else {
                addExec(cur, numOfArgs, &maxExecs);
                printf("Finished Creating Executable\n");
            }
        } 
        else if(!cur->path) 
        {
            cur->path = tokens[i];
            cur->args = malloc(2 * sizeof(char *));
            cur->input = NULL;
            cur->output = NULL;
            printf("Setpath to: %s\n", cur->path);
        } 
        else {
            //If it got here, it's considered an argument
            if(numOfArgs + 1 > maxArgs) 
            {
                maxArgs += 1;
                cur->args = realloc(cur->args, maxArgs * sizeof(char*));
                printf("Realloc from %d to %d args\n", numOfArgs, maxArgs);
            }
            cur->args[numOfArgs] = tokens[i];
            printf("Added Argument: %s\n", cur->args[numOfArgs]);
            numOfArgs++;
        }
        
    }
    return EXIT_SUCCESS;
}

void addExec(Exec *cur, int numOfArgs, int *maxExecs) 
{
    if(numOfExecs == *maxExecs) 
    {
        *maxExecs += 1;
        execs = realloc(execs, *maxExecs * sizeof(Exec *));
    }
    cur->argsAmnt = numOfArgs;
    execs[numOfExecs] = cur;
    numOfExecs++;
}

void freeExecs() 
{
    for(int i = 0; i < numOfExecs; i++) 
    {
        Exec *cur = execs[i];
        printf("Attempting to Free: %p\n", cur);
        free(cur);
    } 
    free(execs);
}

// requires: 
// - linePos is the length of the line in lineBuffer
// - linePos is at least 1
// - final character of current line is '\n'
void tokenize(void)
{

    initToken();

    int l = 0, r = linePos;
    if(lineBuffer[0] == '\n') {
        
    }

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
		if(lineBuffer[l] != ' ' && lineBuffer[l] != '\n') {
			//if it is start of token3
			if(tokenFound == 0 && !(lineBuffer[l] == '<' || lineBuffer[l] == '>' || lineBuffer[l] == '|')) {
				tokenFound = 1;
				size++;
				tokIndex = 0;
				startOfToken[tokIndex] = lineBuffer[l];
				tokIndex++;
			}
			//if we are adding a redirection or pipe that is next to space
			else if ((lineBuffer[l] == '<' || lineBuffer[l] == '>' || lineBuffer[l] == '|') && !tokenFound) {
                if(lineBuffer[l + 1] == '|') {    
                    startOfToken[0] = lineBuffer[l];
                    l++;
                    startOfToken[1] = lineBuffer[l];
                    startOfToken[2] = '\0';
                    addToken(startOfToken,3);
                    memset(startOfToken, 0, sizeof(startOfToken));
                }
                else {
                    startOfToken[0] = lineBuffer[l];
                    startOfToken[1] = '\0';
                    addToken(startOfToken,2);
                    memset(startOfToken, 0, sizeof(startOfToken));

                }
			}
			//if we are adding a redirection or pipe that is not next to space
			else if ((lineBuffer[l] == '<' || lineBuffer[l] == '>' || lineBuffer[l] == '|') && tokenFound) {
				//place token before the special character
                if(lineBuffer[l + 1] == '|') {
                    size++;
				    startOfToken[tokIndex + 1] = '\0';
				    addToken(startOfToken,size + 1);
				    memset(startOfToken, 0, sizeof(startOfToken));
				    size = 0;
				    tokenFound = 0;
				    //place the special character
				    startOfToken[0] = lineBuffer[l];
                    l++;
                    startOfToken[1] = lineBuffer[l];
				    startOfToken[2] = '\0';
				    addToken(startOfToken,3);
				    memset(startOfToken, 0, sizeof(startOfToken));
                }
                else {
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
			}
            
			//else updating the size of token
			else {
				startOfToken[tokIndex] = lineBuffer[l];
				tokIndex++;
				size++;
			}
		}
		//going to be a space and reset
		else if(lineBuffer[l] == ' ') {
			if(size > 0) {
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
		if(l == r && size > 0) {
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
		else if(l == r && size == 0) {
				startOfToken[0] = '\n';
				startOfToken[1] = '\0';
				addToken(startOfToken,2);
				memset(startOfToken, 0, sizeof(startOfToken));
		}
	}
}

void addToken(char tok[],int size) 
{
	numOfTokens++;
    printf("Address of Tokens %p\n", tokens);
	if(tokens == NULL) {
		tokens = (char**)malloc(numOfTokens * sizeof(char*));
		sizeOfToken = (int*)malloc(numOfTokens * sizeof(int));
		tokens[0] = malloc(size *sizeof(char));
		strcpy(tokens[0],tok);
		sizeOfToken[0] = size;
	}
	else {
		tokens = realloc(tokens,numOfTokens * sizeof(char*));
		sizeOfToken = realloc(sizeOfToken,numOfTokens * sizeof(int));

		tokens[numOfTokens - 1] = malloc(size * sizeof(char));
		strcpy(tokens[numOfTokens - 1],tok);
		sizeOfToken[numOfTokens - 1] = size;
	}
}

void freeTokens() 
{
	for(int i = 0; i < numOfTokens; i++) 
    {
		free(tokens[i]);
	}
	free(sizeOfToken);
}

void initToken()
{
	numOfTokens = 0;
	tokens = NULL;
	sizeOfToken = NULL;
}

void append(char *buf, int len)
{
    int newPos = linePos + len;
    
    if (newPos > lineSize) {

        lineSize *= 2;

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