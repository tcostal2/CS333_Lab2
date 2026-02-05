#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

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
	struct passwd* userid;
	struct group* groupid;
	struct tm* tm_mtime;
	struct tm* tm_atime;
	char time_buf[80];

	while ((opt = getopt(argc, argv, TARASAUR_OPTIONS)) != -1){
		switch (opt) {
			case 'x':
				action = ACTION_EXTRACT;
				break;
			case 'c':
				action = ACTION_CREATE;
				break;
			case 't':
				action  = ACTION_TOC_SHORT;
				break;
			case 'T':
				action = ACTION_TOC_LONG;
				break;
			case 'V':
				action = ACTION_VALIDATE;
				break;
			case 'f':
				filename = optarg;
				break;
			case 'h':
				printf("Usage: tarasaur -[cxtTVf:vh] archive-file file...\n");
				printf("\t-c\tcreate a new archive file\n");
				printf("\t-x\textract members from an existing archive file\n");
				printf("\t-t\tshort table of contents of archive file\n");
				printf("\t-T\tlong table of contents of archive file\n");
				printf("\t-V\tvalidate the checksum/hash values\n");
				printf("\t-f filename\tname of archive file to use\n");
				printf("\t-v\tverbose output\n");
				printf("\t-h\tshow help text\n");
						
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				is_verbose = true;
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
	//small TOC reading  
	if(action == ACTION_TOC_SHORT || action == ACTION_TOC_LONG){
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
			if(is_verbose) {printf("Reading archive file: " " %s\n", filename);}
			printf("Table of contents of tarannosaurus file: "
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
			//logic for large TOC 
			if(action == ACTION_TOC_LONG){
				//file permissions
				printf("\tmode:\t" );
				printf("%c", S_ISDIR(header.tarasaur_mode) ? 'd' : '-');
				printf("%c", (header.tarasaur_mode & S_IRUSR) ? 'r' : '-');
				printf("%c", (header.tarasaur_mode & S_IWUSR) ? 'w' : '-');
				printf("%c", (header.tarasaur_mode & S_IXUSR) ? 'x' : '-');

				printf("%c", (header.tarasaur_mode & S_IRGRP) ? 'r' : '-');
				printf("%c", (header.tarasaur_mode & S_IWGRP) ? 'w' : '-');
				printf("%c", (header.tarasaur_mode & S_IXGRP) ? 'x' : '-');
				
				printf("%c", (header.tarasaur_mode & S_IROTH) ? 'r' : '-');
				printf("%c", (header.tarasaur_mode & S_IWOTH) ? 'w' : '-');
				printf("%c", (header.tarasaur_mode & S_IXOTH) ? 'x' : '-');
				printf("\n");
				//userID
				userid = getpwuid(header.tarasaur_uid);
				if(userid == NULL) {printf("\tuser: \t%d\n", header.tarasaur_uid);}
				printf("\tuser: \t%s\n", userid->pw_name);
				//group
				groupid = getgrgid(header.tarasaur_gid);
				if(groupid == NULL) {printf("\tgroup: \t%d\n", header.tarasaur_gid);}
				printf("\tgroup: \t%s\n", groupid->gr_name);
				//size
				printf("\tsize: \t%zu\n", header.tarasaur_size);
				//mtime and atime
				tm_mtime = localtime(&header.tarasaur_mtim.tv_sec);
				strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S %Z", tm_mtime);
				printf("\tmtime: \t%s\n", time_buf);
				tm_atime = localtime(&header.tarasaur_atim.tv_sec);
				strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S %Z", tm_atime);
				printf("\tatime: \t%s\n", time_buf);
				//crc32 checksums
				printf("\tchecksum header: \t0x%08x\n", header.crc32_header);
				printf("\tchecksum header: \t0x%08x\n", header.crc32_data);

			}
		}
		if (filename){
			close(iarch);
		}
	}

	return EXIT_SUCCESS;
}
