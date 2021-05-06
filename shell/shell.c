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

typedef struct process {
    char *command;
    pid_t pid;
} process;

//static variables
static vector *history = NULL;
static char *history_file = NULL;
static vector *processes = NULL;
static FILE *source = NULL;

//functions:
int define_command(char *command);
int is_background(vector* inputs);
int external(char* command, vector* inputs);
int execute_single(int cmd_id, char* cmd, int save_to_history);
int get_process_idx(pid_t pid);
void sig_handler(int sig);
void shell_exit(int s);
int push_to_history(int command_id);
void detect_h_and_f(int argc, char* argv[]);
char *get_directory(char *buffer);
int add_process(char* command, pid_t pid);
int execute(char* command);
int operator_and(char* cmd, char* loc);
int operator_or(char* cmd, char* loc);
int operator_sep(char* cmd, char* loc);
int operator_output(char* cmd, char* loc);
int operator_append(char* cmd, char* loc);
void shell_processes();

//main function
int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    signal(SIGINT, sig_handler);
    history = string_vector_create();
    processes = shallow_vector_create();
    source = stdin;
    detect_h_and_f(argc, argv);
    //add process
    char *cmd = malloc(1);
    cmd[0] = '\0';
    char **curr = argv;
    while (*curr != NULL) {
        cmd = realloc(cmd, strlen(cmd) + strlen(*curr) + 2);
        strcat(cmd, *curr);
        strcat(cmd, " ");
        ++curr;
    }
    cmd[strlen(cmd) - 1] = '\0';
    add_process(cmd, getpid());
    //shell
    size_t byte_read = 0;
    char *line = NULL;
    char *wd_buffer = malloc(256);
    print_prompt(get_directory(wd_buffer), getpid());
    while (getline(&line, &byte_read, source) != -1) {
        for (size_t i = 0; i < vector_size(processes); i++) {
            int s;
            waitpid(((process*)vector_get(processes, i))->pid, &s, WNOHANG);
        }
        if (line[strlen(line) - 1] ==  '\n' && strlen(line) == 1) {}
        else {
            if (line[strlen(line) - 1] == '\n') {line[strlen(line) -1] = '\0';}
            char *line_cpy = malloc(strlen(line));
            strcpy(line_cpy, line);
            execute(line_cpy);
        }
        print_prompt(get_directory(wd_buffer), getpid());
        fflush(stdout);
    }
    free(line);
    shell_exit(0);
    return 0;
}

//dealing with -h and -f
void detect_h_and_f(int argc, char* argv[]) {
    source = stdin;
    int c;
    while ((c = getopt(argc, argv, "h:f:")) != -1) {
        if (c == 'h') {
            FILE *history_file_stream = fopen(optarg, "w");
            if (!history_file_stream) {
                print_history_file_error();
                shell_exit(1);
            }
            char *line = NULL;
            size_t byte_read = 0;
            while (getline(&line, &byte_read, history_file_stream) != -1) {
                if (line[strlen(line) - 1] == '\n')
                    line[strlen(line) -1] = '\0';
                vector_push_back(history, (void *)line);
            }
            free(line);
            fclose(history_file_stream);
            history_file_stream = NULL;
            history_file = get_full_path(optarg);
        } else if (c == 'f') {
            FILE *script_file_stream = fopen(optarg, "r");
            if (!script_file_stream) {
                print_script_file_error();
                shell_exit(1);
            }
            source = script_file_stream;
        } else {
            print_usage();
            shell_exit(1);
        }
    }
}

//define build-int commands
/**
 * 0: invalid command
 * 1: cd
 * 2: !history
 * 3: #<n>
 * 4: !<prefix>
 * 5: ps
 * 6: kill
 * 7: stop
 * 8: cont
 * 9: exit
 * 10: external
 */
int define_command(char *command) {
    sstring* sstr = cstr_to_sstring(command);
    vector* splited = sstring_split(sstr, ' ');
    size_t size = vector_size(splited);
    // validate inputs
    int command_idx;
    char* first_elem = (char*)vector_get(splited, 0);
    if (size == 0) {
        command_idx = 0;
    } else if (strcmp(vector_get(splited, 0), "cd") == 0) {
        command_idx = (size == 2) ? 1 : 0;
    } else if (strcmp(vector_get(splited, 0), "!history") == 0) {
        command_idx = 2;
    } else if (first_elem[0] == '#') {
        command_idx = 3;
    } else if (first_elem[0] == '!') {
        command_idx = 4;
    } else if (strcmp(vector_get(splited, 0), "ps") == 0) {
        command_idx = (size == 1) ? 5 : 0;
    } else if (strcmp(vector_get(splited, 0), "kill") == 0) {
        command_idx = (size == 2) ? 6 : 0;
    } else if (strcmp(vector_get(splited, 0), "stop") == 0) {
        command_idx = (size == 2) ? 7 : 0;
    } else if (strcmp(vector_get(splited, 0), "cont") == 0) {
        command_idx = (size == 2) ? 8 : 0;
    } else if (strcmp(vector_get(splited, 0), "exit") == 0) {
        command_idx = 9;
    } else {
        command_idx = 10;
    }
    // free inputs
    sstring_destroy(sstr);
    vector_destroy(splited);
    return command_idx;
}

