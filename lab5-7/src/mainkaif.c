#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <fcntl.h>


#define FATAL(msg) { fprintf(stderr, "FATAL ERROR: %s, exiting.\n", msg); exit(EXIT_FAILURE); }
#define ERROR(msg) { fprintf(stderr, "ERROR: %s\n", msg); }

typedef struct compute_node_t {
    int id;
    pid_t pid;
    void *socket;
    struct compute_node_t *child;
    struct compute_node_t *sosed;
    int beat_counter;
} compute_node_t;

compute_node_t* create_control_node() {
    compute_node_t *control_node = (compute_node_t *) malloc(sizeof(compute_node_t));
    control_node->id = -1;
    control_node->child = NULL;
    control_node->sosed = NULL;
    control_node->socket = NULL;
}

compute_node_t *create_compute_node(int id) {
    compute_node_t *newNode = (compute_node_t *) malloc(sizeof(compute_node_t));
    newNode->id = id;
    newNode->socket = NULL;
    newNode->child = NULL;
    newNode->sosed = NULL;
    newNode->beat_counter = 0;
    return newNode;
}

compute_node_t* insert_node(compute_node_t *parent, compute_node_t *new_node) {
    if (parent->child == NULL) {
        parent->child = new_node; //если нет детей, узел первый ребенок
        return new_node;
    } else {
        compute_node_t *temp = parent->child;
        while (temp->sosed != NULL) {
            temp = temp->sosed;
        }
        temp->sosed = new_node;
        return new_node;
    }
}

compute_node_t *find_node(compute_node_t *root, int id) {
    if (!root) return NULL;
    if (root->id == id) {
        return root;
    }
    //рекурсивно ищем в певом ребенке
    compute_node_t *foundInChild = find_node(root->child, id);
    if (foundInChild) {
        return foundInChild;
    }
    //если не найдено в ребенке, ищем в sosedях
    return find_node(root->sosed, id);
}

int is_process_zombie(pid_t pid) {
    int status;
    pid_t result = waitpid(pid, &status, WNOHANG | WUNTRACED);

    if (result == 0) {
        return 0; // Процесс существует, но не является зомби
    } else if (result > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            return 1; // Процесс завершен (зомби)
        }
    } else {
        if (errno == ECHILD) {
            return 1; // Процесс не является дочерним
        } else {
            perror("waitpid");
            return -1; // Ошибка
        }
    }

    return 0;
}

void check_process_heartbeat(compute_node_t* root) {

    
    if (is_process_zombie(root->pid) == 0) {
        printf("Ok: %d // got beat\n", root->id);
    }
    else {
        if (root->beat_counter < 4) {
            root->beat_counter++;
        }
        else { 
            printf("НОДА: %d // СДОХЛА\n", root->id);
            root->beat_counter = 0;
        }
    }
    if (root->child != NULL) { 
        check_process_heartbeat(root->child);
    }
    if (root->sosed != NULL) {
        check_process_heartbeat(root->sosed);
    }
}


void send_command_to_node(compute_node_t *node, const char *command) {
    zmq_msg_t request; 
    zmq_msg_init_size(&request, strlen(command) + 1);
    strcpy(zmq_msg_data(&request), command);

    if (zmq_msg_send(&request, node->socket, 0) == -1 || is_process_zombie(node->pid) == 1) {  
        zmq_msg_close(&request);
        if (strcmp(command, "ping") == 0) {
            printf("Ok: 0 // узел %d недоступен\n", node->id);
        } else {
            printf("Error:%d: Node is unavailable\n", node->id);
        }
        return;
    }
    zmq_msg_close(&request);
}


void compute_node_process(int node_id) {

    struct timeval start_time;
    long elapsed_ms = 0;
    bool is_running = false;

    void *zmq_context = zmq_ctx_new();
    void *responder = zmq_socket(zmq_context, ZMQ_PULL);
    char address[50];
    sprintf(address, "tcp://127.0.0.1:%d", 5550 + node_id); // Bind to port based on ID
    int bind_result = zmq_bind(responder, address);
    if (bind_result != 0) {
        printf("Error code: %d", bind_result);
        FATAL("Compute node bind failed");
    }
    printf("Compute Node %d started, listening on %s\n", node_id, address);

    while (1) {
        zmq_msg_t request;
        zmq_msg_init(&request);
        zmq_msg_recv(&request, responder, 0);
        char *command_str = strdup((char *) zmq_msg_data(&request));
        zmq_msg_close(&request);

        // char *response_str = NULL;
        char *command = strtok(command_str, " ");
        
        if (strcmp(command, "exec") == 0) {
            // char buffer[50];
            char *subcommand = strtok(NULL, " "); 
            if (strcmp(subcommand, "start") == 0) {
                gettimeofday(&start_time, NULL);
                is_running = true;
                // response_str = strdup("Ok");
                printf("Ok:%d\n", node_id);
                } 
            else if (strcmp(subcommand, "stop") == 0) {
                struct timeval current_time;
                gettimeofday(&current_time, NULL);
                
                long seconds = current_time.tv_sec - start_time.tv_sec;
                long micros = current_time.tv_usec - start_time.tv_usec;
                if (micros < 0) {
                    seconds--;
                    micros += 1000000;
                }
                elapsed_ms += seconds * 1000 + micros / 1000;
                
                is_running = false;

                // response_str = strdup("Ok");
                printf("Ok:%d\n", node_id);
            } 
            else if (strcmp(subcommand, "time") == 0) {
                if (is_running) {
                    struct timeval current_time;
                    gettimeofday(&current_time, NULL);
                    
                    long seconds = current_time.tv_sec - start_time.tv_sec;
                    long micros = current_time.tv_usec - start_time.tv_usec;
                    if (micros < 0) {
                        seconds--;
                        micros += 1000000;
                    }
                    long current_ms = seconds * 1000 + micros / 1000;
                    printf("Ok:%d:%ld\n", node_id, elapsed_ms + current_ms);
                } else {
                    printf("Ok:%d:%ld\n", node_id, elapsed_ms);
                }
            }
            else {
                perror("Error: Unknown command for compute node\n");
            }
            

        } else if (strcmp(command, "ping") == 0) {
            printf("Ok: 1 // узел %d доступен\n", node_id);
        } else {
            perror("Error: Unknown command for compute node\n");
        }
        free(command_str);
    }
    zmq_close(responder);
    zmq_ctx_destroy(zmq_context);
}

