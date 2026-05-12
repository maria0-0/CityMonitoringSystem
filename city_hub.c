#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_CMD 256

void start_monitor() {
    pid_t hub_mon_pid = fork();

    if (hub_mon_pid < 0) {
        perror("fork hub_mon failed");
        return;
    }

    if (hub_mon_pid == 0) {
        /* Inside hub_mon (Background Child) */
        int pipe_fd[2];
        if (pipe(pipe_fd) < 0) {
            perror("pipe failed");
            exit(1);
        }

        pid_t monitor_pid = fork();
        if (monitor_pid < 0) {
            perror("fork monitor failed");
            exit(1);
        }

        if (monitor_pid == 0) {
            /* Inside modified monitor (Child of hub_mon) */
            close(pipe_fd[0]); /* Close read end */
            
            /* Redirect stdout to pipe */
            if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
                perror("dup2 failed");
                exit(1);
            }
            
            /* Redirect stderr to stdout so we catch errors in the pipe too */
            dup2(STDOUT_FILENO, STDERR_FILENO);
            
            close(pipe_fd[1]);

            execl("./monitor_reports", "monitor_reports", NULL);
            perror("execl failed");
            exit(1);
        } else {
            /* Inside hub_mon parent */
            close(pipe_fd[1]); /* Close write end */
            
            char buffer[512];
            ssize_t bytes_read;
            
            printf("[Hub-Mon] Background monitor manager started (PID: %d)\n", getpid());

            while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';
                /* Print monitor messages to the user's terminal */
                printf("\n--- Monitor Log ---\n%s--------------------\n", buffer);
                fflush(stdout);
                
                /* Check if monitor reported an error and ended */
                if (strstr(buffer, "[ERROR]")) {
                    break;
                }
            }

            if (bytes_read == 0) {
                printf("\n[Hub-Mon] Monitor process has disconnected/ended.\n");
            } else if (bytes_read < 0) {
                perror("read from pipe failed");
            }

            close(pipe_fd[0]);
            
            /* Wait for monitor process to fully cleanup */
            waitpid(monitor_pid, NULL, 0);
            printf("[Hub-Mon] Cleanup complete. Background manager exiting.\n");
            exit(0);
        }
    } else {
        /* Inside main City Hub */
        printf("Background monitor command issued. hub_mon PID: %d\n", hub_mon_pid);
    }
}

int main() {
    char input[MAX_CMD];

    printf("Welcome to City Hub (Phase 3)\n");
    printf("Commands: start_monitor, calculate_scores, exit\n");

    while (1) {
        printf("hub> ");
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0; /* Remove newline */

        if (strcmp(input, "exit") == 0) {
            printf("Exiting City Hub...\n");
            break;
        } else if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        } else if (strlen(input) == 0) {
            continue;
        } else if (strncmp(input, "calculate_scores", 16) == 0) {
            printf("Command 'calculate_scores' will be implemented in Week 11.\n");
        } else {
            printf("Unknown command: %s\n", input);
        }
    }

    return 0;
}
