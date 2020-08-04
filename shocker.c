#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <sched.h> // clone, all the clone Flags
#include <unistd.h> // execvp, sethostname
#include <string.h> // strlen
#include <stdio.h> // puts, printf
#include <sys/mount.h> // mount
#include <sys/stat.h> // mkdir
#include <sys/types.h> // Flags for sys stuff
#include <sys/mman.h>
#include <sys/wait.h> // waitpid
#include <errno.h>

static const size_t stack_size = 2 * 1024 * 1024;  /* Stack size for cloned child */

//   docker run image <cmd> <params>
//./shocker run       <cmd> <params>

int child(void *arg) {
  static const char *hostname = "blah";
  if (sethostname(hostname, strnlen(hostname, 100)) == -1) {
    perror("sethostname failed with: ");
    goto fail_sethostname;
  }

  if (chdir("/root/container") == -1) {
    perror("sethostname failed with: ");
    goto fail_chdir;
  }

  if (chroot("/root/container") == -1) {
    perror("sethostname failed with: ");
    goto fail_chroot;
  }

  if (mount("proc", "/proc", "proc", 0, NULL) == -1) {
    perror("mount failed with: ");
    goto fail_mount;
  }

  char **shell = arg;

  if (execvp(shell[0], shell) == -1) {
    perror("execvp failed with: ");
    goto fail_execvp;
  }

  if (umount("/root/container/proc") == -1) {
    perror("umount failed with: ");
    goto fail_umount;
  }

  exit(EXIT_SUCCESS);

  fail_umount:
  fail_execvp:
    umount("/root/container/proc");
  fail_mount:
  fail_chroot:
  fail_chdir:
  fail_sethostname:
    exit(EXIT_FAILURE);
}

static bool intWriter(char* path, int num) {
  FILE *fp = fopen(path, "w");

  if (fp == NULL) {
    fprintf(stderr, "fopen %s failed with: %s", path, strerror(errno));
    goto fail_fopen;
  }

  if (fprintf(fp, "%d", num) == -1) {
    fprintf(stderr, "fprintf %s with %d failed with: %s", path, num, strerror(errno));
    goto fail_fprintf;
  }

  fclose(fp);
  return true;

  fail_fprintf:
    fclose(fp);
  fail_fopen:
    return false;
}

int run(char *argv[]) {

  /* Allocate memory to be used for the stack of the child */
  char *stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

  if (stack == MAP_FAILED) {
    perror("mmap failed with: ");
    goto fail_mmap;
  }

  // Fork and exec pattern is now the clone and exec pattern
  int child_pid = clone(child, stack + stack_size,
      CLONE_NEWUTS | // unix timesharing, allows changing of hostname 
      CLONE_NEWPID | // procfs is one of a few ways to pass process info, OSX has no procfs
      CLONE_NEWNS | // first namespace (for mounts). Private copy of its namespace.
      CLONE_NEWUSER | // user related
      SIGCHLD, // signal sent to the parent when the child terminates
      argv + 1 // drop `run` arg
  );

  if (child_pid == -1) {
    perror("clone failed with: ");
    goto fail_clone;
  }

  const char *cgroup_dir = "/sys/fs/cgroup/pids/shocker";

  // make a new cgroup
  if (mkdir(cgroup_dir, 0644) == -1) {
    perror("mkdir failed with: ");
    goto fail_mkdir;
  }

  // Restrain procs in this cgroup to a max of 5 processes
  if (!intWriter("/sys/fs/cgroup/pids/shocker/pids.max", 5)) goto fail_pidsmax;

  // Put the child in the new cgroup
  if (!intWriter("/sys/fs/cgroup/pids/shocker/cgroup.procs", child_pid)) goto fail_cgroupprocs;

  if (waitpid(child_pid, NULL , 0) == -1) {
    perror("waitpid failed with ");
    goto fail_waitpid;
  }

  exit(EXIT_SUCCESS);

  fail_waitpid:
  fail_cgroupprocs:
  fail_pidsmax:
    rmdir(cgroup_dir);
  fail_mkdir:
    kill(child_pid, SIGTERM);
  fail_clone:
    munmap(stack, stack_size);
  fail_mmap:
    exit(EXIT_FAILURE);;
}


// need just stdio, and string for main
int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Not enough arguments.\n");
    exit(EXIT_FAILURE);
  }
  argv += 1; // drop binary argument
  if (strncmp(argv[0], "run", 5) == 0) {
      run(argv);
  } else {
    printf( "Invalid arguments.");
    exit(EXIT_FAILURE);
  }
}

