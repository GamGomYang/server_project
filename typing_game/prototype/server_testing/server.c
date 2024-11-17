#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 12345
#define MAX_CLIENTS 2
#define BUF_SIZE 1024

// 단어 카테고리 및 데이터
const char *categories[] = {"동물", "나라", "사자성어"};
const char *words[][10] = {
    {"기린", "코끼리", "사자", "얼룩말", "강아지"},              // 동물
    {"한국", "미국", "일본", "중국", "프랑스"},                  // 나라
    {"유비무환", "천고마비", "동병상련", "일석이조", "오리무중"} // 사자성어
};
int word_counts[] = {5, 5, 5}; // 각 카테고리의 단어 개수

int client_sockets[MAX_CLIENTS] = {0};
pthread_mutex_t lock;
int connected_clients = 0;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;

char game_words[10][BUF_SIZE]; // 선택된 단어 저장
int word_count = 0;
int scores[MAX_CLIENTS] = {0};

// 브로드캐스트 함수
void broadcast(const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
}

// 단어 리스트와 점수 브로드캐스트
void broadcast_words_and_scores() {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "남은 단어: ");
    for (int i = 0; i < word_count; i++) {
        strcat(buffer, game_words[i]);
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

void select_category(int client_id) {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "카테고리를 선택하세요:\n");
    for (int i = 0; i < sizeof(categories) / sizeof(categories[0]); i++) {
        char line[BUF_SIZE];
        snprintf(line, sizeof(line), "%d. %s\n", i + 1, categories[i]);
        strcat(buffer, line);
    }
    send(client_sockets[client_id], buffer, strlen(buffer), 0);

    // 선택받기
    memset(buffer, 0, BUF_SIZE);
    recv(client_sockets[client_id], buffer, BUF_SIZE, 0);
    int category_index = atoi(buffer) - 1;

    if (category_index >= 0 && category_index < (sizeof(categories) / sizeof(categories[0]))) {
        snprintf(buffer, sizeof(buffer), "클라이언트%d가 '%s'를 선택했습니다. 게임을 시작합니다.\n",
                 client_id + 1, categories[category_index]);
        broadcast(buffer);

        // 선택된 카테고리의 단어를 게임에 사용
        word_count = word_counts[category_index];
        for (int i = 0; i < word_count; i++) {
            strncpy(game_words[i], words[category_index][i], BUF_SIZE);
        }

        // 선택된 단어 리스트 브로드캐스트
        snprintf(buffer, sizeof(buffer), "선택된 단어: ");
        for (int i = 0; i < word_count; i++) {
            strcat(buffer, game_words[i]);
            if (i < word_count - 1)
                strcat(buffer, ", ");
        }
        strcat(buffer, "\n");
        broadcast(buffer);

        // 카운트다운 메시지
        for (int i = 5; i > 0; i--) {
            snprintf(buffer, sizeof(buffer), "%d\n", i);
            broadcast(buffer);
            sleep(1); // 1초 대기
        }
        broadcast("게임 시작!\n");

    } else {
        send(client_sockets[client_id], "잘못된 선택입니다. 다시 선택하세요.\n", BUF_SIZE, 0);
        select_category(client_id); // 재귀 호출로 다시 선택
    }
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

    // 첫 번째 클라이언트가 카테고리를 선택
    if (client_id == 0) {
        select_category(client_id);
        pthread_cond_broadcast(&start_cond); // 카테고리 선택 완료 알림
    } else {
        // 다른 클라이언트는 선택을 기다림
        char buffer[BUF_SIZE];
        snprintf(buffer, sizeof(buffer), "클라이언트1이 카테고리를 선택하고 있습니다...\n");
        send(client_socket, buffer, BUF_SIZE, 0);

        // 선택 완료 대기
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&start_cond, &lock);
        pthread_mutex_unlock(&lock);
    }

    // 게임 시작 메시지
    send(client_socket, "게임 시작!\n", BUF_SIZE, 0);

    // 게임 진행
    char buffer[BUF_SIZE];
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
            if (strcmp(buffer, game_words[i]) == 0) {
                correct = 1;
                scores[client_id]++;

                // 단어 제거
                for (int j = i; j < word_count - 1; j++) {
                    strncpy(game_words[j], game_words[j + 1], BUF_SIZE);
                }
                word_count--;

                // 단어와 점수 갱신
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
