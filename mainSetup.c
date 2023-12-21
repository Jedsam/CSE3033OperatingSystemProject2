#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_COMMAND 1000
#define MAX_FOUND_STRING 1000
#define MAX_PATH 1000

typedef struct stringDynam{
    int maxSize;
    int currentSize;
    char *strPtr;
}DynamicString;

DynamicString *createDynamicString(int size){
    DynamicString *newStr = (DynamicString*) malloc(sizeof(DynamicString));
    newStr -> maxSize = size;
    newStr -> currentSize = 0;
    newStr -> strPtr = calloc(sizeof(char), size);
}
DynamicString *createDynamicStringWithStr(char *Str){
    DynamicString *newStr = (DynamicString*) malloc(sizeof(DynamicString));
    newStr -> currentSize = strlen(Str);
    newStr -> maxSize = newStr -> currentSize;
}
void freeDynamicString(DynamicString *dyStr){
    free(dyStr-> strPtr);
    free(dyStr);
}
void addChar(DynamicString *dyStr, char addChar){
    if(dyStr -> currentSize + 3 >= dyStr -> maxSize){
        dyStr -> maxSize *= 2 + 1;
        dyStr -> strPtr = realloc(dyStr ->strPtr,dyStr -> maxSize);
    }
    dyStr -> strPtr[dyStr -> currentSize] = addChar;
    dyStr -> currentSize++;
}

