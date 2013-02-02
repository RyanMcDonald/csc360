#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>  // pid_t 
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // kill(), SIGTERM, SIGKILL, SIGSTOP, SIGCONT
#include <errno.h>      // errno
#include <readline/readline.h>
#include <readline/history.h>

#define DEBUG_MODE 1

/*
	Questions: 
	1. If you use fork() in the middle of a while loop, it creates a new process which basically copies the code from that point down. So the child
		process will go all the way down to the bottom of the while loop, then will it go back to the top? Even though it didn't copy the top of the
		while loop?
	
	TESTS:
	1. mkdir test
	2. cd test
	3. ls
	4. ls -l
	5. cd ..
	6. ps
	7. ps -e
	8. cat rsi.c
	9. ls &
	10. ls -a
	
	TODO:
	1. setbuf(stdin, NULL); // Is this necessary?
*/

void parse_command (char *user_command);
void change_directory(char *directory);
void go_home();

/* SIGCHLD handler. A child process returns SIGCHLD when it is stopped or terminated. */
static void sigchld_handler (int signal)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) {
		if (DEBUG_MODE) printf("\nSIGCHLD handler activated. Received signal %d\n", signal);
	}
}

int main()
{
	char * user_command;
	pid_t child_pid;
	int child_status;
	
	// The sigaction is used to change the action taken by a process on receipt of a specific signal.
	struct sigaction action;
	memset (&action, '\0', sizeof(action));
	// We want to run the sigchld_handler on receipt of a signal
    action.sa_handler = sigchld_handler;
	
	// Attach our sigchld_handler. The new action for a SIGCHLD signal is installed from action, which is the struct holding our sigchld_handler.
	if (sigaction(SIGCHLD, &action, 0)) {
		perror ("sigaction");
		return 1;
	}
	
	int inputting = 1;
	while (inputting) {
		// TAKE INPUT
		char * prompt = malloc(snprintf(NULL, 0, "RSI: %s > ", getcwd(0, 0)) + 1);
		sprintf(prompt, "RSI: %s > ", getcwd(0, 0));
		
		user_command = readline(prompt);

		if (DEBUG_MODE)	printf("Command: %s\n", user_command);
		// END OF TAKE INPUT

		// PARSE INPUT TO GET COMMANDS
		char **arguments = NULL;
		int number_of_arguments = 0;
		int i;
		int background_process = 0;

		// Use strtok to grab first token. This is the command to execute.
		char *argument = strtok(user_command, " ");

		// Loop through the input until strtok returns null. Put each resulting string into an array. This will be the arguments to pass to execvp.
		while (argument != NULL) {
			// We know we have another argument
			number_of_arguments ++;
			
			// Adjust the size of our argument list based on how many arguments we have parsed so far
			arguments = realloc (arguments, sizeof (char*) * number_of_arguments);
			arguments[number_of_arguments - 1] = argument;
			argument = strtok(NULL, " ");
		}

		// If they just hit enter, don't do anything
		if (number_of_arguments == 0) {
			continue;
		}
		
		// Check if they want to run the process in the background
		if (strcmp(arguments[number_of_arguments - 1], "&") == 0) {
			background_process = 1;
			
			if (DEBUG_MODE) printf("They want to run the process in the background!\n");
			
			// We don't need the extra & anymore. null-terminate the array for execvp
			arguments[number_of_arguments - 1] = NULL;
			number_of_arguments --;
		} else {
			// The array needs to be null-terminated for execvp
			arguments = realloc (arguments, sizeof (char*) * (number_of_arguments + 1));
			arguments[number_of_arguments] = NULL;
		}

		if (DEBUG_MODE) {
			for (i = 0; i < (number_of_arguments + 1); i++)
				printf ("Argument %d = %s\n", i, arguments[i]);
		}
		// END OF PARSE INPUT
		
		// EXECUTE INPUT
		if (number_of_arguments >= 1 && strcmp(arguments[0], "cd") == 0) {
		
			// Make sure they only specified two arguments, the "cd" and the directory name (or blank).
			if (number_of_arguments > 2) {
				printf("Usage: cd [directory name]\n");
				continue;
			}
			
			change_directory(arguments[1]);
			
		} else {
			child_pid = fork();
			// If fork returns >= 0, we know it succeeded
			if (child_pid >= 0) {
				// Fork returns 0 to the child process
				if (child_pid == 0) {
					if (DEBUG_MODE) printf("CHILD: PID of Child = %ld\n", (long) getpid());

					if (execvp(arguments[0], arguments) < 0) {
						printf("%s: command not found\n", arguments[0]);
					}
					
				// Fork returns a new pid to the parent process
				} else {
					// Block if we don't want to run in the background
					int opts = 0;
					if (background_process) {
						// Don't block if we want to run in background
						opts = WNOHANG;
					}
					int retVal;
					
					if (DEBUG_MODE) printf("PARENT: PID of Parent = %ld and PID of its child = %ld\n", (long) getpid(), (long) child_pid);
					
					// Wait for the child to exit
					retVal = waitpid(child_pid, &child_status, opts);
				
					if (DEBUG_MODE) printf("PARENT: Return value of waitpid: %d\n", retVal);
					if (DEBUG_MODE) printf("PARENT: Child returned with status: %d\n", child_status);
					
					if (retVal == -1) { 
						perror("Fail on waitpid"); 
						exit(EXIT_FAILURE);
					}
					
					// Macros below can be found by "$ man 2 waitpid"
					 if (WIFEXITED(child_status)) {
						printf("The child terminated normally, status code = %d\n", WEXITSTATUS(child_status));
					} else if (WIFSIGNALED(child_status)) {
						printf("The child was killed by signal %d\n", WTERMSIG(child_status));
					} else if (WIFSTOPPED(child_status)) {
						printf("The child was stopped by delivery of signal %d\n", WSTOPSIG(child_status));
					} else if (WIFCONTINUED(child_status)) {    
						printf("The child was resumed by delivery of SIGCONT.\n");   
					}
				}
				
			// Fork returns -1 on failure.
			} else {
				perror("Error forking.");
				exit(0);
			}

		}
		
		// Clean up! Is this necessary?
		setbuf(stdin, NULL);
		free(arguments);
		free(prompt);
		// END OF EXECUTE INPUT
	}
	
}

void change_directory(char *directory) {
	// Not specifying a directory sends them to their home directory.
	if (directory == NULL) {
		go_home();
		
	// Special argument: "~" sends them to their home directory.	
	} else if (strcmp(directory, "~") == 0) {
		go_home();
		
	} else {
		if (DEBUG_MODE) printf("Changing to directory: %s\n", directory);
		
		if (chdir(directory) < 0) perror ("Error on chdir");
	}
}

void go_home() {
	char *home_dir = getenv("HOME");
		if (DEBUG_MODE) printf("User's home directory: %s\n", home_dir);
		
		if (chdir(home_dir) < 0) perror ("Error on chdir");
}