#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "tarasaur.h"


int main(int argc, char *argv[]){
	
	char* filename = NULL;
	char buf [sizeof(TARASAUR_MAGIC_NUMBER) *2] = {'\0'};
	bool is_verbose = false;
	int iarch = -1; 
	int opt = 0;
	tarasaur_action_t action = ACTION_NONE;
	unsigned short version = 0;
	int num_members = 0;

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
	//validate correct actions
	if(action == ACTION_NONE){
		fprintf(stderr, "no action selected\n");
		exit(NO_ACTION_GIVEN);
	}
	//if theres a filename set it to iarch
	if(filename){
		iarch = open(filename, O_RDONLY);
		if (iarch < 0){
			fprintf(stderr, "error msg 0\n");
			exit(EXIT_FAILURE);
		}
	}
	else{
		iarch = STDIN_FILENO;
	}

	//validate magic number
	read(iarch, buf, strlen(TARASAUR_MAGIC_NUMBER));
	if(strncmp(buf, TARASAUR_MAGIC_NUMBER, 
				strlen(TARASAUR_MAGIC_NUMBER)) != 0){
		fprintf(stderr, "Magic number doesnt match 1\n");
		exit(BAD_MAGIC);
	}
	//validate version
	read(iarch, &version, sizeof(version));
	if(version != TARASAUR_VERSION){
		fprintf(stderr, "version mismatch 2\n");
		exit(BAD_MAGIC);
	}
	//read the datamembers 
	if(action == ACTION_TOC_SHORT){
		read(iarch, &num_members, sizeof(num_members));

		for(int i = 0; i < num_members; ++i){
			size_t data_size = 0;
			read(iarch, &data_size, sizeof(data_size));
			if(is_verbose){
				fprintf(stderr, "\tskip over data %d of %zd bytes\n", i, data_size);
			}
			lseek(iarch, data_size, SEEK_CUR);
		}
		if(filename){
			printf("Table of contents of tarannosaurus file:"
					"\"%s\" with %d members\n",
					filename,
					num_members);
		}
		else{
			printf("Table of contents tarannosaurus from" "stdin with %d members\n", num_members);
		}
		for (int i =0; i <num_members; ++i){
			char buffer[TARASAUR_MAX_NAME_LEN *2] ={'\0'};
			tarasaur_directory_t header;

			memset(buffer, 0, sizeof(buffer));
			read(iarch, &header, sizeof(tarasaur_directory_t));
			strncpy(buffer, header.tarasaur_name, TARASAUR_MAX_NAME_LEN);
			printf("\tfile name: %s\n", buffer);
		}
		if (filename){
			close(iarch);
		}
	}

	return EXIT_SUCCESS;
}
