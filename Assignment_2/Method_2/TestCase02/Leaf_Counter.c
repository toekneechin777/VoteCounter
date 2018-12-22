/*login: champ142, chen4714
date: 03/08/18
name: Samira Champlin, Anthony Chen
id: 5071604, 5220235 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include "util.h"

// linked list of candidates
typedef struct Candidate {
  char name[1024];
  int numOfVotes;
  struct Candidate* next;
};

int parseInput(char* path){
  struct dirent *DirEntry;
  DIR* directory = opendir(path);
  if(directory == NULL){
    //Error Handle: If no such directory
    perror("No directory stream");
    return -1;
  }


  // open votes.txt within leaf node
  char filepath[MAX_FILE_NAME_SIZE];
  strcpy(filepath, path);
  strcat(filepath, "/"); // assume no trailing slash
  strcat(filepath, "votes.txt");

  struct stat buffer;
  if(stat(filepath, &buffer) < 0){
    return -2;
  }
  FILE* file = fopen(filepath, "r");
  if(file == NULL){
    perror("ERROR: Failed to open file containing votes");
    return -1;
  }


  struct Candidate* head = NULL;
  struct Candidate* current = head;
  int candidateExists = 0;
  char* candidateVote = malloc(sizeof(char)*1024);
  candidateVote = fgets(candidateVote, 1024, file);

  // create linked list
  while(candidateVote != NULL) { //makes sure its not empty or reaches till empty
    candidateExists = 0;
    candidateVote[strlen(candidateVote)-1] = 0;
    // traverse linked list and check if candidate exists in linked list
    current = head;
    while(current != NULL) {
      if(strcmp(current->name, candidateVote) == 0){
        candidateExists = 1;
        break;
      }
      current = current->next;
    }
    // if candidate already exists in linked list, update its number of votes
    if(candidateExists){
      current->numOfVotes = current->numOfVotes + 1;
    } else {
      //otherwise push it onto linked list
      struct Candidate* newCandidate = (struct Candidate*) malloc(sizeof(struct Candidate));
      newCandidate->numOfVotes = 1;
      strcpy(newCandidate->name, candidateVote);

      if(head == NULL) {
        // linked list is empty, push first node
        newCandidate->next = NULL;
        head = newCandidate;
      } else {
        // linked list not empty, push to the head of linked list
        current = head;
        while((current->next) != NULL){
          current = current->next;
        }
        newCandidate->next = NULL;
        current->next = newCandidate;
      }
    }
    candidateVote = fgets(candidateVote, 1024, file);

  }

  // create output file
  char*** strings = (char***) malloc(sizeof(path));
  strcpy(filepath,path);
  int n = makeargv(path,"/",strings);
  char* outputName = malloc(sizeof(char)*1024);
  strcpy(outputName, "/");
  strcat(outputName, strings[0][n-1]); // last entry in strings should be current folder name
  strcat(outputName,".txt");
  strcat(filepath,outputName);


  int fd;
  if ((fd = open(filepath, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1) {
    perror("Error opening output file");
    return -1;
  }

  // write each candidate:votes to the output file
  current = head;
  char* buf = malloc(sizeof(char)*1024);
  char numVotesBuf[1024];
  while (current != NULL){
    //printf("SEGFAULT 7.1\n current name: %s\n", current->name);
    strcpy(buf,current->name);
    strcat(buf,":");
    sprintf(numVotesBuf, "%d", current->numOfVotes);
    strcat(buf, numVotesBuf);

    if(current->next != NULL) {
      strcat(buf, ",");
    } else {
      strcat(buf, "\n");
    }
    int bytesWritten = write(fd,buf,strlen(buf));
    if(bytesWritten < 0){
      //Error Handle: If failed writing into output file
      perror("ERROR: Failed writing into output file");
      return -1;
    }
    current = current->next;
  }

  printf("%s\n", filepath);


  // free dynamically allocated memory
  current = head;
  struct Candidate* removal = head;
  while(current != NULL){
    current = current->next;
    free(removal);
    removal = current;
  }
  free(buf);
  free(strings);
  free(outputName);
  free(candidateVote);
  fclose(file);
  close(fd);
  closedir(directory);
  return 1;

}

int main(int argc, char **argv){
  if(argc != 2){
    printf("Error: Incorrect Usage: %s Program; expected 1 args\n", argv[0]);
		return -1;
  }

  int result = parseInput(argv[1]);

  if(result == -2){
    printf("Not a leaf node.\n");
  } else if (result == -1) {
    printf("Error running Leaf_Counter. Aborting.\n");
  }
}
