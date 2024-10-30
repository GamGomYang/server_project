#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define KEY_ESC 27
#define KEY_ENTER '\n'
#define KEY_BS 127

#define MAX_WORD_LENGTH 24
#define MAX_SCREEN_WORD_COUNT 19
#define WORD_COUNT 10
#define BELL 0x07 

int g_word_count = 11;
char *g_words[11] = {
    "programming", "school", "student", "chair", "desk",
    "average", "screen", "mouse", "window", "door", " "
};
int g_fail_count = 0;
int g_score = 0;
char g_input_word[MAX_WORD_LENGTH + 1];
int g_input_word_length = 0;
int g_stage_count = 1;
int term_width, term_height; // 터미널 크기를 저장할 변수

struct ScreenWord {
    int index;
    int x;
    int y;
};
typedef struct ScreenWord ScreenWord;
ScreenWord g_screen_word[MAX_SCREEN_WORD_COUNT];
int g_screen_word_count = 0;

clock_t g_start_time;
double g_falling_speed = 1.5;

// 터미널 크기 감지 함수
void GetTerminalSize(int *width, int *height) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *width = w.ws_col;
    *height = w.ws_row;
}

// 커서 숨기기 함수
void SetCursorVisible(int visible) {
    if (!visible)
        printf("\033[?25l"); // 숨기기
    else
        printf("\033[?25h"); // 보이기
}

