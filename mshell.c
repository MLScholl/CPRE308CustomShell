/**
 *
 * @author Michael Scholl
 *
 */

/* Compile with: gcc -o mshell mshell.c */
/* Run with: ./mshell -m  or  ./mshell -p "Prompt" */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

/* Could make this value larger if required */
#define BUFFER 505

/* Global variables */
static char command[BUFFER];
static char *token;

/* List of builtin commands */
char *builtin_strs[] = {
  "cd",
  "help",
  "exit",
  "pid",
  "ppid",
  "get",
  "pwd"
};

int numof_builtins() {
  return sizeof(builtin_strs) / sizeof(char *);
}

/* Used to print the last time the file was accessed */
static struct stat file_stat;

/* Function Prototypes */
void mshell_cd();
void mshell_run();
void mshell_help();
void mshell_get();
void mshell_set();

/* Main */
int main(int argc, char **argv) {
	
	/* Set the prompt to 308sh> (could be changed later) */
	char *prompt = "308sh> ";
	
	//printf("%s\n", argv[1]);
	//printf("%s\n", argv[2]);
	
	/* Change the prompt */
	if (strcmp(argv[1], "-p") == 0) {
      prompt = argv[2];
    }
  
  /* Print the last time the file was accessed */
  stat(__FILE__, &file_stat);
  printf("Last login: %s", ctime(&file_stat.st_atime));

  /* Control Loop */
  while(1) {
    
    printf("%s", prompt);

    /* Get user input */
    fgets(command, sizeof command, stdin);

    /* Tokenize command */
    token = strtok(command, " \t\n()<>|&;");

    /* If no input, continue loop */
    if(!token){
      continue;
    }

    /* Exit */
    else if(strcmp(token, "exit") == 0) {
      exit(0);
    }
    
    /* Get */
    else if(strcmp(token, "get") == 0) {
      mshell_get();
    }
    
    /* Print Child Process ID */
    else if(strcmp(token, "pid") == 0) {
      printf("ChildPID %u\n", getpid());
    }
    
    /* Print Parent Process ID */
    else if(strcmp(token, "ppid") == 0) {
      printf("ParentPID %u\n", getppid());
    }
    
    /* Set */
    else if(strcmp(token, "set") == 0) {
      mshell_set();
    }
    
    /* Help */
    else if(strcmp(token, "help") == 0) {
      mshell_help();
    }

    /* CD */
    else if(strcmp(token, "cd") == 0) {
      mshell_cd();
    }

    /* Unix Commands */
    else {
      mshell_run();
    }
  }

  return 0;
}

void mshell_get() {
  token = strtok(NULL, " \t\n()<>|;");
  
  /* Checks to see if the variable exists */
  if (getenv(token) == NULL) {
    perror("Error");
  }
  
  /* Print out the enviroment variable based on the token */
  else {
    printf("%s\n", getenv(token));
  }
}

void mshell_set() {
  char *args[BUFFER];
  token = strtok(NULL, " \t\n()<>|;");
  int arg_count;
  
  arg_count = 0;
  
  args[arg_count] = token;
  /* Checks for amount of arguments */
  while (token != NULL){
    token = strtok(NULL, " \t\n()<>|;");
    arg_count++;
    args[arg_count] = token;
  }
  
  /* Checks to see if there is a value for arg if so then set the env variable to it */
  if (args[1] != NULL) {
    setenv(args[0], args[1], 1);
  }
  
  /* Removed the value if nothing was in the argument */
  else {
    unsetenv(args[0]);
  }
}

void mshell_cd() {
	/* Get the home directory and the current directory */
  char *home;
  home = getenv("HOME");
  token = strtok(NULL, " \t\n()<>|&;");

  /* Allows for using "cd" to change to home directory */
  if (token == NULL) {
    if (home == NULL) {
      printf("HOME environment variable not set. Home directory commands will not work.\n");
      return;
    }
    if (chdir(home) == -1) {
      perror("Error");
    }
    return;
  }
  
  /* Allows for using '~' to cd back a directory relative to home  */
  if (token[0] == '~') {
    if (home == NULL) {
      printf("HOME environment variable not set. Home directory commands will not work.\n");
      return;
    }
    char relative[400];
    strcpy(relative, home);
    token = strcat(relative, &token[1]);
  }

  /* Completes the change in directory and prints an error if needed */
  if (chdir(token) == -1) {
    perror("Error");
  }
}

void mshell_help() {
	
	printf("					Michael Scholl's Shell\n");
  printf("			Not really sure how to help you\n");
  printf("	  The following functions are built in:\n");
  int i;
  
  for (i = 0; i < numof_builtins(); i++) {
    printf("  %s\n", builtin_strs[i]);
  }
}


void mshell_run() {
  pid_t pid, wpid;
  char *args[BUFFER];
  int arg_num = 0;
  int status = 0;
  int background = 0;

  pid = fork();

  /* An error occured with the fork */
  if (pid == -1) {
    perror("Error");
    exit(EXIT_FAILURE);
  }
  /* Child process */
  else if (pid == 0) {
  	
  	if (strstr(token, "&") != NULL) {
      background = 1;
      printf("%s\n", token);
    }
  	
    args[arg_num] = token;

    while (token != NULL) {
      token = strtok(NULL, " \t\n()<>|&;");
      /*
      if (strcmp(token, "&") == 0) {
        background = 1;
        token = NULL;
      }
      */
      arg_num++;
      args[arg_num] = token;
    }
    
    
    printf("[%u] %s\n", getpid(), args[0]);
    
    /* Run the program specified */
    if ((execvp(args[0], args) == -1)) {
      perror("Error");
    }
    
    // printf("[%u] %s\n", getpid(), args[0]);
    
    exit(EXIT_SUCCESS);
  }
  
  /* Parent process */
  else {
  	
    do {
        wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    
    printf("[%u] %s Exit %d\n", wpid, token, WIFEXITED(status));
  }
}
