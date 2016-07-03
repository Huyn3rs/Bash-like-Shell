#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
FILE* file_error()
{
    fprintf(stderr, "Error has occurred opening file.\n");
}

int in_redirect(char* file)
{
    int infile = open(file, O_RDONLY);
	if(infile == -1)
		file_error();
    return infile;
}

int out_redirect(char* file)
{
    int outfile = open(file, O_CREAT|O_WRONLY|O_TRUNC, 0666);
	if(outfile == -1)
		file_error();
    return outfile;
}

void open_streams(int in, int out)
{
	if (in != -1)
		dup2(in, 0);
	if (out != -1)
		dup2(out, 1);
}

void close_streams(int in, int out)
{
	if (in != -1)
		close(in);
	if (out != -1)
		close(out);
}


int get_stdin(char* temp[], int used, int pipe)
{
	if(pipe)
		return pipe;
	for(int i = 0; i < used; i++)
	{
		if(!(strcmp(temp[i], "<")))
			return in_redirect(temp[i + 1]);
	}
	return -1;
}

int get_stdout(char* temp[], int used, int pip[2])
{
	for(int i = 0; i < used; i++)
	{
		if(!(strcmp(temp[i], ">")) && (i + 1) < used)
			return out_redirect(temp[i + 1]);
		else if(!(strcmp(temp[i], "|")))
			return pip[1];
	}
	return -1;
}
