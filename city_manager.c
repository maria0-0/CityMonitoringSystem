#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_PATH 512

typedef enum { ROLE_UNKNOWN, ROLE_INSPECTOR, ROLE_MANAGER } Role;

typedef struct {
  int id;
  char inspector[32];
  float lat;
  float lon;
  char category[32];
  int severity;
  time_t timestamp;
  char description[120];
} Report;

void print_permissions_string(mode_t mode) {
  char perms[10] = "---------";
  if (mode & S_IRUSR)
    perms[0] = 'r';
  if (mode & S_IWUSR)
    perms[1] = 'w';
  if (mode & S_IXUSR)
    perms[2] = 'x';
  if (mode & S_IRGRP)
    perms[3] = 'r';
  if (mode & S_IWGRP)
    perms[4] = 'w';
  if (mode & S_IXGRP)
    perms[5] = 'x';
  if (mode & S_IROTH)
    perms[6] = 'r';
  if (mode & S_IWOTH)
    perms[7] = 'w';
  if (mode & S_IXOTH)
    perms[8] = 'x';
  printf("%s", perms);
}

int run_permission_check(const char *filepath, Role role, int check_read,
                         int check_write, int check_exec) {
  struct stat st;
  if (stat(filepath, &st) < 0) {
    fprintf(stderr, "stat failed on %s: %s\n", filepath, strerror(errno));
    return 0;
  }
  mode_t mode = st.st_mode;

  if (role == ROLE_MANAGER) {
    if (check_read && !(mode & S_IRUSR))
      return 0;
    if (check_write && !(mode & S_IWUSR))
      return 0;
    if (check_exec && !(mode & S_IXUSR))
      return 0;
  } else if (role == ROLE_INSPECTOR) {
    if (check_read && !(mode & S_IRGRP))
      return 0;
    if (check_write && !(mode & S_IWGRP))
      return 0;
    if (check_exec && !(mode & S_IXGRP))
      return 0;
  }
  return 1;
}

int parse_condition(const char *input, char *field, char *op, char *value) {
  char buffer[256];
  strncpy(buffer, input, sizeof(buffer) - 1);
  buffer[255] = '\0';

  char *colon1 = strchr(buffer, ':');
  if (!colon1)
    return 0;

  char *colon2 = strchr(colon1 + 1, ':');
  if (!colon2)
    return 0;

  *colon1 = '\0';
  *colon2 = '\0';

  strcpy(field, buffer);
  strcpy(op, colon1 + 1);
  strcpy(value, colon2 + 1);
  return 1;
}

int match_condition(Report *r, const char *field, const char *op,
                    const char *value) {
  if (strcmp(field, "severity") == 0) {
    int v = atoi(value), r_v = r->severity;
    if (strcmp(op, "==") == 0) return r_v == v;
    if (strcmp(op, "!=") == 0) return r_v != v;
    if (strcmp(op, "<") == 0) return r_v < v;
    if (strcmp(op, "<=") == 0) return r_v <= v;
    if (strcmp(op, ">") == 0) return r_v > v;
    if (strcmp(op, ">=") == 0) return r_v >= v;
  } else if (strcmp(field, "category") == 0) {
    if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
    if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
  } else if (strcmp(field, "inspector") == 0) {
    if (strcmp(op, "==") == 0) return strcmp(r->inspector, value) == 0;
    if (strcmp(op, "!=") == 0) return strcmp(r->inspector, value) != 0;
  } else if (strcmp(field, "timestamp") == 0) {
    time_t v = atoll(value), r_v = r->timestamp;
    if (strcmp(op, "==") == 0) return r_v == v;
    if (strcmp(op, "!=") == 0) return r_v != v;
    if (strcmp(op, "<") == 0) return r_v < v;
    if (strcmp(op, "<=") == 0) return r_v <= v;
    if (strcmp(op, ">") == 0) return r_v > v;
    if (strcmp(op, ">=") == 0) return r_v >= v;
  }
  return 0;
}

