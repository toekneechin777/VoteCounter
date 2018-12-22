/*
 * VCforStudents.c
 *
 *  Created on: Feb 2, 2018
 *      Author: ayushi
 */

 /*login: chen4714 champlin42
 * date: 02/21/18
 * name: Anthony Chen, Samira Champlin
 * id: 5220235, 5071604 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include "makeargv.h"

#define MAX_NODES 100

int NUM_CANDIDATES;
char* NUM_CANDIDATES_STRING;
int NUM_NODES;
char** CANDIDATE_NAMES;

//Function signatures

/**Function : parseInput
 * Arguments: 'filename' - name of the input file
 * 			  'n' - Pointer to Nodes to be allocated by parsing
 * Output: Number of Total Allocated Nodes
 * About parseInput: parseInput is supposed to
 * 1) Open the Input File [There is a utility function provided in utility handbook]
 * 2) Read it line by line : Ignore the empty lines [There is a utility function provided in utility handbook]
 * 3) Call parseInputLine(..) on each one of these lines
 ..After all lines are parsed(and the DAG created)
 4) Assign node->"prog" ie, the commands that each of the nodes has to execute
 For Leaf Nodes: ./leafcounter <arguments> is the command to be executed.
 Please refer to the utility handbook for more details.
 For Non-Leaf Nodes, that are not the root node(ie, the node which declares the winner):
 ./aggregate_votes <arguments> is the application to be executed. [Refer utility handbook]
 For the Node which declares the winner:
 This gets run only once, after all other nodes are done executing
 It uses: ./find_winner <arguments> [Refer utility handbook]
 */
int parseInput(char *filename, node_t *n){
	// open intpu.txt
	FILE* file = file_open(filename);
	if(file == NULL){
		printf("Error: Incorrect or no file \n");
		return -1;
	}
	char* buf = malloc(sizeof(char) * 1024);
	int line = 1;
	// read all lines, send to parseInputLine
	buf = read_line(buf,file);
	if(buf == NULL){  // check if file empty
		printf("Error: empty file or empty first line\n");
		return -1;
	}
	while(buf != NULL){
		parseInputLine(buf, n);
		buf = read_line(buf,file);
	}

 	// assign executable names to nodes
	int numOfNodes = NUM_NODES;
	for(int i = 0; i < numOfNodes; i++){
		if (n[i].id == 0) {
			strcpy(n[i].prog,"find_winner");

		} else if (n[i].num_children > 0){ // not a leaf
			strcpy(n[i].prog, "aggregate_votes");

		} else {  // leaf!
			strcpy(n[i].prog, "leafcounter");

		}
	}

	// check for cycles in DAG
	int isCyclic = cycleCheck(n);
	if(isCyclic == 1){
		printf("ERROR: %s contains a cyclic dependency graph\n", filename);
		exit(1);
	}
	free(buf);
	return 1;

}

// prepare for cycle check
int cycleCheck(node_t* n) {
	int visited[NUM_NODES];
	int recursion[NUM_NODES];
	for(int i = 0; i < NUM_NODES; i++){
		// set everything in both arrays to -1
		// -1 indicates false, no cycle
		visited[i] = -1;
		recursion[i] = -1;
	}

	return cycleBackEdgeCheck(n, 0, visited, recursion);

}

// cycle check
int cycleBackEdgeCheck(node_t* n, int iD, int* visited, int* recursion) {
	if(visited[iD] == -1) {
		// current node becomes visited and kept track in recursion array.
		visited[iD] = 1;
		recursion[iD] = 1;

		// check each child for cycle
		for(int i = 0; i < n[iD].num_children; i++){
			int childID = n[iD].children[i];
			// If we reach a node that is already in the recursion[] array then, there is a cycle
			// visited array is for checking if it has been visited. If true then  there is a cycle.
			if(visited[childID] == -1 && cycleBackEdgeCheck(n, childID, visited, recursion) == 1) {
				return 1;
			} else if(recursion[childID] == 1){
				return 1;
			}
		}
	}
	recursion[iD] = -1;
	return 0;
}

