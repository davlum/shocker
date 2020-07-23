#define _GNU_SOURCE
#include <sys/wait.h> // waitpid
#include <sched.h> // clone, CLONE_NEWUTS 
#include <unistd.h> // execvp, sethostname
#include <string.h> // strlen
#include <stdio.h> // puts, printf
#include <sys/mount.h> // mount

#define STACK_SIZE (1024 * 1024)

static char child_stack[STACK_SIZE]; /* Stack size for cloned child */

int child(void *arg) {
  sethostname("Container", 10);
  chdir("/root/container");
  chroot("/root/container");
  char **shell = (char **)arg;
  execvp(shell[0], shell);
  return 0;
}

int run(char *argv[]) {
  int child_pid = clone(child, child_stack+STACK_SIZE, 
      CLONE_NEWUTS | // unix timesharing, allows changing of hostname 
      CLONE_NEWPID | // procfs is one of a few ways to pass process info, OSX has no procfs 
      SIGCHLD, argv + 1);
  waitpid(child_pid, NULL, 0);
  return 0;
}

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

