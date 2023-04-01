#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <glob.h>
#include <sys/wait.h>
#include <sys/stat.h>


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

int hasExited;

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
char* bareNames[6] = {
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
int runExec(Exec *exec);
int createExecutables();
void addExec(Exec *cur, int numOfArgs, int *maxExecs);
void freeExecs();
int PushingP(Exec* A,Exec* B);

//Other
int processLine();
int executeLine();
void append(char *, int);

//features
int cd(Exec* command);
int pwd(Exec* command);
int echo(Exec* command);



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
                if(hasExited) {
                    printf("Exiting\n");
                    goto exit;
                }
                if(argc == 1)
                {
                    if(execLineStatus == EXIT_SUCCESS) {
                        write(0,"mysh> ",7);
                    } else {
                        write(0,"!mysh> ",8);
                    }   
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
    }
    free(lineBuffer);
    close(fin);
    exit:
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
        else { printf("%s %ld\n",tokens[i],strlen(tokens[i])); }
	}
    printf("------------------------\n");
    printf("Done printing tokens\n");
    printf("------------------------\n");
}

int processLine() 
{
    hasExited = 0;

    tokenize();

    // printTokens();

    int createExecExit = createExecutables();
    if(createExecExit == EXIT_FAILURE) {
        freeExecs();
        freeTokens();
        return EXIT_FAILURE;
    }
    
    // printExecs();

    int execLineExit = executeLine();
    if(execLineExit == EXIT_FAILURE) {
        freeExecs();
        freeTokens();
        printf("False\n");
        return EXIT_FAILURE;
    }

    freeExecs();
    freeTokens();

    return EXIT_SUCCESS;
}

int executeLine() 
{ 
    if(numOfExecs == 1) {

        if(strcmp(execs[0]->path, "\n") == 0) {
            return EXIT_SUCCESS;
        }

        return runExec(execs[0]);
    }

    int curExec = 0;
    int lastRunStatus = EXIT_SUCCESS;
    int hasPiped = 0;
    int hasOROR = 0;
    int hasANDAND = 0;

    while(curExec < numOfExecs) 
    {   
        if(curExec + 1 >= numOfExecs) 
        {
            lastRunStatus = runExec(execs[curExec]);
            curExec++;
        }
        else if(strcmp(execs[curExec+1]->path, "|") == 0) 
        {   
            if(hasPiped) {
                printf("Cannot pipe more than once\n");
                return EXIT_FAILURE;
            }
            hasPiped = 1;
            lastRunStatus = PushingP(execs[curExec], execs[curExec+2]); //This should be pipe function
            curExec += 3;
        }
        else if(strcmp(execs[curExec+1]->path, "||") == 0) 
        {
            if(hasOROR) {
                
                return EXIT_FAILURE;
            }    
            hasOROR = 1;
            lastRunStatus = runExec(execs[curExec]);
            if(lastRunStatus == EXIT_FAILURE) {
                curExec += 2;
            } else {
                lastRunStatus = EXIT_SUCCESS;
                if(curExec + 3 >= numOfExecs || !(strcmp(execs[curExec+3]->path, "|") == 0)) {
                    curExec += 3;
                } else {
                    if(curExec + 5 >= numOfExecs) {
                        return lastRunStatus;
                    }
                    if(strcmp(execs[curExec+5]->path, "&&") == 0) {
                        curExec += 7;
                    } else {
                        curExec += 5;
                    }
                }
            }
        }
        else if(strcmp(execs[curExec+1]->path, "&&") == 0) 
        {
            if(hasANDAND) {
                printf("Multiple ANDAND's\n");
                return EXIT_FAILURE;
            }
            hasANDAND = 1;

            lastRunStatus = runExec(execs[curExec]);

            if(lastRunStatus == EXIT_SUCCESS) {
                curExec += 2;
            } else {
                if(curExec + 3 >= numOfExecs || !(strcmp(execs[curExec+3]->path, "|") == 0)) {
                    curExec += 3;
                } else {
                    if(curExec + 5 >= numOfExecs) {
                        return lastRunStatus;
                    }
                    if(strcmp(execs[curExec+5]->path, "||") == 0) {
                        curExec += 7;
                    } else {
                        curExec += 5;
                    }
                }
            }
        }
        else if(strcmp(execs[curExec]->path, "||") == 0) 
        {
            if(hasOROR) {
                return EXIT_FAILURE;
            } 

            if(lastRunStatus == EXIT_FAILURE) {
                curExec++;
            } else {
                curExec += 2;
            }
        }
        else if(strcmp(execs[curExec]->path, "&&") == 0) 
        {
            if(hasANDAND) {
                return EXIT_FAILURE;
            }

            if(lastRunStatus == EXIT_SUCCESS) {
                curExec++;
            } else {
                curExec += 2;
            }
        } 
        else 
        {
            printf("Cannot pipe more than once\n");
            return EXIT_FAILURE;
        }
    }
    return lastRunStatus;
}

