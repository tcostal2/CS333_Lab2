#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#include "tarasaur.h"


int main(int argc, char *argv[]){
	
	int opt = 0;
	while ((opt = getopt(argc, argv, TARASAUR_OPTIONS)) != -1){
		switch (opt) {
			case 'x':
				break;
			case 'c':
				break;
			case 't':
				break;
			case 'T':
				break;
			case 'V':
				break;
			case 'f':
				break;
			case 'h':
				break;
			case 'v':
				break;
			default:
				fprintf(stderr, "invalid option\n");
				exit(EXIT_FAILURE);
				break;
		}
	}
				

	return EXIT_SUCCESS;
}
