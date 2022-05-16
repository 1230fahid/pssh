#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "builtin.h"
#include "parse.h"

static char* builtin[] = {
    "exit",   /* exits the shell */
    "which",  /* displays full path to command */
	 "fg",
	 "bg",
	 "kill",
	 "jobs",
    NULL
};


int is_builtin (char* cmd)
{
    int i;

    for (i=0; builtin[i]; i++) {
        if (!strcmp (cmd, builtin[i]))
            return 1;
    }

    return 0;
}

static void which (Task T)
{
    char* dir;
    char* tmp;
    char* PATH;
    char* state;
    char probe[PATH_MAX+1];
  
    //int ret = 0;

    if (!T.argv || !T.argv[1])
        exit (EXIT_FAILURE);

    if (access (T.argv[1], F_OK) == 0) {
        printf ("%s\n", T.argv[1]);
        exit (EXIT_SUCCESS);
    }

    if (is_builtin (T.argv[1])) {
        printf ("%s: shell built-in command\n", T.argv[1]);
        exit (EXIT_SUCCESS);
    }

    PATH = strdup (getenv ("PATH"));

    for (tmp=PATH; ; tmp=NULL) {
        dir = strtok_r (tmp, ":", &state);
        if (!dir)
            break;

        strncpy (probe, dir, PATH_MAX);
        strncat (probe, "/", PATH_MAX);
        strncat (probe, T.argv[1], PATH_MAX);

        if (access (probe, X_OK) == 0) {
            printf ("%s\n", probe);
            //ret = 1;
            break;
        }
    }

    free (PATH);
    exit(EXIT_SUCCESS);
}

static void fg (Task T) {
	if((!T.argv || !T.argv[1]) || T.argv[2]){
		printf("Usage: fg %<job number>\n");
		exit(EXIT_FAILURE);
	}
	int j;
	j = atoi(T.argv[1]+1);
	if((j == 0) && (strcmp(T.argv[1],"%0") != 0)) {
		printf("pssh: invalid job number: [%s]\n", T.argv[1]);
		exit(EXIT_FAILURE);
	}
	if(job_exists(j)) {
		printf("Couldn't properly implement fg, read README.txt\n");
	}
	else {
		printf("pssh: invalid job number: [%d]\n", j);
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}
// I just couldn't get it right. When I tried, it destroyed 
// ... my pssh and bash when I used putty, couldn't reclaim.
//.. the foreground, even though I set the fg to current...
//pgrp
static void bg (Task T) {
	if((!T.argv || !T.argv[1]) || T.argv[2]){
	   printf("Usage: bg %<job number>\n");
	   exit(EXIT_FAILURE);
   }
   int j;
   j = atoi(T.argv[1]+1);
   if((j == 0) && (strcmp(T.argv[1],"%0") != 0)) {
      printf("pssh: invalid job number: [%s]\n", T.argv[1]);
      exit(EXIT_FAILURE);
   }
   if(job_exists(j)) {
		kill(-1*job[j].pgid, SIGCONT);
	}
	else {
		printf("pssh: invalid job number: [%d]\n", j);
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}
//send sigcont to pgid * -1, which sends sigcont to all...
//processes in the same group.

static void kill_t (Task T) {
   int i = 0;                                              
	int j = 1;                                                   
	int sig = SIGTERM;
	for (i=0; T.argv[i]; i++);
	if (!T.argv || !T.argv[1]) {
	   printf ("Usage: kill [-s <signal>] <pid> ...\n");
		return;
	}
	if (!strcmp (T.argv[1], "-s")) {
		sig = atoi (T.argv[2]);
		j=3;
	}
  // need to check between 2 and 4 arguments
  int a;
	for (a=j; a<i; a++) {
	   if (*T.argv[j] == '%') {
			int jobnum = atoi(T.argv[1] + 1);
		   if (job_exists (jobnum))
			   kill (-1* job[jobnum].pgid, sig);
			else
				printf ("pssh: invalid job number: %s\n", T.argv[j]+1);
		} else {
			  if (kill (atoi (T.argv[j]), sig) < 0)
					printf ("pssh: invalid pid: %s\n", T.argv[j]);
		}
	}
	exit(EXIT_SUCCESS);
}
// kill background or foreground job

static void jobs (Task T) {
   int i;
	   for(i=0; i<100; i++) {
	      if (job[i].name ==  NULL) {
	         continue;
	      }
	      else {
	         printf("\n[%d] + ", i);
	         if(job[i].status == STOPPED) {
	            printf("stopped   %s\n", job[i].name);
	         }
            else {
	            printf("running   %s\n", job[i].name);
	         }
	      }
	   }
	exit(EXIT_SUCCESS);
}
// print all jobs

void builtin_execute (Task T)
{
    if (!strcmp (T.cmd, "exit")) {
        exit(EXIT_SUCCESS);
    }
    else if (!strcmp (T.cmd, "which")) {
        which(T);
    }
	 else if (!strcmp(T.cmd, "fg")) {
		fg(T);
	 }
	 else if (!strcmp(T.cmd, "bg")) {
		bg(T);
	 }
	 else if (!strcmp(T.cmd, "kill")){
	   kill_t(T);
	 }
	 else if (!strcmp(T.cmd, "jobs")){
		jobs(T);
	 }
    else {
        printf ("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}
