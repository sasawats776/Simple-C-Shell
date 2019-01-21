#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"

char history[100][100];
int length = 0;
int tokenNum = 0;
int histLoop = 0;

int my_cd(char **args);
int my_exit(char **args);
int my_history(char **args);
char **split_line(char *line);
char *read_line(void);
int execute(char **args);
int launch(char **args);

char *builtin_str[] = {
  "cd",
  "history",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &my_cd,
  &my_history,
  &my_exit
};

int num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int my_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("directory not found");
    }
  }
  return 1;
}

int my_history(char **args)
{
  int position = length;
  int i;
  char **newArgs = malloc(TOK_BUFSIZE * sizeof(char*));
  char *newLine = malloc(TOK_BUFSIZE * sizeof(char*));
  
  if(histLoop){
    printf("History loop. Nice try :p \n");
    histLoop = 0;
    return 1;
  }
  
  if(tokenNum >= 3){ //Too many elements.
    printf("Too many elements.\n");
  }
  else if(tokenNum == 2 && !strcmp(args[1], "-c")){ //Clear function.
    memset(history, 0, sizeof(history));
    length = 0;
    strcpy(history[length], "history -c\0");
    length++;
  }
  else if(tokenNum == 2){ //Specific number of elements requested.
    int num = atoi(args[1]);
    histLoop = 1;
    if(num <= position){
      strcpy(newLine, history[num]);
      newArgs = split_line(newLine);
      execute(newArgs);
    }
    else{
      printf("Too many requested elements.\n");
    }
  }
  else{//just print history contents
    for(i = 0; i < length; i++) {
			printf("%i  %s\n", i, history[i]);
		}
  }
  
  return 1;
}

int my_exit(char **args)
{
  return 0;
}

char **split_line(char *line)
{
  int bufsize = TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;
  tokenNum = 0;
  if (!tokens) {
    fprintf(stderr, "allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;
    tokenNum++;

    if (position >= bufsize) {
      bufsize += TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		    free(tokens_backup);
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

char *read_line(void)
{
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  getline(&line, &bufsize, stdin);

/* get rid of the '\n' from fgets */
  if (line[strlen(line) - 1] == '\n')
    line[strlen(line) - 1] = '\0';
  
  if(length > 99){
    length = 0;
    free(history[length]);
    strcpy(history[length],"\0");
  }
  strcpy(history[length], line);
  length++;
    
  return line;
}

int execute(char **args)
{
  int i;
  
  if (args[0] == NULL) {
    // An empty command was entered.
    printf("Empty command");
    return 1;
  }

  for (i = 0; i < num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return launch(args);
}

int launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("error\n");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("forking erro\n");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int main(int argc, char**argv){
  int a;
  for(a = 0; a < 100; a++){
    strcpy(history[a],"\0");
  }
  char *line;
  char *temp;
  char **args;
  int status;
  
  printf("C Shell developed by Sasawat Satumtira.\n");
  printf("Features include:\n");
  printf("*Reading commands from stdin one line at a time\n");
  printf("*Built-ins: cd, history, exit\n");
  printf("*No Piping\n");

  do {
    printf("$ ");
    line = read_line();
    args = split_line(line);
    status = execute(args);

    free(line);
    free(args);
  } while (status);
  return EXIT_SUCCESS;
}