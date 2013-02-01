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
#define MAX_INPUT_LENGTH 255

void parse_command (char user_command[], char *arguments[]);

/*
	Questions: 
	1. If you use fork() in the middle of a while loop, it creates a new process which basically copies the code from that point down. So the child
		process will go all the way down to the bottom of the while loop, then will it go back to the top? Even though it didn't copy the top of the
		while loop?
	2. Does cd have to run in the background also? That means it would have to be done in the parent process instead of the child process, so we can't
		use waitpid?
	
	TESTS:
	1. mkdir test
	2. cd test
	3. ls
	4. ls -l
	5. cd ..
	6. ps
	7. ps -e
	8. cat rsi.c
	9. ...
	
	TODO:
	1. setbuf(stdin, NULL); // Is this necessary?
*/

/* SIGCHLD handler. A child process returns SIGCHLD when it is stopped or terminated. */
static void sigchld_handler (int signal)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) {
		if (DEBUG_MODE) printf("\nSIGCHLD handler activated. Received signal %d\n", signal);
	}
	
	if (DEBUG_MODE)	printf("Parent process receives SIGCHLD.\n");
}

int main()
{
	//char user_command[MAX_INPUT_LENGTH];
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
		//printf("RSI: %s > ", getcwd(0, 0));
		//fflush(stdin);
		//setbuf(stdin, NULL);
		
		
		
		// TAKE INPUT
		
		char * prompt = malloc(snprintf(NULL, 0, "RSI: %s > ", getcwd(0, 0)) + 1);
		sprintf(prompt, "RSI: %s > ", getcwd(0, 0));
		
		//fgets(user_command, sizeof(user_command), stdin);
		user_command = readline(prompt);
		
		//fgetc(stdin);
			
		// Get rid of the trailing new line character
		//if (user_command[strlen(user_command) - 1] == '\n') {
		//	  user_command[strlen(user_command) - 1] = '\0';
		//}		

		if (DEBUG_MODE)	printf("Command: %s\n", user_command);
		// END OF TAKE INPUT

		// PARSE INPUT TO GET COMMANDS
		//parse_command(user_command, arguments);
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

			if (DEBUG_MODE) printf("[%s]\n", argument);
			
			// Adjust the size of our argument list based on how many arguments we have parsed so far
			arguments = realloc (arguments, sizeof (char*) * number_of_arguments);
			arguments[number_of_arguments - 1] = argument;
			argument = strtok(NULL, " ");
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
			
			// Not specifying a directory sends them to their home directory.
			if (number_of_arguments == 1) {
				char *home_dir = getenv("HOME");
				if (DEBUG_MODE) printf("User's home directory: %s\n", home_dir);
				
				if (chdir(home_dir) < 0) perror ("Error on chdir");
				
			// Special argument: "~" sends them to their home directory.	
			} else if (strcmp(arguments[1], "~") == 0) {
				char *home_dir = getenv("HOME");
				if (DEBUG_MODE) printf("User's home directory: %s\n", home_dir);
				
				if (chdir(home_dir) < 0) perror ("Error on chdir");
				
			} else {
				if (DEBUG_MODE) printf("Changing to directory: %s\n", arguments[1]);
				
				if (chdir(arguments[1]) < 0) perror ("Error on chdir");
			}
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
					//if (DEBUG_MODE) printf("PARENT: Child Status: %d\n", child_status);
					
					if (retVal == -1) { 
						perror("Fail at waitpid"); 
						exit(EXIT_FAILURE);
					}
					
					//wait(NULL);
					
					//Macros below can be found by "$ man 2 waitpid"
					 if (WIFEXITED(child_status)) {
						printf("The child terminated normally, status code=%d\n", WEXITSTATUS(child_status));  // Display the status code of child process
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
		
		// We are done with the arguments array and the current input on stdin.
		//setbuf(stdin, NULL); // Is this necessary?
		free(arguments);
		//free(prompt);
		// END OF EXECUTE INPUT
	}
	
}

// Take whatever the user typed in and parse it, filling the supplied array with entries for each argument.
// Function doesn't work since I don't know how to pass in an array and return it with the new values
void parse_command (char user_command[], char *arguments[]) {
	
	int number_of_arguments = 0;
	//char **arguments = NULL;
	int i;

	// Use strtok to grab first token. This is the command to execute.
	char *argument = strtok(user_command, " ");

	// Loop through the input until strtok returns null. Put each resulting string into an array. This will be the arguments to pass to execvp.
	while (argument != NULL) {
		// We know we have another argument
		number_of_arguments ++;

		if (DEBUG_MODE) printf("[%s]\n", argument);
		
		// Adjust the size of our argument list based on how many arguments we have parsed so far
		arguments = realloc (arguments, sizeof (char*) * number_of_arguments);
		arguments[number_of_arguments - 1] = argument;
		argument = strtok(NULL, " ");
	}

	// The array needs to be null-terminated for execvp
	arguments = realloc (arguments, sizeof (char*) * (number_of_arguments + 1));
	arguments[number_of_arguments] = NULL;

	if (DEBUG_MODE) {
		for (i = 0; i < (number_of_arguments + 1); i++)
			printf ("Argument %d = %s\n", i, arguments[i]);
	}

	// We're done with the user's command. Now free up the memory we used.
	// TODO: Move this to main()
	//free (arguments);
}
