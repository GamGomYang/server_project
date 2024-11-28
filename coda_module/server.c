// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

// davinci.c�� �ִ� Player ����ü�� �Լ� ���� ����
#include "davinci.h" // davinci.h ���Ͽ� Player ����ü�� �Լ� ������ �ִٰ� ����

#define PORT 8080
#define MAX_PLAYERS 2

typedef struct {
    int socket;
    int player_id;
} ClientData;

Player players[MAX_PLAYERS]; // �÷��̾� ���� ����
int player_count = 0; // �÷��̾� ���� ����
int current_turn = 0; // ���� ���� ����
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *handle_client(void *arg) {
    ClientData *client_data = (ClientData *)arg;
    int client_socket = client_data->socket;
    int player_id = client_data->player_id;
    char buffer[1024] = {0};
    int valread;

    // �� ���� �÷��̾ ������ ������ ���
    pthread_mutex_lock(&lock);
    while (player_count < MAX_PLAYERS) {
        pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    // ���� ���� �޽���
    if (player_id == 0) {
        send(client_socket, "Game Start! You are Player 1\n", 29, 0);
    } else {
        send(client_socket, "Game Start! You are Player 2\n", 29, 0);
    }

    // ù ��° �÷��̾�� �� �ο�
    if (player_id == 0) {
        send(client_socket, "Your turn\n", 10, 0);
    }

    // Ŭ���̾�Ʈ���� ���
    while (1) {
        pthread_mutex_lock(&lock);
        while (current_turn != player_id) {
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);

        // ���� ���� �÷��̾�� Ÿ�� ���� ����
        char tile_info[1024] = "Opponent's tiles: ";
        for (int i = 0; i < players[1 - player_id].num_tiles; i++) {
            strcat(tile_info, "[?");
            strncat(tile_info, &players[1 - player_id].tiles[i].color, 1);
            strcat(tile_info, "?] ");
        }
        strcat(tile_info, "\nYour turn\n");
        send(client_socket, tile_info, strlen(tile_info), 0);

        // ���� ���� �÷��̾�κ��� �Է� �ޱ�
        if ((valread = read(client_socket, buffer, 1024)) > 0) {
            buffer[valread] = '\0';
            printf("Player %d: %s\n", player_id + 1, buffer);

            // �Է� �Ľ�
            int guess_index, guess_number;
            char guess_color;
            sscanf(buffer, "%d %c %d", &guess_index, &guess_color, &guess_number);

            // ���� ���� ó��
            int result = guess_tile(&players[1 - player_id], guess_index, guess_color, guess_number);
            if (result) {
                send(client_socket, "Correct guess!\n", 15, 0);
                if (check_win(&players[1 - player_id])) {
                    send(client_socket, "Game Over: You win!\n", 20, 0);
                    break;
                }
            } else {
                send(client_socket, "Wrong guess. Drawing a new tile.\n", 34, 0);
                draw_tile(&players[player_id]);
            }

            // �� ��ȯ
            pthread_mutex_lock(&lock);
            current_turn = (current_turn + 1) % MAX_PLAYERS;
            send(client_data[current_turn].socket, "Your turn\n", 10, 0);
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&lock);
        }
    }
    close(client_socket);
    free(client_data);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_PLAYERS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    initialize_game(players); // ���� �ʱ�ȭ

    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        printf("New connection\n");
        ClientData *client_data = malloc(sizeof(ClientData));
        client_data->socket = new_socket;
        client_data->player_id = player_count;

        pthread_mutex_lock(&lock);
        player_count++;
        if (player_count == MAX_PLAYERS) {
            pthread_cond_broadcast(&cond);
        }
        pthread_mutex_unlock(&lock);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *)client_data);
        pthread_detach(thread_id);
    }

    if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return 0;
}