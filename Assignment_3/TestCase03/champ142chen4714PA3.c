// Anthony Chen chen4714
// Samira Champling champ142

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include "util.h"
#include <dirent.h>
#include <sys/wait.h>
#include <pthread.h>
#include <limits.h>

pthread_mutex_t mutex;
pthread_mutex_t aggregateMutex;

struct node {
  char* name;
  int hasp;
  int p;
  struct node* next;
  struct node* prev;
};

struct directories {
  char* inputdir;
  char* outputdir;
};

struct candidate {
  char* name;
  int numVotes;
  struct candidate* next;
};

int numOfFiles = 0;
struct node* head = NULL;
struct node* tail = NULL;


// find directory in DAG given its name
char* findDir(char* path, char* name) {
  if (strcmp(path, name) == 0) {
    return path;
  }

  DIR* dir = opendir(path);
  if(dir == NULL) {
    perror("Unable to open directory");
    exit(0);
  }
  struct dirent *d;
  char* newPath = malloc(sizeof(char)*PATH_MAX);

  // see if target directory is in the current directory
  // otherwise recurse through each subdirectory
  for (d = readdir(dir); d != NULL; d = readdir(dir)) {
    if (d->d_name[0] == '.' || d->d_type != DT_DIR){
      continue;
    }

    strcpy(newPath, "");
    strcpy(newPath, path);
    strcat(newPath, "/");
    strcat(newPath, d->d_name);

    if (strcmp(d->d_name, name) == 0) {
      closedir(dir);
      return newPath;
    }

    // recurse through subdirectory
    char* foundPath = findDir(newPath, name);

    // a path was found, return it
    if(foundPath != NULL){
      closedir(dir);
      free(newPath);
      return foundPath;
    }
  }

  free(newPath);
  closedir(dir);

  return NULL;
}

// delete an existing directory
void deleteDir(char* path){
  DIR* dir = opendir(path);
  if(dir == NULL) {
    perror("Unable to open directory");
    exit(0);
  }
  if(dir == NULL){
    perror("Error: Unable to open directory\n");
    exit(0);
  }
  struct dirent *d;
  char* newPath = malloc(sizeof(char)*PATH_MAX+1);

  for (d = readdir(dir); d != NULL; d = readdir(dir)) {
    if (d->d_name[0] == '.'){
      continue;
    }

    strcpy(newPath, "");
    strcpy(newPath, path);
    strcat(newPath, "/");
    strcat(newPath, d->d_name);

    // delete files and recurse through directories
    if(d->d_type == DT_REG){
      remove(newPath);
    } else if (d->d_type == DT_DIR){
      deleteDir(newPath);
    }

  }

  free(newPath);
  closedir(dir);
  // delete current directory
  rmdir(path);
  return NULL;
}

