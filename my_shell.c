#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "my_pipe.h"

#define CMD_NUM 16
#define CMD_SIZE 32

void shell_error(char* funct);
int parse_cmds(char* cmd, char commands[][CMD_SIZE]);
char* append_filename(char* str, char* path, char* name);

int my_execvpe(char* full, char* args[]);
void execute(char* args[], char* envp[]);
void prepare_to_execute(char* args[], char* envp[], int in, int out);

int separate_commands(int total, char commands[][CMD_SIZE], int size, char* args[]);
int get_arguments(char* temp[], int size, char* args[]);

int has_pipe(char* args[], int used, int pip[2]);
int has_bg(char commands[][CMD_SIZE], int size);

void create_processes(char* buffer, char commands[][CMD_SIZE], char* envp[]);

int parse_cmds(char* cmd, char commands[][CMD_SIZE])
{
	int count = 0;
	while(*cmd != 0)
	{
		int num = 0;
		char buffer[CMD_SIZE] = {0};
		while(strchr("\t ", *cmd) != NULL)
			cmd++;
		if(strchr("<>|&", *cmd) != NULL)
			buffer[num++] = *cmd++;
		else
		{
			while(strchr("<>|&\t ", *cmd) == NULL)
				buffer[num++] = *cmd++;
		}
		strcpy(commands[count++], buffer);
	}
	return count;

}

void shell_error(char* funct)
{
	fprintf(stderr, "Error has occurred in %s.\n", funct);
	exit(-1);
}


char* append_filename(char* str, char* path, char* name)
{	
    sprintf(str, "%s%c%s", path, '/', name);
    return str;
}


int my_execvpe(char* full, char* args[])
{
    struct stat access;
    if (stat(full, &access) == 0)
        if(execvpe(full, args, NULL) == -1)
            shell_error("execvpe");
    return 0;
}

void execute(char* args[], char* envp[])
{
	if(my_execvpe(args[0], args) == 0)
	{	
		for (int i = 0; envp[i] != 0; i++)
		{
			if(strncmp(envp[i], "PATH=", 5) == 0)
			{
				char* tok = strtok(strchr(envp[i], '/'), ":");
				while(tok != NULL)
				{
					char full[128] = "";
					append_filename(full, tok, args[0]); 
					if (my_execvpe(full, args) == 0)
						tok = strtok(NULL, ":");
				}
			}
		}
		printf("-my_shell: %s: command not found\n", args[0]);
	}
}
void prepare_to_execute(char* args[], char* envp[], int in, int out)
{
	int pid = fork();
	switch(pid)
	{
		case -1: 
			shell_error("fork()");
			break;
		case 0:
			open_streams(in, out);
			execute(args, envp);
			break;
		default:
			close_streams(in, out);
			if (wait(0) == -1)
				shell_error("wait()");
	}
}


int separate_commands(int total, char commands[][CMD_SIZE], int size, char* args[])
{
	int used = 0;
	for(int i = total; i < size; i++)
	{
		*args++ = commands[i];
		used++;
		if (!(strcmp(commands[i], "|")))
			break;
	}
	return used;
}

int get_arguments(char* temp[], int size, char* args[])
{
	int i = 0;
	for (; i < size; i++)
	{
		if(!(strcmp(temp[i], "|")) || !(strcmp(temp[i], ">")) || !(strcmp(temp[i], "<")))
			break;
		*args++ = temp[i];
	}
	*args = 0;
	return i;;
}

int has_pipe(char* args[], int used, int pip[2])
{
	return (!(strcmp(args[used - 1], "|"))) ? pip[0] : 0;
}

int has_bg(char commands[][CMD_SIZE], int size)
{
	return (!(strcmp(commands[size - 1], "&"))) ? 1 : 0;
}

void print_args(char* args[], int num)
{
	for(int i = 0; i < num; i++)
		printf("%s ", args[i]);
	printf("\n");
}
void print_args2(char* args[])
{
	while(*args != 0)
		printf("%s ", *args++);
		if(*args == 0)
			printf("null");
	printf("\n");
}

void create_processes(char* buffer, char commands[][CMD_SIZE], char* envp[])
{
	int size = parse_cmds(buffer, commands);
	
	int pip[2], total = 0, pipe_in = 0;
	switch(fork())
	{
		case -1:
			shell_error("fork()");
		case 0:
			while(total != size)
			{
				if(pipe(pip) == -1)
					shell_error("pipe()");
				//printf("pip : %d, %d\n", pip[0], pip[1]);
				char* temp[CMD_SIZE];
				char* args[CMD_SIZE];
				int used = separate_commands(total, commands, size, temp);
				get_arguments(temp, used, args); //NULL TERMINATED

				//print_args(temp, used);
				//print_args2(args);
				int in = get_stdin(temp, used, pipe_in); //get instream and outstream.
				int out = get_stdout(temp, used, pip);
				//printf("%d : %d\n", in, out);
				prepare_to_execute(args, envp, in, out);
				pipe_in = has_pipe(temp, used, pip);	
				//printf("[%d]\n", pipe_in);
				total += used;
			}
			exit(1);

		default:
			if(!has_bg(commands, size))
				if (wait(0) == -1)
					shell_error("wait()");
	}
}	

int main(int argc, char* argv[], char* envp[])
{
	char buffer[128];
	while(1)
	{
		char commands[CMD_NUM][CMD_SIZE];
		
		printf("%c ", '%');
		fflush(stdout);
		
		if(fgets(buffer, sizeof(buffer), stdin) == NULL || !(strcmp(buffer, "quit\n")))
			break;
		buffer[strcspn(buffer, "\n")] = 0;
		create_processes(buffer, commands, envp);
	}
	return 0;

}