//determine if pushed to history
int push_to_history(int command_id) {
    if (command_id == 2 || command_id == 3 || command_id == 4 || command_id == 9) {
        return 0;
    }
    return 1;
}

//execute single cmd
int execute_single(int command_id, char *command, int save_to_history) {
    sstring* sstr = cstr_to_sstring(command);
    vector* splited = sstring_split(sstr, ' ');
    size_t size = vector_size(splited);
    if (push_to_history(command_id) && save_to_history) {
        vector_push_back(history, command);
    }
    if (command_id == 0) {
        print_invalid_command(command);
        return 1;
    } else if (command_id == 1) {
        if (chdir((char*)vector_get(splited, 1)) == -1) {
            print_no_directory((char*) vector_get(splited, 1));
            return 1;
        }
        return 0;
    } else if (command_id == 2) {
        if (size != 1) {
            print_invalid_command(command);
            return 1;
        }
        for (size_t i = 0; i < vector_size(history); i++) {
            print_history_line(i, vector_get(history, i));
        }
        return 0;
    } else if (command_id == 3) {
        if (size != 1 || strlen((char*) vector_get(splited, 0)) == 1) {
            print_invalid_command(command);
            return 1;
        }
        size_t index = atoi((char*) vector_get(splited, 0)+1);
        if (index < vector_size(history)) {
            char *cmd = vector_get(history, index);
            print_command(cmd);
            return execute_single(define_command(cmd), cmd, save_to_history);
        }
        print_invalid_index();
        return 1;
    } else if (command_id == 4) {
        if (size != 1) {
            print_invalid_command(command);
            return 1;
        }
        char *prefix = (char*) vector_get(splited, 0) + 1;
        for (size_t i = vector_size(history); i > 0; i--) {
            char *cmd = vector_get(history, i - 1);
            if (strncmp(cmd, prefix, strlen(prefix)) == 0) {
                print_command(cmd);
                return execute(cmd);
            }
        }
        print_no_history_match();
        return 1;
    } else if (command_id == 5) {
        shell_processes();
        return 0;
    } else if (command_id == 6) {
       pid_t t_pid = atoi(vector_get(splited, 1));
       size_t idx = get_process_idx(t_pid);
       if (!idx) {
           print_no_process_found(t_pid);
           return 1;
       }
       kill(t_pid, SIGKILL);
       print_killed_process(t_pid, ((process*)vector_get(processes, idx))->command);
       return 0;
    } else if (command_id == 7) {
        pid_t t_pid = atoi(vector_get(splited, 1));
        size_t idx = get_process_idx(t_pid);
        if (!idx) {
           print_no_process_found(t_pid);
           return 1;
        }
        kill(t_pid, SIGSTOP);
        print_stopped_process(t_pid, ((process*)vector_get(processes, idx))->command);
        return 0;
    } else if (command_id == 8) {
        pid_t t_pid = atoi(vector_get(splited, 1));
        size_t idx = get_process_idx(t_pid);
        if (!idx) {
           print_no_process_found(t_pid);
           return 1;
        }
        kill(t_pid, SIGCONT);
        print_continued_process(t_pid, ((process*)vector_get(processes, idx))->command);
        return 0;
    } else if (command_id == 9) {
        if (size != 1) {
            print_invalid_command(command);
            return 1;
        }
        shell_exit(0);
        return 0;
    } else {
        return external(vector_get(splited, 0), splited);
    }
    sstring_destroy(sstr);
    vector_destroy(splited);
    return 1;
}
int get_process_idx(pid_t proc_id) {
    for (size_t i = 0; i < vector_size(processes); i++) {
        process* proc = vector_get(processes, i);
        if (proc-> pid == proc_id) {
            return i;
        }
    }
    return 0;
}
//execute with consideration of operation
int execute(char* command) {
    char *loc = NULL;
    char *operator = NULL;
    if ((loc = strstr(command, "&&")) != NULL) {
        operator = "&&";
    } else if ((loc = strstr(command, "||")) != NULL) {
        operator = "||";
    } else if ((loc = strstr(command, ";")) != NULL) {
        operator = ";";
    } else if ((loc = strstr(command, ">>")) != NULL) {
        operator = ">>";
    } else if ((loc = strstr(command, ">")) != NULL) {
        operator = ">";
    } else if ((loc = strstr(command, "<")) != NULL) {
        operator = "<";
    }
    //
    if (operator == NULL) {
        return execute_single(define_command(command), command, 1);
    } else if (!strcmp(operator, "&&")) {
        vector_push_back(history, command);
        operator_and(command, loc);
    } else if (!strcmp(operator, "||")) {
        vector_push_back(history, command);
        operator_or(command, loc);
    } else if (!strcmp(operator, ";")) {
        vector_push_back(history, command);
        operator_sep(command, loc);
    } else if (!strcmp(operator, ">")) {
        vector_push_back(history, command);
        operator_output(command, loc);
    } else if (!strcmp(operator, ">>")) {
        vector_push_back(history, command);
        operator_append(command, loc);
    } else if (!strcmp(operator, "<")) {
        vector_push_back(history, command);
        return 0;
    } else {
        return execute_single(define_command(command), command, 1);
    }
    return 1;
}
int operator_and(char* cmd, char* loc) {
    char cmd_1[loc - cmd];
    strncpy(cmd_1, cmd, loc - cmd);
    cmd_1[loc - cmd -1] = '\0';
    char* cmd_2 = loc+3;
    int cmd_1_rel = execute_single(define_command(cmd_1), cmd_1, 0);
    if (!cmd_1_rel) {
        execute_single(define_command(cmd_2), cmd_2, 0);
    }
    return 0;
}

