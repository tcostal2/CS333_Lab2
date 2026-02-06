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
#include <zlib.h>

#include "tarasaur.h"

#define COPY_BUF_SIZE 8192
struct node {
	tarasaur_directory_t hdr;
	struct node* next;
}node;

static void free_list(struct node* head){
	while(head){
		struct node* temp = head;
		head = head->next;
		free(temp);
	}
}

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
	//create archive
	if(action == ACTION_CREATE){
		int member_cnt =0;
		int mem_fd;
		int fd;
		const char* mem_name;
		size_t data_size;
		struct stat st;
		ssize_t n;
		unsigned char buffer[COPY_BUF_SIZE];
		off_t data_offset;
		uint32_t crc_data;
		char** members;
		off_t out =0;
		tarasaur_directory_t head;
		tarasaur_directory_t temp;
		tarasaur_directory_t* headers;

		version = TARASAUR_VERSION;
		member_cnt = argc - optind;

		members = malloc(member_cnt * sizeof(*members));
		headers = malloc(member_cnt * sizeof(*headers));

		for(int i = 0; i < member_cnt; ++i){
			members[i] = argv[optind +i];
		}
		

		if(filename){
			fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
			if(fd < 0){
				perror("open");
				exit(CREATE_FAIL);
			}
		}
		else{
			fd = STDOUT_FILENO;
		}
		
		//write the header, version, member count
		write(fd, TARASAUR_MAGIC_NUMBER, strlen(TARASAUR_MAGIC_NUMBER));
		out += strlen(TARASAUR_MAGIC_NUMBER);
		write(fd, &version, sizeof(version));
		out += sizeof(version);
		write(fd, &member_cnt, sizeof(member_cnt));
		out += sizeof(member_cnt);
		for (int i = 0; i < member_cnt; ++i){

			mem_name = members[i];
			mem_fd = open(mem_name, O_RDONLY);
			if(mem_fd < 0){
				perror(mem_name);
				exit(CREATE_FAIL);
			}
			if(fstat(mem_fd, &st) <0) {perror("fstat"); exit(CREATE_FAIL);}
			data_size = (size_t)st.st_size;

			//write the file size and offset
			write(fd, &data_size, sizeof(data_size));

			out += sizeof(data_size);

			data_offset = lseek(fd, 0, SEEK_CUR);
			if(data_offset == -1){
				data_offset = out;
			}
			
			//copy data members
			crc_data = crc32(0L, Z_NULL, 0);
			while((n= read(mem_fd, buffer, sizeof(buffer))) > 0) {
				ssize_t total =0;
				crc_data = crc32(crc_data, buffer, (uInt)n);
				while(total < n){
					ssize_t w =write(fd, buffer + total, n - total);
					if(w <= 0) {
					perror("write");
					exit(CREATE_FAIL);
					}
					total += w;
					out += w;
				}
				if (n < 0){
					perror("read");
					exit(CREATE_FAIL);
				}
			}
			if(n <0){
				perror("read");
				exit(CREATE_FAIL);
			}
			if(is_verbose){
				fprintf(stderr, "added: %s\n", mem_name);
			}
			memset(&head, 0, sizeof(head));
			strncpy(head.tarasaur_name, mem_name, TARASAUR_MAX_NAME_LEN -1);
			head.tarasaur_size = data_size;
			head.tarasaur_mode = st.st_mode;
			head.tarasaur_uid = st.st_uid;
			head.tarasaur_gid = st.st_gid;
			head.tarasaur_atim = st.st_atim;
			head.tarasaur_mtim = st.st_mtim;
			head.tarasaur_data_offset = data_offset;
			head.crc32_data = crc_data;
			temp = head;
			temp.crc32_data = 0;
			temp.crc32_header = 0;
			head.crc32_header = crc32(0L, (const Bytef *)&temp, (uInt)sizeof(temp));
			headers[i] = head;
			close(mem_fd);

		}
		for(int i =0; i< member_cnt; ++i){
			write(fd, &headers[i], sizeof(headers[i]));
			out += sizeof(headers[i]);
		}
		close(fd);
		free(headers);
		free(members);
		exit(EXIT_SUCCESS);

	}
	else{
		//if theres a filename set it to iarch
		if(filename){
			iarch = open(filename, O_RDONLY);
			if (iarch < 0){
				fprintf(stderr, "error msg 0\n");
				exit(CREATE_FAIL);
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

                 if(action == ACTION_EXTRACT){
                         struct node* head = NULL, *tail = NULL;
			 unsigned char buff[COPY_BUF_SIZE];
                         off_t end, start;
			if(read(iarch, &num_members, sizeof(num_members)) != sizeof(num_members)){
                         	exit(READ_FAIL);
                 	}

                         end = lseek(iarch, 0, SEEK_END);
                         if(end == (off_t)-1) exit(EXTRACT_FAIL);

                         start = end- (off_t)num_members *(off_t)sizeof(tarasaur_directory_t);
                         if (start <0) exit(EXTRACT_FAIL);

                         if(lseek(iarch, start, SEEK_SET) == (off_t)-1) {exit(EXTRACT_FAIL);}

                         for(int i = 0; i < num_members; i++){
                                 struct node * nd = calloc(1, sizeof(*nd));
                                 if(!nd){
                                         free_list(head);
                                         if(filename) close(iarch);
                                         exit(EXTRACT_FAIL);
                                 }
                         
                         if(read(iarch, &nd->hdr, sizeof(nd->hdr)) != sizeof(nd->hdr)){
                                 free(nd);
                                 free_list(head);
                                 if(filename) close(iarch);
                                 exit(EXTRACT_FAIL);
                         }
                         if(!head) head = tail = nd;
                         else { tail->next = nd; tail = nd;}
			 }

			 for(struct node *cur = head; cur; cur = cur->next){
				 int ofd;
				 ssize_t left, off, n, w, chunk;
				 struct timespec ts[2];
				 char name[TARASAUR_MAX_NAME_LEN +1];
				 tarasaur_directory_t *h = &cur->hdr;
				 memcpy(name, h->tarasaur_name, TARASAUR_MAX_NAME_LEN);
				 name[TARASAUR_MAX_NAME_LEN]= '\0';

				 ofd = open(name, O_WRONLY | O_CREAT | O_TRUNC, h->tarasaur_mode);
				 if(ofd < 0) {
					 free_list(head);
					 if(filename) close(iarch);
					 exit(EXTRACT_FAIL);
				 }

				 if(lseek(iarch, h->tarasaur_data_offset, SEEK_SET) == (off_t)-1){
					 close(ofd);
					 free_list(head);
					 if(filename) close(iarch);
					 exit(EXTRACT_FAIL);
				 }

				left = h->tarasaur_size;
				while(left > 0){
					chunk = MIN(left, COPY_BUF_SIZE);
					n = read(iarch, buff, chunk);
					if(n <= 0){
						close(ofd);
						free_list(head);
						if(filename) close(iarch);
						exit(EXTRACT_FAIL);
					}

					off = 0; 
					while(off < n){
						w = write(ofd, buff + off, (size_t)(n-off));
						if(w <=0){
						close(ofd);
						free_list(head);
						if(filename) close(iarch);
						exit(EXTRACT_FAIL);
						}
						off += w;
					}
					left -= (size_t)n;
				}

				(void)fchmod(ofd, h->tarasaur_mode);
				close(ofd);
				ts[0] = h->tarasaur_atim;
				ts[1] = h->tarasaur_mtim;
				(void)utimensat(AT_FDCWD, h->tarasaur_name, ts, 0);
			 }
			 free_list(head);
			 if(filename) close(iarch);

		 }


		 if(action == ACTION_VALIDATE){
			 struct node* head = NULL, *tail = NULL;
			 off_t end, start;
			 unsigned char v_buf[COPY_BUF_SIZE];

			 if(read(iarch, &num_members, sizeof(num_members)) != sizeof(num_members)){
				 if(filename) close(iarch);
				 exit(VALIDATE_ERROR);
			 }
			 end = lseek(iarch, 0, SEEK_END);
			 if(end == (off_t)-1){
				 if(filename) close(iarch);
				 exit(VALIDATE_ERROR);
			 }

			 start = end - (off_t)num_members* (off_t)sizeof(tarasaur_directory_t);
			 if(start <0){
				 if(filename) close(iarch);
				 exit(VALIDATE_ERROR);
			 }
			 if(lseek(iarch, start, SEEK_SET) == (off_t)-1){
				 if(filename) close(iarch);
				 exit(VALIDATE_ERROR);
			 }
			 for(int i = 0; i < num_members; i++){
				 struct node * nd = calloc(1, sizeof(*nd));
				 if(!nd){
					 free_list(head);
					 if(filename) close(iarch);
					 exit(VALIDATE_ERROR);
				 }
				 if(read(iarch, &nd->hdr, sizeof(nd->hdr)) != sizeof(nd->hdr)){
					 free(nd);
					 free_list(head);
					 if(filename) close(iarch);
					 exit(VALIDATE_ERROR);
				 }
				 if(!head) head = tail = nd;
				 else { tail->next = nd; tail = nd;}
			 }

			 for(struct node* cur = head; cur; cur = cur->next){
				 uint32_t crc_d, stored_hdr, crc32_hdr;
				 size_t left;
				 size_t stored_size = 0;
				 tarasaur_directory_t dir = cur->hdr;
				 if(read(iarch, &stored_size, sizeof(stored_size)) != sizeof(stored_size)){
					 free_list(head);
					 if(filename) close(iarch);
					 exit(VALIDATE_ERROR);
				 }
				 if(stored_size != dir.tarasaur_size){
					 free_list(head);
					 if(filename) close(iarch);
					 exit(VALIDATE_ERROR);
				 }
				 crc_d = crc32(0L, Z_NULL, 0);
				 left = dir.tarasaur_size;
				 while (left > 0){
					 ssize_t n = read(iarch, v_buf, MIN(left, COPY_BUF_SIZE));
					 if (n <= 0){
						 free_list(head);
						 if(filename) close(iarch);
						 exit(VALIDATE_ERROR);
					 }
					 crc_d = crc32(crc_d, (Bytef*)v_buf, (uInt)n);
					 left -= (size_t)n;
				 }
				 if(crc_d != cur->hdr.crc32_data){
					 free_list(head);
					 if(filename) close(iarch);
					 exit(VALIDATE_ERROR);
				 }
				 stored_hdr = cur->hdr.crc32_header;
				 dir.crc32_data =0;
				 dir.crc32_header =0;
				 crc32_hdr = crc32(0L, (Bytef *)&dir, (uInt)sizeof(dir));
				 if(crc32_hdr != stored_hdr){
					 free_list(head);
					 if(filename) close(iarch);
					 exit(VALIDATE_ERROR);
				 }
			 }
			 free_list(head);
			 if(filename) close(iarch);
		 }

		 //small TOC and large TOC reading  
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
					 printf("\t\tmode:\t" );
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
					 printf("\t\tuser:\t\t%s\n", userid->pw_name);
					 //group
					 groupid = getgrgid(header.tarasaur_gid);
					 if(groupid == NULL) {printf("\tgroup: \t%d\n", header.tarasaur_gid);}
					 printf("\t\tgroup:\t\t%s\n", groupid->gr_name);
					 //size
					 printf("\t\tsize:\t\t%zu\n", header.tarasaur_size);
					 //mtime and atime
					 tm_mtime = localtime(&header.tarasaur_mtim.tv_sec);
					 strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S %Z", tm_mtime);
					 printf("\t\tmtime:\t\t%s\n", time_buf);
					 tm_atime = localtime(&header.tarasaur_atim.tv_sec);
					 strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S %Z", tm_atime);
					 printf("\t\tatime:\t\t%s\n", time_buf);
					 //crc32 checksums
					 printf("\t\tcrc32 header:\t\t0x%08x\n", header.crc32_header);
					 printf("\t\tcrc32 data:\t\t0x%08x\n", header.crc32_data);

				 }
			 }
			 if (filename){
				 close(iarch);
			 }
		 }
	}
	return EXIT_SUCCESS;

}
