#ifndef _builtin_h_
#define _builtin_h_

#include "parse.h"

typedef enum {
   STOPPED,
   TERM,
   BG,
   FG,
} JobStatus;
typedef struct {
   char* name;
   pid_t* pids;
   unsigned int npids;
   pid_t pgid;
   JobStatus status;
} Job;
//created a structure and enum to parse


Job job[100];
// declared globally so it can be changed in different functions

int is_builtin (char* cmd);
void builtin_execute (Task T);
int builtin_which (Task T);

int job_exists(int num);
void set_fg_pgrp(pid_t pid);
void change_to_stop(int num);
void change_to_fg(int num);
void change_to_bg(int num);
#endif /* _builtin_h_ */

