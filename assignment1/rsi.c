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

#define DEBUG_MODE 0
#define MAX_INPUT_LENGTH 255

void parse_command (char user_command[], char *arguments[]);

/*
	Questions: 
	1. Do we only need to implement the ls, mkdir, cd, cat, nohup, ps, and kill commands?
	2. If not, there's no good way to decide whether or not to use execvp() or chdir() based on their input.
	3. So we just have to parse the first argument and run a certain function based on if it's one of the above?
	4. After executing ls using execvp, why doesn't it pring a new line anymore?
	
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
	1. Make it execute all commands other than cd. Do this by moving the cd compare to the top, and then have
		the execvp be in the else block rather than in the ls|mkdir|ps compare block.
	2. 
*/
int main()
{
	char user_command[MAX_INPUT_LENGTH];
	pid_t child_pid;
	int child_status;
	
	int inputting = 1;
	while (inputting) {
		printf("RSI: %s > ", getcwd(0, 0));
		
		// TAKE INPUT
		fgets(user_command, sizeof(user_command), stdin);
		// Get rid of the trailing new line character
		if (user_command[strlen(user_command) - 1] == '\n') {
			  user_command[strlen(user_command) - 1] = '\0';
		}
		
		if (DEBUG_MODE)	printf("Command: %s\n", user_command);
		// END OF TAKE INPUT

		// PARSE INPUT TO GET COMMANDS
		//parse_command(user_command, arguments);
		char **arguments = NULL;
		int number_of_arguments = 0;
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
		// END OF PARSE INPUT
		
		// EXECUTE INPUT
		if (strcmp(arguments[0], "cd") == 0) {
			// Use getcwd() and chdir()
			
			// Make sure they only specified two arguments, the "cd" and the directory name (or blank).
			if (number_of_arguments > 2) {
				printf("Usage: cd [directory name]\n");
				exit(EXIT_FAILURE);
			}
			
			// They didn't specify a directory after cd; send them to home directory.
			if (number_of_arguments == 1) {
				char *home_dir = getenv("HOME");
				if (DEBUG_MODE) printf("User's home directory: %s\n", home_dir);
				
				if (chdir(home_dir) < 0) {
					perror ("Error on chdir.");
					//exit(EXIT_FAILURE);
				}
			// Special argument: "~"; send them to home directory.	
			} else if (strcmp(arguments[1], "~") == 0) {
				char *home_dir = getenv("HOME");
				if (DEBUG_MODE) printf("User's home directory: %s\n", home_dir);
				
				if (chdir(home_dir) < 0) {
					perror ("Error on chdir.");
					//exit(EXIT_FAILURE);
				}
				
			} else {
				if (DEBUG_MODE) printf("Changing to directory: %s\n", arguments[1]);
				
				int result = chdir(arguments[1]);
				if (DEBUG_MODE) printf("Result of chdir(): %d\n", result);
				
				if (result < 0) {
					perror ("Error on chdir.");
					//exit(EXIT_FAILURE);
				}
			}
		} else {
			child_pid = fork();
			// If fork returns >= 0, we know it succeeded
			if (child_pid >= 0) {
				// Fork returns 0 to the child process
				if (child_pid == 0) {
					if (DEBUG_MODE) printf("Child: PID of Child = %ld\n", (long) getpid());
					
					if (execvp(arguments[0], arguments) < 0) perror ("Error on execvp.");
					
				// Fork returns a new pid to the parent process
				} else {
					int opts = WNOHANG | WUNTRACED | WCONTINUED;
					int retVal;
					if (DEBUG_MODE) printf("Parent: PID of Parent = %ld and PID of its child = %ld\n", (long) getpid(), (long) child_pid);
					
					// Wait for the child to exit
					// retVal = waitpid(child_pid, &child_status, opts);
					
					// if (DEBUG_MODE) printf("Return value of waitpid: %d\n", retVal);
					// if (DEBUG_MODE) printf("Child Status: %d\n", child_status);
					
					// if (retVal == -1) { 
						// perror("Fail at waitpid"); 
						// exit(EXIT_FAILURE);
					// }
					
					//Macros below can be found by "$ man 2 waitpid"
					// if (WIFEXITED(child_status)) {
						// printf("The child terminated normally, status code=%d\n", WEXITSTATUS(child_status));  // Display the status code of child process
					// } else if (WIFSIGNALED(child_status)) {
						// printf("The child was killed by signal %d\n", WTERMSIG(child_status));
					// } else if (WIFSTOPPED(child_status)) {
						// printf("The child was stopped by delivery of signal %d\n", WSTOPSIG(child_status));
					// } else if (WIFCONTINUED(child_status)) {    
						// printf("The child was resumed by delivery of SIGCONT.\n");   
					// }
					
					wait(NULL);
				}
				
			// Fork returns -1 on failure.
			} else {
				perror("Error forking.");
				exit(0);
			}

		}
		
		// We are done with the arguments array
		free(arguments);
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
