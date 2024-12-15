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
//타일비교
int compare_tiles(const void *a, const void *b) {
    Tile *tileA = (Tile *)a;
    Tile *tileB = (Tile *)b;

    if (tileA->number != tileB->number) {
        return tileA->number - tileB->number;
    }

    return tileA->color - tileB->color;
}
//랜덤한 타일 분배
void shuffle_deck() {
    srand((unsigned int)time(NULL));
    for (int i = TOTAL_TILES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Tile temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}
//게임 규칙 관련
void initialize_game(Player players[]) {
    int index = 0;
    //타일 정렬
    for (int i = 1; i <= MAX_TILES; i++) {
        deck[index].number = i;
        deck[index].color = 'B';
        deck[index++].revealed = 0;
        deck[index].number = i;
        deck[index].color = 'W';
        deck[index++].revealed = 0;
    }
    //타일 분배
    shuffle_deck();
    int tile_index = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].num_tiles = 4;
        for (int j = 0; j < players[i].num_tiles; j++) {
            players[i].tiles[j] = deck[tile_index++];
        }
        //받은 타일 정렬
        qsort(players[i].tiles, players[i].num_tiles, sizeof(Tile), compare_tiles);

    }
    deck_index = tile_index;
}

//새로운 타일 뽑기
void draw_tile(Player *player) {
    if (deck_index < TOTAL_TILES) {
        player->tiles[player->num_tiles++] = deck[deck_index++];
        qsort(player->tiles, player->num_tiles, sizeof(Tile), compare_tiles);
        printf("Drew a new tile!\n");
    } else {
        printf("No more tiles to draw.\n");
    }
}

//타일 추측
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
//승리조건 확인
int check_win(Player *opponent) {
    for (int i = 0; i < opponent->num_tiles; i++) {
        if (!opponent->tiles[i].revealed) {
            return 0;
        }
    }
    return 1;
}

