#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PID_FILE ".monitor_pid"

void handle_sigint(int sig) {
  (void)sig; /* Unused */
  printf("\n[Monitor] Received SIGINT (Ctrl+C). Cleaning up and terminating "
         "gracefully.\n");

  /* The requirement: when it ends, it deletes the above file */
  if (unlink(PID_FILE) < 0) {
    perror("[Monitor] Failed to remove .monitor_pid on exit");
  } else {
    printf("[Monitor] Successfully removed %s\n", PID_FILE);
  }

  exit(0);
}

void handle_sigusr1(int sig) {
  (void)sig; /* Unused */
  /* The requirement: responds to SIGUSR1 signals by writing a message on the
   * standard output */
  printf("[Monitor] Notification Received: A new report has been added to the "
         "system!\n");
}

int main(void) {
  /* 1. Create or overwrite the .monitor_pid file */
  int fd = open(PID_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (fd < 0) {
    perror("[Monitor] Failed to create .monitor_pid");
    return 1;
  }

  pid_t my_pid = getpid();
  char pid_str[32];
  int len = snprintf(pid_str, sizeof(pid_str), "%d\n", my_pid);

  if (write(fd, pid_str, len) < 0) {
    perror("[Monitor] Failed to write PID to file");
    close(fd);
    return 1;
  }
  close(fd);

  printf("[Monitor] Started successfully. PID: %d saved to %s\n", my_pid,
         PID_FILE);
  printf("[Monitor] Waiting for SIGUSR1 notifications...\n");

  /* 2. Setup sigaction (DO NOT USE signal() as per requirements) */
  struct sigaction sa_int;
  memset(&sa_int, 0, sizeof(sa_int));
  sa_int.sa_handler = handle_sigint;
  /* Block other signals while in handler */
  sigfillset(&sa_int.sa_mask);
  if (sigaction(SIGINT, &sa_int, NULL) < 0) {
    perror("[Monitor] sigaction for SIGINT failed");
    return 1;
  }

  struct sigaction sa_usr1;
  memset(&sa_usr1, 0, sizeof(sa_usr1));
  sa_usr1.sa_handler = handle_sigusr1;
  /* Ensure read/write operations restart if interrupted */
  sa_usr1.sa_flags = SA_RESTART;
  sigfillset(&sa_usr1.sa_mask);
  if (sigaction(SIGUSR1, &sa_usr1, NULL) < 0) {
    perror("[Monitor] sigaction for SIGUSR1 failed");
    return 1;
  }

  /* 3. Infinite Run Loop */
  while (1) {
    /* Suspend execution until any signal arrives. Extremely CPU efficient. */
    pause();
  }

  return 0;
}
