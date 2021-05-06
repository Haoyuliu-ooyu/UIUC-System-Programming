/**
 * shell
 * CS 241 - Spring 2021
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sstring.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

extern char *optarg;

typedef struct process
{
   char *command;
   pid_t pid;
} process;

static vector *process_vector;
static vector *history_vector;
static char *history_file = NULL;
static FILE *source = NULL;
static int redir_flag = 0;

//functions
process *create_process(pid_t pid, char *comm);
size_t process_index(pid_t pid);
char* handle_history(char *file);
void handle_sig(int sig_name);
int external(char *cmd);
int execute(char *cmd, int logic);
int operator_and(char *cmd);
int operator_or(char *cmd);
int operator_sep(char *cmd);
int operator_append(char *cmd);
int operator_out(char *cmd);
void print_ps();
void exit_shell(int status);

//main function:
int shell(int argc, char *argv[]) {
   // TODO: This is the entry point for your shell.
   if (!(argc == 1 || argc == 3 || argc == 5)) {
      print_usage();
      exit(1);
   }
   signal(SIGINT, handle_sig);
   int pid = getpid();
   //
   process *shell = create_process(pid, argv[0]);
   process_vector = shallow_vector_create();
   vector_push_back(process_vector, shell);
   //
   history_vector = string_vector_create();
   //
   char *cwd = malloc(256);
   source = stdin;
   if (getcwd(cwd, 256) == NULL) {
      exit_shell(1);
   }
   print_prompt(getcwd(cwd, 256), pid);
   //
   int argument;
   while ((argument = getopt(argc, argv, "f:h:")) != -1) {
      if (argument == 'h') {
         history_file = handle_history(optarg);
      }
      else if (argument == 'f') {
         FILE *script = fopen(optarg, "r");
         if (script == NULL) {
            print_script_file_error();
            exit_shell(1);
         }
         source = script;
      }
      else {
         print_usage();
         exit_shell(1);
      }
   }
   //
   char *buffer = NULL;
   size_t length = 0;
   while (getline(&buffer, &length, source) != -1) {
      int status;
      // waitpid((pid_t) -1, &status, WNOHANG);
      size_t i = 0;
      for (; i < vector_size(process_vector); i++) {
         process *p = vector_get(process_vector, i);
         waitpid(p->pid, &status, WNOHANG);
      }
      // deal with newline
      if (strcmp(buffer, "\n") == 0){}
      else {
         if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
         }
         char *copy = malloc(strlen(buffer));
         strcpy(copy, buffer);
         execute(copy, 0);
      }
      if (getcwd(cwd, 256) == NULL) {
         exit_shell(1);
      }
      print_prompt(getcwd(cwd, 256), pid);
      fflush(stdout);
   }
   free(buffer);
   exit_shell(0);
   return 0;
}

process *create_process(pid_t pid, char *comm) {
   process *ptr = malloc(sizeof(process));
   ptr->pid = pid;
   ptr->command = malloc(sizeof(char) * (strlen(comm) + 1));
   strcpy(ptr->command, comm);
   return ptr;
}

size_t process_index(pid_t pid) {
   ssize_t index = -1;
   size_t i = 0;
   for (; i < vector_size(process_vector); i++) {
      process *ptr = vector_get(process_vector, i);
      if (ptr->pid == pid) {
         index = i;
         break;
      }
   }
   return index;
}

char *handle_history(char *file) {
   FILE *fd = fopen(get_full_path(file), "r");
   if (fd) {
      char *buffer = NULL;
      size_t length = 0;
      while (getline(&buffer, &length, fd) != -1) {
         if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
            vector_push_back(history_vector, buffer);
         }
      }
      free(buffer);
      fclose(fd);
      return get_full_path(file);
   }
   else {
      // treat as empty. create file
      fd = fopen(file, "w");
   }
   fclose(fd);
   return get_full_path(file);
}

void handle_sig(int sig_name) {
   if (sig_name == SIGINT) {/**/}
}

