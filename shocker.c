#define _GNU_SOURCE
#include <sys/wait.h> // waitpid
#include <sched.h> // clone, all the clone Flags
#include <unistd.h> // execvp, sethostname
#include <string.h> // strlen
#include <stdio.h> // puts, printf
#include <sys/mount.h> // mount

#define STACK_SIZE (1024 * 1024)

//   docker run image <cmd> <params>
//./shocker run       <cmd> <params>

static char child_stack[STACK_SIZE]; /* Stack size for cloned child */

int child(void *arg) {
  sethostname("Container", 10);
  chdir("/root/container");
  chroot("/root/container"); // this is the main part of hiding
  mount("proc", "/proc", "proc", 0, NULL); // proc is a pseudo-fs, used by many tools
  char **shell = (char **)arg;
  execvp(shell[0], shell);
  return 0;
}

int run(char *argv[]) {
  // Fork and exec pattern in now the clone and exec pattern
  int child_pid = clone(child, child_stack+STACK_SIZE, 
      CLONE_NEWUTS | // unix timesharing, allows changing of hostname 
      CLONE_NEWPID | // procfs is one of a few ways to pass process info, OSX has no procfs
      CLONE_NEWNS | // first namespace (for mounts). Private copy of its namespace.
      CLONE_NEWUSER | // user related
      SIGCHLD, // signal sent to the parent when the child terminates
      argv + 1 // drop `run` arg
  );
  // create /sys/fs/cgroup/pids/shocker prior to running
  FILE *fp = fopen("/sys/fs/cgroup/pids/shocker/cgroup.procs", "w");
  fprintf(fp, "%d", child_pid); // write child pid to the cgroup.procs procfs
  fclose(fp);
  waitpid(child_pid, NULL , 0); // NULL is whether to write status, 0 
  umount("/root/container/proc");
  return 0;
}

// need just stdio, and string for main

int main(int argc, char *argv[]) {
  if (argc < 3) {
    puts("Not enough arguments.\n");
    return 1;
  }
  argv += 1; // drop binary argument
  if (strcmp(argv[0], "run") == 0) {
      run(argv);
  } else {
    puts( "Invalid arguments.");
    return 1;
  }
}

