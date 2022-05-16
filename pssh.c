#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "builtin.h"
#include "parse.h"
#include <sys/types.h>
char *jobname;

void jobs_create(pid_t pid) {
	printf("%d ", pid);
}
//for every job created, it prints out pid

void job_make() {
	int i;
	for (i=0; i<100; i++) {
		job[i].name = NULL;
		job[i].pids = NULL;
		job[i].npids = 0;
		job[i].status = STOPPED;
	}
}
//initialize jobs

int jobnumber;
void job_create(Parse *P, pid_t* pid, char *name, JobStatus st) {
	int i;
	for(i = 0; i < 100; i++) {
		if(job[i].name == NULL) {
			break;
		}
	}
	job[i].name = strdup(name);
	job[i].pids = pid;
	job[i].npids = P->ntasks;
	job[i].status = st;
	job[i].pgid = pid[0];
	jobnumber = i;
}
//for each job made in terminal, you get the properties

int job_num(pid_t pid) {
	int pidnum, jobpid;
	for(pidnum = 0; pidnum < 100; pidnum++) {
		for(jobpid = 0; jobpid < job[pidnum].npids; jobpid++) {
			if(pid == job[pidnum].pids[jobpid]) {
				return pidnum;
			}
		}
	}
	return -1;
}


//look for specific job number, return -1 if not found

char *job_name(int number) {
	char *name = strdup(job[number].name);
	//name = job[number].name;
	//tried mallocing, but didn't work because of double free
	return name;
}
//get the job name

void remove_job_pid (int num, pid_t pid) {
	int i;
	for(i = 0; i < job[num].npids; i++) {
	    if(pid == job[num].pids[i]) {
	       job[num].pids[i] = 0;
       } 
   }																   
}

int check_pid_job (int jobnum) {
	int jobpid;
   for(jobpid = 0; jobpid < job[jobnum].npids; jobpid++) {
	   if(job[jobnum].pids[jobpid] != 0) {
			return 0;
		}
	}
	return 1;
}

int job_exists(int num) {
	if(job[num].name != NULL) {
		return 1;
	}
	return 0;
}

//pid_t pgrp = 0;
int our_tty;
void set_fg_pgrp(pid_t pgrp)
{
    void (*sav)(int sig);

    if (pgrp == 0)
        pgrp = getpgrp();

    sav = signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, pgrp);
	 tcsetpgrp(STDOUT_FILENO, pgrp);
    signal(SIGTTOU, sav);
}

void change_to_stop(int num) {
	job[num].status = STOPPED;
}

void change_to_bg (int num) {
	job[num].status = BG;
}
void change_to_fg (int num) {
	job[num].status = FG;
}

