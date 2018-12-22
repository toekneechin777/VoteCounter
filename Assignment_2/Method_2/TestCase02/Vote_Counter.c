/*login: champ142, chen4714
date: 03/08/18
name: Samira Champlin, Anthony Chen
id: 5071604, 5220235 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include "util.h"
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>

// linked list of candidates
typedef struct Candidate {
  char name[1024];
  int numOfVotes;
  struct Candidate* next;
};

typedef struct Cycle {
  char source[1024];
  char target[1024];
  struct Cycle* next;
};

struct Cycle* detectCycle(char*, struct Cycle*);

int count_votes(char* path){

  // fork and run Aggregate_Votes on path
  pid_t childpid;
  childpid = fork();
  if(childpid < 0) {
    //Error Handle: If Fork fails
    perror("ERROR: fork failure");
    return -1;
  } else if(childpid == 0) {
    fclose(stdout);
    if(execl("Aggregate_Votes","Aggregate_Votes",path,(char*)NULL) == -1){
      //Error Handle: If Exec of Aggregate_Votes fails
      perror("ERROR: Aggregate exec failure");
      return -1;
    }
  } else {
    // wait for all children
    while(childpid = waitpid(-1,NULL,0)) {
      if (errno == ECHILD){
        break;
      }
    }
  }

  // create path for winner output file
  char*** strings = (char***) malloc(sizeof(path));
  char winnerFilePath[MAX_FILE_NAME_SIZE];
  strcpy(winnerFilePath,path);
  int n = makeargv(path,"/",strings);
  char* winnerFile = malloc(sizeof(char)*1024);
  strcpy(winnerFile, strings[0][n-1]);
  strcat(winnerFile,".txt");
  strcat(winnerFilePath,"/");
  strcat(winnerFilePath,winnerFile);


  FILE* file = fopen(winnerFilePath, "r");
  if(file == NULL){
    //Error Handle: If file open failed
    perror("ERROR: Unable to open file in Vote_Counter");
    return -1;
  }

  // separate candidates in winner output file
  char *buf = (char*)malloc((sizeof(char) * 1024));
  buf = fgets(buf, 1024, file);
  if(buf == NULL){    //Error Handle: If file opened is empty
    perror("ERROR: No votes aggregated. No possible winner");
    return -1;
  }
  buf[strlen(buf) - 1] = 0;
  char*** nodeStrings = (char***) malloc(sizeof(buf));
  int numstrings = makeargv(buf,",",nodeStrings);
  struct Candidate* head = NULL;

  // create linked list for candidates
  for(int i = 0; i < numstrings; i++) {
    // separate each candidate into name:votes
    char*** candidateStrings = (char***)malloc(sizeof(*nodeStrings[i]));
    int entry = makeargv(nodeStrings[0][i],":",candidateStrings);
    struct Candidate* newCandidate = (struct Candidate*) malloc(sizeof(struct Candidate));
    strcpy(newCandidate->name, candidateStrings[0][0]);
    newCandidate->numOfVotes = atoi(candidateStrings[0][1]);
    newCandidate->next = head;
    head = newCandidate;
    free(candidateStrings);
  }


  // find highest vote count
  struct Candidate* current = head;
  int largest_votes = -10;
  char winner[1024];
  while(current != NULL){
    if(current->numOfVotes > largest_votes) {
      largest_votes = current->numOfVotes;
      strcpy(winner, current->name);
    }
    current = current->next;
  }

  // create line to be written to winner file
  char* writeLine = malloc(sizeof(char)*1024);
  strcpy(writeLine, "Winner:");
  strcat(writeLine, winner);
  strcat(writeLine, "\n");
  printf("%s\n",winnerFilePath);

  int fd;
  if ((fd = open(winnerFilePath,O_RDWR)) == -1) {
    //Error Handle: If failed opening the outputfile
    perror("ERROR: Failed opening output file");
    return -1;
  }
  // write winner line to end of winner file
  if(lseek(fd, 0, SEEK_END) == -1) {
    //Error Handle: If failed to offset output file
    perror("ERROR: Offset of the pointer failed");
    return -1;
  }
  int bytesWritten = write(fd,writeLine,strlen(writeLine));
  if(bytesWritten < 0){
    //Error Handle: If failed writing into output file
    perror("ERROR: Failed writing into output file");
    return -1;
  }

  // extra credit
  // get linked list of all cycle information
  struct Cycle* headCyc = NULL;
  struct Cycle* currentCyc = detectCycle(path,headCyc);
  // add all cycle information to winner file
  while(currentCyc != NULL){
    if(lseek(fd, 0, SEEK_END) == -1) {
      //Error Handle: If failed to offset output file
      perror("ERROR: Offset of the pointer failed");
      return -1;
    }

    // create line to be written
    // "There is a cycle from " +name + " to " + name + \n =
    //  22+1024+4+1024+1 = 2075
    char buf[2075];
    strcpy(buf,"There is a cycle from ");
    strcat(buf,currentCyc->source);
    strcat(buf," to ");
    strcat(buf,currentCyc->target);
    strcat(buf,"\n");
    // write to winner file
    int bytesWritten = write(fd,buf,strlen(buf));
    if(bytesWritten < 0){
      //Error Handle: If failed writing into output file
      perror("ERROR: Failed writing into output file");
      return -1;
    }
    currentCyc = currentCyc->next;
  }

  // free dynamically allocted memory
  current = head;
  struct Candidate* removal = head;
  while(current != NULL){
    current = current->next;
    free(removal);
    removal = current;
  }
  free(strings);
  free(nodeStrings);
  free(winnerFile);
  free(writeLine);
  free(buf);
  return 1;
}


// extra credit
struct Cycle* detectCycle(char* path, struct Cycle* head) {
  DIR* dir = opendir(path);
  if(dir == NULL){
    perror("Error opening directory");
    exit(0);
  }

  struct dirent* ent;
  struct stat st;

  if(lstat(path,&st) == -1){
    perror("lstat");
    exit(0);
  }

  // get name of node that symlink is in
  char*** strings = (char***) malloc(sizeof(path));
  char name[MAX_FILE_NAME_SIZE];
  int n = makeargv(path,"/",strings);
  strcpy(name,strings[0][n-1]);
  char pathName[MAX_FILE_NAME_SIZE];

  while((ent = readdir(dir)) != NULL){
    // if subdirectory found, recurse through it
    if(ent->d_type == DT_DIR){
      if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0)){
        continue;
      }
      char filePath[MAX_FILE_NAME_SIZE];
      strcpy(filePath,path);
      strcat(filePath,"/");
      strcat(filePath,ent->d_name);
      head = detectCycle(filePath,head);
    } else if (ent->d_type == DT_LNK){
      struct Cycle* newCycle = (struct Cycle*)malloc(sizeof(struct Cycle));

      // get path of current subdirectory (path of symlink)
      strcpy(pathName,path);
      strcat(pathName,"/");
      strcat(pathName,ent->d_name);

      // read path that symlink points to
      char buf[1024];
      ssize_t bufsize = 1024;
      int bytesRead = readlink(pathName,buf,bufsize);
      if(bytesRead == -1){
        perror("readlink");
        exit(EXIT_FAILURE);
      }

      // clear leftover bytes in buf
      buf[(int)bytesRead] = '\0';
      // set cycle's source
      strcpy(newCycle->source,name);
      // get the name of the node that symlink points to
      char*** targetstrings = (char***) malloc(sizeof(pathName));
      int x = makeargv(buf,"/",targetstrings);
      // set target of cycle
      strcpy(newCycle->target,targetstrings[0][x-1]); // second to last string is name
      newCycle->next = head;
      head = newCycle;

    }
  }

  return head;

}

int main(int argc, char** argv){
  if(argc != 2){
    printf("Error: got %d args; expected 2 args\n", argc);
    return -1;
  }

  if(count_votes(argv[1]) == -1){
    printf("Error running Vote_Counter. Aborting.\n");
    exit(0);
  }

/*
  struct Cycle* head = NULL;
  struct Cycle* ec = detectCycle(argv[1],head); */
}