// make DAG
int create_dag(char* dag, char* outputdir){
  char* cwd = malloc(PATH_MAX);
  cwd = getcwd(cwd,PATH_MAX);

  // if DAG of same name already exists, delete it and remake it
  if(mkdir(outputdir,0777) == -1) {
    deleteDir(outputdir);
    if(mkdir(outputdir,0777) == -1) {
      perror("Error: Making <output_dir> failed\n");
      exit(0);
    }
  }
  if(chmod(outputdir,0777) == -1) {
    perror("Error: Unable to make permissions for <output_dir> \n");
    exit(0);
  }

  // make log.txt
  char logFile[PATH_MAX] = "";
  strcpy(logFile, outputdir);
  strcat(logFile, "/log.txt");
  int logfd;
  if ((logfd = open(logFile, O_CREAT | O_TRUNC | O_RDWR, 0666)) == -1) {
    perror("Error opening log.txt file");
    return -1;
  }

  // get path of outputdir into cwd
  strcat(cwd, "/");
  strcat(cwd, outputdir);

  struct dirent* ent;
  FILE *fp = fopen(dag,"r");
  if(fp == NULL) {
    perror("Error opening dag file");
    exit(0);
  }
  char buf[1024];
  int isFirstLine = 1;
  char* line = malloc(1024 * sizeof(char));
  int size = 1024;
  strcpy(line,"");

  int isEmpty = 1;

  // read entire line
  while((fgets(buf,1024,fp)) != NULL){
    isEmpty = 0;
    if(strchr(buf,'\n') == NULL) { // line longer than 1024, buf does not contain /n
      line = (char*)realloc(line, size+1024);
      size += 1024;
      strcat(line,buf);
      strcpy(buf, "");
      continue;
    } else { // line has ended
      buf[strlen(buf)-1] = 0;
      strcpy(line,buf);

      // if line is empty, skip it
      if(strcmp(line, "") == 0) {
        continue;
      }

      // split line by ':' character
      char*** strings = malloc(sizeof(line));
      int numStrings;
      if((numStrings = makeargv(line,":",strings)) == -1){
        printf("makeargv failure\n");
      }

      // if first line, make root node
      if(isFirstLine) {
        char* nodePath = malloc(PATH_MAX+1);
        strcpy(nodePath,cwd);
        strcat(nodePath, "/");
        strcat(nodePath, *strings[0]);
        int status = mkdir(nodePath, 0777);
        if(chmod(nodePath,0777) == -1) {
          perror("Unable to change permissions");
          exit(0);
        }
        if(status != 0) {
          perror("mkdir error: ");
          exit(0);
        }
        isFirstLine = 0;
        free(nodePath);
      }

      // go through the rest of the line and create subdirectories
      char* path = malloc(PATH_MAX+1);
      strcpy(path, findDir(cwd, strings[0][0]));
      if(path != NULL) { // found directory
        for(int i = 1; i < numStrings; i++){
          char* nodePath = malloc(PATH_MAX);
          strcpy(nodePath,path);
          strcat(nodePath, "/");
          strcat(nodePath, strings[0][i]);
          if(mkdir(nodePath,0666) == -1){
            perror("mkdir error");
            exit(0);
          }
          if(chmod(nodePath,0777) == -1){
            perror("permission error");
            exit(0);
          }
          free(nodePath);
        }
      }
      free(strings);
      strcpy(line,""); // reset line
      size = 1024;
      strcpy(buf, ""); // reset buf
    }
  }
  free(cwd);
  free(line);
  return isEmpty;
}


// find winner to attach to winner file
char* findWinner(struct candidate** candHead){
  struct candidate* current = *candHead;

  if(current == NULL) { // linked list is empty
    return NULL;
  }

  int maxVotes = 0;
  int size = strlen(current->name);
  char* maxName = malloc(size+1);

  // find candidate with max votes
  while(current != NULL){
    if(current->numVotes > maxVotes) {
      maxVotes = current->numVotes;
      if(sizeof(current->name) > size){
        size = strlen(current->name);
        maxName = (char*)realloc(maxName, size+1);
      }
      strcpy(maxName,current->name);
    }
    current = current->next;
  }

  return maxName;
}