int main() {

    void *zmq_context = zmq_ctx_new();
    compute_node_t* control_node = create_control_node();
    printf("Controller Node started. Listening for commands from console...\n");
    char command_buffer[256];

    while (1) {
        if (fgets(command_buffer, sizeof(command_buffer), stdin) == NULL) {
            if (feof(stdin)) {
                printf("Exiting on EOF.\n");
                zmq_ctx_destroy(zmq_context);
                exit(0);
            } else {
                ERROR("Error reading from console.");
                continue;
            }
        }

        command_buffer[strcspn(command_buffer, "\n")] = 0; //Возвращает колличество символов до \n => Просто заменяет \n на \0

        if (strlen(command_buffer) == 0) {
            continue;
        }
        printf("Received command: %s\n", command_buffer);
        char *command = strtok(command_buffer, " "); 

        if (command != NULL) {
            if (strcmp(command, "create") == 0) { // если строки равны

                char *id_str = strtok(NULL, " "); // продолжает поиск в оригинальной строке
                char *parent_id_str = strtok(NULL, " "); 

                if (id_str == NULL || parent_id_str == NULL) {
                    printf("Error: Missing arguments for 'create' command\n");
                    continue;
                }

                int id = atoi(id_str);
                int parent_id = atoi(parent_id_str);

                if (find_node(control_node, id) != NULL) {
                    printf("Error: Already exists\n");
                } else {

                    compute_node_t *new_node = create_compute_node(id); 
                    insert_node(control_node, new_node);

                    pid_t pid = fork();
                    if (pid == 0) {
                        compute_node_process(id);
                    } else if (pid > 0) {
                        new_node->pid = pid;
                        char address[50];
                        sprintf(address, "tcp://127.0.0.1:%d", 5550 + id);
                        new_node->socket = zmq_socket(zmq_context, ZMQ_PUSH);
                        int connect_result = zmq_connect(new_node->socket, address);
                        if (connect_result != 0) {
                            ERROR("Failed to connect to compute node, cleanup needed.");
                            printf("Error: Parent is unavailable\n"); 
                        }
                        printf("Ok: %d\n", new_node->pid);
                    } else {
                        ERROR("Fork failed");
                        printf("Error: [System error]\n"); 
                    }
                }
            } else if (strcmp(command, "exec") == 0) {
                char *id_str = strtok(NULL, " ");
                int id = atoi(id_str);
                compute_node_t *node = find_node(control_node, id);
                if (node == NULL) {
                    printf("Error:%d: Not found\n", id);
                } else {
                    char *subcommand = strtok(NULL, "");
                    char exec_command[200];
                    sprintf(exec_command, "exec %s", subcommand);

                    send_command_to_node(node, exec_command);
                }
                } else if (strcmp(command, "heartbeat") == 0){
                    char *str_seconds = strtok(NULL, " ");
                    int seconds = atoi(str_seconds); // с консоли

                    // Сохраняем оригинальные настройки stdin
                    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
                    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
                    char input[256];

                    time_t lastTime = time(NULL);
                    compute_node_t* node = control_node->child;
                    while (1) {
                        time_t currentTime = time(NULL);
                        if (difftime(currentTime, lastTime) >= seconds) {
                            // printf("илья гадит в консоль\n");
                            check_process_heartbeat(node);
                            lastTime = currentTime;
                        }   
                        
                        // Проверяем ввод пользователя
                        char cmd[10];
                        ssize_t bytes_read = read(STDIN_FILENO, cmd, sizeof(cmd)-1);
                        if (bytes_read > 0) {
                            cmd[bytes_read] = '\0';
                            if (strcmp(cmd, "stop\n") == 0) {
                                break;
                            }
                        }   
                    }
                    fcntl(STDIN_FILENO, F_SETFL, flags);
                } else if (strcmp(command, "ping") == 0) {
                    char *id_str = strtok(NULL, " ");

                    int id = atoi(id_str);
                    compute_node_t *node = find_node(control_node, id);
                    if (node == NULL) {
                        printf("Error: Not found\n");
                    } else {
                        send_command_to_node(node, "ping");

                    }   
                } 
                 else {
                printf("Error: [Unknown command]\n");
                }
        }
    }

    return 0;
}