int external(char *cmd) {
   int background = 0;
   pid_t pid = fork();
   process *p = create_process(pid, cmd);
   vector_push_back(process_vector, p);
   sstring *s = cstr_to_sstring(cmd);
   vector *splited = sstring_split(s, ' ');
   size_t size = vector_size(splited);
   char *first_string = vector_get(splited, 0);
   // check if it is a background process
   char *last_string = vector_get(splited, size - 1);
   if (size >= 2) {
      if (strcmp(last_string, "&") == 0) {
         background = 1;
         vector_set(splited, size - 1, NULL);
      }
   }
   char *arr[size + 1];
   size_t i = 0;
   for (; i < size; i++) {
      arr[i] = vector_get(splited, i);
   }
   arr[size] = NULL;
   if (pid == -1) {
      print_fork_failed();
      exit_shell(1);
      return 1;
   }
   if (pid > 0) {
      if (!redir_flag) {
         print_command_executed(pid);
      }
      int status = 0;
      if (background) {
         waitpid(pid, &status, WNOHANG);
      }
      else {
         pid_t pid_w = waitpid(pid, &status, 0);
         if (pid_w == -1) {
            print_wait_failed();
         }
         else if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
               //don't exit
               return 1;
            }
            fflush(stdout);
         }
         else if (WIFSIGNALED(status)) {
         }
         sstring_destroy(s);
         vector_destroy(splited);
         return status;
      }
   }
   else if (pid == 0) {
      if (background) {
         if (setpgid(getpid(), getpid()) == -1)
         {
            print_setpgid_failed();
            fflush(stdout);
            exit_shell(1);
         }
      }
      fflush(stdout);
      execvp(first_string, arr);
      // failed
      print_exec_failed(cmd);
      exit(1);
   }
   sstring_destroy(s);
   vector_destroy(splited);
   return 1;
}

int execute(char *cmd, int logic) {
   if ((strstr(cmd, "&&")) != NULL) {
      vector_push_back(history_vector, cmd);
      return operator_and(cmd);
   }
   else if ((strstr(cmd, "||")) != NULL) {
      vector_push_back(history_vector, cmd);
      return operator_or(cmd);
   }
   else if ((strstr(cmd, ";")) != NULL) {
      vector_push_back(history_vector, cmd);
      return operator_sep(cmd);
   }
   else if ((strstr(cmd, ">>")) != NULL) {
      redir_flag = 1;
      vector_push_back(history_vector, cmd);
      return operator_append(cmd);
   }
   else if ((strstr(cmd, ">")) != NULL) {
      redir_flag = 1;
      vector_push_back(history_vector, cmd);
      return operator_out(cmd);
   }
   else if ((strstr(cmd, "<")) != NULL) {
      vector_push_back(history_vector, cmd);
      return 0;
   }
   sstring *s = cstr_to_sstring(cmd);
   vector *splited = sstring_split(s, ' ');
   int valid_command = 0;
   size_t size = vector_size(splited);
   char *first_string = vector_get(splited, 0);
   if (size != 0) {
      if (strcmp(first_string, "cd") == 0) {
         if (size > 1) {
            valid_command = 1;
            if (!logic) {
               vector_push_back(history_vector, cmd);
            }
            int result = chdir(vector_get(splited, 1));
            if (result != 0) {
               print_no_directory(vector_get(splited, 1));
               result = 1;
            }
            result = 0;
            sstring_destroy(s);
            vector_destroy(splited);
            return result;
         }
      }
      else if (strcmp(first_string, "!history") == 0) {
         if (size == 1) {
            valid_command = 1;
            for (size_t i = 0; i < vector_size(history_vector); i++) {
               print_history_line(i, vector_get(history_vector, i));
            }
            sstring_destroy(s);
            vector_destroy(splited);
            return 0;
         }
      }
      else if (first_string[0] == '#') {
         if (size == 1 && strlen(first_string) != 1) {
            valid_command = 1;
            size_t index = atoi(first_string + 1);
            if (index < vector_size(history_vector)) {
               char *exec_cmd = vector_get(history_vector, index);
               print_command(exec_cmd);
               sstring_destroy(s);
               vector_destroy(splited);
               return execute(exec_cmd, 0);
            }
            print_invalid_index();
            sstring_destroy(s);
            vector_destroy(splited);
            return 1;
         }
      }
      else if (first_string[0] == '!') {
         if (size >= 1) {
            valid_command = 1;
            char *prefix = first_string + 1;
            for (size_t i = vector_size(history_vector); i > 0; --i) {
               char *another_cmd = vector_get(history_vector, i - 1);
               if (strncmp(another_cmd, prefix, strlen(prefix)) == 0) {
                  print_command(another_cmd);
                  sstring_destroy(s);
                  vector_destroy(splited);
                  return execute(another_cmd, 0);
               }
            }
            print_no_history_match();
            sstring_destroy(s);
            vector_destroy(splited);
            return 1;
         }
      }
      else if (strcmp(first_string, "ps") == 0) {
         if (!logic) {
            vector_push_back(history_vector, cmd);
         }
         if (size == 1) {
            valid_command = 1;
            print_ps();
            return 0;
         }
      }
      else if (strcmp(first_string, "kill") == 0) {
         if (!logic) {
            vector_push_back(history_vector, cmd);
         }
         if (size == 2) {
            valid_command = 1;
            pid_t target = atoi(vector_get(splited, 1));
            ssize_t index = process_index(target);
            if (index == -1) {
               print_no_process_found(target);
               sstring_destroy(s);
               vector_destroy(splited);
               return 1;
            }
            kill(target, SIGKILL);
            print_killed_process(target, ((process *)vector_get(process_vector, index))->command);
            sstring_destroy(s);
            vector_destroy(splited);
            return 0;
         }
      }
      else if (strcmp(first_string, "stop") == 0) {
         if (!logic) {
            vector_push_back(history_vector, cmd);
         }
         if (size == 2) {
            valid_command = 1;
            pid_t target = atoi(vector_get(splited, 1));
            ssize_t index = process_index(target);
            if (index == -1) {
               print_no_process_found(target);
               sstring_destroy(s);
               vector_destroy(splited);
               return 1;
            }
            kill(target, SIGSTOP);
            print_stopped_process(target, ((process *)vector_get(process_vector, index))->command);
            sstring_destroy(s);
            vector_destroy(splited);
            return 0;
         }
      }
      else if (strcmp(first_string, "cont") == 0) {
         if (!logic) {
            vector_push_back(history_vector, cmd);
         }
         if (size == 2) {
            valid_command = 1;
            pid_t target = atoi(vector_get(splited, 1));
            ssize_t index = process_index(target);
            if (index == -1) {
               print_no_process_found(target);
               sstring_destroy(s);
               vector_destroy(splited);
               return 1;
            }
            kill(target, SIGCONT);
            print_continued_process(target, ((process *)vector_get(process_vector, index))->command);
            sstring_destroy(s);
            vector_destroy(splited);
            return 0;
         }
      }
      else if (strcmp(first_string, "exit") == 0) {
         if (size == 1) {
            valid_command = 1;
            exit_shell(0);
         }
      }
      else {
         if (!logic) {
            vector_push_back(history_vector, cmd);
         }
         valid_command = 1;
         sstring_destroy(s);
         vector_destroy(splited);
         fflush(stdout);
         return external(cmd);
      }
   }
   if (valid_command == 0) {
      print_invalid_command(cmd);
   }
   sstring_destroy(s);
   vector_destroy(splited);
   return 1;
}