// go through ancestor nodes and add leaf votes
void aggregateVotes(char* path, char* inputFile, char* outputdir){
  // if this directory is the output dir, stop recursion
  if(strcmp(path,outputdir) == 0){
    return;
  }

  struct candidate* candHead = NULL;

  char*** strings = malloc(sizeof(path));
  int numStrings = makeargv(path,"/",strings);

  char* outputFilePath = malloc(PATH_MAX);
  strcpy(outputFilePath,path);
  strcat(outputFilePath,"/");
  strcat(outputFilePath,strings[0][numStrings-1]);
  strcat(outputFilePath,".txt");


  int size = 1024;
  char buf[1024];
  char* line = malloc(size);
  strcpy(line,"");
  FILE* parentfile;
  int fileExists = 1;
  if((parentfile = fopen(outputFilePath, "r")) == NULL){
    fileExists = 0;
  }

// fill linked list with candidates and votes to be written to output file
  while(fileExists && (fgets(buf,1024,parentfile)) != NULL){
    if(strchr(buf,'\n') == NULL) { // line longer than 1024, buf does not contain /n
      if(feof(parentfile)) {
        break;  //is at the end of file. This is WINNER: of Who_Won. Should ignore it.
      }
      line = (char*)realloc(line, size+1024);
      size += 1024;
      strcat(line,buf);
      strcpy(buf, "");
      continue;
    } else {
      buf[strlen(buf)-1] = 0;
      strcat(line,buf);

      char*** candidateLine = malloc(sizeof(line));
      int numStringsLine = makeargv(line,":",candidateLine);
      // add candidate to linkedlist
      struct candidate* newNode = malloc (sizeof(struct candidate));
      newNode->name = malloc(strlen(candidateLine[0][0]) + 1);
      strcpy(newNode->name,candidateLine[0][0]);
      newNode->numVotes = atoi(candidateLine[0][1]);
      newNode->next = candHead;
      candHead = newNode;
      strcpy(line,"");
      size = 1024;
      free(candidateLine);
    }
  }


  // update linked list with info from leaf
  FILE* childfile;
  if((childfile = fopen(inputFile, "r")) == NULL){
    perror("error opening leaf file");
    exit(0);
  }

  int candidateExists = 0;

  while((fgets(buf,1024,childfile)) != NULL){
    if(strchr(buf,'\n') == NULL) { // line longer than 1024, buf does not contain /n
      line = (char*)realloc(line, size+1024);
      size += 1024;
      strcat(line,buf);
      strcpy(buf, "");
      continue;
    } else {
      buf[strlen(buf)-1] = 0;
      strcat(line,buf);
      char*** candidateLine = malloc(sizeof(line));
      int numStringsLine = makeargv(line,":",candidateLine);

      // add votes to candidate in list
      struct candidate* current = candHead;
      while(current != NULL){
        if(strcmp(current->name,candidateLine[0][0]) == 0){
          current->numVotes += atoi(candidateLine[0][1]);
          candidateExists = 1;
          break;
        }
        current = current->next;
      }
      if(candidateExists == 0){ // candidate doesn't exist in ancestor, add it to linked list
        struct candidate* newNode = malloc (sizeof(struct candidate));
        newNode->name = malloc(strlen(candidateLine[0][0]) + 1);
        strcpy(newNode->name,candidateLine[0][0]);
        newNode->numVotes = atoi(candidateLine[0][1]);
        newNode->next = candHead;
        candHead = newNode;
      }

      candidateExists = 0;
      strcpy(line,"");
      size = 1024;
      free(candidateLine);
    }
  }


  int outputfd;
  if ((outputfd = open(outputFilePath, O_CREAT | O_TRUNC | O_RDWR, 0666)) == -1) {
    perror("Error opening output file");
    return -1;
  }

  // write each candidate to output file
  struct candidate* current = candHead;
  char numVotesBuf[1024];
  while (current != NULL) {
    int nameLength = strlen(current->name);
    sprintf(numVotesBuf, "%d", current->numVotes);
    int voteLength = strlen(numVotesBuf);

    char* writeLine = (char*) malloc(sizeof(char)*(nameLength + voteLength+3));
    strcpy(writeLine, current->name);
    strcat(writeLine, ":");
    strcat(writeLine, numVotesBuf);
    strcat(writeLine, "\n");
    int bytesWritten = write(outputfd, writeLine, strlen(writeLine));
    if(bytesWritten < 0){
      //Error Handle: If failed writing into output file
      perror("ERROR: Failed writing into output file");
      return -1;
    }
    current = current->next;
  }


  // find if this is root node
  char* parentDir;
  char*** parentstrings = (char***)malloc(sizeof(outputFilePath));
  int parentnumStrings = makeargv(outputFilePath,"/",parentstrings);
  parentDir = findDir(outputdir,parentstrings[0][parentnumStrings-3]);

  if(strcmp(parentDir,outputdir) == 0){ // this is winner directory
    // add WINNER: line to winner file
    char* winner = findWinner(&candHead);
    char* writeLine;

    if(winner != NULL) {
      writeLine = (char*) malloc((strlen(winner)) + 9); // WINNER: + name
      strcpy(writeLine, "WINNER:");
      strcat(writeLine, winner);
    } else {
      writeLine = (char*) malloc(sizeof(char)*9);
      strcpy(writeLine, "WINNER:");
    }

    int bytesWritten = write(outputfd, writeLine, strlen(writeLine));
    if(bytesWritten < 0){
      //Error Handle: If failed writing into output file
      perror("ERROR: Failed writing into output file");
      return -1;
    }

    free(writeLine);
  }

  current = candHead;
  struct candidate* prev;
  while(current != NULL){
    free(current->name);
    prev = current;
    current = current->next;
    free(prev);
  }

  free(strings);
  free(outputFilePath);
  free(line);
  fclose(childfile);
  free(parentstrings);
  close(outputfd);
  aggregateVotes(parentDir,inputFile, outputdir);
}



