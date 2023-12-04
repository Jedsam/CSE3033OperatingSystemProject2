#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_COMMAND 1000
#define MAX_FOUND_STRING 100


typedef struct myArray{
    int length;
    int *lineCountList;
    char **foundStrings;
}ArrayWithLength;



ArrayWithLength *createArrayWithLength(int length){
    ArrayWithLength *resultArray = (ArrayWithLength*)calloc(1, sizeof(ArrayWithLength));
    resultArray -> lineCountList = (int*)calloc(length, sizeof(int));
    resultArray -> foundStrings = (char**)calloc(length, sizeof(char*));
    int i;
    for(i = 0; i < length; i++){
        resultArray -> foundStrings[i] = (char*)calloc(MAX_LINE, sizeof(char));
    }
    return resultArray;
}

ArrayWithLength* findStringInString(char *stringToSearchInside, char* stringToSearchWith){
    const char deliminator = ' ';
    char *currentStr = strtok(stringToSearchInside, &deliminator);
    
    ArrayWithLength *resultArray = createArrayWithLength(MAX_FOUND_STRING);
    int i = 0;
    int currentLength = 0;
    while(currentStr){
        if(strcmp(currentStr, stringToSearchWith) == 0){
            currentLength = resultArray -> length;
            resultArray -> lineCountList[currentLength] = i;
            strcpy(resultArray -> foundStrings[currentLength], currentStr);
            resultArray -> length++;   
            currentLength++;
        }
        i++;
        currentStr = strtok(NULL, &deliminator);
    }
    return resultArray;
}
char* getDirectoryString(char *directoryPath){
    struct dirent *directoryInformation;
    char *tempString, *resultString;
    DIR *currentDirectory = opendir(directoryPath);
    if(currentDirectory == NULL){
        perror("Could not find the directory of the PATH");
        return 0;
    }
    if(directoryInformation = readdir(currentDirectory)) {
        resultString = (char*)calloc(100000,sizeof(char));
        strcpy(resultString, directoryInformation -> d_name);
    }
    while((directoryInformation = readdir(currentDirectory))){
        tempString = directoryInformation -> d_name;
        strcat(resultString, tempString);
        strcat(resultString, " ");
    }
    closedir(currentDirectory);
    return resultString;
}
char **getTokenList(char *fullString, const char *deliminator){
    char **tokenList = (char**)calloc(100,sizeof(char*));
    char *tempToken = strtok(fullString,deliminator);
    int i;
    for(i = 0; i < 100 && tempToken; i++){
        tokenList[i] = tempToken;
        tempToken = strtok(NULL, deliminator);
    }
    return tokenList;
}
char *copyString(char* strToCopy){
    char *returnStr = calloc(1,strlen(strToCopy));
    strcpy(returnStr, strToCopy);
    return returnStr;
}
char *findCommand(char* commandName){
    const char deliminator = ':';
    char *currentToken, *currentDirectoryString, *pathString;
    char **tokenList;
    pathString = getenv("PATH");
    pathString = copyString(pathString);
    ArrayWithLength *currentArrayWithLength;
    tokenList = getTokenList(pathString, &deliminator);
    int i = 0;
    currentToken = tokenList[i];
    if(currentToken == 0){
        printf("No path found!");
        exit(1);
    }
    currentDirectoryString = getDirectoryString(currentToken);
    currentArrayWithLength = findStringInString(currentDirectoryString, commandName);
    free(currentDirectoryString);
    while (currentArrayWithLength && (currentArrayWithLength->length == 0) && currentToken){
        i++;
        currentToken = tokenList[i];
        if(currentToken){
            currentDirectoryString = getDirectoryString(currentToken);
            currentArrayWithLength = findStringInString(currentDirectoryString, commandName);
            free(currentDirectoryString);
        }
    } 
    free(pathString);
    if(currentArrayWithLength->length){
        strcat(currentToken, "/");
        return strcat(currentToken, currentArrayWithLength->foundStrings[0]);
    }
    else{
        return 0;
    } 
}
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

	printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
                    args[ct] = &inputBuffer[start];     
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		    *background  = 1;
                    inputBuffer[i-1] = '\0';
		}
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */

	for (i = 0; i <= ct; i++)
		printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */
 int checkPID(pid_t *childpidList, int length){
    int i;
    int status;
    for(i = 0; i < length && childpidList[i]; i++){

        if(!waitpid(childpidList[i],&status,WNOHANG)){
            return 1;
        }
    }
    return 0;
 }
int main(void)
{
            char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
            int background; /* equals 1 if a command is followed by '&' */
            char *args[MAX_LINE/2 + 1]; /*command line arguments */
            pid_t childpid;
            pid_t childpidList[MAX_COMMAND];
            int i = 0;
            int status;
            while (1){
                        background = 0;
                        printf("myshell: ");
                        /*setup() calls exit() when Control-D is entered */
                        setup(inputBuffer, args, &background);
                        if(strcmp(args[0], "exit") == 0){
                            if(checkPID(childpidList, MAX_COMMAND)){
                                perror("There are background processes still running and do not terminate the shell process unless the user terminates all background processes.");
                            }
                            else {
                                exit(0);
                            }
                        }
                        else if(strcmp(args[0], "bookmark") == 0){

                        }
                        else if(strcmp(args[0], "search") == 0){

                        }
                        else{
                            char *commandDirectory = findCommand(args[0]);
                            childpid = fork();
                            if (childpid == -1){
                                printf("Error creating fork for executing the command");
                            }
                            else if(childpid == 0){
                                
                                if(!commandDirectory){
                                    printf("Could not find the command!\n");
                                    exit(1); /*Command not found error*/
                                }
                                execv(commandDirectory,&args[0]);
                            }
                            else if(background == 0){
                                waitpid(childpid, &status, 0);
                            }
                            childpidList[i] = childpid;
                            childpid = 0;
                            i++;
                        }
                        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
						(3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */
            }
    
}
