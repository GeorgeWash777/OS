#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256
// #define MAX_HISTORY 3  //что показать перезапись истории переписок
#define MAX_HISTORY 1000
#define MAX_COMMANDS 20

typedef struct {
    char sender[BUFFER_SIZE];
    char text[BUFFER_SIZE];
    time_t timestamp;
} Message;

typedef struct {
    char login[BUFFER_SIZE];
    char mmap_name[BUFFER_SIZE];
    Message messages[MAX_HISTORY];
    int message_index;  //текущая позиция для записи
    int is_full;        //флаг заполненности буфера
} Client;

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

Client clients[MAX_CLIENTS];
int client_count = 0;
SharedData* shared_data;

void handle_signal(int sig) {
    printf("\nServer shutting down...\n");
    munmap(shared_data, sizeof(SharedData));
    shm_unlink("/chat_server");
    exit(0);
}

void save_message(Client* client, const char* sender, const char* message) {
    //записываем сообщение в текущую позицию
    // strncpy для предотвращения переполнения буфера
    strncpy(client->messages[client->message_index].sender, sender, BUFFER_SIZE-1);
    strncpy(client->messages[client->message_index].text, message, BUFFER_SIZE-1);
    client->messages[client->message_index].timestamp = time(NULL);
    
    //обновляем индекс и флаг заполненности
    client->message_index = (client->message_index + 1) % MAX_HISTORY;
    if (!client->is_full && client->message_index == 0) {
        client->is_full = 1;
    }
}

void search_messages(const Client* client, const char* query, char* result) {
    result[0] = '\0';
    int count = 0;
    int start = 0;
    int total = client->is_full ? MAX_HISTORY : client->message_index;

    //итерация в обратном порядке (новые сообщения первыми)
    for (int i = 0; i < total; i++) {
        int idx = (client->message_index - 1 - i + MAX_HISTORY) % MAX_HISTORY;
        
        if (strstr(client->messages[idx].text, query) != NULL) {
            char time_buf[26];
            ctime_r(&client->messages[idx].timestamp, time_buf);
            time_buf[24] = '\0';
            
            snprintf(result + strlen(result), BUFFER_SIZE - strlen(result),
                    "[%s] %s: %s\n",
                    time_buf,
                    client->messages[idx].sender,
                    client->messages[idx].text);
            
            if (++count >= 10) break; //ограничение результатов поиска
        }
    }
}

void process_command(Command* cmd) {
    if (strcmp(cmd->command, "CONNECT") == 0) {
        printf("Client connected: %s\n", cmd->login);
        strncpy(clients[client_count].login, cmd->login, BUFFER_SIZE-1);
        strncpy(clients[client_count].mmap_name, cmd->message, BUFFER_SIZE-1);
        clients[client_count].message_index = 0;
        clients[client_count].is_full = 0;
        client_count++;
    }
    else if (strcmp(cmd->command, "SEND") == 0) {
        printf("Message from %s to %s: %s\n", cmd->login, cmd->recipient, cmd->message);

        //сохраняем у получателя
        for (int i = 0; i < client_count; i++) {
            if (strcmp(clients[i].login, cmd->recipient) == 0) {
                save_message(&clients[i], cmd->login, cmd->message);
                int fd = shm_open(clients[i].mmap_name, O_RDWR, 0666);
                char* mmap_ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                snprintf(mmap_ptr, BUFFER_SIZE, "[%s]: %s", cmd->login, cmd->message);
                munmap(mmap_ptr, BUFFER_SIZE);
                close(fd);
                break;
            }
        }

        //сохр у отправителя
        for (int i = 0; i < client_count; i++) {
            if (strcmp(clients[i].login, cmd->login) == 0) {
                save_message(&clients[i], cmd->login, cmd->message);
                break;
            }
        }
    }
    else if (strcmp(cmd->command, "SEARCH") == 0) {
        char result[BUFFER_SIZE * 10] = {0};
        for (int i = 0; i < client_count; i++) {
            if (strcmp(clients[i].login, cmd->login) == 0) {
                search_messages(&clients[i], cmd->message, result);
                int fd = shm_open(clients[i].mmap_name, O_RDWR, 0666);
                char* mmap_ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                strncpy(mmap_ptr, result, BUFFER_SIZE-1);
                munmap(mmap_ptr, BUFFER_SIZE);
                close(fd);
                break;
            }
        }
    }
    else if (strcmp(cmd->command, "DISCONNECT") == 0) {
        printf("Client disconnected: %s\n", cmd->login);
        for (int i = 0; i < client_count; i++) {
            if (strcmp(clients[i].login, cmd->login) == 0) {
                shm_unlink(clients[i].mmap_name);
                clients[i] = clients[client_count - 1];
                client_count--;
                break;
            }
        }
    }
}

int main() {
    signal(SIGINT, handle_signal);

    int shm_fd = shm_open("/chat_server", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData));
    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_data->mutex, &mutex_attr);

    printf("Server started. Waiting for clients...\n");

    while (1) {
        pthread_mutex_lock(&shared_data->mutex);
        for (int i = 0; i < MAX_COMMANDS; i++) {
            if (shared_data->commands[i].processed == 0) {
                process_command(&shared_data->commands[i]);
                shared_data->commands[i].processed = 1;
            }
        }
        pthread_mutex_unlock(&shared_data->mutex);
        usleep(100000);
    }

    return 0;
}