// 커서 이동 함수
void GotoXY(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

// _kbhit 대체 함수
int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void InitScreen(void);
void InitData(void);
void Run(void);
void CompareWords(void);
void ProcessInput(int key);
void NormalMode(void);
void DrawWord(int i);
void EraseWord(int i);
void randomWord(int i);
void UpdateScore(void);
void UpdateFailCount(void);
void inputkey(int key);
void stageboard(void);
void pause_game(void);
void stage(void);

int main(void) {
    int key;
    SetCursorVisible(0);

    GetTerminalSize(&term_width, &term_height); // 터미널 크기 가져오기

    GotoXY(term_width / 2 - 15, term_height / 2 - 1); 
    printf("====== 시작하려면 아무 키나 누르세요 ======");
    getchar();
    srand(time(NULL));
    InitScreen();
    Run();

    return 0;
}

void InitScreen(void) {
    int i;

    system("clear");

    GotoXY(term_width / 2 - 25, 1);
    printf("┌───────────────────────────────────────────────────────────┐");
    for (i = 2; i < term_height - 5; i++) {
        GotoXY(term_width / 2 - 25, i);
        printf("│                                                           │");
    }
    GotoXY(term_width / 2 - 25, term_height - 5);
    printf("└───────────────────────────────────────────────────────────┘");

    // Programmed by
    GotoXY(term_width / 2 - 12, term_height - 3);
    printf("┌─────────────────────┐");
    GotoXY(term_width / 2 - 12, term_height - 2);
    printf("│  Programmed by User │");
    GotoXY(term_width / 2 - 12, term_height - 1);
    printf("└─────────────────────┘");

    // 입력창
    GotoXY(term_width / 2 - 12, term_height - 7);
    printf("┌─────────────────────┐");
    GotoXY(term_width / 2 - 12, term_height - 6);
    printf("│                     │");
    GotoXY(term_width / 2 - 12, term_height - 5);
    printf("└─────────────────────┘");

    // 점수
    GotoXY(term_width - 20, term_height - 7);
    printf("┌───────┐");
    GotoXY(term_width - 20, term_height - 6);
    printf("│  %d   │", g_score);	
    GotoXY(term_width - 20, term_height - 5);
    printf("└───────┘");

    // 실패 횟수
    GotoXY(term_width - 10, term_height - 7);
    printf("┌───────┐");
    GotoXY(term_width - 10, term_height - 6);
    printf("│  %d   │", g_fail_count);	
    GotoXY(term_width - 10, term_height - 5);
    printf("└───────┘");
}

void Run(void) {
    int i = 0, j, key;

    while(1) {
        randomWord(i);  
        while(1) {
            if (kbhit()) {
                key = getchar();
                inputkey(key);
                if (key == '1') {
                    system("clear");
                    pause_game();
                }
            }
            stage();
            if (g_fail_count == 6) {
                system("clear");
                GotoXY(term_width / 2 - 10, term_height / 2 - 1); 
                printf(" G A M E  O V E R");
                GotoXY(term_width / 2 - 15, term_height / 2 + 1); 
                printf("아무 키나 눌러 게임을 종료합니다.");
                getchar();
                exit(0);
            }
            if ((clock() - g_start_time) / CLOCKS_PER_SEC > g_falling_speed) {
                for (j = 0; j < 19; j++) {
                    EraseWord(j);
                    if (g_screen_word[j].y == term_height - 8) {
                        i = j - 1;
                        if (g_screen_word[j].index != 10) {
                            g_fail_count++;
                            UpdateFailCount();
                        }
                    } else {
                        if (g_screen_word[j].x > 1) {
                            g_screen_word[j].y++;
                            DrawWord(j);
                        }
                    }
                }
                g_start_time = clock();
                break;
            }
        }
        i++;
    }
}
// 터미널 크기 감지, SetCursorVisible, GotoXY, kbhit 등의 함수는 동일하므로 생략하고 주요 누락된 함수만 추가합니다

// 새로운 단어의 위치를 랜덤으로 설정하는 함수
void randomWord(int i) {
    g_screen_word[i].x = rand() % (term_width - 2); // 너비에 맞게 조정
    g_screen_word[i].y = 0; // 단어는 화면 상단에서 시작
    g_screen_word[i].index = rand() % 10; // 랜덤 단어 선택
}

// 입력된 키에 따라 적절한 동작을 수행하는 함수
void inputkey(int key) {
    int i;
    switch (key) {
        case KEY_ENTER:
            CompareWords();
            for (i = 0; i < g_input_word_length; i++) {
                GotoXY(30 + i, term_height - 6);
                printf(" ");
            }
            memset(g_input_word, 0, sizeof(g_input_word));
            g_input_word_length = 0;
            break;
        case KEY_BS:
            if (g_input_word_length > 0) {
                g_input_word_length--;
                GotoXY(30 + g_input_word_length, term_height - 6);
                printf(" ");
                g_input_word[g_input_word_length] = 0;
            }
            break;
        case KEY_ESC:
            exit(0);
            break;
        default:
            ProcessInput(key);
            break;
    }
}

// 단어를 그리기 함수
void DrawWord(int i) {
    GotoXY(g_screen_word[i].x, g_screen_word[i].y);
    printf("%s", g_words[g_screen_word[i].index]);
}

// 단어를 지우는 함수
void EraseWord(int i) {
    int c;
    if (g_screen_word[i].y > 0) {
        GotoXY(g_screen_word[i].x, g_screen_word[i].y);
        for (c = 0; c < strlen(g_words[g_screen_word[i].index]); c++)
            printf(" ");
    }
}

// 점수를 업데이트하는 함수
void UpdateFailCount(void) {
    GotoXY(term_width - 10, term_height - 6);
    printf("%d", g_fail_count);
}

// 일시 정지 기능 함수
void pause_game(void) {
    GotoXY(term_width / 2 - 10, term_height / 2);
    printf("게임이 일시정지되었습니다.");
    GotoXY(term_width / 2 - 15, term_height / 2 + 2);
    printf("계속하려면 아무 키나 누르세요.");
    getchar();
    system("clear");
    InitScreen();
}

// 스테이지 클리어 상태를 확인하고 다음 스테이지로 넘어가는 함수
void stage(void) {
    if (g_score == 6 && g_stage_count == 1) {
        stageboard();
        g_stage_count++;
    }
    if (g_score == 6 && g_stage_count == 2) {
        stageboard();
        g_stage_count++;
    }
    if (g_score == 5 && g_stage_count == 3) {
        stageboard();
        g_stage_count++;
    }
    if (g_score == 5 && g_stage_count == 4) {
        stageboard();
        g_stage_count++;
    }
    if (g_score == 4 && g_stage_count == 5) {
        stageboard();
        g_stage_count++;
    }
    if (g_score == 4 && g_stage_count == 6) {
        system("clear");
        GotoXY(term_width / 2 - 10, term_height / 2);
        printf("축하합니다! 모든 스테이지를 클리어하셨습니다.");
        getchar();
        exit(0);
    }
}

// 스테이지 완료 시 화면 표시 및 초기화 함수
void stageboard(void) {
    g_falling_speed -= 0.2;
    g_score = 0;
    g_fail_count = 0;
    system("clear");
    GotoXY(term_width / 2 - 10, term_height / 2);
    printf("====== STAGE %d clear ======", g_stage_count);
    GotoXY(term_width / 2 - 15, term_height / 2 + 2);
    printf("다음 STAGE로 진행하려면 아무 키나 누르세요.");
    getchar();
    InitScreen();
}


void CompareWords(void) {
    int i, a = 0, b = 0;

    for (i = 0; i < MAX_SCREEN_WORD_COUNT; i++) {
        if (strcmp(g_input_word, g_words[g_screen_word[i].index]) == 0) {
            if (a < g_screen_word[i].y) {
                b = i;
                a = g_screen_word[i].y;
            }
        }
    }
    if (a > 0) {
        putchar(BELL); // 알림음
        EraseWord(b);
        g_screen_word[b].index = 10; // 단어 제거 표시
        g_score++;
        UpdateScore();
    }
}


void ProcessInput(int key) { 
    if (g_input_word_length < MAX_WORD_LENGTH) {
        GotoXY(30 + g_input_word_length, term_height - 6);
        printf("%c", key);
        g_input_word[g_input_word_length] = key;
        g_input_word_length++;
    }
}

// 점수를 업데이트하고 화면에 표시하는 함수
void UpdateScore(void) {
    GotoXY(term_width - 20, term_height - 6);
    printf("%d", g_score);
    fflush(stdout); // 출력 버퍼 즉시 반영
}
