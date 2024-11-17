#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <curses.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define MAX 100
#define BASIC_MODE 0
#define HIDE_MODE 1
#define BLINK_MODE 2
#define REVERSE_MODE 3
#define DIAGONAL_MODE 4

// 단어 노드
typedef struct node * nodePointer;
typedef struct node {
	nodePointer right, left;
	char str[MAX]; // 출력 문자
	int row, col; // 출력 행열
	int mode; // 출력 모드
	int modeset;
}node;

//HP
int hp = 20;
char hp_bar[3] = "";
//SCORE
int score = 0;
char score_bar[MAX] = "";
//LEVEL
int level = 1;
int level_mode = 1;
char level_bar[2] = "";

nodePointer ptr = NULL;

//TypingText
char typingText[20] = { 0 };
int enter_position = 0;

int timer = 1000000;
int word_interval = 2;
int word_clock = 0;
int levelUP_Clock = 20;
int level_Clock = 0;
int speed_up = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void all_Clear();
void word_Check(char *str);
nodePointer makeNode();
void down_Word();
void add_Word(char *str, int col);
char *wordDB();
void draw(int row, int col, char* str);
void exit(int);

//game
void start_game();
void *game_Board(void *);
void level_up();
//mode
void reverse(nodePointer *);
void diagonal(nodePointer *);
void fast();

// 메인 함수
void main() {

	char option;

	// 초기화
	initscr();
	clear();
	srand(time(NULL));
	// signal 처리
	signal(SIGQUIT, exit);
	signal(SIGALRM, fast);						// 5초 뒤부터 속도 빨라짐

	move(3, 10);
	addstr("******************************************************");
	move(4, 10);
	addstr("*                                                    *");
	move(5, 10);
	addstr("*                   TYPING GAME                      *");
	move(6, 10);
	addstr("*                                                    *");
	move(7, 10);
	addstr("*                                                    *");
	move(8, 10);
	addstr("******************************************************");
	move(11, 10);
	addstr("1. Start Game");
	move(12, 10);
	addstr("2. User score");
	move(13, 10);
	addstr("3. EXIT");

	move(16, 20);
	refresh();

	// 입력 받기
	option = getch();

	// option 값 따라
	switch (option) {
		// 게임하기
	case '1':
		// 게임
		start_game();
		break;
	case '2':
		break;
		// 종료하기
	case '3':
		break;
	}

	endwin();
}
// 게임
void start_game() {
	clear();
	int i = 0;
	pthread_t th;

	// Game Name
	move(0, 75);
	addstr("Level");
	move(1, 78);
	sprintf(level_bar, "%02d", level);
	addstr(level_bar);
	move(0, 15);
	addstr("ADDING_MODE");

	// score print
	move(1, 1);
	addstr("score : ");
	sprintf(score_bar, "%03d", score);
	addstr(score_bar);

	// hp print
	move(0, 1);
	addstr("life  : ");
	sprintf(hp_bar, "%3d", hp);
	addstr(hp_bar);
	refresh();

	// enter_position
	draw(20, 20, "	| Enter | : ");

	// 쓰레드 생성
	pthread_create(&th, NULL, game_Board, NULL);

	// hp > 0 이상일 동안 반복
	while (hp > 0) {
		// 반복문
		for (enter_position = 0; enter_position < 20;) {
			int c = getch();

			if (hp == 0) break;
			else {
				if (enter_position == 19 && c != '\n' && c != 127) {
					move(20, 36);
					addstr(typingText);
				}
				// enter 들어오면 문자열 찾아서 삭제
				else if (c == '\n') {
					typingText[enter_position] = '\0';
					word_Check(typingText);

					for (i = 0; i < 20; i++)
						typingText[i] = '\0';

					draw(20, 20, "	| Enter | : ");
					move(20, 36);

					break;

				}
				// backspace 들어오면 문자 하나 삭제
				else if (c == 127) {
					// 남아있으면 삭제
					if (enter_position > 0) {
						typingText[--enter_position] = '\0';
						draw(20, 20, "	| Enter | : ");
						move(20, 36);
						addstr(typingText);
					}
					// 없으면 빈문자열 출력
					else {
						draw(20, 20, "	| Enter | : ");
						move(20, 36);
						addstr("              	   	");
					}
				}
				// 입력이 되면 하나추가해서 보여주기
				else {
					typingText[enter_position++] = c;
					draw(20, 20, "	| Enter | : ");
					move(20, 36);
					addstr(typingText);
				}

				refresh();
			}
		}

	}
	// 쓰레드 조인
	pthread_join(th, NULL);

	all_Clear();
	clear();
}

// 강제 종료
void exit(int signum) {
	all_Clear();
	curs_set(1);
	endwin();
	exit(1);
}