void handler(int sig)
{
    pid_t chld;
    int status;
	 int jobnum;
	 char *name;
    switch(sig) {
    case SIGTTOU:
        while(tcgetpgrp(STDOUT_FILENO) != getpgrp())
            pause();
        break;
    case SIGTTIN:
        while(tcgetpgrp(STDIN_FILENO) != getpgrp())
            pause();
        break;
    case SIGCHLD:
        while((chld = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
			   jobnum = job_num(chld);
				char *p;
            if (WIFSTOPPED(status)) {
					    if(job[jobnum].status == STOPPED) {
						/*originally == STOPPED*/
						    continue;
					    }
					     change_to_stop(jobnum);
					     name = job_name(jobnum);
					     set_fg_pgrp(0);
					     printf("\n[%d] + suspended      %s\n", jobnum, name);
					   //change_to_stop(jobnum);
					     fflush(stdout);
					     free(name);
					//continue;
				   //}
					//continue;
            } 
				else if (WIFCONTINUED(status)){
					if(job[jobnum].status == FG) {
		           // pause();
					//	continue;
						break;
					}
            name = job_name(jobnum);
					  printf("\n[%d] + continued      %s\n", jobnum, name);
					  change_to_bg(jobnum);
					  fflush(stdout);
					  free(name);
            }
				else {
					remove_job_pid(jobnum, chld);
					// got a bit of help from TA for this section. Just on understanding
					// the idea to remove each pid and then destroy the whole job.
				//	printf("Child %d, has terminated\n", chld);
			   }
				
				if(check_pid_job(jobnum)) {
					name = job_name(jobnum);
				   if(job[jobnum].status != FG) {
						p = strstr(name, "sleep");
						if(p) {
							printf("\n");
						}
					   printf("[%d] + done   %s\n", jobnum, name);
						fflush(stdout);
				   }
				   else if (job[jobnum].status == FG) {
				   	set_fg_pgrp(0);
						//pause(); trying to cause a small delay so it goes back to normal
						//not here originally
				   }
				   job[jobnum].name = NULL;
				   job[jobnum].pids = NULL;
				   job[jobnum].npids = 0;
					free(name);

			   }	
        }
        break;

    default:
        break;
    }
}

/*******************************************
 * Set to 1 to view the command line parse *
 *******************************************/
#define DEBUG_PARSE 0

#define READ_SIDE  0
#define WRITE_SIDE 1

static void print_banner ()
{
    printf ("                    ________   \n");
    printf ("_________________________  /_  \n");
    printf ("___  __ \\_  ___/_  ___/_  __ \\ \n");
    printf ("__  /_/ /(__  )_(__  )_  / / / \n");
    printf ("_  .___//____/ /____/ /_/ /_/  \n");
    printf ("/_/ Type 'exit' or ctrl+c to quit\n\n");
}

static char* build_prompt ()
{
    char* full;

    char* cwd = getcwd (NULL, 0);
    char prompt[] = "$ ";


    full = malloc (strlen (cwd) + strlen(prompt) + 1);
    sprintf (full, "%s%s", cwd, prompt);
    free (cwd);

    return full;
}

static int command_found (const char* cmd)
{
    char* dir;
    char* tmp;
    char* PATH;
    char* state;
    char probe[PATH_MAX+1];

    int ret = 0;

    if (access (cmd, X_OK) == 0)
        return 1;

    PATH = strdup (getenv("PATH"));

    for (tmp=PATH; ; tmp=NULL) {
        dir = strtok_r (tmp, ":", &state);
        if (!dir)
            break;

        strncpy (probe, dir, PATH_MAX);
        strncat (probe, "/", PATH_MAX);
        strncat (probe, cmd, PATH_MAX);

        if (access (probe, X_OK) == 0) {
            ret = 1;
            break;
        }
    }

    free (PATH);
    return ret;
}

static void redirect (int fd_old, int fd_new)
{
    if (fd_new != fd_old) {
        if(dup2(fd_new, fd_old) == -1) {
          close (fd_new);
        }
    }
}

static int close_safe (int fd)
{
    if ((fd != STDIN_FILENO) && fd != (STDOUT_FILENO))
        return close (fd);

    return -1;
}

static void run (Task* T, int in, int out)
{
    redirect (STDIN_FILENO, in);
    redirect (STDOUT_FILENO, out);
    if (is_builtin (T->cmd))
        builtin_execute (*T);
    else if (command_found (T->cmd))
        execvp (T->cmd, T->argv);
}

static int get_infile (Parse* P)
{
    if (P->infile)
        return open (P->infile, 0);
    else
        return STDIN_FILENO;
}

static int get_outfile (Parse* P)
{
    if (P->outfile)
        return creat (P->outfile, 0664);
    else
        return STDOUT_FILENO;
}

static int is_possible (Parse* P)
{
    unsigned int t;
    Task* T;
    int fd;

    for (t=0; t<P->ntasks; t++) {
        T = &P->tasks[t];
        if (!is_builtin (T->cmd) && !command_found (T->cmd)) {
            fprintf (stderr, "pssh: command not found: %s\n", T->cmd);
            return 0;
        }

        if (!strcmp (T->cmd, "exit"))
            exit (EXIT_SUCCESS);
    }

    if (P->infile) {
        if (access (P->infile, R_OK) != 0) {
            fprintf (stderr, "pssh: no such file or directory: %s\n", P->infile);
            return 0;
        }
    }

    if (P->outfile) {
        if ((fd = creat (P->outfile, 0664)) == -1) {
            fprintf (stderr, "pssh: permission denied: %s\n", P->outfile);
            return 0;
        }
        close (fd);
    }

    return 1;
}

static void execute_tasks (Parse* P)
{
    unsigned int t;
    int fd[2];
    int in, out;
    pid_t* pid;
    
    if (!is_possible (P))
        return;

    pid = malloc (P->ntasks * sizeof(*pid));
    our_tty = dup(STDERR_FILENO);
    in = get_infile (P);
    for (t=0; t<P->ntasks-1; t++) {
        pipe (fd);
        pid[t] = fork ();
        setpgid(pid[t], pid[0]);
        if(pid[t] < 0) {
          fprintf(stderr, "Failed to fork\n");
          exit(EXIT_FAILURE);
        }
        if(pid[t] > 0) {
          close (fd[WRITE_SIDE]);
          close_safe (in);
          in = fd[READ_SIDE];
        }
        else if (pid[t] == 0) {
            close (fd[READ_SIDE]);
            run (&P->tasks[t], in, fd[WRITE_SIDE]);
        }
    }

    out = get_outfile (P);
    pid[t] = fork ();
    setpgid(pid[t], pid[0]);
	 JobStatus stat;
    if(pid[t] < 0) {
      fprintf(stderr, "Failed to fork\n");
      exit(EXIT_FAILURE);
    }
    if (pid[t] > 0) {
		if(P->background) {
			stat = BG;
			job_create(P, pid, jobname, stat);
			printf("[%d] ", jobnumber);
			for(t=0; t<P->ntasks; t++) {
				jobs_create(pid[t]);
			}
			printf("\n");
			fflush(stdout);
			//job_create(P, pid, jobname, stat);
		}
		else {
   	  set_fg_pgrp(0);
		  stat = FG;
		  job_create(P, pid, jobname, stat);
		  pause();
//        for (t=0; t<P->ntasks; t++) {
  //        waitpid (pid[t], NULL, 0);
    //    }
	   }
		//job_create(P, pid, jobname, stat);
      close(our_tty);
      close_safe (in);
      close_safe (out);
    //  free (pid);
    }
    else if (pid[t] == 0){
		if (!P->background) {
			set_fg_pgrp(0);
		}
      run (&P->tasks[t], in, out);
	 }
}



int main (int argc, char** argv)
{
    char* prompt;
    char* cmdline;
    Parse* P;
	 signal(SIGCHLD, handler);
	 signal(SIGTTIN, handler);
	 signal(SIGTTOU, handler);
    print_banner ();
	 job_make();
	 //jobs initialized
    while (1) {
        prompt = build_prompt ();
        cmdline = readline (prompt);
        free (prompt);

        if (!cmdline)       /* EOF (ex: ctrl-d) */
            exit (EXIT_SUCCESS);
		  jobname = strdup(cmdline);
        P = parse_cmdline (cmdline);
        if (!P)
            goto next;
        if (P->invalid_syntax) {
            printf ("pssh: invalid syntax\n");
				fflush(stdout);
            goto next;
        }

#if DEBUG_PARSE
        parse_debug (P);
#endif

        execute_tasks (P);
		  free(jobname);

    next:
        parse_destroy (&P);
        free(cmdline);
    }
}