void enqueue(char* name, int pBool, int priority) {
  struct node* newNode = (struct node*) malloc(sizeof(struct node));
  newNode->name = malloc(strlen(name)+1);
  strcpy(newNode->name,name);
  newNode->hasp = pBool;
  newNode->p = priority;
  //strcpy(newNode->name, name);
  newNode->next = NULL;
  newNode->prev = tail;
  if(head == NULL){
    head = newNode;
    tail = newNode;
  } else {
    tail->next = newNode;
    tail = newNode;
  }
}

// dequeue 1 filename
struct node* dequeue() {
  if(head == NULL){
    printf("ERROR: No node to dequeue\n");
    return NULL;
  }
  struct node* current = head;
  struct node* max_n = NULL;
  int max_p = INT_MAX;

  // find highest priority
  while(current != NULL) {
    if(current->hasp && current->p < max_p) {
      max_p = current->p;
      max_n = current;
    }
    current = current->next;
  }

// dequeue the highest priority node
  if(max_n != NULL) {
    if(max_n->prev == NULL) {
      head = head->next;
    } else
    if(max_n->next == NULL) {
      tail = max_n->prev;
      tail->next = NULL;
    } else {
      struct node* temp = max_n->prev;
      max_n->prev->next = max_n->next;
      max_n->next->prev = temp;
    }
    return max_n;
  } else {
    struct node* temp = head;
    head = head->next;
    return temp;
  }
}

// build shared queue
int queue_input(char* inputdir) {
  DIR* dir = opendir(inputdir);
  if(dir == NULL){
    perror("Unable to open directory");
    exit(0);
  }
  struct dirent *d;
  for (d = readdir(dir); d != NULL; d = readdir(dir)) {
    if(d->d_name[0] == '.') {
      continue;
    }
    if(d->d_type == DT_DIR) {
      continue;
    }

    int p;
    int hasp;
    numOfFiles++;
    char* priority_name = strstr(d->d_name, "_p_");
    if(priority_name != NULL) {
      hasp = 1;
      p = atoi(priority_name + 3);
    } else {
      p = 10;
      hasp = 0;
    }

    // enqueue each file in the input dir
    enqueue(d->d_name, hasp, p);

  }
  closedir(dir);
}


// decrypt a string
void decrypt(char* candidate) {
  for(int i = 0; i < strlen(candidate); i++){
    if((candidate[i] >= 'A' && candidate[i] <= 'X')
  || candidate[i] >= 'a' && candidate[i] <= 'x') {
    candidate[i] = (char)(candidate[i] + 2);
  } else if (candidate[i] == 'Y'){
    candidate[i] = 'A';
  } else if (candidate[i] == 'Z'){
    candidate[i] = 'B';
  } else if (candidate[i] == 'y'){
    candidate[i] = 'a';
  } else if (candidate[i] == 'z'){
    candidate[i] = 'b';
  }
  }
}

