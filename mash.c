/***********************************************************************
 * Copyright (c) 2020
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "types.h"
#include "parser.h"

#include <unistd.h>
#include <sys/wait.h> 
#include <signal.h>

/*======================================================================*/
/*          ****** DO NOT MODIFY ANYTHING FROM THIS LINE ******         */
/**_
 * String used as the prompt (see @main()). You may change this to
 * change the prompt */
static char __prompt[MAX_TOKEN_LEN] = "$";

/**
 * Time out value. It's OK to read this value, but SHOULD NOT CHANGE
 * IT DIRECTLY. Instead, use @set_timeout() function below.
 */
static unsigned int __timeout = 2;

static void set_timeout(unsigned int timeout)
{
	__timeout = timeout;

	if (__timeout == 0) {
		fprintf(stderr, "Timeout is disabled\n");
	} else {
		fprintf(stderr, "Timeout is set to %d second%s\n",
				__timeout,
				__timeout >= 2 ? "s" : "");
	}
}
/*          ****** DO NOT MODIFY ANYTHING UP TO THIS LINE ******      */
/*====================================================================*/


/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

pid_t pid,c_pid;
static char name[MAX_TOKEN_LEN];
static int run_command(int nr_tokens, char *tokens[])
{
	/* This function is all yours. Good luck! */
	int status;
	strcpy(name, tokens[0]);

	if (strncmp(tokens[0], "exit", strlen("exit")) == 0) {
		return 0;
	} 

	else if (strncmp(tokens[0], "prompt", strlen("prompt")) == 0) {
		if(nr_tokens == 2){
        	strcpy(__prompt, tokens[1]);
			return 1;
		}
		else return -1;
    }
	
	else if (strncmp(tokens[0], "/bin/pwd", strlen("/bin/pwd")) == 0) {
		char buf[MAX_TOKEN_LEN];
		getcwd(buf,MAX_TOKEN_LEN);
		fprintf(stderr, "%s\n", buf);
		return 1;
	}

	else if (strncmp(tokens[0], "cd", strlen("cd")) == 0) {
        if (nr_tokens == 1 || strncmp(tokens[1], "~", strlen("~")) == 0){
			chdir(getenv("HOME"));
			return 1;
		}
		else{
			if(chdir(tokens[1])==-1) return -1;
			else return 1;
		}
	
    }
	else if (strncmp(tokens[0], "timeout", strlen("timeout")) == 0) {
		if (nr_tokens ==1){
       		fprintf(stderr, "Current timeout is %d second\n",__timeout);
			return 1;
		}
		else if(nr_tokens ==2){
			set_timeout(atoi(tokens[1]));
			return 1;
		}
		else return -1;
    }
	else if (strncmp(tokens[0], "for", strlen("for")) == 0){
		int for_cnt = 1;
		int start =0;
		for_cnt *= atoi(tokens[1]);
	
		for(int i=2; i<nr_tokens; i+=2){
			if(strncmp(tokens[i], "for", strlen("for")) == 0){
				for_cnt *= atoi(tokens[i+1]);
			}
			else{
				start = i;	
				break;
			}
		}
		for(int i=0; i<for_cnt; i++){
			run_command(nr_tokens,tokens+start);
		}		
		return 1;
	}
	else {
		void signal_handler(int signal_number);
		struct sigaction sa;
		sa.sa_handler= signal_handler;
		sa.sa_flags = 0;
		sigaction(SIGALRM,&sa,0);

		if ((pid = fork()) == 0){
			if(execvp(tokens[0],tokens) == -1){
				close(0);
				fprintf(stderr, "No such file or directory\n");
				exit(EXIT_FAILURE);
			}
		} 

		else {
			c_pid = pid;
			alarm(__timeout);
			waitpid(c_pid, &status, 0);
			alarm(0);
		}
		return 1;
	}
}
void signal_handler(int signal_number){
	kill(pid,SIGKILL);
	fprintf(stderr, "%s is timed out\n", name);
}
/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Callback function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char *argv[])
{
	return 0;
}

/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char *argv[])
{

}


/*======================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******        */

static bool __verbose = true;
static char *__color_start = "[0;31;40m";
static char *__color_end = "[0m";

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char *argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	if (__verbose)
		fprintf(stderr, "%s%s%s ", __color_start, __prompt, __color_end);

	while (fgets(command, sizeof(command), stdin)) {	
		char *tokens[MAX_NR_TOKENS] = { NULL };
		int nr_tokens = 0;

		if (parse_command(command, &nr_tokens, tokens) == 0)
			goto more; /* You may use nested if-than-else, however .. */

		ret = run_command(nr_tokens, tokens);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			fprintf(stderr, "Error in run_command: %d\n", ret);
		}

more:
		if (__verbose)
			fprintf(stderr, "%s%s%s ", __color_start, __prompt, __color_end);
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
