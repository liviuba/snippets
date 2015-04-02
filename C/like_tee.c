/*****
	The Linux Programming Interface, pag87, ex4-1
	Like 'tee'.
	Reads from stdin, writes to stdout and argv[1]
	***/

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>

// Try not to do many syscalls
// TODO : what happens if stdin stops getting data for a while (buffer empties out ), but didn't actually stop?
const size_t BUF_SIZE = 512;

void BadStuffHappened(char* msg){
	printf("%s",msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]){
	int out_fd;
	char* buf;
	ssize_t bytes_read;	//s(igned)size -- to represent -1 in case of falure
	int opt; // not char, to handle -1

	
	if(argc > 3) BadStuffHappened("Usage A: like_tee [-p] <filename>\n");
	opt = getopt(argc, argv, "p");
	if( opt != 'p' && opt != -1) BadStuffHappened("Usage B: like_tee [-p] <filename>\n");

	if(opt == 'p')
		out_fd = open( argv[2], O_RDWR | O_CREAT | O_APPEND,
								S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );	//(rw-)
	else
			out_fd = open( argv[1], O_RDWR | O_CREAT,
								S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );	//(rw-)


	if(out_fd == -1)
		BadStuffHappened("Cannot open argv[1]!\n");

	buf = malloc(BUF_SIZE);
	if( buf == NULL)
		BadStuffHappened("malloc\n");
	
	for( bytes_read = read( STDIN_FILENO, buf, BUF_SIZE ); bytes_read != 0; bytes_read = read( STDIN_FILENO, buf, BUF_SIZE )){
		if(bytes_read == -1)
			BadStuffHappened("You just broke stdin");

		if( write(out_fd, buf, bytes_read) != bytes_read )
			BadStuffHappened("Can't write to output file.");

		if( write( STDOUT_FILENO, buf, bytes_read ) != bytes_read )
			BadStuffHappened("You broke stdout");
	}

	free(buf);
	close(out_fd);

	return 0;
}
