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
  printf("RUNNING AGGR VOTES on %s\n",path);
  DIR *d = opendir(path);
  struct dirent* ent;

  pid_t childpid;
  int subDirs = 0;
  while((ent = readdir(d)) != NULL){
    if(strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0) {
      continue;
    }

    if(ent->d_type == DT_DIR) {
      subDirs++;
      childpid = fork();
      if(childpid < 0) {
        // ERROR
      } else if (childpid == 0){
        char* pathName = (char*)malloc(1024);
        strcpy(pathName, path);
        strcat(pathName, "/");
        strcat(pathName, ent->d_name);
        aggregate_votes(pathName);
        free(pathName);
      } else {
        // wait for all children
        while(childpid = waitpid(-1,NULL,0)) {
          if (errno == ECHILD){
            break;
          }
        }
      }
    }

  }
  printf("went through directories and forking for %s\n",path);

  if(subDirs == 0) { // this is a leaf
      if(execl("Leaf_Counter","Leaf_Counter",path,(char*)NULL) == -1){
        perror("Leaf exec failure");
      }
    } else {
      DIR *d = opendir(path);
      struct dirent* ent;
      struct Candidate* head = NULL;
      struct Candidate* current = head;
      int candidateExists = 0;

      while((ent = readdir(d)) != NULL){
        if(strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0 || ent->d_type == DT_REG) {
          continue;
        }

        // get candidate count from each output file
        char* pathName = (char*)malloc(1024);
        printf("current path in aggr votes: %s\n",path);
        strcpy(pathName,path);
        strcat(pathName, "/");
        strcat(pathName, ent->d_name);
        strcat(pathName,"/");
        strcat(pathName, ent->d_name);
        strcat(pathName,".txt");
        printf("output file being read in aggr votes: %s\n", pathName);
        FILE *fp = file_open(pathName);

        char *buf = (char*)malloc((sizeof(char) * 1024));
        buf = read_line(buf,fp);
        // split up string into sections of candidate:votes
        char*** strings = (char***) malloc(sizeof(buf));
        int numstrings = makeargv(buf,",",strings);
        printf("reading canddidates\n");
        for(int i = 0; i < numstrings; i++) { // go through each candidate
          current = head;
          candidateExists = 0;
          printf("setup successful, strings[i]: %s\n", strings[0][i]);
          char*** candidateStrings = (char***)malloc(sizeof(*strings[i])); // how much space per each candidate entry?
          int entry = makeargv(strings[0][i],":",candidateStrings);
          printf("successfully split string\n");
          // add data for next candidate
          while(current != NULL) {
            printf("candidate name: %s\n",candidateStrings[0][0]);
            if(strcmp(current->name, candidateStrings[0][0]) == 0){
              printf("Candidate Exists in aggr votes!\n");
              candidateExists = 1;
              break;
            }
            current = current->next;
          }
          if(candidateExists){
            current->numOfVotes += atoi(candidateStrings[0][1]);
          } else { // make new linked list entry
          struct Candidate* newCandidate = (struct Candidate*) malloc(sizeof(struct Candidate));
          strcpy(newCandidate->name, candidateStrings[0][0]);
          newCandidate->numOfVotes = atoi(candidateStrings[0][1]);
          // append to linked list
          newCandidate->next = head;
          head = newCandidate;
        }
        }
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
      printf("output name for aggr votes: %s\n",filepath);

      int fd;
      if ((fd = open(filepath,O_CREAT | O_RDWR)) == -1) {
        perror("error opening output file");
        return 0;
      }
      current = head;
      char* buf = malloc(sizeof(char)*1024);
      char numVotesBuf[1024];

      while (current != NULL){
        strcpy(buf,current->name);
        strcat(buf,":");
        sprintf(numVotesBuf, "%d", current->numOfVotes);
        strcat(buf, numVotesBuf);
        // strcat(buf,current->numOfVotes);
        if(current->next != NULL) {
          strcat(buf, ",");   // WE NEED TO END THE LINE IN A \n
        }
        int bytesWritten = write(fd,buf,strlen(buf));
        current = current->next;
      }
      strcpy(buf,"\n");
      int bytesWritten = write(fd,buf,strlen(buf));
    }

    return 1;
  }




int main(int argc, char** argv){
  if(argc != 2){
    printf("Error: got %d args; expected 1 args\n", argc);
    return -1;
  }
aggregate_votes(argv[1]);

}