int operator_or(char* cmd, char* loc) {
    char cmd_1[loc - cmd];
    strncpy(cmd_1, cmd, loc - cmd);
    cmd_1[loc - cmd - 1] = '\0';
    char* cmd_2 = loc+3;
    int cmd_1_rel = execute_single(define_command(cmd_1), cmd_1, 0);
    if (cmd_1_rel) {
        execute_single(define_command(cmd_2), cmd_2, 0);
    }
    return 0;
}

int operator_sep(char* cmd, char* loc) {
    char cmd_1[loc - cmd + 1];
    strncpy(cmd_1, cmd, loc - cmd);
    cmd_1[loc - cmd] = '\0';
    char* cmd_2 = loc+2;
    execute_single(define_command(cmd_1), cmd_1, 0);
    execute_single(define_command(cmd_2), cmd_2, 0);
    return 0;
}
//
int operator_output(char* cmd, char* loc) {
    char cmd_1[loc - cmd + 1];
    strncpy(cmd_1, cmd, loc - cmd);
    cmd_1[loc - cmd] = '\0';
    char* cmd_2 = loc+2;
    FILE* f = fopen(cmd_2, "w");
    int fildes = fileno(f);
    int saved_out = dup(fileno(stdout));
    dup2(fildes, fileno(stdout));
    execute_single(define_command(cmd_1), cmd_1, 0);
    fflush(stdout);
    close(fildes);
    dup2(saved_out, fileno(stdout));
    return 0;
}
//
int operator_append(char* cmd, char* loc) {
    char cmd_1[loc - cmd];
    strncpy(cmd_1, cmd, loc - cmd);
    cmd_1[loc - cmd -1] = '\0';
    char* cmd_2 = loc+3;
    FILE* f = fopen(cmd_2, "a");
    int fildes = fileno(f);
    int saved_out = dup(fileno(stdout));
    dup2(fildes, fileno(stdout));
    execute_single(define_command(cmd_1), cmd_1, 0);
    fflush(stdout);
    close(fildes);
    dup2(saved_out, fileno(stdout));
    return 0;
}
//
//externel
int external(char* command, vector* inputs) {
    int background = is_background(inputs);
    pid_t pid = fork();
    add_process(command, pid);
    if (pid > 0) {

        print_command_executed(pid);
        int status = 0;
        if (background) {
            waitpid(pid, &status, WNOHANG);
        } else {
            int error = waitpid(pid, &status, 0);
            if (error == -1) {
                print_wait_failed();
            } else if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) != 0){
                    return 1;
                }
                fflush(stdout);
                //set_process_status(pid, STATUS_KILLED);
            } else if (WIFSIGNALED(status)) {
                //set_process_status(pid, STATUS_KILLED);
            }
        }
        return status;
    } else if (pid == 0) {
        if (background) {
            if (setpgid(getpid(), getpid()) == -1) {
                print_setpgid_failed();
                fflush(stdout);
                shell_exit(1);
            }
        }
        fflush(stdout);
        char* argus[vector_size(inputs) + 1];
        for (size_t i = 0; i < vector_size(inputs); i++) {
            argus[i] = vector_get(inputs, i);
        }
        argus[vector_size(inputs)] = (char*) NULL;
        execvp(vector_get(inputs, 0), argus);
        print_exec_failed(command);
        fflush(stdout);
        exit(1);
        return 1;
    } else {
        print_fork_failed();
        shell_exit(1);
        return 1;
    }
}

