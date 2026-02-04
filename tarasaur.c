#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>

#include "tarasaur.h"


int main(int argc, char *argv[]){
	
	char* filename = NULL;
	bool is_verbose = false;
	int iarch = -1; 
	int opt = 0;
	tarasaur_action_t action = ACTION_NONE;
	while ((opt = getopt(argc, argv, TARASAUR_OPTIONS)) != -1){
		switch (opt) {
			case 'x':
				if(action != ACTION_NONE){
					fprintf(stderr, "multiple actions selected\n");
					exit(INVALID_CMD_OPTION);
				}
				action = ACTION_EXTRACT;
				break;
			case 'c':
				if(action != ACTION_NONE){
					fprintf(stderr, "multiple actions selected\n");
					exit(INVALID_CMD_OPTION);
				}
				action = ACTION_CREATE;
				break;
			case 't':
				if(action != ACTION_NONE){
					fprintf(stderr, "multiple actions selected\n");
					exit(INVALID_CMD_OPTION);
				}
				action  = ACTION_TOC_SHORT;
				fprintf(stderr, "action is toggled to: %d\n", action);
				break;
			case 'T':
				if(action != ACTION_NONE){
					fprintf(stderr, "multiple actions selected\n");
					exit(INVALID_CMD_OPTION);
				}
				action = ACTION_TOC_LONG;
				break;
			case 'V':
				if(action != ACTION_NONE){
					fprintf(stderr, "multiple actions selected\n");
					exit(INVALID_CMD_OPTION);
				}
				action = ACTION_VALIDATE;
				break;
			case 'f':
				filename = optarg;
				if(filename){
					fprintf(stderr, "filenameset to: %s\n", filename);
				}
				else{
					fprintf(stderr, "no filename\n");
				}
				break;
			case 'h':
				fprintf(stderr, "list of helpful things\n");
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				is_verbose = true;
				fprintf(stderr, "verbose is set to: %d\n", is_verbose);
				break;
			default:
				fprintf(stderr, "invalid option\n");
				exit(INVALID_CMD_OPTION);
				break;
		}
	}
	if(action == ACTION_NONE){
		fprintf(stderr, "no action selected\n");
		exit(NO_ACTION_GIVEN);
	}
	return EXIT_SUCCESS;
}