// update candidate list
int addCandidate(struct candidate** candHead, char* name){
  struct candidate* current = (*candHead);

  // candidate already exists in list
  // update its votes
  while (current != NULL){
    if(strcmp(current->name,name) == 0){
      current->numVotes++;
      return 1; // did not make a new candidate
    } else {
      current = current->next;
    }
  }

  // if candidate is not in list yet, add it
  struct candidate* newCandidate = (struct candidate*) malloc(sizeof(struct candidate));
  newCandidate->name = malloc(strlen(name) + 1);
  strcpy(newCandidate->name, name);
  newCandidate->numVotes = 1;
  newCandidate->next = (*candHead);
  (*candHead) = newCandidate;
  return 0; // made a new candidate
}


// run on each child thread
int childTask(void* arg){
  struct directories* dirs = (struct directories*)arg;

  // set up log file
  char logFile[PATH_MAX] = "";
  strcpy(logFile, dirs->outputdir);
  strcat(logFile, "/log.txt");

  pthread_t tid;
  tid = pthread_self();

  char logLine[FILENAME_MAX];

  char numThreadID[1024];
  sprintf(numThreadID, "%u", tid);
  int logfd;
  if ((logfd = open(logFile, O_WRONLY | O_APPEND, 0666)) == -1) {
    perror("Error opening log file");
    return -1;
  }

  // dequeue
  if(pthread_mutex_lock(&mutex) != 0) {
    perror("Unable to lock mutex");
    exit(0);
  }
  char* filename;
  char* outputFilePath;
  int bytesWritten;

  // keep dequeueing until an appropriate leaf file is obtained
  while(1) {
    struct node* qFile = dequeue();   //free this node

    // no more files in queue
    if(qFile == NULL) {
      if(pthread_mutex_unlock(&mutex) != 0) {
        perror("Unable to unlock mutex");
        exit(0);
      }
      pthread_exit(NULL);
      return -1;
    }

    filename = qFile->name;
    outputFilePath = findDir(dirs->outputdir, filename);
    if(outputFilePath == NULL) {
      printf("Error: Unable to find Leaf for inputfile in DAG. Picking new file from queue\n");
      continue;
    }

    // write to log file
    strcpy(logLine, filename);
    strcat(logLine, ":");
    strcat(logLine, numThreadID);
    strcat(logLine, ":");
    strcat(logLine, "start\n");
    bytesWritten = write(logfd, logLine, strlen(logLine));

    if(bytesWritten < 0){
      //Error Handle: If failed writing into output file
      perror("ERROR: Failed writing into log file");
      return -1;
    }
    break;
  }
  if(pthread_mutex_unlock(&mutex) != 0) {
    perror("Unable to unlock mutex");
    exit(0);
  }

  struct candidate* candHead = NULL;

  // open leaf file
  char* filepath = malloc(sizeof(char)*PATH_MAX);
  strcpy(filepath, dirs->inputdir);
  strcat(filepath,"/");
  strcat(filepath, filename);
  FILE* file;
  if((file = fopen(filepath, "r")) == NULL){
    perror("opening input dir");
    exit(0);
  }


  char* candidateVote = malloc(1024 * sizeof(char));
  strcpy(candidateVote,"");
  char buf[1024];


  // open output file in DAG
  strcat(outputFilePath,"/");
  strcat(outputFilePath,filename);
  strcat(outputFilePath,".txt");
  int outputfd;
  if ((outputfd = open(outputFilePath, O_CREAT | O_TRUNC | O_RDWR, 0666)) == -1) {
    perror("Error opening output file");
    return -1;
  }


// parse input file and put candidate votes into linked list

  int size = 1024;
  while((fgets(buf,1024,file)) != NULL){
    if(strchr(buf,'\n') == NULL) { // line longer than 1024, buf does not contain /n
      printf("WHOA LONG LINEEE\n");
      candidateVote = (char*)realloc(candidateVote, size+1024);
      size += 1024;
      strcat(candidateVote,buf);
      strcpy(buf, "");
      continue;
    } else {
      buf[strlen(buf)-1] = 0;
      strcat(candidateVote,buf);
      if(strcmp(candidateVote, "") == 0) {
        continue;
      }
      decrypt(candidateVote);
      addCandidate(&candHead,candidateVote);
      strcpy(candidateVote,"");
      size = 1024;
    }

  }


  // put candidate votes into output file
  struct candidate* current = candHead;
  char numVotesBuf[1024];
  while (current != NULL) {
    int nameLength = strlen(current->name);
    sprintf(numVotesBuf, "%d", current->numVotes);
    int voteLength = strlen(numVotesBuf);
    char* writeLine = malloc(sizeof(char)*(nameLength + voteLength+3));
    strcpy(writeLine, current->name);
    strcat(writeLine, ":");
    strcat(writeLine, numVotesBuf);
    strcat(writeLine, "\n");
    int bytesWritten = write(outputfd, writeLine, strlen(writeLine));
    if(bytesWritten < 0){
      //Error Handle: If failed writing into output file
      perror("ERROR: Failed writing into output file");
      return -1;
    }

    free(writeLine);
    current = current->next;
  }

  close(outputfd);


  // send results to ancestors

  // get parent directory and start aggregating votes
  char* parentDir;
  char*** strings = malloc(sizeof(outputFilePath));
  int numStrings = makeargv(outputFilePath,"/",strings);
  parentDir = findDir(dirs->outputdir,strings[0][numStrings-3]);
  if(pthread_mutex_lock(&aggregateMutex) != 0) {
    perror("Unable to lock mutex");
    exit(0);
  }
  aggregateVotes(parentDir,outputFilePath, dirs->outputdir);
  if(pthread_mutex_unlock(&aggregateMutex) != 0) {
    perror("Unable to unlock mutex");
    exit(0);
  }


  // add to log file
  if(pthread_mutex_lock(&mutex) != 0) {
    perror("Unable to lock mutex");
    exit(0);
  }
  strcpy(logLine, filename);
  strcat(logLine, ":");
  strcat(logLine, numThreadID);
  strcat(logLine, ":");
  strcat(logLine, "end\n");
  bytesWritten = write(logfd, logLine, strlen(logLine));
  if(bytesWritten < 0){
    //Error Handle: If failed writing into output file
    perror("ERROR: Failed writing into log file");
    return -1;
  }
  if(pthread_mutex_unlock(&mutex) != 0) {
    perror("Unable to unlock mutex");
    exit(0);
  }


  current = candHead;
  struct candidate* prev;
  while(current != NULL){
    free(current->name);
    prev = current;
    current = current->next;
    free(prev);
  }
  free(filepath);
  free(candidateVote);
  free(strings);
  // free(parentDir);  CAUSE OF SEGFAULT
  close(logfd);
}