int is_background(vector* inputs) {
    size_t size = vector_size(inputs);
    char *last_elem = vector_get(inputs, size-1);
    if (!strcmp(last_elem, "&")) {
        vector_erase(inputs, size-1);
        return 1;
    }
    return 0;
}

int add_process(char* command, pid_t pid) {
    for (size_t i = 0; i < vector_size(processes); ++i) {
        process *p = vector_get(processes, i);
        if (p->pid == pid) {
            p->command = command;
            return 0;
        }
    }
    process *p = malloc(sizeof(process));
    p->command = command;
    p->pid = pid;
    vector_push_back(processes, p);
    return 0;
}

//dealing with signals
void sig_handler(int sig) {
    if (sig == SIGINT) {
        return;
    }
}

//get directory
char *get_directory(char *buffer) {
    if (getcwd(buffer, 256) == NULL) {
        shell_exit(1);
    }
    return buffer;
}

//ps
void shell_processes() {
    print_process_info_header();
    size_t i = 0;
    for (; i < vector_size(processes); i++) {
        process* p = vector_get(processes, i);
        if(kill(p -> pid,0) != -1){
            process_info psinfo;
            psinfo.command = p -> command;
            psinfo.pid = p -> pid;
            unsigned long long start_time = 0;
            unsigned long long btime = 0;
            //
            FILE* fildes = fopen("/proc/stat", "r");
            if (fildes) {
                char* buffer = NULL;
                size_t lenth;
                ssize_t bytes_reads = getdelim( &buffer, &lenth, '\0', fildes );
                if ( bytes_reads != -1) {
                    sstring* s = cstr_to_sstring(buffer);
                    vector* splited = sstring_split(s, '\n');
                    size_t i = 0;
                    for (; i < vector_size(splited); i++) {
                        if (strncmp("btime", vector_get(splited, i), 5) == 0) {
                            char str[10];
                            sscanf(vector_get(splited, i), "%s %llu", str, &btime);
                        }
                    }
                }
            }
            fclose(fildes); 
            //
            char path[256];
            snprintf(path, sizeof(path), "/proc/%d/stat", p->pid);
            FILE* fildes_sec = fopen(path, "r");
            char time_str[256];
            if (fildes_sec) {
                char* buffer = NULL;
                size_t len;
                ssize_t bytes_reads = getdelim( &buffer, &len, '\0', fildes_sec);
                if ( bytes_reads != -1) {
                    sstring* s = cstr_to_sstring(buffer);
                    vector* nums = sstring_split(s, ' ');
                    //threads
                    long int num_threads = 0;
                    sscanf(vector_get(nums, 19), "%ld", &num_threads);
                    psinfo.nthreads = num_threads;
                    //virtual memory size
                    unsigned long int v_m_size = 0;
                    sscanf(vector_get(nums, 22), "%lu", &v_m_size);
                    psinfo.vsize = v_m_size / 1024;
                    //stat
                    char stat;
                    sscanf(vector_get(nums, 2), "%c", &stat);
                    psinfo.state = stat;
                    //time
                    unsigned long utime = 0;
                    sscanf(vector_get(nums, 13), "%lu", &utime);
                    unsigned long stime = 0;
                    unsigned long time = 0;
                    sscanf(vector_get(nums, 14), "%lu", &stime);
                    time = (utime + stime) / sysconf(_SC_CLK_TCK);
                    unsigned long seconds = time % 60;
                    unsigned long mins = (time - seconds) / 60;
                    execution_time_to_string(time_str, 20, mins, seconds);
                    psinfo.time_str = time_str;

                    sscanf(vector_get(nums, 21), "%llu", &start_time);
                }
            }
            fclose(fildes);
            char start_str[256];
            time_t t = btime + (start_time / sysconf(_SC_CLK_TCK));
            time_struct_to_string(start_str, 20, localtime(&t));
            psinfo.start_str = start_str;
            print_process_info(&psinfo);  
        }
    }
}


//exit
void shell_exit(int s) {
    if (history_file != NULL) {
        FILE *history_file_stream = fopen(history_file, "w");
        for (size_t i = 0; i < vector_size(history); ++i) {
            fprintf(history_file_stream, "%s\n", vector_get(history, i));
        }
        fclose(history_file_stream);
    }
    vector_destroy(history);
    vector_destroy(processes);
    if (source != stdin) {
        fclose(source);
    }
    exit(s);
}