void log_action(const char *district_id, const char *username,
                const char *role_str, const char *action) {
  char filepath[MAX_PATH];
  snprintf(filepath, MAX_PATH, "%s/logged_district", district_id);

  Role r = strcmp(role_str, "manager") == 0 ? ROLE_MANAGER : ROLE_INSPECTOR;
  if (!run_permission_check(filepath, r, 0, 1, 0)) {
    fprintf(stderr, "Permission denied: %s cannot write to log %s\n", role_str,
            filepath);
    return;
  }

  int fd = open(filepath, O_WRONLY | O_APPEND);
  if (fd < 0) {
    perror("Failed to open log");
    return;
  }
  char buffer[256];
  time_t now = time(NULL);
  int len = snprintf(buffer, sizeof(buffer), "%ld\t%s\t%s\t%s\n", now, username,
                     role_str, action);
  write(fd, buffer, len);
  close(fd);
}

void ensure_district(const char *district_id) {
  if (mkdir(district_id, S_IRWXU | S_IRGRP | S_IXGRP) < 0 && errno != EEXIST) {
    perror("mkdir");
    exit(1);
  }
  chmod(district_id, S_IRWXU | S_IRGRP | S_IXGRP);

  char filepath[MAX_PATH];
  int fd;

  snprintf(filepath, MAX_PATH, "%s/reports.dat", district_id);
  if ((fd = open(filepath, O_CREAT | O_RDWR, 0664)) >= 0) {
    close(fd);
  }
  chmod(filepath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

  char symlink_path[MAX_PATH];
  snprintf(symlink_path, MAX_PATH, "active_reports-%s", district_id);
  struct stat st;
  if (lstat(symlink_path, &st) < 0) {
    symlink(filepath, symlink_path);
  } else {
    if (stat(symlink_path, &st) < 0) {
      printf("Warning: dangling symlink %s detected!\n", symlink_path);
    }
  }

  snprintf(filepath, MAX_PATH, "%s/district.cfg", district_id);
  if ((fd = open(filepath, O_CREAT | O_RDWR, 0640)) >= 0) {
    fstat(fd, &st);
    if (st.st_size == 0) {
      write(fd, "0\n", 2);
    }
    close(fd);
  }
  chmod(filepath, S_IRUSR | S_IWUSR | S_IRGRP);

  snprintf(filepath, MAX_PATH, "%s/logged_district", district_id);
  if ((fd = open(filepath, O_CREAT | O_RDWR, 0644)) >= 0) {
    close(fd);
  }
  chmod(filepath, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void cmd_add(const char *district_id, const char *username,
             const char *role_str) {
  char filepath[MAX_PATH];
  snprintf(filepath, MAX_PATH, "%s/reports.dat", district_id);

  Role role = strcmp(role_str, "manager") == 0 ? ROLE_MANAGER : ROLE_INSPECTOR;
  if (!run_permission_check(filepath, role, 0, 1, 0)) {
    printf("Permission denied: %s cannot write to %s\n", role_str, filepath);
    return;
  }

  int fd = open(filepath, O_RDWR | O_APPEND);
  if (fd < 0) {
    perror("open");
    return;
  }

  Report r;
  memset(&r, 0, sizeof(Report));

  struct stat st;
  fstat(fd, &st);
  r.id = st.st_size / sizeof(Report);
  strncpy(r.inspector, username, sizeof(r.inspector) - 1);
  r.timestamp = time(NULL);

  printf("X: ");
  if (scanf("%f", &r.lat) != 1) {
    close(fd);
    return;
  }
  printf("Y: ");
  if (scanf("%f", &r.lon) != 1) {
    close(fd);
    return;
  }

  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;

  printf("Category (road/lighting/flooding/other): ");
  if (fgets(r.category, sizeof(r.category), stdin)) {
    r.category[strcspn(r.category, "\n")] = 0;
  }

  printf("Severity level (1/2/3):");
  if (scanf("%d", &r.severity) != 1) {
    close(fd);
    return;
  }
  while ((c = getchar()) != '\n' && c != EOF)
    ;

  printf("Description:");
  if (fgets(r.description, sizeof(r.description), stdin)) {
    r.description[strcspn(r.description, "\n")] = 0;
  }

  write(fd, &r, sizeof(Report));
  close(fd);
  log_action(district_id, username, role_str, "add");
}

void cmd_list(const char *district_id, const char *username,
              const char *role_str) {
  char filepath[MAX_PATH];
  snprintf(filepath, MAX_PATH, "%s/reports.dat", district_id);

  Role role = strcmp(role_str, "manager") == 0 ? ROLE_MANAGER : ROLE_INSPECTOR;
  if (!run_permission_check(filepath, role, 1, 0, 0)) {
    printf("Permission denied: %s cannot read %s\n", role_str, filepath);
    return;
  }

  int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return;
  }

  struct stat st;
  fstat(fd, &st);
  printf("File size: %lld bytes\n", (long long)st.st_size);
  printf("Last modified: %s", ctime(&st.st_mtime));
  printf("Permissions: ");
  print_permissions_string(st.st_mode);
  printf("\n");

  Report r;
  while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
    printf("Report ID: %d, Inspector: %s, Severity: %d, Category: %s\n", r.id,
           r.inspector, r.severity, r.category);
  }
  close(fd);
  log_action(district_id, username, role_str, "list");
}

void cmd_view(const char *district_id, const char *username,
              const char *role_str, int report_id) {
  char filepath[MAX_PATH];
  snprintf(filepath, MAX_PATH, "%s/reports.dat", district_id);

  Role role = strcmp(role_str, "manager") == 0 ? ROLE_MANAGER : ROLE_INSPECTOR;
  if (!run_permission_check(filepath, role, 1, 0, 0)) {
    printf("Permission denied: %s cannot read %s\n", role_str, filepath);
    return;
  }

  int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return;
  }

  struct stat st;
  fstat(fd, &st);

  off_t target_offset = report_id * sizeof(Report);
  if (target_offset >= st.st_size) {
    printf("Error: Report ID %d not found (file too small).\n", report_id);
    close(fd);
    return;
  }

  lseek(fd, target_offset, SEEK_SET);

  Report r;
  if (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
    printf("=== Report %d ===\n", r.id);
    printf("Inspector: %s\n", r.inspector);
    printf("Location: %.6f, %.6f\n", r.lat, r.lon);
    printf("Category: %s\n", r.category);
    printf("Severity: %d\n", r.severity);
    printf("Time: %s", ctime(&r.timestamp));
    printf("Description: %s\n", r.description);
  }
  close(fd);
  log_action(district_id, username, role_str, "view");
}

