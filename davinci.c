#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TILES 12
#define MAX_PLAYERS 2

typedef struct {
    int number;
    char color; // 'B' for Black, 'W' for White
    int revealed;
} Tile;

typedef struct {
    Tile tiles[MAX_TILES];
    int num_tiles;
} Player;

Tile deck[MAX_TILES * 2]; // 전체 타일을 담는 덱 (12개의 숫자 * 2개의 색상)

void shuffle_deck() {
    for (int i = MAX_TILES * 2 - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Tile temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void initialize_game(Player players[]) {
    int index = 0;
    // 덱 생성: 0~11의 숫자를 흑, 백 타일로 각각 생성
    for (int i = 0; i < MAX_TILES; i++) {
        deck[index].number = i;
        deck[index].color = 'B';
        deck[index++].revealed = 0;

        deck[index].number = i;
        deck[index].color = 'W';
        deck[index++].revealed = 0;
    }

    // 덱 셔플
    shuffle_deck();

    // 플레이어에게 타일 배분
    int tile_index = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].num_tiles = MAX_TILES / MAX_PLAYERS;
        printf("Player %d's tiles:\n", i + 1); // 디버깅: 플레이어 타일 정보 출력
        for (int j = 0; j < players[i].num_tiles; j++) {
            players[i].tiles[j] = deck[tile_index++];
            printf("[%c%d] ", players[i].tiles[j].color, players[i].tiles[j].number); // 디버깅
        }
        printf("\n"); // 줄 바꿈
    }
}

// 플레이어의 타일을 출력
void display_tiles(Player player) {
    printf("Your tiles: ");
    for (int i = 0; i < player.num_tiles; i++) {
        if (player.tiles[i].revealed) {
            printf("[%c%d] ", player.tiles[i].color, player.tiles[i].number);
        } else {
            printf("[??] ");
        }
    }
    printf("\n");
}

// 추리 기능
int guess_tile(Player *opponent, int index, char color, int number) {
    if (index < 0 || index >= opponent->num_tiles) {
        printf("Invalid index.\n");
        return 0;
    }

    // 디버깅: 실제 타일 색상과 숫자를 출력하여 확인
    printf("Opponent's tile at index %d: [%c%d]\n", index, opponent->tiles[index].color, opponent->tiles[index].number);

    if (opponent->tiles[index].color == color && opponent->tiles[index].number == number) {
        opponent->tiles[index].revealed = 1;
        printf("Correct guess!\n");
        return 1;
    } else {
        printf("Wrong guess.\n");
        return 0;
    }
}


// 게임 종료 조건 체크Q
int check_win(Player *opponent) {
    for (int i = 0; i < opponent->num_tiles; i++) {
        if (!opponent->tiles[i].revealed) {
            return 0;
        }
    }
    return 1;
}

// 게임 실행
void play_game(Player players[]) {
    int turn = 0;
    while (1) {
        int guess_index, guess_number;
        char guess_color;
        
        printf("\nPlayer %d's turn\n", turn + 1);
        display_tiles(players[turn]);

        printf("Guess opponent's tile (index, color, number): ");
        scanf("%d %c %d", &guess_index, &guess_color, &guess_number);

        if (guess_tile(&players[1 - turn], guess_index, guess_color, guess_number)) {
            if (check_win(&players[1 - turn])) {
                printf("Player %d wins!\n", turn + 1);
                break;
            }
        }
        
        turn = 1 - turn; // 턴 전환
    }
}

int main() {
    Player players[MAX_PLAYERS];
    initialize_game(players);
    play_game(players);
    return 0;
}
