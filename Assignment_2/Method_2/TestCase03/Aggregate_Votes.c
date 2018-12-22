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

typedef struct Candidate {
  char name[1024];
  int numOfVotes;
  struct Candidate* next;
};


int aggregate_votes(char* path){

  DIR *d = opendir(path);
  if(d == NULL) {
    // Error Handle: Failed to open directory
    perror("ERROR: Failed to open directory at path");
    return -1;
  }

  struct dirent* ent;

  pid_t childpid;
  int subDirs = 0;
  int isParent = 0;

  // examine all subdirectories
  while((ent = readdir(d)) != NULL){
    if(strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0) {
      continue;
    }

    if(ent->d_type == DT_DIR) {
      subDirs++;
      childpid = fork();
      if(childpid < 0) {
        // Error Handle: Failed to fork
        perror("ERROR: Forking error in Aggregate_Votes");
        return -1;
      } else if (childpid == 0){
        // child runs aggregate_votes on current subdirectory
        fclose(stdout);
        char* pathName = (char*)malloc(1024);
        strcpy(pathName, path);
        strcat(pathName, "/");
        strcat(pathName, ent->d_name);
        aggregate_votes(pathName);
        free(pathName);
      } else {
        isParent = 1;
        // wait for all children
        while(childpid = waitpid(-1,NULL,0)) {
          if (errno == ECHILD){
            break;
          }
        }
      }
    }
  }

  // handle current directory after examining its subdirectories
  if(subDirs == 0) { // this is a leaf
      if(execl("Leaf_Counter","Leaf_Counter",path,(char*)NULL) == -1){
        // Error Handle: Failed to execute Leaf_Counter
        perror("ERROR: Leaf_Counter exec failure");
        return -1;
      }
  } else {
      // not a leaf, collect data from each subdirectory
      DIR *d = opendir(path);
      struct dirent* ent;
      struct Candidate* head = NULL;
      struct Candidate* current = head;
      int candidateExists = 0;

      while((ent = readdir(d)) != NULL){
        if(strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0
        || ent->d_type == DT_REG || ent->d_type == DT_LNK) {
          continue;
        }

        // get candidate count from each output file of this directory's subdirectories

        // get path of desired output file of the current directory's subdirectory
        char* pathName = (char*)malloc(1024);
        strcpy(pathName,path);
        strcat(pathName, "/");
        strcat(pathName, ent->d_name);
        strcat(pathName,"/");
        strcat(pathName, ent->d_name);
        strcat(pathName,".txt");

        FILE *fp = fopen(pathName, "r");
        if(fp == NULL) {
          // Error Handle: Failed to open file
          perror("ERROR: Incorrect or no file");
          return -1;
        }
        free(pathName);

        char *buf = (char*)malloc((sizeof(char) * 1024));
        buf = fgets(buf, 1024, fp);
        if(buf == NULL) { // Error Handle: if file empty continue to other subdirectory output file
          printf("WHATTTTTTTT\n");
          continue;
        }
        buf[strlen(buf) - 1] = 0;

        // split up string into sections of candidate:votes
        char*** strings = (char***) malloc(sizeof(buf));
        int numstrings = makeargv(buf,",",strings);

        for(int i = 0; i < numstrings; i++) { // go through all candidates in file
          current = head;
          candidateExists = 0;
          char*** candidateStrings = (char***)malloc(sizeof(*strings[i])); // how much space per each candidate entry?
          int entry = makeargv(strings[0][i],":",candidateStrings);

          // add data for next candidate
          while(current != NULL) {
            if(strcmp(current->name, candidateStrings[0][0]) == 0){
              candidateExists = 1;
              break;
            }
            current = current->next;
          }
          if(candidateExists){
            // add votes from this node
            current->numOfVotes += atoi(candidateStrings[0][1]);
          } else { // make new linked list entry
            struct Candidate* newCandidate = (struct Candidate*) malloc(sizeof(struct Candidate));
            strcpy(newCandidate->name, candidateStrings[0][0]);
            newCandidate->numOfVotes = atoi(candidateStrings[0][1]);
            // append to linked list
            newCandidate->next = head;
            head = newCandidate;
          }
          free(candidateStrings);
        }
        free(buf);
        free(strings);
        fclose(fp);
      }

      // make output file
      char*** strings = (char***) malloc(sizeof(path));
      char filepath[MAX_FILE_NAME_SIZE];
      strcpy(filepath,path);
      int n = makeargv(path,"/",strings);
      char* outputName = malloc(sizeof(char)*1024);
      strcpy(outputName, strings[0][n-1]); // last entry in strings should be current folder name
      strcat(outputName,".txt");
      strcat(filepath,"/");
      strcat(filepath,outputName);
      if(isParent){
        printf("%s\n",filepath);
      }

      int fd;
      if ((fd = open(filepath, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1) {
        // Error Handle: Failed to open new output file
        perror("ERROR: Failed opening output file");
        return -1;
      }
      current = head;
      char* buf = malloc(sizeof(char)*1024);
      char numVotesBuf[1024];

      // write each candidate to output file
      while (current != NULL){
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

      // Free dynamically allocated variables
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
      closedir(d);
      close(fd);
;
    }
    closedir(d);
    return 1;
  }




int main(int argc, char** argv){
  if(argc != 2){
    printf("Error: got %d args; expected 1 args\n", argc);
    return -1;
  }
  if(aggregate_votes(argv[1]) == -1){
    printf("Error running Aggregate_Votes. Aborting.\n");
    exit(0);
  }

}
