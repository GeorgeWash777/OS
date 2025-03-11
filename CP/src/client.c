#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define BUFFER_SIZE 256
#define MAX_COMMANDS 20

typedef struct {
    char command[BUFFER_SIZE];
    char login[BUFFER_SIZE];
    char recipient[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    int processed;
} Command;

typedef struct {
    Command commands[MAX_COMMANDS];
    pthread_mutex_t mutex;
} SharedData;

char login[BUFFER_SIZE];
char mmap_name[BUFFER_SIZE];
int server_shm;
SharedData* shared_data;
int client_shm;
char* client_mmap;

void handle_signal(int sig) {
    printf("\nClient shutting down...\n");
    munmap(client_mmap, BUFFER_SIZE);
    close(client_shm);
    shm_unlink(mmap_name);
    exit(0);
}

void* receive_messages(void* arg) {
    while (1) {
        if (strlen(client_mmap) > 0) {
            printf("\n%s\n", client_mmap);
            client_mmap[0] = '\0';
            // printf("Enter command (send, search, exit): ");
            fflush(stdout);
        }
        usleep(100000);
    }
    return NULL;
}

void send_command(const char* command, const char* recipient, const char* message) {
    pthread_mutex_lock(&shared_data->mutex);
    for (int i = 0; i < MAX_COMMANDS; i++) {
        if (shared_data->commands[i].processed == 1) {
            strncpy(shared_data->commands[i].command, command, BUFFER_SIZE-1);
            strncpy(shared_data->commands[i].login, login, BUFFER_SIZE-1);
            strncpy(shared_data->commands[i].recipient, recipient, BUFFER_SIZE-1);
            strncpy(shared_data->commands[i].message, message, BUFFER_SIZE-1);
            shared_data->commands[i].processed = 0;
            break;
        }
    }
    pthread_mutex_unlock(&shared_data->mutex);
}

int main() {
    signal(SIGINT, handle_signal);

    printf("Enter your login (max 242 chars): ");
    char raw_login[BUFFER_SIZE];
    scanf("%255s", raw_login);
    
    //ограничение длины логина с учетом "/chat_client_"
    strncpy(login, raw_login, BUFFER_SIZE - strlen("/chat_client_") - 1);
    login[BUFFER_SIZE - strlen("/chat_client_") - 1] = '\0';
    
    snprintf(mmap_name, BUFFER_SIZE, "/chat_client_%s", login);
    client_shm = shm_open(mmap_name, O_CREAT | O_RDWR, 0666);
    ftruncate(client_shm, BUFFER_SIZE);
    client_mmap = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, client_shm, 0);

    server_shm = shm_open("/chat_server", O_RDWR, 0666);
    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, server_shm, 0);

    send_command("CONNECT", "", mmap_name);

    printf("Connected to server. You can start sending messages.\n");

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, NULL);

    while (1) {
        printf("Enter command (send, search, exit): ");
        char command[BUFFER_SIZE];
        scanf("%s", command);

        if (strcmp(command, "send") == 0) {
            char recipient[BUFFER_SIZE], text[BUFFER_SIZE];
            printf("Enter recipient and message: ");
            scanf("%255s %255[^\n]", recipient, text);
            send_command("SEND", recipient, text);
        }
        else if (strcmp(command, "search") == 0) {
            char query[BUFFER_SIZE];
            printf("Enter search query: ");
            scanf(" %255[^\n]", query);
            send_command("SEARCH", "", query);
            sleep(1);
        }
        else if (strcmp(command, "exit") == 0) {
            send_command("DISCONNECT", "", "");
            break;
        }
    }

    munmap(client_mmap, BUFFER_SIZE);
    shm_unlink(mmap_name);
    munmap(shared_data, sizeof(SharedData));
    close(server_shm);
    return 0;
}