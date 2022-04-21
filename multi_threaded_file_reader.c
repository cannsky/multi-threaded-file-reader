#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

struct ThreadArgument{
    long id;
};

//initialization of our array index
//mutex and memory indicator for realloc()
pthread_mutex_t mutexStruct;
pthread_cond_t condStruct;
int arrayIndex;
char** array;
int totalAllocatedMemory = 0;

//initialization of our file holder struct for the given directory
struct AvailableFiles{
    char *fileName;
    struct AvailableFiles* next;
    struct AvailableFiles* previous;
} typedef AvailableFiles;

struct ReadFileString{
    char *readFileString;
} typedef ReadFileString;

//initialization of the struct that allows us to create dynamic array
struct DynamicArray
{
    char *array[100];
} typedef DynamicArray;

DynamicArray* dynamicArray;
//updates the file name
AvailableFiles* UpdateAvailableFileData(AvailableFiles* newAvailableFile, char *availableFile){
    newAvailableFile = (AvailableFiles*)malloc(sizeof(AvailableFiles));
    newAvailableFile->fileName = malloc(400);
    strcpy(newAvailableFile->fileName, availableFile);
    return newAvailableFile;
}
//reading the file names in the given directory.
AvailableFiles* CreateAvailableFile(AvailableFiles* rootAvailableFile, char *availableFile){
    AvailableFiles* newAvailableFile = rootAvailableFile;
    AvailableFiles* previousAvailableFile;
    int isFileAlreadyAvailable = 0;
    while(newAvailableFile != NULL){
        if(strcmp(newAvailableFile->fileName, availableFile) == 0) isFileAlreadyAvailable = 1;
        previousAvailableFile = newAvailableFile;
        newAvailableFile = newAvailableFile->next;
    }
    if(isFileAlreadyAvailable == 0){
        newAvailableFile = UpdateAvailableFileData(newAvailableFile, availableFile);
        if(previousAvailableFile != NULL) previousAvailableFile->next = newAvailableFile;
        newAvailableFile->previous = previousAvailableFile;
    }
    else fprintf(stderr, "%s", "Duplicated Files Exits, skipping duplicated files...");
    return (rootAvailableFile == NULL ? newAvailableFile : rootAvailableFile);
}
//deletes the current file that a thread is working on to block another thread to use it.
AvailableFiles* RemoveAvailableFile(AvailableFiles* rootAvailableFile, char *availableFile){
    AvailableFiles* currentAvailableFile = NULL;
    AvailableFiles* previousAvailableFile = NULL;
    int isDeletingRoot = 0;
    currentAvailableFile = rootAvailableFile;
    while(currentAvailableFile != NULL){
        if(currentAvailableFile->fileName != NULL)
            if(currentAvailableFile->fileName != NULL && availableFile != NULL){
                if(strcmp(currentAvailableFile->fileName, availableFile) == 0){
                    if(previousAvailableFile != NULL){
                        previousAvailableFile->next = currentAvailableFile->next;
                        if(currentAvailableFile->next != NULL) (currentAvailableFile->next)->previous = previousAvailableFile;
                        break;
                    }
                    else{
                        //We are deleting the root!
                        if(currentAvailableFile->next != NULL) (currentAvailableFile->next)->previous = NULL;
                        currentAvailableFile = currentAvailableFile->next;
                        isDeletingRoot = 1;
                        break;
                    }
                }
                previousAvailableFile = currentAvailableFile;
                currentAvailableFile = currentAvailableFile->next;
            }
    }
    return (isDeletingRoot == 0 ? rootAvailableFile : currentAvailableFile);
}
//test case function.
void PrintAvailableFiles(AvailableFiles* rootAvailableFile){
    AvailableFiles *currentAvailableFile;
    currentAvailableFile = rootAvailableFile;
    while(currentAvailableFile != NULL){
        if(currentAvailableFile->fileName != NULL){
            fprintf(stderr, "%s ", currentAvailableFile->fileName);
            fprintf(stderr, "\n");
            currentAvailableFile = currentAvailableFile->next;
        }
    }
}

int CheckAvailableFiles(AvailableFiles* rootAvailableFile){
    int i = 0;
    AvailableFiles *currentAvailableFile;
    currentAvailableFile = rootAvailableFile;
    while(currentAvailableFile != NULL){
        if(currentAvailableFile->fileName != NULL){
            i++;
            currentAvailableFile = currentAvailableFile->next;
        }
    }
    return i;
}

char *directoryString;
long numberOfThreads;
AvailableFiles* rootAvailableFile;
int totalFileCount;

