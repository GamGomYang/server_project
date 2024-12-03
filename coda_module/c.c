#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ncurses.h>
#define PORT 8080

WINDOW *output_win, *input_win;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    initscr();
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        endwin();
        perror("\nConnection Failed");
        return -1;
    }

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

    mvprintw(LINES / 2, (COLS - strlen("Waiting for the server to start...")) / 2, "Waiting for the server to start...");
    refresh();

    while (1) {
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            buffer[valread] = '\0';

            if (strstr(buffer, "Game Start!") != NULL) {
                wclear(output_win);
                wprintw(output_win, "%s", buffer);
                wrefresh(output_win);
            }

            if (strstr(buffer, "Opponent's tiles:") != NULL) {
                wclear(output_win);
                wprintw(output_win, "%s", buffer);
                wrefresh(output_win);
            }

            if (strstr(buffer, "Your turn") != NULL) {
                char input[1024];
                mvwprintw(input_win, 1, 1, "Enter your move (index color number): ");
                wrefresh(input_win);

                echo();
                wgetnstr(input_win, input, 1024);
                noecho();

                send(sock, input, strlen(input), 0);
                werase(input_win);
                box(input_win, 0, 0);
                wrefresh(input_win);
            }

            if (strstr(buffer, "Do you want to guess again?") != NULL) {
                char choice[3];
                mvwprintw(input_win, 1, 1, "Enter your choice (y/n): ");
                wrefresh(input_win);

                echo();
                wgetnstr(input_win, choice, sizeof(choice));
                noecho();

                send(sock, choice, strlen(choice), 0);
                werase(input_win);
                box(input_win, 0, 0);
                wrefresh(input_win);
            }

            if (strstr(buffer, "Game Over: You win!") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, LINES / 2, (COLS - strlen("You Win")) / 2, "You Win");
                wrefresh(output_win);
                sleep(3);
                break;
            }
            if (strstr(buffer, "Game Over: You lose!") != NULL) {
                wclear(output_win);
                mvwprintw(output_win, LINES / 2, (COLS - strlen("You Lose")) / 2, "You Lose");
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
