#include <arpa/inet.h>
#include <locale.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUF_SIZE 4096 // 버퍼 크기를 늘림

WINDOW *output_win, *input_win, *word_list_win, *score_win;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *receive_messages(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUF_SIZE];
    char word_list[BUF_SIZE] = ""; // 남은 단어 목록 저장
    char scores[BUF_SIZE] = "";    // 점수 정보 저장

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int len = recv(client_socket, buffer, BUF_SIZE - 1, 0);
        if (len <= 0) {
            if (len == 0) {
                pthread_mutex_lock(&mutex);
                wprintw(output_win, "서버가 연결을 종료했습니다.\n");
                wrefresh(output_win);
                pthread_mutex_unlock(&mutex);
            } else {
                pthread_mutex_lock(&mutex);
                wprintw(output_win, "recv 오류\n");
                wrefresh(output_win);
                pthread_mutex_unlock(&mutex);
            }
            close(client_socket);
            endwin(); // ncurses 모드 종료
            exit(1);
        }
        buffer[len] = '\0'; // 수신된 데이터의 끝에 널 문자 추가

        pthread_mutex_lock(&mutex);
        // 메시지 처리
        if (strncmp(buffer, "WORD_LIST:", 10) == 0) {
            // 남은 단어 목록 업데이트
            strncpy(word_list, buffer + 10, BUF_SIZE - 1);
            // 단어 목록 창 업데이트
            werase(word_list_win);
            box(word_list_win, 0, 0);
            mvwprintw(word_list_win, 1, 1, "남은 단어:");
            mvwprintw(word_list_win, 2, 1, "%s", word_list);
            wrefresh(word_list_win);
        } else if (strncmp(buffer, "SCORES:", 7) == 0) {
            // 점수 정보 업데이트
            strncpy(scores, buffer + 7, BUF_SIZE - 1);
            // 점수 창 업데이트
            werase(score_win);
            box(score_win, 0, 0);
            mvwprintw(score_win, 1, 1, "점수 정보:");
            mvwprintw(score_win, 2, 1, "%s", scores);
            wrefresh(score_win);
        } else {
            // 출력 창에 메시지 출력
            wprintw(output_win, "%s\n", buffer);
            wrefresh(output_win);
        }

        // 입력 창을 다시 그려서 덮어씌워지지 않도록 함
        box(input_win, 0, 0);
        wrefresh(input_win);

        // 커서를 입력 창으로 이동
        wmove(input_win, 1, 1);
        wrefresh(input_win);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t receive_thread;
    wchar_t input[BUF_SIZE]; // wchar_t 배열로 변경
    char mb_input[BUF_SIZE]; // 멀티바이트 문자열 저장

    // 로케일 설정
    setlocale(LC_ALL, "");

    // 클라이언트 소켓 생성
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("소켓 생성 실패");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    // 서버 연결
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("서버 연결 실패");
        close(client_socket);
        exit(1);
    }

    // ncurses 초기화
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);

    // 출력 창 생성 (전체 화면의 60%)
    output_win = newwin(height - 9, width, 0, 0);
    scrollok(output_win, TRUE);

    // 단어 목록 창 생성 (하단에 위치)
    word_list_win = newwin(3, width, height - 9, 0);
    box(word_list_win, 0, 0);
    wrefresh(word_list_win);

    // 점수 창 생성 (하단에 위치)
    score_win = newwin(3, width, height - 6, 0);
    box(score_win, 0, 0);
    wrefresh(score_win);

    // 입력 창 생성 (하단에 위치)
    input_win = newwin(3, width, height - 3, 0);
    box(input_win, 0, 0);
    wrefresh(input_win);

    // 서버 연결 성공 메시지 출력
    pthread_mutex_lock(&mutex);
    wprintw(output_win, "서버에 연결되었습니다!\n");
    wrefresh(output_win);
    pthread_mutex_unlock(&mutex);

    // 서버 메시지 수신 스레드 생성
    if (pthread_create(&receive_thread, NULL, receive_messages, &client_socket) != 0) {
        pthread_mutex_lock(&mutex);
        wprintw(output_win, "스레드 생성 실패\n");
        wrefresh(output_win);
        pthread_mutex_unlock(&mutex);
        close(client_socket);
        endwin(); // ncurses 모드 종료
        exit(1);
    }
    pthread_detach(receive_thread);

    // 사용자 입력 처리
    while (1) {
        pthread_mutex_lock(&mutex);
        wmove(input_win, 1, 1);
        wclrtoeol(input_win);
        wrefresh(input_win);
        pthread_mutex_unlock(&mutex);

        // 입력 받기 (뮤텍스 잠금 없이)
        wgetn_wstr(input_win, input, BUF_SIZE - 1);

        // wchar_t 입력을 멀티바이트 문자열로 변환
        size_t len = wcstombs(mb_input, input, BUF_SIZE - 1);
        if (len == (size_t)-1) {
            pthread_mutex_lock(&mutex);
            wprintw(output_win, "입력 변환 오류 발생\n");
            wrefresh(output_win);
            pthread_mutex_unlock(&mutex);
            continue; // 다음 입력으로 넘어감
        }
        mb_input[len] = '\0';

        // 서버에 입력 전송
        if (send(client_socket, mb_input, strlen(mb_input), 0) == -1) {
            pthread_mutex_lock(&mutex);
            wprintw(output_win, "send 오류\n");
            wrefresh(output_win);
            pthread_mutex_unlock(&mutex);
            break;
        }
    }

    close(client_socket);
    endwin(); // ncurses 모드 종료
    return 0;
}