// ALL CLEAR
void all_Clear() {
	nodePointer temp = NULL;
	nodePointer btemp = NULL;

	if (ptr)
	{
		while (temp) {
			temp = ptr->right;
			btemp = ptr;

			free(btemp);

			ptr = temp;
		}
	}
}

// 단어 출력 쓰레드
void * game_Board(void *m)
{
	nodePointer temp = NULL;
	nodePointer del = NULL;

	while (hp > 0) {

		word_clock++;
		level_Clock++;

		if (word_clock == word_interval) {
			add_Word(wordDB(), (rand() % 55) + 4);
			word_clock = 0;
		}
		else
			add_Word("", 0);

		if ((levelUP_Clock == level_Clock) && (level < 9)) level_up();

		temp = ptr;
		// 문자열 전부 출력
		while (temp) {
			// 빈 문자열 아닌 애들만 출력
			if (temp->row < 19 && strcmp(temp->str, ""))
			{
				if (temp->mode == HIDE_MODE && temp->modeset == 0)
					draw(temp->row, temp->col, temp->str);
				else if (temp->mode == HIDE_MODE && temp->modeset == 1)
					draw(temp->row, temp->col, "                   ");
				else if (temp->mode == BLINK_MODE && temp->modeset == 0)
					draw(temp->row, temp->col, temp->str);
				else if (temp->mode == BLINK_MODE && temp->modeset == 1)
					draw(temp->row, temp->col, "                   ");
				else
					draw(temp->row, temp->col, temp->str);
			}
			else if (temp->row < 19 && !strcmp(temp->str, ""))
				draw(temp->row, temp->col, "                   ");

			// row가 18 이상이면 없애고 점수까기
			if (temp->row >= 19 && strcmp(temp->str, "")) {
				pthread_mutex_lock(&lock); // 제어
										   // 삭제
				del = temp;
				temp->left->right = NULL;
				temp = temp->right;
				free(del);

				// hp print
				hp--;
				pthread_mutex_unlock(&lock); // 제어 unlock

				move(0, 1);
				addstr("life  : ");
				sprintf(hp_bar, "%3d", hp);
				addstr(hp_bar);
				refresh();
			}

			// row 18 이상이고 빈문자열일 경우
			else if (temp->row >= 19) {
				pthread_mutex_lock(&lock); // 제어
										   // 삭제
				del = temp;
				temp->left->right = NULL;
				temp = temp->right;
				free(del);
				pthread_mutex_unlock(&lock); // 제어 unlock

			}

			else
				temp = temp->right;
		}

		move(20, 36); // 커서 이동

		if (speed_up)
			usleep(timer / 2);
		else
			usleep(timer); // 대기

	}
}

void level_up()
{
	timer -= 100000;//간격 0.1초 감소
	level++;//레벨업
	move(0, 75);
	addstr("Level");
	move(1, 78);
	sprintf(level_bar, "%02d", level);
	addstr(level_bar);
	if (level == 2)
	{
		move(1, 15);
		addstr("HIDE_MODE");
		addstr(" FAST_MODE");
		alarm(5);//FAST_MODE
	}
	if (level == 3)
	{
		move(1, 15);
		addstr("HIDE_MODE");
		addstr(" FAST_MODE");
		addstr(" BLINK_MODE");
	}
	if (level == 4)
	{
		move(1, 15);
		addstr("HIDE_MODE");
		addstr(" FAST_MODE");
		addstr(" BLINK_MODE");
		addstr(" REVERSE_MODE");
	}
	if (level == 5)
	{
		move(1, 15);
		addstr("HIDE_MODE");
		addstr(" FAST_MODE");
		addstr(" BLINK_MODE");
		addstr(" REVERSE_MODE");
		addstr(" DIAGONAL_MODE");
	}
	
	if (level == 6)
	{
		move(1, 15);
		addstr("HIDE_MODE");
		addstr(" BLINK_MODE");
		addstr(" REVERSE_MODE");
		addstr(" DIAGONAL_MODE");
		addstr("         ");
	}

	level_Clock = 0;//시계 초기화

	if (level < 6)
		level_mode++;
}

// 해당 단어 찾기 함수
void word_Check(char *str) {

	nodePointer temp = NULL;

	temp = ptr;

	// 맨끝으로 가기
	while (temp->right)
		temp = temp->right;
	// 밑에서부터 탐색
	while (temp) {

		if (!strcmp(str, ""))
			return;

		// 같은 걸 찾으면 빈문자열로 바꿔준 후 종료
		if (!strcmp(temp->str, str)) {
			strcpy(temp->str, "");

			score++;

			// score print
			move(1, 1);
			addstr("score : ");
			sprintf(score_bar, "%03d", score);
			addstr(score_bar);
			refresh();
			return;
		}
		// 다음 노드
		else
			temp = temp->left;

	}

}

