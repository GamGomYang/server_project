#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TILES 12
#define MAX_PLAYERS 2
#define TOTAL_TILES (MAX_TILES * 2)

typedef struct {
    int number;
    char color;
    int revealed;
} Tile;

typedef struct {
    Tile tiles[MAX_TILES * 2];
    int num_tiles;
} Player;

Tile deck[TOTAL_TILES];
int deck_index = 0;

int compare_tiles(const void *a, const void *b) {
    Tile *tileA = (Tile *)a;
    Tile *tileB = (Tile *)b;

    if (tileA->number != tileB->number) {
        return tileA->number - tileB->number;
    }

    return tileA->color - tileB->color;
}

void shuffle_deck() {
    srand((unsigned int)time(NULL));
    for (int i = TOTAL_TILES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Tile temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void initialize_game(Player players[]) {
    int index = 0;

    for (int i = 1; i <= MAX_TILES; i++) {
        deck[index].number = i;
        deck[index].color = 'B';
        deck[index++].revealed = 0;

        deck[index].number = i;
        deck[index].color = 'W';
        deck[index++].revealed = 0;
    }

    shuffle_deck();

    int tile_index = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].num_tiles = 4;
        for (int j = 0; j < players[i].num_tiles; j++) {
            players[i].tiles[j] = deck[tile_index++];
        }

        qsort(players[i].tiles, players[i].num_tiles, sizeof(Tile), compare_tiles);

        printf("Player %d's tiles:\n", i + 1);
        for (int j = 0; j < players[i].num_tiles; j++) {
            printf("[%c%d] ", players[i].tiles[j].color, players[i].tiles[j].number);
        }
        printf("\n");
    }

    deck_index = tile_index;
}

void display_tiles(Player player) {
    printf("Your tiles: ");
    for (int i = 0; i < player.num_tiles; i++) {
        if (player.tiles[i].revealed) {
            printf("[%c%d] ", player.tiles[i].color, player.tiles[i].number);
        } else {
            printf("[?%c?] ", player.tiles[i].color);
        }
    }
    printf("\n");
}

void draw_tile(Player *player) {
    if (deck_index < TOTAL_TILES) {
        player->tiles[player->num_tiles++] = deck[deck_index++];
        qsort(player->tiles, player->num_tiles, sizeof(Tile), compare_tiles);
        printf("Drew a new tile!\n");
    } else {
        printf("No more tiles to draw.\n");
    }
}

void display_opponent_tiles(Player opponent) {
    printf("Opponent's tiles: ");
    for (int i = 0; i < opponent.num_tiles; i++) {
        if (opponent.tiles[i].revealed) {
            printf("[%c%d] ", opponent.tiles[i].color, opponent.tiles[i].number);
        } else {
            printf("[%d ?%c?] ", i + 1, opponent.tiles[i].color);
        }
    }
    printf("\n");
}

int guess_tile(Player *opponent, int index, char color, int number) {
    index--;
    if (index < 0 || index >= opponent->num_tiles) {
        printf("Invalid index. Please try again.\n");
        return 0;
    }
    if (opponent->tiles[index].color == color && opponent->tiles[index].number == number) {
        opponent->tiles[index].revealed = 1;
        printf("Correct guess!\n");
        return 1;
    } else {
        printf("Wrong guess. Drawing a new tile.\n");
        return 0;
    }
}

int check_win(Player *opponent) {
    for (int i = 0; i < opponent->num_tiles; i++) {
        if (!opponent->tiles[i].revealed) {
            return 0;
        }
    }
    return 1;
}

void play_game(Player players[]) {
    int turn = 0;
    while (1) {
        int guess_index, guess_number;
        char guess_color;

        printf("\nPlayer %d's turn\n", turn + 1);
        draw_tile(&players[turn]);
        display_tiles(players[turn]);
        display_opponent_tiles(players[1 - turn]);

        printf("Guess opponent's tile (index, color, number): ");
        scanf("%d %c %d", &guess_index, &guess_color, &guess_number);

        if (guess_tile(&players[1 - turn], guess_index, guess_color, guess_number)) {
            if (check_win(&players[1 - turn])) {
                printf("Player %d wins!\n", turn + 1);
                break;
            }

            char choice;
            printf("Do you want to guess again? (y/n): ");
            scanf(" %c", &choice);
            if (choice == 'n' || choice == 'N') {
                turn = 1 - turn;
            }
        } else {
            draw_tile(&players[turn]);
            turn = 1 - turn;
        }
    }
}