int operator_and(char *cmd) {
   char *location = strstr(cmd, "&&");
   size_t total_len = strlen(cmd);
   size_t first_cmd_len = location - cmd;
   size_t second_cmd_len = total_len - (first_cmd_len + 3);
   char first_cmd[first_cmd_len];
   char second_cmd[second_cmd_len + 1];
   strncpy(first_cmd, cmd, first_cmd_len);
   strncpy(second_cmd, (location + 3), second_cmd_len);
   first_cmd[first_cmd_len - 1] = '\0';
   second_cmd[second_cmd_len] = '\0';
   //
   int first_status;
   if ((first_status = execute(first_cmd, 1)) == 0) {
      return execute(second_cmd, 1);
   }
   return 1;
}

int operator_or(char *cmd) {
   char *location = strstr(cmd, "||");
   size_t total_len = strlen(cmd);
   size_t first_cmd_len = location - cmd;
   size_t second_cmd_len = total_len - (first_cmd_len + 3);
   char first_cmd[first_cmd_len];
   char second_cmd[second_cmd_len + 1];
   strncpy(first_cmd, cmd, first_cmd_len);
   strncpy(second_cmd, (location + 3), second_cmd_len);
   first_cmd[first_cmd_len - 1] = '\0';
   second_cmd[second_cmd_len] = '\0';
   //
   int first_status;
   if ((first_status = execute(first_cmd, 1)) != 0) {
      return execute(second_cmd, 1);
   }
   return 0;
}

int operator_sep(char *cmd) {
   char *location = strstr(cmd, ";");
   size_t total_len = strlen(cmd);
   size_t first_cmd_len = location - cmd;
   size_t second_cmd_len = total_len - (first_cmd_len + 2);
   char first_cmd[first_cmd_len + 1];
   char second_cmd[second_cmd_len + 1];
   strncpy(first_cmd, cmd, first_cmd_len);
   strncpy(second_cmd, (location + 2), second_cmd_len);
   first_cmd[first_cmd_len] = '\0';
   second_cmd[second_cmd_len] = '\0';
   //
   int result1 = execute(first_cmd, 1);
   int result2 = execute(second_cmd, 1);
   return result1 && result2;
}

int operator_append(char *cmd) {
   char *location = strstr(cmd, ">>");
   size_t total_len = strlen(cmd);
   size_t first_cmd_len = location - cmd;
   size_t second_cmd_len = total_len - (first_cmd_len + 3);
   char first_cmd[first_cmd_len];
   char second_cmd[second_cmd_len + 1];
   strncpy(first_cmd, cmd, first_cmd_len);
   strncpy(second_cmd, (location + 3), second_cmd_len);
   first_cmd[first_cmd_len - 1] = '\0';
   second_cmd[second_cmd_len] = '\0';
   //
   FILE *fd = fopen(second_cmd, "a");
   int fd_num = fileno(fd);
   int original = dup(fileno(stdout));
   fflush(stdout);
   dup2(fd_num, fileno(stdout));
   execute(first_cmd, 1);
   fflush(stdout);
   close(fd_num);
   dup2(original, fileno(stdout));
   //
   redir_flag = 0;
   return 0;
}