// 노드 생성
nodePointer makeNode() {
	nodePointer temp;

	temp = (nodePointer)malloc(sizeof(*temp));
	temp->right = temp->left = NULL;
	temp->mode = rand() % level_mode;
	temp->modeset = 0;
	return temp;
}

// 한 칸씩 내리기
void down_Word() {

	nodePointer temp = ptr->right;

	// row 1 증가
	while (temp) {
		temp->row += 1;

		if (temp->mode == HIDE_MODE && temp->row >= 9)
			temp->modeset = 1;

		if (temp->mode == BLINK_MODE && temp->modeset == 0)
			temp->modeset = 1;
		else if (temp->mode == BLINK_MODE && temp->modeset == 1)
			temp->modeset = 0;

		if (temp->mode == DIAGONAL_MODE)
			diagonal(&temp);

		temp = temp->right;
	}

}

// 단어 추가
void add_Word(char *str, int col) {

	nodePointer temp;

	// 첫 노드 일 경우, 단어 추가
	if (ptr == NULL) {
		ptr = makeNode();
		strcpy(ptr->str, str); // 받은 문자열로 단어 만들기

		if (ptr->mode == REVERSE_MODE)
			reverse(&temp);

		ptr->row = 2;
		ptr->col = col;
	}

	// 단어 추가
	else {
		temp = makeNode();
		strcpy(temp->str, str); // 받은 문자열로 단어 만들기

		if (temp->mode == REVERSE_MODE)
			reverse(&temp);

		temp->row = 2;
		temp->col = col;
		temp->right = ptr; // 양방향 연결
		ptr->left = temp; // 양방향 연결
		ptr = temp;//ptr == 맨 마지막에 생성된 노드(가장 윗줄의 단어) ptr->left == NULL, 가장 먼저 성생된 노드(가장 아랫줄의 단어)->right == NULL
		down_Word();
	}
}

// 문자열 선정
char * wordDB() {

	char *DB1[] = { "paper", "chair", "desk", "book", "note", "computer", "cloth", "closet", "sign", "door", "room", "pencil", "pen", "pants", "shop", "building", "keyboard", "dictionary", "mouse", "cellphone", "speaker", "hospital", "car", "bicycle", "cup" };
	char *DB2[] = { "anxiety","boredom","confuse","dragon","excitement","frustrate","greed","happiness","impatience","jealousy","kindess","loyalty","master","necessary","overwhelming","pessimism","quietus","relief","satisfaction","thrill","union","view","worriation","yell","zealot" };
	char *DB3[] = { "grape","orange","potato","watermelon","melon","peach","tomato","onion","spinach","cabbage","carrot","strawberry","pepper","pear","persimmon","pumpkin","mandarin","mushroom","apple","banana","cherry","plum","jujube","citron","pineapple" };
	char *DB4[] = { "Lion", "Tiger", "Elephant", "Dog", "Penguin", "Frog", "Horse", "Giraffe", "Sunflower", "Mugunghwa", "Rose", "Cherry blossoms", "Maple", "Tulip", "Cosmos" };

	int dice = rand() % 90;

	if (dice < 25)
		return DB1[dice];
	else if (dice < 50)
		return DB2[dice - 25];
	else if (dice < 75)
		return DB3[dice - 50];
	else
		return DB4[dice - 75];
}

// 그리기
void draw(int row, int col, char* str) {
	move(row, 0);	addstr("														");
	move(row, col);
	addstr(str);
	refresh();
}


void reverse(nodePointer *node)//노드의 데이터를 반대로 표현.
{
	int size = strlen((*node)->str);
	char temp;
	if ((*node)->modeset == 0) {
		(*node)->modeset = 1;
		for (int i = 0; i < size / 2; i++) {
			temp = (*node)->str[i];
			(*node)->str[i] = (*node)->str[(size - 1) - i];
			(*node)->str[(size - 1) - i] = temp;
		}
	}
}

void diagonal(nodePointer *node)//대각선 이동
{
	int n = rand() % 2;// 0 왼쪽 1 오른쪽

	if (n == 0)
	{
		if ((*node)->col - 2 > 0)
			(*node)->col -= 3;
	}

	if (n == 1) {
		if ((*node)->col + strlen((*node)->str) - 3 < 80)
			(*node)->col += 3;
	}
}

void fast() {
	if (speed_up == 0)
		speed_up = 1;
	else
		speed_up = 0;

	if (level < 6)
		alarm(5);		// 함수가 호출된 후 5초 뒤 속도 
}