/**Function : parseInputLine
 * Arguments: 's' - Line to be parsed
 * 			  'n' - Pointer to Nodes to be allocated by parsing
 * Output: Number of Region Nodes allocated
 * About parseInputLine: parseInputLine is supposed to
 * 1) Split the Input file [Hint: Use makeargv(..)]
 * 2) Recognize the line containing information of
 * s(You can assume this will always be the first line containing data).
 * You may want to store the 's information
 * 3) Recognize the line containing "All Nodes"
 * (You can assume this will always be the second line containing data)
 * 4) All the other lines containing data, will show how to connect the nodes together
 * You can choose to do this by having a pointer to other nodes, or in a list etc-
 * */
int parseInputLine(char *s, node_t *n){
	//Split the string by spaces
	char*** strings = (char***) malloc(sizeof(s));

	// split line by spaces and trim off white space
	int numOfStrings = makeargv(s, " ", strings);
	for(int i = 0; i < numOfStrings; i++){
		strings[0][i] = trimwhitespace(strings[0][i]);
	}

	//Recognize the line containing candidates (starts with an int)
	char* first_word = strings[0][0];
	int i = 0;
	int isNumber = 1;
	// check if first string in the line is an int
	while(first_word[i] != NULL){
		if (first_word[i] < '0' || first_word[i] > '9'){
			isNumber = 0;
			break;
		}
		i++;
	}

	if(isNumber){  // first line
		// save number and names of candidates
		NUM_CANDIDATES_STRING = strings[0][0];
		int numOfCandidates = atoi(strings[0][0]);
		NUM_CANDIDATES = numOfCandidates;
		CANDIDATE_NAMES = (char**)malloc(numOfCandidates * sizeof(char*));
		for (int i = 0; i < numOfCandidates; i++){
			CANDIDATE_NAMES[i] = (char*)malloc(1024 * sizeof(char)); // malloc space for candidate name
			CANDIDATE_NAMES[i] = strings[0][i+1]; // add candidate name to CANDIDATE_NAMES
		}
	}

	//Recognize the line containing all node names
	if(!isNumber){ // first string not a number -> not the first line
		if(strings[0][1][0] != ':'){  // second string not ":" -> this is second line
			// create all nodes
			for(int i = 0; i < numOfStrings; i++){
				strcpy(n[i].name, strings[0][i]);    // set name
				n[i].id = i;                         // set id
				n[i].num_children = 0;               // set num_children
				char originalName[1024];             // set output
				strcpy(originalName, n[i].name);
				char outputName[1024];
				prepend(n[i].name,"Output_");
				strcpy(outputName, n[i].name);
				strcpy(n[i].name, originalName);
				strcpy(n[i].output, outputName);
			}

			NUM_NODES = numOfStrings;

		} else {  // not second line and not first line, but dependency lines
			//All other lines
			node_t* currentNode;
			char* currentNodeName = strings[0][0];

			//find current node in node array by name
			for(int j = 0; j < NUM_NODES; j++){
					if(strcmp(n[j].name, currentNodeName) == 0){
							currentNode = &n[j];
					}

			}

			// Node that starts the line
			currentNode->num_children = (numOfStrings - 2);

			// Each of the nodes following ":"
			// loop over each child in dependency line
			for (int i = 2; i < numOfStrings; i++){
				for(int j = 0; j < NUM_NODES; j++){   // search for child node matching name in input.txt
					if(strcmp(n[j].name, strings[0][i]) == 0){
						currentNode->children[i-2] = n[j].id;
						break;
					}
				}
			}
		}
	}
	free(strings);
}


/**Function : execNodes
 * Arguments: 'n' - Pointer to Nodes to be allocated by parsing
 * About execNodes: parseInputLine is supposed to
 * If the node passed has children, fork and execute them first
 * Please note that processes which are independent of each other
 * can and should be running in a parallel fashion
 * */
