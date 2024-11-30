#include <arpa/inet.h>
#include <errno.h>
#include <locale.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define GRID_SIZE 10
#define SHIP_NUM 5

char *id = 0;
short sport = 0;
int sock = 0; /* 통신 소켓 */

// 배 정보
typedef struct {
    char name[20];
    int size;
} Ship;

// 커서 구조체
typedef struct {
    int x;
    int y;
} Cursor;

// 초기화 및 배치 관련 함수
void initGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '~'; // 물
        }
    }
}

ssize_t readLine(int sockfd, char *buffer, size_t maxlen) {
    ssize_t n, rc;
    char c;
    for (n = 0; n < maxlen - 1; n++) {
        rc = read(sockfd, &c, 1);
        if (rc == 1) {
            buffer[n] = c;
            if (c == '\n') {
                n++;
                break;
            }
        } else if (rc == 0) {
            break;
        } else {
            if (errno == EINTR)
                continue;
            return -1;
        }
    }
    buffer[n] = '\0';
    return n;
}

// 그리드 표시 함수 (ncurses 사용)
void displayGridWithCursor(char grid[GRID_SIZE][GRID_SIZE], Cursor cursor) {
    clear();
    // 열 번호 출력
    mvprintw(0, 2, " ");
    for (int i = 0; i < GRID_SIZE; i++) {
        printw(" %d", i + 1);
    }
    printw("\n");

    for (int i = 0; i < GRID_SIZE; i++) {
        // 행 번호 출력
        printw("%d ", i + 1);
        for (int j = 0; j < GRID_SIZE; j++) {
            if (i == cursor.y && j == cursor.x) {
                attron(A_REVERSE); // 커서 위치 강조
                printw("X ");
                attroff(A_REVERSE);
            } else {
                printw("%c ", grid[i][j]);
            }
        }
        printw("\n");
    }
    refresh();
}

// 배치 함수 (ncurses 사용)
void placeShips(char grid[GRID_SIZE][GRID_SIZE], Ship ships[SHIP_NUM]) {
    Cursor cursor = {0, 0};
    int orientation = 0; // 0: 가로, 1: 세로

    for (int i = 0; i < SHIP_NUM; i++) {
        bool placed = false;
        while (!placed) {
            clear();
            // 그리드 표시
            displayGridWithCursor(grid, cursor);
            // 배치할 배 정보 표시
            mvprintw(GRID_SIZE + 1, 0, "배치할 배: %s (크기: %d)", ships[i].name, ships[i].size);
            mvprintw(GRID_SIZE + 2, 0, "방향: %s", orientation == 0 ? "가로" : "세로");
            mvprintw(GRID_SIZE + 3, 0, "방향 변경: 'o' = 가로, 'v' = 세로");
            mvprintw(GRID_SIZE + 4, 0, "화살표 키로 위치 이동, 엔터 키로 배치 시작");

            int ch = getch();
            switch (ch) {
            case KEY_UP:
                if (cursor.y > 0)
                    cursor.y--;
                break;
            case KEY_DOWN:
                if (cursor.y < GRID_SIZE - 1)
                    cursor.y++;
                break;
            case KEY_LEFT:
                if (cursor.x > 0)
                    cursor.x--;
                break;
            case KEY_RIGHT:
                if (cursor.x < GRID_SIZE - 1)
                    cursor.x++;
                break;
            case 'o':
            case 'O':
                orientation = 0;
                break;
            case 'v':
            case 'V':
                orientation = 1;
                break;
            case '\n':
            case KEY_ENTER: {
                int x = cursor.x;
                int y = cursor.y;
                int dir = orientation;
                int canPlace = 1;

                for (int j = 0; j < ships[i].size; j++) {
                    int nx = x + (dir == 0 ? j : 0);
                    int ny = y + (dir == 1 ? j : 0);

                    if (nx >= GRID_SIZE || ny >= GRID_SIZE || grid[ny][nx] != '~') {
                        canPlace = 0;
                        break;
                    }
                }

                if (canPlace) {
                    for (int j = 0; j < ships[i].size; j++) {
                        int nx = x + (dir == 0 ? j : 0);
                        int ny = y + (dir == 1 ? j : 0);
                        grid[ny][nx] = 'S'; // 배를 배치
                    }
                    placed = true;
                } else {
                    mvprintw(GRID_SIZE + 5, 0, "유효하지 않은 위치입니다. 다른 위치를 선택하세요.");
                    refresh();
                    sleep(2);
                }
            } break;
            default:
                break;
            }
        }
    }
}

