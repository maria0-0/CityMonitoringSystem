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
  (void)sig;
  printf("\n[INFO] Termination signal received. Cleaning up...\n");
  
  if (unlink(PID_FILE) < 0) {
    perror("[ERROR] Failed to remove .monitor_pid on exit");
  } else {
    printf("[INFO] Successfully removed %s\n", PID_FILE);
  }
  
  exit(0);
}

void handle_sigusr1(int sig) {
  (void)sig;
  printf("[NOTIFY] A new report has been added to the system!\n");
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  int fd = open(PID_FILE, O_RDONLY);
  if (fd >= 0) {
    char pid_buf[32];
    memset(pid_buf, 0, sizeof(pid_buf));
    if (read(fd, pid_buf, sizeof(pid_buf) - 1) > 0) {
      pid_t existing_pid = (pid_t)atoi(pid_buf);
      if (existing_pid > 0 && kill(existing_pid, 0) == 0) {
        printf("[ERROR] Monitor already running with PID %d\n", existing_pid);
        close(fd);
        return 1;
      }
    }
    close(fd);
  }

  fd = open(PID_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (fd < 0) {
    perror("[ERROR] Failed to create .monitor_pid");
    return 1;
  }

  pid_t my_pid = getpid();
  char pid_str[32];
  int len = snprintf(pid_str, sizeof(pid_str), "%d\n", my_pid);
  
  if (write(fd, pid_str, len) < 0) {
    perror("[ERROR] Failed to write PID to file");
    close(fd);
    return 1;
  }
  close(fd);

  printf("[INFO] Monitor started. PID: %d\n", my_pid);
  printf("[INFO] Waiting for notifications...\n");

  struct sigaction sa_int;
  memset(&sa_int, 0, sizeof(sa_int));
  sa_int.sa_handler = handle_sigint;
  sigfillset(&sa_int.sa_mask); 
  if (sigaction(SIGINT, &sa_int, NULL) < 0) {
    perror("[ERROR] sigaction for SIGINT failed");
    return 1;
  }

  struct sigaction sa_usr1;
  memset(&sa_usr1, 0, sizeof(sa_usr1));
  sa_usr1.sa_handler = handle_sigusr1;
  sa_usr1.sa_flags = SA_RESTART; 
  sigfillset(&sa_usr1.sa_mask);
  if (sigaction(SIGUSR1, &sa_usr1, NULL) < 0) {
    perror("[ERROR] sigaction for SIGUSR1 failed");
    return 1;
  }

  while (1) {
    pause(); 
  }

  return 0;
}
