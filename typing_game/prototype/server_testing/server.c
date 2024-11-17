#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 12345
#define MAX_CLIENTS 2
#define BUF_SIZE 1024

char *words[] = {"기린", "코끼리", "사자", "얼룩말", "강아지"};
int word_count = 5;
int scores[MAX_CLIENTS] = {0};
int client_sockets[MAX_CLIENTS] = {0};
pthread_mutex_t lock;
int connected_clients = 0;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;

// 브로드캐스트 함수
void broadcast(const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
}

// 단어 리스트 브로드캐스트
void broadcast_words_and_scores() {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "남은 단어: ");
    for (int i = 0; i < word_count; i++) {
        strcat(buffer, words[i]);
        if (i < word_count - 1)
            strcat(buffer, " ");
    }
    strcat(buffer, "\n점수: ");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        char temp[32];
        snprintf(temp, sizeof(temp), "클라이언트%d: %d ", i + 1, scores[i]);
        strcat(buffer, temp);
    }
    strcat(buffer, "\n");
    broadcast(buffer);
}

// 클라이언트 핸들러
void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    int client_id = -1;

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = client_socket;
            client_id = i;
            connected_clients++;
            break;
        }
    }

    if (connected_clients == MAX_CLIENTS) {
        pthread_cond_broadcast(&start_cond);
    } else {
        char wait_message[] = "다른 플레이어를 기다리는 중...\n";
        send(client_socket, wait_message, strlen(wait_message), 0);
    }
    pthread_mutex_unlock(&lock);

    pthread_mutex_lock(&lock);
    while (connected_clients < MAX_CLIENTS) {
        pthread_cond_wait(&start_cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    printf("클라이언트 %d 연결\n", client_id + 1);

    // 게임 시작 메시지
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "게임 시작! 단어: %s %s %s %s %s\n", words[0], words[1], words[2], words[3], words[4]);
    send(client_socket, buffer, strlen(buffer), 0);

    while (word_count > 0) {
        memset(buffer, 0, BUF_SIZE);
        int len = recv(client_socket, buffer, BUF_SIZE - 1, 0);
        if (len <= 0)
            break;

        buffer[len] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0; // 개행 및 공백 제거

        int correct = 0;

        pthread_mutex_lock(&lock);
        for (int i = 0; i < word_count; i++) {
            if (strcmp(buffer, words[i]) == 0) {
                correct = 1;
                scores[client_id]++;

                // 단어 제거
                for (int j = i; j < word_count - 1; j++) {
                    words[j] = words[j + 1];
                }
                word_count--;

                // 갱신된 단어와 점수를 브로드캐스트
                broadcast_words_and_scores();
                break;
            }
        }
        pthread_mutex_unlock(&lock);

        if (correct) {
            snprintf(buffer, sizeof(buffer), "정답!\n");
        } else {
            snprintf(buffer, sizeof(buffer), "오답! 다시 입력하세요.\n");
        }
        send(client_socket, buffer, strlen(buffer), 0);
    }

    // 게임 종료 메시지
    snprintf(buffer, sizeof(buffer), "게임 종료! 최종 점수: %d\n", scores[client_id]);
    send(client_socket, buffer, strlen(buffer), 0);

    close(client_socket);
    pthread_mutex_lock(&lock);
    client_sockets[client_id] = 0;
    pthread_mutex_unlock(&lock);

    printf("클라이언트 %d 연결 종료\n", client_id + 1);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pthread_t tid;

    pthread_mutex_init(&lock, NULL);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("소켓 생성 실패");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("바인딩 실패");
        close(server_socket);
        exit(1);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("리슨 실패");
        close(server_socket);
        exit(1);
    }

    printf("서버가 포트 %d에서 실행 중...\n", PORT);

    while (1) {
        client_addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_socket == -1) {
            perror("클라이언트 연결 실패");
            continue;
        }

        int *arg = malloc(sizeof(int));
        *arg = client_socket;

        pthread_create(&tid, NULL, handle_client, arg);
        pthread_detach(tid);
    }

    close(server_socket);
    pthread_mutex_destroy(&lock);

    return 0;
}