void cmd_remove_report(const char *district_id, const char *username,
                       const char *role_str, int report_id) {
  if (strcmp(role_str, "manager") != 0) {
    printf("Error: Only managers can remove reports.\n");
    return;
  }

  char filepath[MAX_PATH];
  snprintf(filepath, MAX_PATH, "%s/reports.dat", district_id);

  if (!run_permission_check(filepath, ROLE_MANAGER, 1, 1, 0)) {
    printf("Permission denied: Manager cannot write to %s\n", filepath);
    return;
  }

  int fd = open(filepath, O_RDWR);
  if (fd < 0) {
    perror("open");
    return;
  }

  struct stat st;
  fstat(fd, &st);
  off_t file_size = st.st_size;
  off_t target_offset = report_id * sizeof(Report);
  if (target_offset >= file_size) {
    printf("Error: Report ID %d out of bounds.\n", report_id);
    close(fd);
    return;
  }

  off_t current_offset = target_offset + sizeof(Report);
  Report temp;
  while (current_offset < file_size) {
    lseek(fd, current_offset, SEEK_SET);
    if (read(fd, &temp, sizeof(Report)) != sizeof(Report))
      break;

    temp.id = temp.id - 1;

    lseek(fd, current_offset - sizeof(Report), SEEK_SET);
    write(fd, &temp, sizeof(Report));
    current_offset += sizeof(Report);
  }

  ftruncate(fd, file_size - sizeof(Report));
  close(fd);
  log_action(district_id, username, role_str, "remove_report");
}

void cmd_update_threshold(const char *district_id, const char *username,
                          const char *role_str, int new_val) {
  if (strcmp(role_str, "manager") != 0) {
    printf("Error: Only managers can update threshold.\n");
    return;
  }
  char filepath[MAX_PATH];
  snprintf(filepath, MAX_PATH, "%s/district.cfg", district_id);

  if (!run_permission_check(filepath, ROLE_MANAGER, 0, 1, 0)) {
    printf("Permission denied: Manager cannot write to %s\n", filepath);
    return;
  }

  struct stat st;
  if (stat(filepath, &st) == 0) {
    if ((st.st_mode & 0777) != 0640) {
      printf("Error: Permissions for district.cfg must be exactly 640!\n");
      return;
    }
  }

  int fd = open(filepath, O_WRONLY | O_TRUNC);
  if (fd < 0) {
    perror("open");
    return;
  }
  char buf[16];
  int len = snprintf(buf, sizeof(buf), "%d\n", new_val);
  write(fd, buf, len);
  close(fd);
  log_action(district_id, username, role_str, "update_threshold");
}

