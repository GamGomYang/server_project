#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_TILES 12
#define MAX_PLAYERS 2
#define TOTAL_TILES (MAX_TILES * 2)

typedef struct {
    int number;
    char color; // 'B' for Black, 'W' for White
    int revealed;
} Tile;

typedef struct {
    Tile tiles[MAX_TILES * 2];
    int num_tiles;
} Player;

Tile deck[TOTAL_TILES]; // 전체 타일을 담는 덱 (12개의 숫자 * 2개의 색상)
int deck_index = 0; // 덱에서 타일을 가져올 인덱스

// 정렬 함수: 숫자 우선, 숫자가 같을 경우 'B'(검정색)이 'W'(흰색)보다 우선
int compare_tiles(const void *a, const void *b) {
    Tile *tileA = (Tile *)a;
    Tile *tileB = (Tile *)b;

    // 숫자 기준으로 먼저 정렬
    if (tileA->number != tileB->number) {
        return tileA->number - tileB->number;
    }

    // 숫자가 같을 경우 색상 기준으로 정렬 ('B' < 'W')
    return tileA->color - tileB->color;
}

// 덱을 셔플
void shuffle_deck() {
    srand((unsigned int)time(NULL));
    for (int i = TOTAL_TILES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Tile temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

// 게임 초기화
void initialize_game(Player players[]) {
    int index = 0;
    // 덱 생성: 1~12의 숫자를 흑, 백 타일로 각각 생성
    for (int i = 1; i <= MAX_TILES; i++) {
        deck[index].number = i;
        deck[index].color = 'B';
        deck[index++].revealed = 0;

        deck[index].number = i;
        deck[index].color = 'W';
        deck[index++].revealed = 0;
    }

    // 덱 셔플
    shuffle_deck();

    // 각 플레이어에게 초기 타일 4개씩 배분
    int tile_index = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].num_tiles = 4; // 초기 타일 수 4개
        for (int j = 0; j < players[i].num_tiles; j++) {
            players[i].tiles[j] = deck[tile_index++];
        }

        // 플레이어의 타일 정렬
        qsort(players[i].tiles, players[i].num_tiles, sizeof(Tile), compare_tiles);

        // 정렬된 플레이어 타일 출력
        printf("Player %d's tiles:\n", i + 1);
        for (int j = 0; j < players[i].num_tiles; j++) {
            printf("[%c%d] ", players[i].tiles[j].color, players[i].tiles[j].number);
        }
        printf("\n");
    }

    deck_index = tile_index; // 남은 타일은 덱에서 가져올 수 있도록 인덱스 설정
}

// 플레이어의 타일을 출력
void display_tiles(Player player) {
    printf("Your tiles: ");
    for (int i = 0; i < player.num_tiles; i++) {
        if (player.tiles[i].revealed) {
            printf("[%c%d] ", player.tiles[i].color, player.tiles[i].number); // 공개된 타일
        } else {
            printf("[?%c?] ", player.tiles[i].color); // 색상 힌트만 표시
        }
    }
    printf("\n");
}

// 새로운 타일을 가져오는 함수
void draw_tile(Player *player) {
    if (deck_index < TOTAL_TILES) {
        player->tiles[player->num_tiles++] = deck[deck_index++];
        printf("Drew a new tile!\n");
    } else {
        printf("No more tiles to draw.\n");
    }
}

// 추리 기능
int guess_tile(Player *opponent, int index, char color, int number) {
    index--; // 1부터 시작하는 입력을 0부터 시작하는 배열에 맞춤
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

// 게임 종료 조건 체크
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

            // 추리 성공 시 계속할지 멈출지 선택
            char choice;
            printf("Do you want to guess again? (y/n): ");
            scanf(" %c", &choice);
            if (choice == 'n' || choice == 'N') {
                turn = 1 - turn; // 턴을 넘김
            }
        } else {
            // 추리 실패 시 타일을 가져옴
            draw_tile(&players[turn]);
            turn = 1 - turn; // 턴 전환
        }
    }
}

int main() {
    srand(time(NULL));
    Player players[MAX_PLAYERS];
    initialize_game(players);
    play_game(players);
    return 0;
}
