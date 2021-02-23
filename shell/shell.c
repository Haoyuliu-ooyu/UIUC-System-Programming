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
int external(char* command, vector* inputs);
int execute_single(int cmd_id, char* cmd, int save_to_history);
void sig_handler(int sig);
void shell_exit(int s);
int push_to_history(int command_id);
void detect_h_and_f(int argc, char* argv[]);
char *get_directory(char *buffer);
int execute(char* command);

//main function
int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    signal(SIGINT, sig_handler);
    history = string_vector_create();
    processes = shallow_vector_create();
    source = stdin;
    detect_h_and_f(argc, argv);

    //
    size_t byte_read = 0;
    char *line = NULL;
    char *wd_buffer = malloc(256);
    print_prompt(get_directory(wd_buffer), getpid());
    while (getline(&line, &byte_read, source) != -1) {
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) -1] = '\0';
        char *line_cpy = malloc(strlen(line));
        strcpy(line_cpy, line);
        execute(line);
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
                return execute_single(define_command(cmd), cmd, save_to_history);
            }
        }
        print_no_history_match();
        return 1;
    } else if (command_id == 5) {
        return 0;
    } else if (command_id == 6) {
        return 0;
    } else if (command_id == 7) {
        return 0;
    } else if (command_id == 8) {
        return 0;
    } else if (command_id == 9) {
        if (size != 1) {
            print_invalid_command(command);
            return 1;
        }
        shell_exit(0);
        return 0;
    } else {
        external(vector_get(splited, 0), splited);
    }
    sstring_destroy(sstr);
    vector_destroy(splited);
    return 1;
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
    }
    if (loc != NULL) {
        memcpy(loc, "\0\0", strlen(operator));
        char *command_1 = command;
        char *command_2 = loc + strlen(operator);
        int command_id_1 = define_command(command_1);
        int command_id_2 = define_command(command_2);
        if (strcmp(operator, "&&") == 0) {
            if (push_to_history(command_id_1)) {
                int retval = execute_single(command_id_1, command_1, 0);
                if (retval == 0) {
                    if (push_to_history(command_id_2)) {
                        memcpy(loc, operator, strlen(operator));
                        vector_push_back(history, command);
                        return execute_single(command_id_2, command_2, 0);
                    }
                } else {
                    memcpy(loc, operator, strlen(operator));
                    vector_push_back(history, command);
                    return retval;
                }
            }
        } else if (strcmp(operator, "||") == 0) {
            if (push_to_history(command_id_1)) {
                int retval = execute_single(command_id_1, command_1, 0);
                if (retval != 0) {
                    if (push_to_history(command_id_2)) {
                        memcpy(loc, operator, strlen(operator));
                        vector_push_back(history, command);
                        return execute_single(command_id_2, command_2, 0);
                    }
                } else {
                    memcpy(loc, operator, strlen(operator));
                    vector_push_back(history, command);
                    return 0;
                }
            }
        } else {
            if (push_to_history(command_id_1)) {
                execute_single(command_id_1, command_1, 0);
                if (push_to_history(command_id_2)) {
                    memcpy(loc, operator, strlen(operator));
                    vector_push_back(history, command);
                    return execute_single(command_id_2, command_2, 0);
                }
            }
        }
        memcpy(loc, operator, strlen(operator));
        vector_push_back(history, command);
        print_invalid_command(command);
        return 1;
    } else {
        return execute_single(define_command(command), command, 1);
    }
}
//
//externel
int external(char* command, vector* inputs) {
    pid_t pid = fork();
    if (pid > 0) {
        print_command_executed(pid);
        int status = 0;
        if (waitpid(pid, &status, 0) == -1) {
            print_wait_failed();
            shell_exit(-1);
        } else if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0){
                shell_exit(1);
            }
            fflush(stdout);
            kill(pid, SIGKILL);
        } else if (WIFSIGNALED(status)) {
            kill(pid, SIGKILL);
        }
        return status;
    } else if (pid == 0) {
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

//exit
void shell_exit(int s) {
    if (history_file != NULL) {
        FILE *history_file_stream = fopen(history_file, "a+");
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