void cmd_filter(const char *district_id, const char *username,
                const char *role_str, int arg_start, int argc, char *argv[]) {
  char filepath[MAX_PATH];
  snprintf(filepath, MAX_PATH, "%s/reports.dat", district_id);

  Role role = strcmp(role_str, "manager") == 0 ? ROLE_MANAGER : ROLE_INSPECTOR;
  if (!run_permission_check(filepath, role, 1, 0, 0)) {
    printf("Permission denied: %s cannot read %s\n", role_str, filepath);
    return;
  }

  int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return;
  }

  Report r;
  while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
    int all_match = 1;
    for (int i = arg_start; i < argc; i++) {
      char field[64], op[16], value[128];
      if (parse_condition(argv[i], field, op, value)) {
        if (!match_condition(&r, field, op, value)) {
          all_match = 0;
          break;
        }
      } else {
        printf("Invalid condition format: %s\n", argv[i]);
        all_match = 0;
        break;
      }
    }
    if (all_match) {
      printf("Report ID: %d, Inspector: %s, Severity: %d, Category: %s\n", r.id,
             r.inspector, r.severity, r.category);
    }
  }
  close(fd);
  log_action(district_id, username, role_str, "filter");
}

int main(int argc, char *argv[]) {
  Role role = ROLE_UNKNOWN;
  char username[64] = "";
  char command[64] = "";
  char district_id[128] = "";
  int extra_arg = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
      i++;
      if (strcmp(argv[i], "inspector") == 0)
        role = ROLE_INSPECTOR;
      else if (strcmp(argv[i], "manager") == 0)
        role = ROLE_MANAGER;
      else {
        fprintf(stderr, "Unknown role: %s\n", argv[i]);
        return 1;
      }
    } else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
      i++;
      strncpy(username, argv[i], sizeof(username) - 1);
    } else if (strncmp(argv[i], "--", 2) == 0 && strlen(argv[i]) > 2) {
      strncpy(command, argv[i], sizeof(command) - 1);
      if (i + 1 < argc) {
        i++;
        strncpy(district_id, argv[i], sizeof(district_id) - 1);
      }
      if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0) {
        i++;
        extra_arg = atoi(argv[i]);
      }
    }
  }

  if (role == ROLE_UNKNOWN) {
    fprintf(stderr, "Role is required (--role manager|inspector)\n");
    return 1;
  }
  if (strlen(username) == 0) {
    fprintf(stderr, "User name is required (--user <name>)\n");
    return 1;
  }
  if (strlen(command) == 0 || strlen(district_id) == 0) {
    fprintf(stderr,
            "Command and district are required (e.g. --add downtown)\n");
    return 1;
  }

  const char *role_str = (role == ROLE_MANAGER) ? "manager" : "inspector";

  ensure_district(district_id);

  if (strcmp(command, "--add") == 0) {
    cmd_add(district_id, username, role_str);
  } else if (strcmp(command, "--list") == 0) {
    cmd_list(district_id, username, role_str);
  } else if (strcmp(command, "--view") == 0) {
    cmd_view(district_id, username, role_str, extra_arg);
  } else if (strcmp(command, "--remove_report") == 0) {
    cmd_remove_report(district_id, username, role_str, extra_arg);
  } else if (strcmp(command, "--update_threshold") == 0) {
    cmd_update_threshold(district_id, username, role_str, extra_arg);
  } else if (strcmp(command, "--filter") == 0) {
    int arg_start_idx = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], district_id) == 0 && strcmp(argv[i-1], "--filter") == 0) {
            arg_start_idx = i + 1;
            break;
        }
    }
    if (arg_start_idx != -1 && arg_start_idx < argc) {
        cmd_filter(district_id, username, role_str, arg_start_idx, argc, argv);
    } else {
        printf("Error: --filter requires at least one condition\n");
    }
  } else {
    printf("Command %s is not yet implemented.\n", command);
  }

  return 0;
}
