/**
 * nonstop_networking
 * CS 241 - Spring 2021
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include "common.h"
#include "format.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>

#include "./includes/dictionary.h"
#include "./includes/vector.h"

static int epoll_file_descriptor;
static dictionary* client_dictionary;
static char* dir_;
static vector* file_vector;
static dictionary* file_size;

typedef struct C_info {
    int status;
    verb method;
    char filename[255];
    char header[1024];
    size_t t;
} C_info;

void sig_pipe();
void quit(char*);
void epoll_mod(int);
void process_method(int, C_info*);
int read_PUT_info(int, C_info*);
void read_header(int, C_info*);
void run_client(int);
void shutdown_server();
void init_server(char*);
size_t read_head(int socket, char *buffer, size_t count);
int err_detect(size_t s1, size_t s2);

int main(int argc, char **argv) {
    // good luck!
    //sigpipe
    signal(SIGPIPE, sig_pipe);
    if (argc != 2) {
        print_server_usage();
        exit(1);
    }
    //sigint
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = shutdown_server;
    if (sigaction(SIGINT, &act, NULL) != 0) {
        perror("sigaction");
	    exit(1);
    }
    // set up dictionaries
    char dirname[] = "XXXXXX";
    dir_ = mkdtemp(dirname);
    print_temp_directory(dir_);
    client_dictionary = int_to_shallow_dictionary_create();
    file_size = string_to_unsigned_long_dictionary_create();
    file_vector = string_vector_create();
    //run server
    init_server(argv[1]);
}

void sig_pipe(){}

void shutdown_server() {
    close(epoll_file_descriptor);
    vector* v = dictionary_values(client_dictionary);
    VECTOR_FOR_EACH(v, i, {free(i);});
    vector_destroy(v);
    dictionary_destroy(client_dictionary);
    vector_destroy(file_vector);
    dictionary_destroy(file_size);
    remove(dir_);
    exit(1);
}

void init_server(char* port) {
    //create server
    int socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;       
	hints.ai_socktype = SOCK_STREAM; 
	hints.ai_flags = AI_PASSIVE;  
    
    int s = getaddrinfo(NULL, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(1);
	}
	int optval = 1;
	//reuesed socket
	if ( setsockopt(socket_file_descriptor, SOL_SOCKET,  SO_REUSEADDR, &optval, sizeof(optval))) {
    	perror("setsockeopt");
        exit(1);
	}
    if ( setsockopt(socket_file_descriptor, SOL_SOCKET,  SO_REUSEPORT, &optval, sizeof(optval))) {
    	perror("setsockeopt");
        exit(1);
	}
	if ( bind(socket_file_descriptor, result->ai_addr, result->ai_addrlen) != 0 ) {
    	perror("bind");
        exit(1);
	}
	if ( listen(socket_file_descriptor, 100) != 0 ) {
    	perror("listen");
        exit(1);
	}
  	freeaddrinfo(result);

    //create epoll
	epoll_file_descriptor = epoll_create(50);
	if (epoll_file_descriptor == -1) {
        perror("epoll_create");
        exit(1);
    };
  	//server epoll event  with socket fd
	struct epoll_event eve;
  	eve.events = EPOLLIN;
  	eve.data.fd = socket_file_descriptor;
	epoll_ctl(epoll_file_descriptor, EPOLL_CTL_ADD, socket_file_descriptor, &eve);
	struct epoll_event epoll_arr[100];
	while (1) {
		int num_events = epoll_wait(epoll_file_descriptor, epoll_arr, 100, 10000);
		if (num_events == -1) exit(1);
		if (num_events == 0) continue;
		for (int i = 0; i < num_events; i++) {
			if (epoll_arr[i].data.fd == socket_file_descriptor) {
				int client_file_descriptor = accept(socket_file_descriptor, NULL, NULL);
				if (client_file_descriptor < 0) {
                    perror("accept");
                    exit(1);
                }
        		//client epoll event with client fd
				struct epoll_event eve2;
        		eve2.events = EPOLLIN;
        		eve2.data.fd = client_file_descriptor;
				epoll_ctl(epoll_file_descriptor, EPOLL_CTL_ADD, client_file_descriptor, &eve2);
				C_info *client_info = calloc(1, sizeof(C_info));
        		dictionary_set(client_dictionary, &client_file_descriptor, client_info);
			} else {
				run_client(epoll_arr[i].data.fd);
			}
		}
	}
}

void run_client(int client_file_descriptor) {
    C_info* info = dictionary_get(client_dictionary, &client_file_descriptor);
    int status = info->status;
    //process header
    if (status == 0) {
        read_header(client_file_descriptor, info);
    } else if (status == 1) {
        process_method(client_file_descriptor, info);
    } else {
        //error
        write_to_socket(client_file_descriptor, "ERROR\n", 6);
        if (status == -1) {
           write_to_socket(client_file_descriptor, err_bad_request, strlen(err_bad_request)); 
        }
        if (status == -2) {
            write_to_socket(client_file_descriptor, err_bad_file_size, strlen(err_bad_file_size));
        }
        if (status == -3) {
            write_to_socket(client_file_descriptor, err_no_such_file, strlen(err_no_such_file));
        }
        //shutdown and remove this client fd from epoll
        epoll_ctl(epoll_file_descriptor, EPOLL_CTL_DEL, client_file_descriptor, NULL);
        dictionary_remove(client_dictionary, &client_file_descriptor);
        shutdown(client_file_descriptor, SHUT_RDWR);
        close(client_file_descriptor);
        free(info);
    }
}

void read_header(int client_file_descriptor, C_info *info) { 
    size_t count = read_head(client_file_descriptor, info->header, 1024);
    //when bad request
    if (count == 1024) {
        info->status = -1;
        epoll_mod(client_file_descriptor);
        return;
    }
    //setting up infor struct for each method
    if (!strncmp(info->header, "GET", 3)) {
        info->method = GET;
        strcpy(info->filename, info->header+4);
        info->filename[strlen(info->filename) -1] = '\0';
    } else if (!strncmp(info->header, "PUT", 3)) {
        info->method = PUT;
        strcpy(info->filename, info->header+4);
        info->filename[strlen(info->filename) -1] = '\0';
        if (read_PUT_info(client_file_descriptor, info) != 0) {
            info->status = -2;
            epoll_mod(client_file_descriptor);
            return;
        }
    } else if (!strncmp(info->header, "DELETE", 6)) {
        info->method = DELETE;
        strcpy(info->filename, info->header+7);
        info->filename[strlen(info->filename) -1] = '\0';
    } else if (!strncmp(info->header, "LIST", 4)) {
        info->method = LIST;
    } else {
        print_invalid_response();
        info->status = -1;
        epoll_mod(client_file_descriptor);
        return;
    }
    info->status = 1;
    epoll_mod(client_file_descriptor);
}

void epoll_mod(int client_file_descriptor) {
    struct epoll_event eve_temp;
  	eve_temp.events = EPOLLOUT;
  	eve_temp.data.fd = client_file_descriptor;
	epoll_ctl(epoll_file_descriptor, EPOLL_CTL_MOD, client_file_descriptor, &eve_temp);
}

void process_method(int client_file_descriptor, C_info *info) {
  
  // DELETE
    if (info->method == DELETE) {
		int len = strlen(dir_) + strlen(info->filename) + 2;
		char path[len];
		memset(path, 0, len);
		sprintf(path, "%s/%s", dir_, info->filename);
		if (remove(path) < 0) {
			info->status = -3;
			return;
		}
		size_t i = 0;
		VECTOR_FOR_EACH(file_vector, name, {
	        if (!strcmp((char *) name, info->filename)) {
                break;
            }
			i++;
	 	});
		size_t v_size= vector_size(file_vector);
		if (i == v_size) {
			info->status = -3;
			return;
		}
		vector_erase(file_vector, i);
		dictionary_remove(file_size, info->filename);
		write_to_socket(client_file_descriptor, "OK\n", 3);
        //GET 
    } else if (info->method == GET) {
		int len = strlen(dir_) + strlen(info->filename) + 1;
		char path[len];
		memset(path, 0, len);
		sprintf(path, "%s/%s", dir_, info->filename);
		FILE *r_mode = fopen(path, "r");
		if (!r_mode) {
			info->status = -3;
			return;
		}
		write_to_socket(client_file_descriptor, "OK\n", 3);
		size_t size;
		size = *(size_t *) dictionary_get(file_size, info->filename);
		write_to_socket(client_file_descriptor, (char*)&size, sizeof(size_t));
		size_t w_count = 0;
    	while (w_count < size) {
      		size_t size_head;
      		if( (size-w_count) > 1024){
        		size_head = 1024;
      		}else{
        		size_head =  (size - w_count);
      		}
      		char buffer[size_head + 1];
      		fread(buffer, 1, size_head, r_mode);
      		write_to_socket(client_file_descriptor, buffer, size_head);
      		w_count += size_head;
    	}
		fclose(r_mode);
    // PUT
	} else if (info->method == PUT) {
		write_to_socket(client_file_descriptor, "OK\n", 3);
    // LIST
	} else if (info->method == LIST) {
		write_to_socket(client_file_descriptor, "OK\n", 3);
		size_t size = 0;
		VECTOR_FOR_EACH(file_vector, i, {
	         size += strlen(i)+1;
	    });
		if (size) {
            size--;
        }
		write_to_socket(client_file_descriptor, (char*)& size, sizeof(size_t));
		VECTOR_FOR_EACH(file_vector, i, {
		        write_to_socket(client_file_descriptor, i, strlen(i));
				if (_it != _iend-1) {
                    write_to_socket(client_file_descriptor, "\n", 1);
                }
		});
	}
	epoll_ctl(epoll_file_descriptor, EPOLL_CTL_DEL, client_file_descriptor, NULL);
  	free(dictionary_get(client_dictionary, &client_file_descriptor));
	dictionary_remove(client_dictionary, &client_file_descriptor);
  	shutdown(client_file_descriptor, SHUT_RDWR);
	close(client_file_descriptor);
}

// if command is PUT 
int read_PUT_info(int client_file_descriptor, C_info *info) {
	int len = strlen(dir_) + strlen(info->filename) + 2;
	char path[len];
	memset(path , 0, len);
	sprintf(path, "%s/%s", dir_, info->filename);
	FILE *r_mode = fopen(path, "r");
	FILE *w_mode = fopen(path, "w");
	//cannot open the file 
	if (!w_mode) {
		perror("fopen");
		return 1;
	}
	// read bytes from clients and store it 
	size_t size;
	read_from_socket(client_file_descriptor, (char*) & size, sizeof(size_t));
	size_t r_count = 0;
	while (r_count < size + 5) {
		size_t size_head;
    	if((size + 5 - r_count) > 1024){
      		size_head = 1024;
    	}else{
      		size_head = (size + 5 - r_count);
    	}
		char buffer[1025] = {0};
		ssize_t count = read_from_socket(client_file_descriptor, buffer, size_head);
		if (count == -1) continue;
		fwrite(buffer, 1, count, w_mode);
		r_count += count;
		if( count == 0) break;
	}
	fclose(w_mode);
	if (err_detect(r_count, size)) {
		remove(path);
		return 1;
	}
	if (!r_mode) {
		vector_push_back(file_vector, info->filename);
	} else {
		fclose(r_mode);
	}
	dictionary_set(file_size, info->filename, &size);
	return 0;
}

size_t read_head(int socket, char *buffer, size_t count) {
	size_t r_count = 0;
	while (r_count < count) {
		ssize_t ret = read(socket, buffer + r_count, 1);
		if (ret == 0 || buffer[strlen(buffer) - 1] == '\n') {
			break;
		}
		if (ret == -1){
      if (errno == EINTR){
        continue;
      }else {
        perror("read_head()");
      }
    }
		r_count += ret;
	}
	return r_count;
}


int err_detect(size_t s1, size_t s2) {
  if (s1 == 0 && s1 != s2) {
    return 1;
  } 
  if (s1 < s2) {
    return 1;
  }
  if (s1 > s2) {
    return 1;
  }
  return 0;
}