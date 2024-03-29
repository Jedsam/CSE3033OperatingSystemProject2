#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>  // Include for file control options
#include <signal.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_COMMAND 1000
#define MAX_FOUND_STRING 1000
#define MAX_PATH 1000
#define MAX_BOOKMARKS 10  // Define MAX_BOOKMARKS

char *bookmarks[MAX_BOOKMARKS];
int bookmarkCount = 0;
pid_t foregroundProcessPid = -1;
pid_t foregroundProcessGroupId = -1;


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
    return newStr;
}
DynamicString *createDynamicStringWithStr(char *Str){
    DynamicString *newStr = (DynamicString*) malloc(sizeof(DynamicString));
    newStr -> currentSize = strlen(Str);
    newStr -> maxSize = newStr -> currentSize;
    return newStr;
}

void freeDynamicString(DynamicString *dyStr){
    free(dyStr-> strPtr);
    free(dyStr);
}
void addChar(DynamicString *dyStr, char addChar){
    if(dyStr -> currentSize + 3 >= dyStr -> maxSize){
        dyStr -> maxSize *= 2;
        dyStr -> maxSize ++;
        dyStr -> strPtr = realloc(dyStr ->strPtr,dyStr -> maxSize);
    }
    dyStr -> strPtr[dyStr -> currentSize] = addChar;
    dyStr -> currentSize++;
}