int operator_out(char *cmd) {
   char *location = strstr(cmd, ">");
   size_t total_len = strlen(cmd);
   size_t first_cmd_len = location - cmd;
   size_t second_cmd_len = total_len - (first_cmd_len + 2);
   char first_cmd[first_cmd_len];
   char second_cmd[second_cmd_len + 1];
   strncpy(first_cmd, cmd, first_cmd_len);
   strncpy(second_cmd, (location + 2), second_cmd_len);
   first_cmd[first_cmd_len - 1] = '\0';
   second_cmd[second_cmd_len] = '\0';
   //
   FILE *fd = fopen(second_cmd, "w");
   int fd_num = fileno(fd);
   int original = dup(fileno(stdout));
   fflush(stdout);
   dup2(fd_num, fileno(stdout));
   execute(first_cmd, 1);
   fflush(stdout);
   close(fd_num);
   dup2(original, fileno(stdout));
   redir_flag = 0;
   return 0;
}

void print_ps() {
   print_process_info_header();
   size_t i = 0;
   for (; i < vector_size(process_vector); i++) {
      process *p = vector_get(process_vector, i);
      // if process is running
      if (kill(p->pid, 0) != -1) {
         process_info info;
         info.command = p->command;
         info.pid = p->pid;
         char path[100];
         snprintf(path, sizeof(path), "/proc/%d/stat", p->pid);
         unsigned long long starttime = 0;
         unsigned long time = 0;
         unsigned long long btime = 0;
         char time_str[20];
         //
         FILE *fd = fopen("/proc/stat", "r");
         if (fd){
            char *buffer2 = NULL;
            size_t len;
            ssize_t bytes_read = getdelim(&buffer2, &len, '\0', fd);
            if (bytes_read != -1) {
               sstring *s = cstr_to_sstring(buffer2);
               // printf("%s\n", buffer);
               vector *lines = sstring_split(s, '\n');
               size_t i = 0;
               for (; i < vector_size(lines); i++) {
                  if (strncmp("btime", vector_get(lines, i), 5) == 0) {
                     char str[10];
                     sscanf(vector_get(lines, i), "%s %llu", str, &btime);
                  }
               }
               vector_destroy(lines);
               sstring_destroy(s);
            }
         }
         fclose(fd);
         FILE *f_d = fopen(path, "r");
         if (f_d) {
            char *buffer = NULL;
            size_t len;
            ssize_t bytes_read = getdelim(&buffer, &len, '\0', fd);
            if (bytes_read != -1) {
               sstring *s = cstr_to_sstring(buffer);
               vector *nums = sstring_split(s, ' ');
               long int nthreads = 0;
               sscanf(vector_get(nums, 19), "%ld", &nthreads);
               info.nthreads = nthreads;
               unsigned long int vsize = 0;
               sscanf(vector_get(nums, 22), "%lu", &vsize);
               info.vsize = vsize / 1024;
               char state;
               sscanf(vector_get(nums, 2), "%c", &state);
               info.state = state;
               unsigned long utime = 0;
               unsigned long stime = 0;
               sscanf(vector_get(nums, 14), "%lu", &stime);
               sscanf(vector_get(nums, 13), "%lu", &utime);
               time = utime / sysconf(_SC_CLK_TCK) + stime / sysconf(_SC_CLK_TCK);
               unsigned long seconds = time % 60;
               unsigned long mins = (time - seconds) / 60;
               execution_time_to_string(time_str, 20, mins, seconds);
               info.time_str = time_str;
               sscanf(vector_get(nums, 21), "%llu", &starttime);
               vector_destroy(nums);
               sstring_destroy(s);
            }
         }
         fclose(f_d);
         char start_str[20];
         time_t start_time_final = btime + (starttime / sysconf(_SC_CLK_TCK));
         time_struct_to_string(start_str, 20, localtime(&start_time_final));
         info.start_str = start_str;
         print_process_info(&info);
      }
   }
}

void exit_shell(int status) {
   if (history_file != NULL) {
      FILE *history_fd = fopen(history_file, "w");
      for (size_t i = 0; i < vector_size(history_vector); ++i) {
         fprintf(history_fd, "%s\n", (char *)vector_get(history_vector, i));
      }
      fclose(history_fd);
   }
   if (history_vector) {
      vector_destroy(history_vector);
   }
   if (process_vector) {
      vector_destroy(process_vector);
   }
   if (source != stdin) {
      fclose(source);
   }
   exit(status);
}