int printError(char *errorString){
    fprintf(stderr, "%s", errorString);
    fprintf(stderr, "Exiting Program...");
    return 1;
}
//gets the file names for the availables.
void getFileNames(){
    DIR *directory;
    struct dirent *dir;
    directory = opendir(directoryString);
    if (directory) {
        while ((dir = readdir(directory)) != NULL) {
            if(dir->d_name[0] != '.') rootAvailableFile = CreateAvailableFile(rootAvailableFile, dir->d_name);
        }
        closedir(directory);
    }
    totalFileCount = CheckAvailableFiles(rootAvailableFile);
}
// operates to read files, copy them into array and realloc() the array
char* readFile(char *fileName){
    ReadFileString* readFileString = malloc(400);
    ReadFileString* filePath = malloc(400);
    ReadFileString* token = malloc(400);
    FILE *fp;
    filePath->readFileString = (char *) malloc(100);
    readFileString->readFileString = (char *) malloc(100);
    token->readFileString = (char *) malloc(100);
    strcpy(filePath->readFileString, directoryString);
    strcat(filePath->readFileString, "/");
    strcat(filePath->readFileString, fileName);
    fp = fopen(filePath->readFileString, "r");
    if(fp == NULL) printError("File couldn't be opened");
    while(fgets(readFileString->readFileString, 256,fp)) {
        token->readFileString = strtok (readFileString->readFileString, " ");
        while ((token->readFileString) != NULL) {
            pthread_mutex_lock(&mutexStruct);
            arrayIndex++;
            pthread_mutex_unlock(&mutexStruct);
            dynamicArray->array[arrayIndex] = malloc(100);
            int z;
            for(z = 0; z < 100; z++){
                if(token->readFileString[z] == '\n') break;
                if(token->readFileString[z] != '\0'){
                    dynamicArray->array[arrayIndex][z] = token->readFileString[z];
                }
                else{
                    dynamicArray->array[arrayIndex][z] = '\0';
                }
            }
            fprintf(stderr, "Thread %ld Added \"%s\" at index %d \n", pthread_self(), dynamicArray->array[arrayIndex], arrayIndex);
            token->readFileString = strtok(NULL, " &");
        }
    }
    fclose(fp);
}



// allows thread to read and operate on files.
void executeThread(char* fileName){
    printf("Main Thread, Assigned %s, to worker thread %ld\n", fileName, pthread_self());
    readFile(fileName);
}

// creates thread and operates as a thread pool.
void* callThread(void* args){
    int returnValue = 0, counter = 0, a = 0;
    char *fileName;
    while(1){
        pthread_mutex_lock(&mutexStruct);
            if(counter++ > (totalFileCount / numberOfThreads)) pthread_cond_wait(&condStruct, &mutexStruct);
        if(rootAvailableFile != NULL) {
            fileName = rootAvailableFile->fileName;
            rootAvailableFile = RemoveAvailableFile(rootAvailableFile, rootAvailableFile->fileName);
        }
        else returnValue = 1;
        pthread_mutex_unlock(&mutexStruct);
        if(returnValue == 1) return NULL;
        executeThread(fileName);
        if(CheckAvailableFiles(rootAvailableFile) == 0) for(a = 0; a < numberOfThreads; a++) pthread_cond_signal(&condStruct);
    }
}
// manages the thread operations.
void processor(){
    pthread_t pthread_id[numberOfThreads];
    getFileNames();
    int i;
    //MUTEX IS CREATED FOR BLOCKING THREADS TO GET NULL
    pthread_mutex_init(&mutexStruct, NULL);
    pthread_cond_init(&condStruct, NULL);
    for(i = 0; i < numberOfThreads; i++) pthread_create(&pthread_id[i], NULL, &callThread, NULL);
    for(i = 0; i < numberOfThreads; i++) if(pthread_join(pthread_id[i], NULL) != 0) printError("Failed to join thread.");
    pthread_cond_destroy(&condStruct);
    pthread_mutex_destroy(&mutexStruct);
}
// main function
int main(int argc, char **argv) {
    dynamicArray = malloc(2500);
    arrayIndex = -1;
    char *ptr;
    if(argc != 5) return printError("Number of arguments must be equal to 5!\n");
    directoryString = argv[2];
    fprintf(stderr, "%s", directoryString);
    numberOfThreads = strtol(argv[4], &ptr, 10);
    fprintf(stderr, "%ld", numberOfThreads);
    processor();
    return 0;
}
