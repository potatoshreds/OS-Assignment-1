/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define NV 20  /* max number of command tokens */
#define NL 100 /* input buffer size */
char line[NL]; /* command input buffer */

// Job table for background processes
struct job
{
  pid_t pid;
  int id;
  char *cmd;
};

static struct job jobs[128];
static int job_count = 0;
static int next_job_id = 1;

/*
  shell prompt
 */

static void prompt(void)
{
  // ## REMOVE THIS 'fprintf' STATEMENT BEFORE SUBMISSION
  // fprintf(stdout, "\n msh> ");
  fflush(stdout);
}

// join argv[0...argc-1] into a single string
static char *join_argv(char *argv[], int argc)
{
  size_t need = 1;
  for (int i = 0; i < argc; i++)
    need += strlen(argv[i]) + (i ? 1 : 0);
  char *buf = (char *)malloc(need);
  if (!buf)
  {
    perror("malloc");
    return NULL;
  }
  buf[0] = '\0';
  for (int i = 0; i < argc; i++)
  {
    if (i)
      strcat(buf, " ");
    strcat(buf, argv[i]);
  }
  return buf;
}

// Identify and remove the trailing '&'
static int detect_background_and_trim(char *v[], int *argc_inout)
{
  int argc = *argc_inout;
  if (argc == 0)
    return 0;

  // last token is "&"
  if (strcmp(v[argc - 1], "&") == 0)
  {
    v[argc - 1] = NULL;
    *argc_inout = argc - 1;
    return 1;
  }

  // last token ends with "&"
  size_t L = strlen(v[argc - 1]);
  if (L > 0 && v[argc - 1][L - 1] == '&')
  {
    v[argc - 1][L - 1] = '\0';
    if (v[argc - 1][0] == '\0')
    { // if it was just "&"
      v[argc - 1] = NULL;
      *argc_inout = argc - 1;
    }
    return 1;
  }
  return 0;
}

// Report "Done" for a terminated child process
static void sigchild_handler(int sig)
{
  (void)sig;
  int status;

  // Non-blocking wait for child processes
  for (int i = 0; i < job_count; i++)
  {
    pid_t pid = waitpid(jobs[i].pid, &status, WNOHANG);

    if (jobs[i].pid == pid)
    {
      if (jobs[i].cmd)
      {
        printf("[%d]+ Done %s\n", jobs[i].id, jobs[i].cmd);
        free(jobs[i].cmd);
      }
      else
      {
        printf("[%d]+ Done\n", jobs[i].id);
      }
      // Remove the job from the list
      for (int j = i; j < job_count - 1; j++)
        jobs[j] = jobs[j + 1];
      job_count--;
      i--; 
    }
    else if (pid == 0)
    {
    }
    else if (pid == -1 && errno != ECHILD)
    {
      perror("waitpid");
    }
  }
}

// Install signal handlers
static void install_signal_handler(void)
{
  if (signal(SIGCHLD, sigchild_handler) == SIG_ERR)
  {
    perror("signal(SIGCHLD)");
  }
}

// build cd
static void build_cd(char *path)
{
  const char *dst = path ? path : getenv("HOME");
  if (!dst)
    dst = "/";
  if (chdir(dst) == -1)
  {
    perror("chdir");
  }
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[])
{
  int frkRtnVal;       /* value returned by fork sys call */
  char *v[NV];         /* array of pointers to command line tokens */
  char *sep = " \t\n"; /* command line token separators    */
  int i;               /* parse index */

  install_signal_handler();

  /* prompt for and process one command line at a time  */

  while (1)
  { /* do Forever */
    prompt();

    if (fgets(line, NL, stdin) == NULL)
    {
      if (feof(stdin))
        exit(0);
      perror("fgets");
      continue;
    }

    if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
    {
      continue; /* to prompt */
    }

    v[0] = strtok(line, sep);
    if (!v[0])
    {
      continue;
    }

    int argc_cmd = 1;
    for (i = 1; i < NV; i++)
    {
      v[i] = strtok(NULL, sep);
      if (v[i] == NULL)
        break;
      argc_cmd = i + 1;
    }
    v[argc_cmd] = NULL;

    // Detect '&' and trim it
    int is_bg = detect_background_and_trim(v, &argc_cmd);

    // build cd
    if (strcmp(v[0], "cd") == 0)
    {
      build_cd(v[1]);
      continue;
    }

    /* assert i is number of tokens + 1 */

    /* fork a child process to exec the command in v[0] */
    frkRtnVal = fork();
    if (frkRtnVal == -1) /* fork returns error to parent process */
    {
      perror("fork");
      continue;
    }
    if (frkRtnVal == 0) /* code executed only by child process */
    {
      execvp(v[0], v);
      perror("execvp"); // execvp failed
      exit(127);
    }
    else /* code executed only by parent process */
    {
      if (!is_bg)
      {
        int st;
        if (waitpid(frkRtnVal, &st, 0) == -1)
        {
          perror("waitpid");
        }
      }
      else
      {
        // background: print job header and remember it
        int id = next_job_id++;
        printf("[%d] %d\n", id, frkRtnVal);

        char *cmd = join_argv(v, argc_cmd);
        if (!cmd)
          cmd = strdup(v[0] ? v[0] : "");
        if (!cmd)
        {
          perror("strdup");
          cmd = NULL;
        }

        if (job_count < (int)(sizeof(jobs) / sizeof(jobs[0])))
        {
          jobs[job_count].pid = frkRtnVal;
          jobs[job_count].id = id;
          jobs[job_count].cmd = cmd;
          job_count++;
        }
        else
        {
          free(cmd);
        }
      }
    } /* switch */
  } /* while */
} /* main */
