#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

// Structure for every node
typedef struct node{
	char name[1024];
	char prog[1024];
	char input[50][1024];
	char output[1024];
	int children[10];
	int num_children;
	int status;
	pid_t pid;
	int id;
}node_t;

FILE* file_open(char* file_name);
char* read_line(char* buffer, FILE* fp);
int makeargv(const char*s, const char *delimiters, char ***argvp){

	int error;
	int i;
	int numtokens;
	const char *snew;
	char *t;

	if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)){

		errno = EINVAL;
		return -1;

	}

	*argvp = NULL; // already assigned as a new var, just blanking out

	snew = s + strspn(s, delimiters);

	if ((t = malloc(strlen(snew) + 1)) == NULL)
		return -1;

	strcpy(t, snew);

	numtokens = 0;

	if (strtok(t, delimiters) != NULL) // count number of tokens in s
		for (numtokens = 1; strtok(NULL, delimiters) != NULL; numtokens++);

	// create arg array for pointers to tokens
	if ((*argvp = malloc((numtokens + 1)*sizeof(char *))) == NULL){
		error = errno;
		free(t);
		errno = error;
		return -1;
	}

	// insert pointers to tokens into the arg array
	if (numtokens == 0)
		free(t);

	else{
		strcpy(t, snew);
		**argvp = strtok(t, delimiters);
		for(i = 1; i < numtokens; i++)
			*((*argvp) + i) = strtok(NULL, delimiters);
	}

	*((*argvp) + numtokens) = NULL; // put in final NULL pointer

	return numtokens;
}

char *trimwhitespace(char *str)
{
  char *end;
  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;

  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}



node_t* findnode(node_t* start, char* tobefound){
	//Find the node in question
		node_t* temp = start;
		while(temp->id!=NULL){
			if( (strcmp(temp->name, tobefound)==0)){
				return temp;
			}

			temp++;
		}
		return NULL;
}

node_t* findNodeByID(node_t* start, int tobefound){
	//Find the node in question
		node_t* temp = start;
		while(temp->id!=NULL){
			if( temp->id == tobefound){
				return temp;
			}

			temp++;
		}

		return NULL;
}


char* prepend(char* s, const char* t)
{
    size_t len = strlen(t);
    size_t i;

    memmove(s + len, s, strlen(s) + 1);

    for (i = 0; i < len; ++i)
    {
        s[i] = t[i];
    }
    return s;
}

void printgraph(node_t* mainnodes, int num){
			int p;
				for (p = 0; p < num; p++){
					if(mainnodes[p].num_children==0){
						printf("%s","\n Leaf Node : ");
						printf("%s",mainnodes[p].name);
					}
					else{
						printf("\n Non-Leaf Nodes %s",mainnodes[p].name);
						printf("\n Listing their children: ");
						int x;
						for(x=0;x<mainnodes[p].num_children;x++){
							printf("\n %d ",mainnodes[p].children[x]);
							printf(" %s ",(findNodeByID(mainnodes, mainnodes[p].children[x])->name));
						}

					}
				}
}