char* getDirectoryString(char *directoryPath){
    struct dirent *directoryInformation;
    char *tempString, *resultString;
    resultString = 0;
    if(!directoryPath){
        return 0;
    }
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
        strcat(resultString, " ");
        strcat(resultString, tempString);
        
    }
    closedir(currentDirectory);
    return resultString;
}
char **getTokenList(char *fullString, const char deliminator){
    char **tokenList = (char**)calloc(1000,sizeof(char*));
    if(!fullString){
        exit(10);
    }

    char *tempToken = strtok(fullString,&deliminator);
    int i;
    for(i = 0; i < 100 && tempToken; i++){
        tokenList[i] = tempToken;
        tempToken = strtok(NULL, &deliminator);
    }
    return tokenList;
}
char *copyString(char* strToCopy){
    if(!strToCopy){
        return 0;
    }
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

void setup(char inputBuffer[], char *args[], int *background) {
    int length;
    int i;
    int start = -1;
    int ct = 0;
    int inQuote = 0; // Flag to indicate if we are inside quotes

    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    if (length == 0)
        exit(0);

    if (length < 0 && errno != EINTR) {
        perror("error reading the command");
        exit(-1);
    }

    for (i = 0; i < length; i++) {
        switch (inputBuffer[i]) {
            case ' ':
            case '\t':
                if (inQuote == 0) {
                    if (start != -1) {
                        args[ct] = &inputBuffer[start];
                        ct++;
                    }
                    inputBuffer[i] = '\0';
                    start = -1;
                }
                break;

            case '\"':
                if (inQuote) {
                    inQuote = 0;
                } else {
                    inQuote = 1;
                    if (start == -1) {
                        start = i; // Include the opening quote in the argument
                    }
                }
                break;

            case '\n':
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL;
                break;

            default:
                if (start == -1 && inQuote == 0)
                    start = i;
                if (inputBuffer[i] == '&' && inQuote == 0) {
                    *background = 1;
                    inputBuffer[i] = '\0';
                }
        }
    }
    args[ct] = NULL; // Null-terminate the argument list
}


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
    int str1len;
    if(str2)
        str2len = strlen(str2);
    else str2len = 0;
    if(str1)
    str1len = strlen(str1);
    else str1len = 0;

    char *str3 = calloc(str1len + str2len + 2, sizeof(char));
    if(str2)
        strcpy(str3, str2);

    strcat(str3, "/");
    if(str1)
    strcat(str3, str1);

    return str3;
}
int strncmpEdited(char *checkedString, char *stringToCheckWith, int length){
    int i;
    char current;
    if(!checkedString){
        return 0;
    }
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
    if(!pathName){
        pathName = calloc(1,1);
    }
    if(!searchedString){
        return;
    }
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
        perror("Error getting current working directory");
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

void handleIOredirection(char *args[]) {
    int i;
    int fd;
    for(i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            args[i] = NULL;
            fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (strcmp(args[i], ">>") == 0) {
            args[i] = NULL;
            fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (strcmp(args[i], "<") == 0) {
            args[i] = NULL;
            fd = open(args[i + 1], O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
        } else if (strcmp(args[i], "2>") == 0) {
            args[i] = NULL;
            fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
    }
}


void executeBookmarkCommand(char *command) {
    char *args[MAX_LINE / 2 + 1];
    int background = 0;
    pid_t pid;
    int status;

    // Split the command into arguments
    int ct = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[ct++] = strdup(token); // Duplicate and store the argument
        token = strtok(NULL, " ");
    }
    args[ct] = NULL; // Null-terminate the arguments array

    char *commandPath = findCommand(args[0]);
    if (commandPath == NULL) {
        printf("Command not found in PATH: %s\n", args[0]);
        // Free duplicated arguments
        for (int i = 0; i < ct; i++) {
            free(args[i]);
        }
        return;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork failed");
        // Free duplicated arguments
        for (int i = 0; i < ct; i++) {
            free(args[i]);
        }
        return;
    }
    if (pid == 0) {
        // Child process
        execv(commandPath, args);
        // If execv returns, there was an error
        perror("execv failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        if (!background) {
            waitpid(pid, &status, 0);
        }
    }

    // Free duplicated arguments in the parent process
    for (int i = 0; i < ct; i++) {
        free(args[i]);
    }
}


void handleSignal(int sig) {
    if (sig == SIGTSTP) {
        if (foregroundProcessGroupId != -1) {
            // Send SIGSTOP only to the foreground process group
            kill(-foregroundProcessGroupId, SIGSTOP);
            printf("\nForeground process stopped.\n");
        } else {
            printf("\nNo foreground process to stop.\n");
        }
    }
}



void manageBookmark(char *args[]) {
    if (args[1] == NULL) {
        printf("No arguments provided for bookmark command.\n");
        return;
    }

    if (strcmp(args[1], "-l") == 0) {
        // List bookmarks
        for (int i = 0; i < bookmarkCount; i++) {
            printf("%d: %s\n", i, bookmarks[i]);
        }
    } else if (strcmp(args[1], "-d") == 0) {
        // Delete a bookmark
        int index = atoi(args[2]);
        if (index >= 0 && index < bookmarkCount) {
            free(bookmarks[index]);
            for (int i = index; i < bookmarkCount - 1; i++) {
                bookmarks[i] = bookmarks[i + 1];
            }
            bookmarkCount--;
            printf("Bookmark %d deleted.\n", index);
        } else {
            printf("Invalid index for bookmark deletion.\n");
        }
    } else if (strcmp(args[1], "-i") == 0) {
        // Execute a bookmarked command
        int index = atoi(args[2]);
        if (index >= 0 && index < bookmarkCount) {
            executeBookmarkCommand(bookmarks[index]);
        } else {
            printf("Invalid index for bookmark execution.\n");
        }
    } else {
        // Add a new bookmark
        if (bookmarkCount < MAX_BOOKMARKS) {
            bookmarks[bookmarkCount++] = strdup(args[1]);
            printf("Bookmark added: %s\n", args[1]);
        } else {
            printf("Bookmark limit reached.\n");
        }
    }
}

int main(void) {
    char inputBuffer[MAX_LINE]; /* Buffer to hold the command entered */
    int background; /* Equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /* Command line arguments */
    pid_t childpid;
    pid_t childpidList[MAX_COMMAND];
    int status;
    int i = 0; // Initialize the index variable for childpidList

    // Set up signal handling
    //signal(SIGINT, handleSignal);
    signal(SIGTSTP, handleSignal);

    while (1) {
        background = 0;
        printf("myshell: ");

             //ensure myshell is printed immediately
        fflush(stdout); 

        setup(inputBuffer, args, &background); // Setup command

   
        // Handle I/O redirection
        handleIOredirection(args);

        // Handle exit command
        if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
            if (checkPID(childpidList, MAX_COMMAND)) {
                printf("There are background processes still running.\n");
            } else {
                exit(0);
            }
        }
            // Handle bookmark command
        else if (args[0] != NULL && strcmp(args[0], "bookmark") == 0) {
            manageBookmark(args);
        }
            // Handle search command
        else if (args[0] != NULL && strcmp(args[0], "search") == 0) {
            if (!args[1]) {
                perror("Please enter a text to search");
                exit(3);
            }
            if (strcmp(args[1], "-r") == 0) {
                if (args[2])
                    startSearching(args[2], 1);
                else {
                    perror("Please enter a text to search");
                    exit(3);
                }
            } else {
                startSearching(args[1], 0);
            }
        }
            // Handle other commands
        else if (args[0] != NULL) {
            char *commandDirectory = findCommand(args[0]);
            childpid = fork();
            if (childpid == -1) {
                perror("Error creating fork for executing the command");
            } else if (childpid == 0) {
            // Set the process group for the child process
                setpgid(0, 0);
                if (!commandDirectory) {
                    printf("Could not find the command!\n");
                    exit(1); /* Command not found error */
                }
                execv(commandDirectory, args);
                perror("execv"); // If execv returns, it's an error
                exit(EXIT_FAILURE);
            } else {
                if (!background) {
                // Set the process group for the foreground process
		    setpgid(childpid, childpid);
		    foregroundProcessPid = childpid;
		    foregroundProcessGroupId = getpgid(childpid);
		    waitpid(childpid, &status, WUNTRACED);
		    if (WIFSTOPPED(status)) {
		        printf("\nProcess %d stopped\n", childpid);
		    }
		    foregroundProcessPid = -1;
		    foregroundProcessGroupId = -1;
                }
                if (i < MAX_COMMAND) {
                    childpidList[i++] = childpid;
                } else {
                    printf("Maximum number of child processes reached.\n");
                }
            }
        }
    }
    return 0;
}
