#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h> 

/*
  Deklaracija funkcija za ugrađene shell naredbe. Ovo je potrebno jer bi se program slomio kad bismo pozvali "help" za samu naredbu "help"
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_mkdir(char **args);
int lsh_clear(char **args);
int lsh_pwd(char **args);
int lsh_ls(char **args);

/*
  Lista ugrađenih naredbi, nakon nje slijede odgovarajuće funkcije.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "mkdir",
  "clear",
  "pwd",
  "ls"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_mkdir,
  &lsh_clear,
  &lsh_pwd,
  &lsh_ls
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Implementacija
*/

/**
	Naredba cd - change directory.
	Parametar args je lista argumenata, gdje je args[0] = "cd", a args[1] ime direktorija u koji se želimo premjestiti.
	Funkcija uvijek vraća 1 kako bi se program mogao nastaviti izvršavati.
 */ 
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   	Naredba help ispisuje pomoć.
	Parametar args je lista argumenata koju u ovom slučaju uopće ne ispitujemo.
	Funkcija uvijek vraća 1 kako bi se program mogao nastaviti izvršavati.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   	Naredba exit izlazi iz programa.
	Parametar args je lista argumenata koju u ovom slučaju uopće ne ispitujemo.
	Funkcija uvijek vraća 0 kako bi se program zaustavio.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
	Naredba mkdir kreira novi direktorij sa zadanim imenom.
	Parametar args je lista argumenata. args[0] = "mkdir" a args[1] ime direktorija kojeg kreiramo.
	Funkcija kreira direktorij i uvijek vraća 1 kako bi se program mogao nastaviti izvršavati.
*/
int lsh_mkdir(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"mkdir\"\n");
  } else {
    mkdir(args[1], 0700);
  }
  return 1;
}

/**
	Naredba clear "čisti prozor terminala".
	Parametar args je lista argumenata koju ne proučavamo.
	Funkcija postavi pokazivač na poziciju 0,0 i uvijek vraća 1 kako bi se program mogao nastaviti izvršavati.
*/
int lsh_clear(char **args)
{
	printf("\x1b[H\x1b[J");
	return 1;
}
/**
	Naredba pwd ispisuje putanju do trenutnog direktorija.
	Parametar args je lista argumenata koju ne proučavamo.
	Funkcija ispisuje put od roota do trenutnog direktorija i uvijek vraća 1 kako bi se program mogao nastaviti izvršavati.
*/

int lsh_pwd(char **args)
{
  char pwd[1024];
  getcwd(pwd, sizeof(pwd));
  printf("%s\n", pwd);
  
  return 1;
}
/**
	Naredba ls ispisuje sadržaj trenutnog direktorija.
	Parametar args je lista argumenata koju ne proučavamo.
	Funkcija otvara direktorij i ispisuje njegov sadržaj te uvijek vraća 1 kako bi se program mogao nastaviti izvršavati.
*/
int lsh_ls(char **args)
{
  struct dirent *de;
  
  DIR *dr = opendir("."); 
  
  if (dr == NULL)
  { 
    printf("Could not open current directory" ); 
    return 0; 
  } 
  
  while ((de = readdir(dr)) != NULL)
	printf("%s\n", de->d_name);
		
  closedir(dr);
  return 1;
}

/**
   Funkcija uzima listu argumenata, nakon toga forka proces i spremi povratnu vrijednost u pid.
   Sada imamo dva procesa koja se istodobno izvršavaju.
   U procesu-djetetu ulazimo u prvi if (pid == 0) i želimo pokrenuti naredbu koju je korisnik unio.
   Za to koristimo jednu varijantu naredbe exec, konkretno execvp (očekuje ime programa i niz)
   Ako naredba vrati -1 znači da je došlo do greške i to ispisujemo, nakon toga proces dijete umire i vraća se 1 kako bi se program nastavio izvršavati.
   Ako je pid manji od 0 znači da se dogodila greška u forkanju te se ispiše greška i vrati 1 kako bi se program nastavio izvršavati.
   Proces roditelj ulazi u else i čeka da se proces dijete izvrši.
   To činimo naredbom waitpid(), odnosno čekamo da se promijeni status procesa. 
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
  Pokreće ugrađene naredbe ili poziva lsh_launch kako bi se pokrenuo neki program
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

#define LSH_RL_BUFSIZE 1024
/**
   Funkcija za čitanje linije. U while petlji čitamo upisane znakove i spremamo ih u buffer kao int vrijednosti, 
   sve dok ne dođemo do novog reda ili EOF (EOF je integer i zbog toga se sve sprema kao int).
 */
char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   Pročitanu liniju razdvajamo u listu tokena pomoću strtok() i vraćamo ju.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   Funkcija lsh_loop ispisuje ">", pročita liniju (funkcija lsh_read_line),
   podijeli liniju u argumente (funkcija lsh_split_line) i iste pošalje u funkciju lsh_execute.
   Ukoliko lsh_execute vrati 0, program prestaje s radom, a ukoliko vrati 1, kreće se ponovo od početka petlje.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   	Početna točka programa, sve što radi je poziva lsh_loop i zatvara program
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