char* getDirectoryString(char *directoryPath){
    struct dirent *directoryInformation;
    char *tempString, *resultString;
    DIR *currentDirectory = opendir(directoryPath);
    if(currentDirectory == NULL){
        printf("Could not find the directory of the PATH");
        return 0;
    }
    if(directoryInformation = readdir(currentDirectory)) {
        resultString = (char*)calloc(100000,sizeof(char));
        strcpy(resultString, directoryInformation -> d_name);
    }
    while((directoryInformation = readdir(currentDirectory))){
        tempString = directoryInformation -> d_name;
        strcat(resultString, " ");
        strcat(resultString, tempString);
    }
    closedir(currentDirectory);
    return resultString;
}
char **getTokenList(char *fullString, const char deliminator){
    char **tokenList = (char**)calloc(1000,sizeof(char*));
    char *tempFullString = calloc(sizeof(char), strlen(fullString));
    strcpy(tempFullString, fullString);
    char *tempToken = strtok(tempFullString,&deliminator);
    int i;
    for(i = 0; i < 100 && tempToken; i++){
        tokenList[i] = tempToken;
        tempToken = strtok(NULL, &deliminator);
    }
    return tokenList;
}
char *copyString(char* strToCopy){
    char *returnStr = calloc(1,strlen(strToCopy));
    strcpy(returnStr, strToCopy);
    return returnStr;
}
int findStrInsideStr(char *searchedString, char* stringToSearchWith){
    int i = 0;
    int length = strlen(stringToSearchWith);
    char current = searchedString[i];
    while(current){
        if(strncmp(searchedString + i, stringToSearchWith, length) == 0 && 
        (searchedString[i + length] == ' ' || searchedString[i + length] == 0)){
            return 1;
        }
        i++;
        current = searchedString[i];
    }
    return 0;
}
char *copyStringPlusSpace(char *strToCopy){
    int length = strlen(strToCopy);
    char *returnStr = calloc(1, length + 1);
    returnStr[0] = ' ';
    strcpy(returnStr + 1, strToCopy);
    return returnStr;
}
char *findCommand(char* commandNameOrigin){
    char *commandName = copyStringPlusSpace(commandNameOrigin);
    const char deliminator = ':';
    char *currentToken, *currentDirectoryNameString, *pathString;
    char **tokenList;
    pathString = getenv("PATH");
    pathString = copyString(pathString);
    tokenList = getTokenList(pathString, deliminator);
    int i = 0;
    currentToken = tokenList[i];
    if(currentToken == 0){
        printf("No path found!");
        exit(1);
    }
    currentDirectoryNameString = getDirectoryString(currentToken);
    int isFound = findStrInsideStr(currentDirectoryNameString, commandName);
    free(currentDirectoryNameString);
    while (!isFound && currentToken){
        i++;
        currentToken = tokenList[i];
        if(currentToken){
            currentDirectoryNameString = getDirectoryString(currentToken);
            isFound = findStrInsideStr(currentDirectoryNameString, commandName);
            free(currentDirectoryNameString);
        }
    } 
    free(pathString);

    if(isFound){
        strcat(currentToken, "/");
        return strcat(currentToken, commandNameOrigin);
    }
    else{
        return 0;
    } 
}
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */
void emptyBuffer(char inputBuffer[], int length){
    int i;
    for(i = 0; i < length; i++){
        inputBuffer[i] = 0;
    }
}
void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
    emptyBuffer(inputBuffer, MAX_LINE);
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
        printf("error reading the command");
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
int checkForNoDot(char *stringToCheck){
    int i;
    char current = stringToCheck[0];
    for(i = 0; current != 0 && current != EOF; i++){
        current = stringToCheck[i];
        if(current == '.')
            return 0;
    }
    return 1;
}
char *getWholeFileString(char *fileName){
    if(checkForNoDot(fileName))
        return 0;
    FILE *tempFile = fopen(fileName, "r");
    if(!tempFile)
        return 0;
    DynamicString *fileOutput = createDynamicString(100); 
    char current = fgetc(tempFile);
    while(current != EOF){
        addChar(fileOutput, current);
        current = fgetc(tempFile);
    }
    
    fclose(tempFile);
    char *temp = fileOutput -> strPtr;
    free(fileOutput);
    return temp;
}
int isDir(char *fileName){
    DIR *directory = opendir(fileName);
    if(directory){
        closedir(directory);
        return 1;
    }
    return 0;
}
char *combineTwoStringPath(char *str1, char *str2){
    int str2len;
    if(str2)
    str2len = strlen(str2);
    else str2len = 0;

    char *str3 = calloc(strlen(str1) + str2len + 1, sizeof(char));
    if(str2)
    strcpy(str3, str2);

    strcat(str3, "/");
    strcat(str3, str1);
    return str3;
}
int strncmpEdited(char *checkedString, char *stringToCheckWith, int length){
    int i;
    char current;
    for(i = 0; i < length; i++){
        current = checkedString[i];
        if(current == 0 || current != stringToCheckWith[i] || current == EOF)
            return 0;
    }
    return 1;
} 
void searchString(char *searchedString, char *stringToSearchWith, char *pathName){
    int i = 0;
    int k;
    int startingInt;
    int lineCounter = 0;
    int lineLength = 0;
    DynamicString *lineString = createDynamicString(1000);
    int length = strlen(stringToSearchWith);
    char *tempPointer = calloc(1000, sizeof(char));
    char tempCurrent;
    char current = searchedString[i];
    while(current && current != EOF){
        if(current == '\n'){
            lineLength = 0;
            lineCounter++;
            freeDynamicString(lineString);
            lineString = createDynamicString(1000);
        }
        else{
            lineLength++;
            addChar(lineString, current);
        }
        if(strncmpEdited(searchedString + i, stringToSearchWith, length)){
            k = i - lineLength + 1;
            startingInt = k;

            do{
                tempCurrent = searchedString[k];
                tempPointer[k - startingInt] = tempCurrent;
                k++;
            }while(tempCurrent != '\n' && tempCurrent != 0 && tempCurrent != EOF);
            printf("\t%d: %s -> %s", lineCounter + 1, pathName, tempPointer);
        }
        i++;
        current = searchedString[i];
    }
    if(lineLength != 0){
        freeDynamicString(lineString);
    }
    free(tempPointer);
}
void *findStringInFile(char *directoryPathString, char *searchStr, int isRecursive, char *normalDirectory){
    char *fileString;
    char *directoryString = getDirectoryString(directoryPathString);
    const char deliminator = ' ';
    char **tokenList = getTokenList(directoryString, deliminator);
    int i = 0;
    char *currentToken = tokenList[i];
    char *currentDirectoryPathString;
    for(i = 0; currentToken;){
            if((strcmp(currentToken, "..") != 0) &&
                (strcmp(currentToken, ".") != 0)){
                    
                
                currentDirectoryPathString = combineTwoStringPath(currentToken, directoryPathString);
                if(isDir(currentToken)){
                    if(isRecursive){
                        chdir(currentToken);
                        findStringInFile(copyString(currentDirectoryPathString), searchStr, isRecursive, combineTwoStringPath(currentToken, normalDirectory));
                        chdir("..");
                }
            }
            else{
                fileString = getWholeFileString(currentToken);
                if(fileString){
                    searchString(fileString, searchStr, combineTwoStringPath(currentToken, normalDirectory));
                    free(fileString);
                }
            }
            free(currentDirectoryPathString);
        }
        i++;
        currentToken = tokenList[i];
        
    }
    free(directoryString);
}   
void *startSearching(char* searchStr, int isRecursive){
    char cwd[MAX_PATH];
    //REMOVE THIS BEFORE SUBMITTING
    chdir(".");
    
    if((getcwd(cwd, MAX_PATH)) == NULL){
        printf("Error getting current working directory");
        exit(2);
    }
    int length = strlen(searchStr);
    if(searchStr[0] == '\"' && searchStr[length - 1] == '\"'){    
        
        char* tempStr = calloc(length - 2,sizeof(char));
        int i;
        length -= 2;
        for(i = 0; i < length; i++){
            tempStr[i] =  searchStr[i + 1]; 
        }
        if(isRecursive){
        return findStringInFile(cwd, tempStr, 1, ".");
    }
    else{
        
        return findStringInFile(cwd , tempStr, 0, "."); 
    }   
    free(tempStr);
    }
    if(isRecursive){
        return findStringInFile(cwd, searchStr, 1, ".");
    }
    else{
        
        return findStringInFile(cwd , searchStr, 0, "."); 
    }   
}


int main(void)
{
            char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
            int background; /* equals 1 if a command is followed by '&' */
            char *args[MAX_LINE/2 + 1]; /*command line arguments */
            pid_t childpid;
            pid_t *childpidList = calloc(sizeof(pid_t), MAX_COMMAND);
            int i = 0;
            int status;
            while (1){
                        background = 0;
                        printf("myshell: ");
                        fflush(stdout);
                        /*setup() calls exit() when Control-D is entered */
                        setup(inputBuffer, args, &background);
                        if(strcmp(args[0], "exit") == 0){
                            if(checkPID(childpidList, MAX_COMMAND)){
                                printf("There are background processes still running and do not terminate the shell process unless the user terminates all background processes.\n");
                            }
                            else {
                                exit(0);
                            }
                        }
                        else if(strcmp(args[0], "bookmark") == 0){

                        }
                        else if(strcmp(args[0], "search") == 0){
                            if(!args[1]){
                                printf("Please enter a text to search\n");
                                exit(3);
                            }
                            if(strcmp(args[1], "-r") == 0){
                                if(args[2])
                                    startSearching(args[2],1);
                                else{
                                    printf("Please enter a text to search\n");
                                    exit(3);
                                }

                            }
                            else{
                                startSearching(args[1],0);
                            }
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