void checkPath(char **path) {
    //check if string has *
    //go glob if has result 
    if(strchr(*path,'*') != NULL){
        glob_t result;
        glob("*.txt", 0, NULL, &result);

        if(strchr(*path,'*') != NULL){
            glob_t result;
            glob("*.txt", 0, NULL, &result);
            if(result.gl_pathc == 1){
                printf("here\n");
                path = result.gl_pathv;
            }
            else{
                return;
            }

        globfree(&result);

        }
    }


    //if current apth doesn't exist, check the others, and return new path, or return path
    struct stat file_stat;
    int found = stat(*path, &file_stat);
    char* final_path = NULL;


    if(found == -1){
        int sizeOfPath = 0;
        for(int i = 0;i < 6;i++){
            sizeOfPath = strlen(bareNames[i]) + strlen("/") + strlen(*path)+ 1;
            char fpath[sizeOfPath];
            strcpy(fpath,bareNames[i]);
            strcat(fpath,"/");
            strcat(fpath,*path);
            if(access(fpath,F_OK) == 0){
                final_path = malloc(sizeOfPath * sizeof(char));
                strcpy(final_path,fpath);
                break;
            }
        }
        if(final_path != NULL){
            *path = final_path;
        }
    }

}

void checkArgs(Exec *exec) {

    int size = exec->argsAmnt;

    //Add NULL to end of args
    exec->args = realloc(exec->args, (++size) * sizeof(char *));
    exec->args[size - 1] = NULL;

    //Loop through and check for wildcards
    int i = 0;
    glob_t globbuf;
    while(exec->args[i] != NULL) {
        if (strchr(exec->args[i], '*') != NULL) {
            glob(exec->args[i], 0, NULL, &globbuf);
            int j = globbuf.gl_pathc;
            if (j == 1) {
                exec->args[i] = globbuf.gl_pathv[0];
            } else if ( j >  1) {
                size += (j - 1);
                exec->args = realloc(exec->args, size * sizeof(char*));
                memmove(exec->args + i + j, exec->args + i + 1, (size - i - j) * sizeof(char*));
                memcpy(exec->args + i, globbuf.gl_pathv, j * sizeof(char *));
            } else {
                memmove(exec->args + i, exec->args + i + 1, (size - i - 1) * sizeof(char*));
                size--;
                exec->args = realloc(exec->args, size * sizeof(char*));
                i--;
            }
        }
        i++;
    }
    size++;
    exec->args = realloc(exec->args, size * sizeof(char*));
    memmove(exec->args + 1, exec->args, (size - 1) * sizeof(char*));
    exec->args[0] = exec->path;
}

int PushingP(Exec* A,Exec* B)
{   
    return EXIT_SUCCESS;
}