void execNodes(node_t *n, int iD) {

	// check if current node is parent node
	if (n[iD].num_children > 0) {
		// fork once for each child
		for(int i = 0; i < (n[iD].num_children); i++){
			if((n[iD].pid = fork()) == 0){ // child process
				int childID = n[iD].children[i];
				execNodes(n, childID);   // recurse execNodes on child
			} else if (n[iD].pid < 0){  // error
				printf("ERROR: fork failure\n");
				exit(1);
			}
		}
		if(n[iD].pid > 0) { // parent process, has children
			while (n[iD].pid = waitpid(-1,NULL,0)) {  // wait for all children to finish
					if (errno == ECHILD) { // ECHILd = no children left
						break;
					}
					// create input array from all children's outputs
					for(int i = 0; i < (n[iD].num_children); i++){
						int childID = n[iD].children[i];
						strcpy(n[iD].input[i], n[childID].output);
					}
			}

			// create params list matching aggregate_votes and find_winner
			int numOfParams = 5 + NUM_CANDIDATES + (n[iD].num_children);
			char* paramList[numOfParams];
			paramList[0] = n[iD].prog;
			// get num_children as a string
			switch(n[iD].num_children) {
				case 1 :
					paramList[1] = "1";
					break;
				case 2 :
					paramList[1] = "2";
					break;
				case 3 :
					paramList[1] = "3";
					break;
				case 4 :
					paramList[1] = "4";
					break;
				case 5 :
					paramList[1] = "5";
					break;
				case 6 :
					paramList[1] = "6";
					break;
				case 7 :
					paramList[1] = "7";
					break;
				case 8 :
					paramList[1] = "8";
					break;
				case 9 :
					paramList[1] = "9";
					break;
				case 10 :
					paramList[1] = "10";
					break;
			}
			int j;
			for(j = 2; j < (n[iD].num_children + 2); j++){
				paramList[j] = n[iD].input[j-2];
			}
			paramList[j] = n[iD].output;
			paramList[j+1] = NUM_CANDIDATES_STRING;
			for(int k = j+2; k < (NUM_CANDIDATES + (j+2)); k++){
				paramList[k] = CANDIDATE_NAMES[k-j-2];
			}
			paramList[numOfParams-1] = (char*) NULL;

			// execute aggregate_votes or find_winner
			if((execv(n[iD].prog,paramList) == -1)) {
				printf("ERROR: EXECV FAIL\n");
			}
	}

} else {  // leaf node, no children
		// create params list matching leafcounter
		strcpy(n[iD].input, n[iD].name);
		int numOfParams = 5 + NUM_CANDIDATES;
		char* paramList[numOfParams];
		paramList[0] = n[iD].prog;
		paramList[1] = n[iD].input;
		strcpy(paramList[1] , n[iD].input);
		paramList[2] = n[iD].output;
		paramList[3] = NUM_CANDIDATES_STRING;
		for(int j = 4; j < (NUM_CANDIDATES + 4); j++){
			paramList[j] = CANDIDATE_NAMES[j-4];
		}
		paramList[numOfParams-1]  = (char*)NULL;
		// execute leafcounter
		if((execv(n[iD].prog,paramList) == -1)) {
			printf("ERROR: EXECV FAIL LEAF\n");
		}
	}
}


int main(int argc, char **argv){

	//Allocate space for MAX_NODES to node pointer
	struct node* mainnodes=(struct node*)malloc(sizeof(struct node)*MAX_NODES);

	if (argc != 2){
		printf("Usage: %s Program\n", argv[0]);
		return -1;
	}

	//call parseInput
	int num = parseInput(argv[1], mainnodes);

	//Call execNodes on the root node
	if(num == 1){
		execNodes(mainnodes, 0);
	} else if(num == -1){
		printf("Aborting: Error in parseInput function\n");
		exit(1);
	} else {
		printf("Unknown error: parseInput returns unknown number\n");
	}

	free(mainnodes);
	for(int i = 0; i < NUM_CANDIDATES; i++){
		free(CANDIDATE_NAMES[i]);
	}
	free(CANDIDATE_NAMES);
	return 0;
}
