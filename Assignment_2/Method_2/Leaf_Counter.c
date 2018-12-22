#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include "util.h"
//#include <makeargv.h>

typedef struct Candidate {
  char name[1024];
  int numOfVotes;
  struct Candidate* next;
};

int parseInput(char* path){
  struct dirent *DirEntry;
  printf("Path: %s\n", path);
  DIR* directory = opendir(path);
  if(directory == NULL){
    printf("error: no directory stream \n");
    exit(1);
  }

  /* Use for aggregate votes: Check if has subdirectory
  int hasSubDir = 0;
  while((DirEntry = readdir(directory)) != NULL) {
    if(DirEntry->d_name[0] == '.') {  // will include directories begin w/ .*?
      continue;
    }
    if(d->d_type == DT_DIR) {
      hasSubDir = 1;
    }
  }
  */
  printf("SEGFAULT 1\n");
  char filepath[MAX_FILE_NAME_SIZE];
  strcpy(filepath, path);
  strcat(filepath, "/"); // assume no trailing slash
  strcat(filepath, "votes.txt");

  printf("SEGFAULT 2\n");
  struct stat buffer;
  if(stat(filepath, &buffer) < 0){
    return -1;
  }
  FILE* file = file_open(filepath);
  if(file == NULL){
    printf("Not a leaf node. \n");
    return -1;
  }

  printf("SEGFAULT 3\n");
  struct Candidate* head = NULL;
  printf("SEGFAULT 3\n");
  struct Candidate* current = head;
  printf("SEGFAULT 3\n");
  int candidateExists = 0;
  printf("SEGFAULT 3.1\n");
  char* candidateVote = malloc(sizeof(char)*1024);
  candidateVote = read_line(candidateVote, file);
  while(candidateVote != NULL) {
    candidateExists = 0;
    candidateVote[strlen(candidateVote)-1] = 0;
    printf("Candidate Vote: %s\n", candidateVote);
    printf("SEGFAULT 3.2\n");
    //traverse linked list and check if vote for candidate has already existed.
    current = head;
    while(current != NULL) {
      printf("SEGFAULT 3.3\n");
      if(strcmp(current->name, candidateVote) == 0){
        printf("Candidate Exists!\n");
        candidateExists = 1;
        break;
      }
      current = current->next;
    }
    printf("SEGFAULT 4\n");
    //if candidate already voted for, update its number of votes
    if(candidateExists){
      current->numOfVotes = current->numOfVotes + 1;
    } else {
      //otherwise push it onto linked list
      printf("Candidate Doesn't Exist\n");
      struct Candidate* newCandidate = (struct Candidate*) malloc(sizeof(struct Candidate));
      newCandidate->numOfVotes = 1;
      strcpy(newCandidate->name, candidateVote);

      //if linked list is empty, push as first one
      if(head == NULL) {
        newCandidate->next = NULL;
        head = newCandidate;
      } else {
        //if linked list contains something, push to the head of linked list
        // newCandidate->next = head->next;
        // head = newCandidate;
        current = head;
        while((current->next) != NULL){
          current = current->next;
        }
        newCandidate->next = NULL;
        current->next = newCandidate;
      }
    }
    printf("SEGFAULT 5\n");
    candidateVote = read_line(candidateVote,file);

  }

  printf("1st Node: %s\n", head->name);
  printf("1st Node Votes: %d\n", head->numOfVotes);
  printf("2nd Node: %s\n", head->next->name);
  printf("2nd Node Votes: %d\n", head->next->numOfVotes);
  printf("3rd Node: %s\n", head->next->next->name);
  printf("3rd Node Votes: %d\n", head->next->next->numOfVotes);
  printf("4th Node: %s\n", head->next->next->next->name);
  printf("4th Node Votes: %d\n", head->next->next->next->numOfVotes);

  printf("SEGFAULT 6, PATH: %s\n", path);
  // create output file
  char*** strings = (char***) malloc(sizeof(path));
  strcpy(filepath,path);
  printf("SEGFAULT 6, PATH: %s\n", path);
  int n = makeargv(path,"/",strings);
  char* outputName = malloc(sizeof(char)*1024);
  printf("SEGFAULT 6, strings: %s\n", strings[0][n-1]);
  strcpy(outputName, "/");
  strcat(outputName, strings[0][n-1]); // last entry in strings should be current folder name
  strcat(outputName,".txt");
  printf("output name: %s\n",outputName);
  strcat(filepath,outputName);
  printf("filepath in leafcounter: %s\n",filepath);


  int fd;
  if ((fd = open(filepath,O_CREAT | O_RDWR)) == -1) {
    perror("error opening output file");
    return 0;
  }
  current = head;
  char* buf = malloc(sizeof(char)*1024);
  char numVotesBuf[1024];
  while (current != NULL){
    printf("SEGFAULT 7.1\n current name: %s\n", current->name);
    strcpy(buf,current->name);
    printf("BUF: %s\n", buf);
    strcat(buf,":");
    printf("BUF: %s\n", buf);
    printf("BUF: %s\n", buf);
    sprintf(numVotesBuf, "%d", current->numOfVotes);
    strcat(buf, numVotesBuf);
    printf("BUF: %s\n", buf);
    // strcat(buf,current->numOfVotes);
    if(current->next != NULL) {
      strcat(buf, ",");   // WE NEED TO END THE LINE IN A \n
    }
    printf("BUF: %s\n", buf);
    int bytesWritten = write(fd,buf,strlen(buf));
    printf("Bytes Written: %d\n", bytesWritten);
    current = current->next;
  }

  printf("SEGFAULT 8\n");
  printf("%s\n", filepath);
  return 1;

}

int main(int argc, char **argv){
  if(argc != 2){
    printf("Error: Incorrect Usage: %s Program; expected 1 args\n", argv[0]);
		return -1;
  }

  int num = parseInput(argv[1]);
  if(num == -1){
    printf("%s is not a leaf node. \n", (char*)argv[1]);
  } else {

  }
}