int runExec(Exec *exec) 
{
    if(strcmp(exec->path, "cd") == 0) 
    {
        return cd(exec);
    }
    else if(strcmp(exec->path, "pwd") == 0) 
    {
        return pwd(exec);
    }
    else if(strcmp(exec->path, "exit") == 0) {
        hasExited = 1;
        return EXIT_SUCCESS;
    } 

    char *path = exec->path;
    checkPath(&path);

    checkArgs(exec);
     

    //Set Standard Input
    int input = 0;
    int saved_stdin = dup(0);
    if(exec->input != NULL) {
        if(strchr(exec->input,'*') != NULL) {
            printf("Wildcards in redirection not allowed\n");
            return EXIT_FAILURE;
        }
        input = open(exec->input, O_RDONLY);
        if(input == -1) {
            perror("Error");
            return EXIT_FAILURE;
        }
        dup2(input, STDIN_FILENO);
    }

    //Set Standard Out
    int output = 1;
    if(exec->output != NULL) {
        if(strchr(exec->output,'*') != NULL) {
            printf("Wildcards in redirection not allowed\n");
            return EXIT_FAILURE;
        }
        int output = open(exec->output, O_WRONLY | O_TRUNC | O_CREAT, 0640);
        dup2(output, STDOUT_FILENO);
    }

    //Fork and Run Program
    int pid = fork();
    if (pid == -1) { return EXIT_FAILURE; }
    if(pid == 0) 
    {
        execv(path, exec->args);
        perror("Error");
        return EXIT_FAILURE;
    }

    int wstatus;
    int tpid = wait(&wstatus);

    dup2(saved_stdin, 1);
    close(saved_stdin);

    if(output != 1) {
        dup2(1, STDOUT_FILENO);
        close(output);
    }

    if(path != exec->path) {
        free(path);
    }

    if(tpid == -1 || WEXITSTATUS(wstatus) != EXIT_SUCCESS) {
        perror("Error");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int cd(Exec* command)
{
    if(command->argsAmnt == 0 || strcmp(command->args[0],"home")== 0){
        chdir(getenv("HOME"));
        return 0;
    }
    else if(chdir(command->args[0]) == 0){
        return 0;
    }
    else{
        perror("Directory Not Found");
        return 1;
    }
}

int pwd(Exec* command) {
    char wd[BUFSIZ];
    
    getcwd(wd,sizeof(wd));
    if(command->output != NULL){
        int file;
        file =  open(command->output, O_WRONLY |O_TRUNC | O_CREAT, 0640);
        write(file, wd,strlen(wd));
        write(file,"\n",1);
       close(file);
    }
    else{
        write(0,wd,strlen(wd));
        write(0,"\n",1);
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
                } else {
                    cur->output = tokens[i+1];
                }
                i++;
            }
        }
        else if (strcmp(tokens[i], "\n") == 0) 
        {
            if(!cur->path) 
            {
                //Current executable has no path, happens right after |, ||, or &&
                free(cur);
            } else {
                addExec(cur, numOfArgs, &maxExecs);
            }
        } 
        else if(!cur->path) 
        {
            cur->path = tokens[i];
            cur->args = malloc(2 * sizeof(char *));
            cur->input = NULL;
            cur->output = NULL;
        } 
        else {
            //If it got here, it's considered an argument
            if(numOfArgs + 1 > maxArgs) 
            {
                maxArgs += 1;
                cur->args = realloc(cur->args, maxArgs * sizeof(char*));
            }
            cur->args[numOfArgs] = tokens[i];
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
    execs[numOfExecs++] = cur;
}

void freeExecs() 
{
    for(int i = 0; i < numOfExecs; i++) 
    {
        Exec *cur = execs[i];
        free(cur->args);
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
    int l = 0, r = linePos;
    initToken();


    assert(lineBuffer[linePos-1] == '\n');

	//keeps track of if we are in a token or not
	int tokenFound = 0;
	//size of token
	int size = 0;
	//keeps track of the index of the tok
	int tokIndex = 0;
	//token
	char startOfToken[BUFSIZE];
    memset(startOfToken, 0, sizeof(startOfToken));

    //iterates through line
    while (l < r) {
		//token
		if(lineBuffer[l] != ' ' && lineBuffer[l] != '\n') {
			//if it is start of token3
			if(!tokenFound && !(lineBuffer[l] == '<' || lineBuffer[l] == '>' || lineBuffer[l] == '|')) {
                tokenFound = 1;
                if(lineBuffer[l] == '~'){
                    tokIndex = 0;
                    char* temp = getenv("HOME");
                    int sizeOfHome = strlen(temp);
                    for(int i = 0;i < sizeOfHome;i++){
                        startOfToken[tokIndex] = temp[i];
                        tokIndex++;
                    }
                    size = sizeOfHome;
                }
                else{
                    
                    size = 1;
                    tokIndex = 0;
                    startOfToken[tokIndex] = lineBuffer[l];
                    tokIndex++;
                }
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
                    tokIndex++;
				    startOfToken[tokIndex] = '\0';
				    addToken(startOfToken,size);
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
                    tokIndex++;
				    startOfToken[tokIndex] = '\0';
				    addToken(startOfToken,size);
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
                size++;
				startOfToken[tokIndex] = lineBuffer[l];
				tokIndex++;
				
			}
		}
		//going to be a space and reset
		else if(lineBuffer[l] == ' ' || lineBuffer[l] == '\n') {
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
				addToken(startOfToken,size);
				memset(startOfToken, 0, sizeof(startOfToken));
				tokIndex = 0;
				size = 0;
				tokenFound = 0;

				//add new line
				startOfToken[0] = '\n';
				startOfToken[1] = '\0';
				addToken(startOfToken,2);
				memset(startOfToken, 0,sizeof(startOfToken));
		}
		else if(l == r && size == 0) {
				startOfToken[0] = '\n';
				startOfToken[1] = '\0';
				addToken(startOfToken,2);
				memset(startOfToken, 0,sizeof(startOfToken));
		}
	}
}

void addToken(char tok[],int size) 
{
	numOfTokens++;
	if(tokens == NULL) {
		tokens = (char**)malloc(sizeof(char*));
		sizeOfToken = (int*)malloc(sizeof(int));
		tokens[0] = malloc(size * sizeof(char));
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
    free(tokens);
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