int main (int argc, char** argv) {
  char* dag = malloc(FILENAME_MAX * sizeof(char));
  char* inputdir = malloc(FILENAME_MAX * sizeof(char));
  char* outputdir = malloc(FILENAME_MAX * sizeof(char));
  int num_threads;
  if(argc != 5){
    if (argc == 4) {
      num_threads = 4;
    } else {
      printf("Error: got %d args; expected 4 args\n", argc);
      return -1;
    }
  } else {
    num_threads = atoi(argv[4]);
  }


  dag = argv[1];
  inputdir = argv[2];
  outputdir = argv[3];

  if(create_dag(dag,outputdir)) {
    printf("error: empty dag file\n");
    return -1;
  }

  // build shared queue
  queue_input(inputdir);

  if(numOfFiles == 0) {
    printf("error: input directory is empty\n");
    return -1;
  }

  // set appropriate number of threads
  int numOfThreads = numOfFiles;
  if(numOfThreads > num_threads) {
    numOfThreads = num_threads;
  }

  pthread_t threads[numOfThreads];
  struct directories* dirs = malloc(sizeof(struct directories));
  dirs->inputdir = inputdir;
  dirs->outputdir = outputdir;

  pthread_mutex_init(&mutex, NULL);

  for(int i=0; i < numOfThreads; ++i) {
    pthread_create(&threads[i],NULL,childTask,(void*)dirs);
  }


  for(int i = 0; i < numOfThreads; ++i) {
		pthread_join(threads[i], NULL);
	}

}
