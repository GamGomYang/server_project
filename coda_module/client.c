#include <arpa/inet.h>
#include <locale.h>           // 로케일 설정을 위한 헤더
#include <ncursesw/ncurses.h> // 넓은 문자 지원 ncurses 헤더
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PORT 8080

WINDOW *output_win, *input_win;

int main() {
    // 로케일 설정: 프로그램이 다국어 문자를 제대로 처리할 수 있도록 함
    setlocale(LC_ALL, "");

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n소켓 생성 오류\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n잘못된 주소/ 주소를 지원하지 않음\n");
        return -1;
    }

    // ncurses 초기화
    initscr();
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        endwin();
        perror("\n연결 실패");
        return -1;
    }

    char *title_frames[] = {
        " _____             _        ",
        "/  __ \\           | |       ",
        "| /  \\/  ___    __| |  __ _ ",
        "| |     / _ \\  / _` | / _` |",
        "| \\__/\\| (_) || (_| || (_| |",
        " \\____/ \\___/  \\__,_| \\__,_|",
        "                            ",
        "                            ",
        NULL};

    int frame_delay = 200;
    int i = 0;

    while (title_frames[i]) {
        mvprintw(LINES / 2 - 4 + i, (COLS - strlen(title_frames[i])) / 2, "%s", title_frames[i]);
        refresh();
        usleep(frame_delay * 1000);
        i++;
    }

    refresh();
    sleep(2);

    int output_win_height = LINES - 5;
    int output_win_width = COLS;
    output_win = newwin(output_win_height, output_win_width, 0, 0);

    int input_win_height = 5;
    int input_win_width = COLS;
    input_win = newwin(input_win_height, input_win_width, output_win_height, 0);

    scrollok(output_win, TRUE);
    box(output_win, 0, 0);
    box(input_win, 0, 0);
    wrefresh(output_win);
    wrefresh(input_win);

    mvprintw(LINES / 2, (COLS - strlen("서버가 시작되기를 기다리는 중...")) / 2, "서버가 시작되기를 기다리는 중...");
    refresh();

    while (1) {
        int valread = read(sock, buffer, 1023); // 버퍼 오버플로우 방지를 위해 1023으로 설정
        if (valread > 0) {
            buffer[valread] = '\0';

            if (strstr(buffer, "게임에 참여하시겠습니까?") != NULL) {
                char choice[3];
                mvwprintw(input_win, 1, 1, "게임을 수락하시겠습니까? (y/n): ");
                wrefresh(input_win);

                echo();
                wgetnstr(input_win, choice, sizeof(choice) - 1);
                noecho();

                send(sock, choice, strlen(choice), 0);
                werase(input_win);
                box(input_win, 0, 0);
                wrefresh(input_win);
            }

            if (strstr(buffer, "다른 플레이어를 기다리는 중입니다...") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, 1, 1, "다른 플레이어가 수락을 기다리고 있습니다...");
                wrefresh(output_win);
            }

            if (strstr(buffer, "상대 플레이어가 게임을 거부하여 연결을 종료합니다...") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, 1, 1, "다른 플레이어가 거절했습니다. 게임 종료.");
                wrefresh(output_win);
                sleep(2);
                break;
            }

            if (strstr(buffer, "게임을 거부하셨습니다.") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, 1, 1, "당신이 게임을 거절했습니다. 게임 종료.");
                wrefresh(output_win);
                sleep(2);
                break;
            }

            if (strstr(buffer, "게임 시작!") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, 1, 1, "%s", buffer);
                wrefresh(output_win);
            }

            if (strstr(buffer, "상대의 타일:") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, 1, 1, "%s", buffer);
                wrefresh(output_win);
            }

            if (strstr(buffer, "당신의 턴입니다.") != NULL) {
                char input[1024];
                mvwprintw(input_win, 1, 1, "당신의 턴입니다. 움직임을 입력하세요 (인덱스 색상 번호): ");
                wrefresh(input_win);

                echo();
                wgetnstr(input_win, input, sizeof(input) - 1);
                noecho();

                send(sock, input, strlen(input), 0);
                werase(input_win);
                box(input_win, 0, 0);
                wrefresh(input_win);
            }

            if (strstr(buffer, "다시 추측하시겠습니까?") != NULL) {
                char choice[3];
                mvwprintw(input_win, 1, 1, "다시 추측하시겠습니까? (y/n): ");
                wrefresh(input_win);

                echo();
                wgetnstr(input_win, choice, sizeof(choice) - 1);
                noecho();

                send(sock, choice, strlen(choice), 0);
                werase(input_win);
                box(input_win, 0, 0);
                wrefresh(input_win);
            }

            if (strstr(buffer, "게임 종료: 당신이 이겼습니다!") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, LINES / 2, (COLS - strlen("당신이 이겼습니다!")) / 2, "당신이 이겼습니다!");
                wrefresh(output_win);
                sleep(3);
                break;
            }

            if (strstr(buffer, "게임 종료: 당신이 졌습니다!") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, LINES / 2, (COLS - strlen("당신이 졌습니다.")) / 2, "당신이 졌습니다.");
                wrefresh(output_win);
                sleep(3);
                break;
            }
        }
    }

    close(sock);
    endwin();
    return 0;
}