// 서버에 그리드 전송 함수
void sendGridToServer(char grid[GRID_SIZE][GRID_SIZE]) {
    char buffer[GRID_SIZE * GRID_SIZE + 1];
    int index = 0;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            buffer[index++] = grid[i][j];
        }
    }
    buffer[index] = '\0'; // 문자열 종료

    int ret = send(sock, buffer, strlen(buffer), 0);
    if (ret < 0) {
        mvprintw(GRID_SIZE + 6, 0, "배치 정보 전송 실패: %s", strerror(errno));
        refresh();
        sleep(2);
        return;
    }
    mvprintw(GRID_SIZE + 6, 0, "배치 정보를 서버에 전송했습니다. 전송 크기: %d 바이트", ret);
    mvprintw(GRID_SIZE + 7, 0, "상대방이 배치중입니다. 잠시만 기다려 주세요...");
    refresh();
    sleep(2);
}

// 사용자 입력을 처리하여 커서 이동 및 좌표 선택
void inputLoop(char grid[GRID_SIZE][GRID_SIZE]) {
    Cursor cursor = {0, 0}; // 초기 커서 위치 (0,0)
    int ch;

    while (1) {
        displayGridWithCursor(grid, cursor);
        mvprintw(GRID_SIZE + 1, 0, "공격할 좌표를 선택하고 엔터 키를 누르세요. 종료하려면 'q'를 누르세요.");
        refresh();

        ch = getch();

        switch (ch) {
        case KEY_UP:
            if (cursor.y > 0)
                cursor.y--;
            break;
        case KEY_DOWN:
            if (cursor.y < GRID_SIZE - 1)
                cursor.y++;
            break;
        case KEY_LEFT:
            if (cursor.x > 0)
                cursor.x--;
            break;
        case KEY_RIGHT:
            if (cursor.x < GRID_SIZE - 1)
                cursor.x++;
            break;
        case '\n':
        case KEY_ENTER: {
            int x = cursor.x + 1; // 1 기반 인덱싱
            int y = cursor.y + 1;
            char buf_write[20];
            sprintf(buf_write, "(%d %d)\n", x, y);
            int ret = write(sock, buf_write, strlen(buf_write));
            if (ret < strlen(buf_write)) {
                mvprintw(GRID_SIZE + 2, 0, "전송 오류 (전송된 바이트=%d, 오류=%s)", ret, strerror(errno));
                refresh();
                sleep(2);
                continue;
            }

            // 서버 응답 읽기
            char buf_read[256];
            ret = readLine(sock, buf_read, sizeof(buf_read));
            if (ret > 0) {
                mvprintw(GRID_SIZE + 3, 0, "서버 응답: %s", buf_read);
                refresh();
                sleep(2);
            } else {
                mvprintw(GRID_SIZE + 3, 0, "읽기 실패 (바이트 수=%d: %s)", ret, strerror(errno));
                refresh();
                sleep(2);
                return;
            }

            // 서버로부터 그리드 데이터 읽기
            char grid_data[GRID_SIZE][128];
            for (int i = 0; i < GRID_SIZE; i++) {
                ret = readLine(sock, grid_data[i], sizeof(grid_data[i]));
                if (ret <= 0) {
                    mvprintw(GRID_SIZE + 4 + i, 0, "그리드 데이터 읽기 실패: %s", strerror(errno));
                    refresh();
                    sleep(2);
                    return;
                }
                // 필요한 경우 grid를 업데이트
            }
        } break;
        case 'q':
        case 'Q':
            return; // 입력 루프 종료
        default:
            break;
        }
    }
}

// 게임 루프 함수 (ncurses 기반 입력으로 대체)
void gameLoopNcurses(char grid[GRID_SIZE][GRID_SIZE]) {
    inputLoop(grid);
}

// main 함수
int main(int argc, char **argv) {
    struct sockaddr_in server;

    setlocale(LC_ALL, "");

    // 인자와 관련된 오류 처리
    if (argc != 4) {
        fprintf(stderr, "사용법: %s id 서버주소 포트\n", argv[0]);
        exit(1);
    }
    id = argv[1];
    sport = atoi(argv[3]);

    // ncurses 초기화
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        endwin();
        fprintf(stderr, "소켓 생성 오류: %s\n", strerror(errno));
        exit(1);
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(sport);
    if (inet_aton(argv[2], &server.sin_addr) == 0) {
        endwin();
        fprintf(stderr, "유효하지 않은 서버 주소: %s\n", argv[2]);
        close(sock);
        exit(1);
    }

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        endwin();
        fprintf(stderr, "연결 실패: %s\n", strerror(errno));
        close(sock);
        exit(1);
    }

    // 그리드 초기화 및 배치
    char grid[GRID_SIZE][GRID_SIZE];
    Ship ships[SHIP_NUM] = {
        {"항공모함", 5},
        {"전함", 4},
        {"순양함", 3},
        {"잠수함", 3},
        {"구축함", 2}};
    initGrid(grid);
    placeShips(grid, ships);

    // 배치 정보 서버에 전송
    sendGridToServer(grid);

    // 게임 루프로 이동 (ncurses 기반)
    gameLoopNcurses(grid);

    // ncurses 종료
    endwin();
    close(sock);
    return 0;
}
