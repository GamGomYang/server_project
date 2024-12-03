#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "davinci.h"

#define PORT 8080
#define MAX_PLAYERS 2

typedef struct {
    int socket;
    int player_id;
} ClientData;

Player players[MAX_PLAYERS];
int player_count = 0;
int current_turn = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int client_sockets[MAX_PLAYERS];

int compare_tiles(const void *a, const void *b);

void *handle_client(void *arg) {
    ClientData *client_data = (ClientData *)arg;
    int client_socket = client_data->socket;
    int player_id = client_data->player_id;
    client_sockets[player_id] = client_socket;
    char buffer[1024] = {0};
    int valread;

    pthread_mutex_lock(&lock);
    if (player_count == MAX_PLAYERS) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            send(client_sockets[i], "Do you want to start the game? (y/n): \n", 40, 0);
        }
    }
    pthread_mutex_unlock(&lock);

    char start_responses[MAX_PLAYERS][10];
    int responses_received = 0;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        int valread = read(client_sockets[i], start_responses[i], 10);
        if (valread > 0) {
            start_responses[i][valread] = '\0';
            responses_received++;

            if (strcmp(start_responses[i], "n") == 0) {
                send(client_sockets[i], "You chose not to start. Disconnecting...\n", 41, 0);
                close(client_sockets[i]);

                int other_client = (i == 0) ? 1 : 0;
                send(client_sockets[other_client], "The other player chose not to start. Disconnecting...\n", 57, 0);
                close(client_sockets[other_client]);
                player_count = 0;
                pthread_mutex_lock(&lock);
                pthread_cond_broadcast(&cond);
                pthread_mutex_unlock(&lock);
                exit(0);
            }
        }
    }

    if (responses_received == MAX_PLAYERS &&
        strcmp(start_responses[0], "y") == 0 &&
        strcmp(start_responses[1], "y") == 0) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            send(client_sockets[i], "Game Start!\n", 12, 0);
        }
        initialize_game(players);
    } else {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            send(client_sockets[i], "Game will not start. Disconnecting...\n", 39, 0);
            close(client_sockets[i]);
        }
        exit(0);
    }

    if (player_id == 0) {
        send(client_socket, "Game Start! You are Player 1\n", 29, 0);
    } else {
        send(client_socket, "Game Start! You are Player 2\n", 29, 0);
    }

    while (1) {
        pthread_mutex_lock(&lock);
        while (current_turn != player_id) {
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);

        if (current_turn == player_id) {
            char tile_info[2048] = "Opponent's tiles: ";
            for (int i = 0; i < players[1 - player_id].num_tiles; i++) {
                if (players[1 - player_id].tiles[i].revealed) {
                    char tile[10];
                    sprintf(tile, "[%c%d] ", players[1 - player_id].tiles[i].color, players[1 - player_id].tiles[i].number);
                    strcat(tile_info, tile);
                } else {
                    char tile[10];
                    sprintf(tile, "[%c?] ", players[1 - player_id].tiles[i].color);
                    strcat(tile_info, tile);
                }
            }

            strcat(tile_info, "\nYour tiles: ");
            for (int i = 0; i < players[player_id].num_tiles; i++) {
                char tile[10];
                sprintf(tile, "[%c%d] ", players[player_id].tiles[i].color, players[player_id].tiles[i].number);
                strcat(tile_info, tile);
            }

            strcat(tile_info, "\nYour turn\n");
            send(client_socket, tile_info, strlen(tile_info), 0);
        }

        if ((valread = read(client_socket, buffer, 1024)) > 0) {
            buffer[valread] = '\0';
            printf("Player %d: %s\n", player_id + 1, buffer);

            int guess_index, guess_number;
            char guess_color;
            sscanf(buffer, "%d %c %d", &guess_index, &guess_color, &guess_number);

            int result = guess_tile(&players[1 - player_id], guess_index, guess_color, guess_number);
            if (result) {
                send(client_socket, "Correct guess!\n", 15, 0);
                if (check_win(&players[1 - player_id])) {
                    send(client_socket, "Game Over: You win!\n", 20, 0);
                    int opponent_socket = client_sockets[1 - player_id];
                    send(opponent_socket, "Game Over: You lose!\n", 21, 0);
                    exit(1);
                }
                send(client_socket, "Do you want to guess again? (y/n): \n", 38, 0);
                valread = read(client_socket, buffer, 1024);
                if (valread > 0) {
                    buffer[valread] = '\0';
                    if (buffer[0] == 'n' || buffer[0] == 'N') {
                        pthread_mutex_lock(&lock);
                        current_turn = (current_turn + 1) % MAX_PLAYERS;
                        send(client_sockets[current_turn], "Your turn\n", 10, 0);
                        pthread_cond_broadcast(&cond);
                        pthread_mutex_unlock(&lock);
                    }
                }
            } else {
                send(client_socket, "Wrong guess. Drawing a new tile.\n", 34, 0);
                draw_tile(&players[player_id]);
                char draw_msg[100];
                sprintf(draw_msg, "You drew a new tile: [%c%d]\n", players[player_id].tiles[players[player_id].num_tiles - 1].color, players[player_id].tiles[players[player_id].num_tiles - 1].number);
                send(client_socket, draw_msg, strlen(draw_msg), 0);

                pthread_mutex_lock(&lock);
                current_turn = (current_turn + 1) % MAX_PLAYERS;
                send(client_sockets[current_turn], "Your turn\n", 10, 0);
                pthread_cond_broadcast(&cond);
                pthread_mutex_unlock(&lock);
            }
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

    initialize_game(players);

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