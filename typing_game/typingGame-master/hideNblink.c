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

// 노드 구조체 선언
typedef struct node * nodePointer;
typedef struct node {
	nodePointer right,left;
	char str[MAX]; // 출력 문자
	int row,col; // 출력 행열
	int mode; // 출력 모드
}node;

// 전역 변수
int hp = 5;
char hp_str[2]="";
int score = 0;
char score_str[MAX]="";
int i = 0; 
int length = 0;
nodePointer ptr = NULL;
char enterText[20] = { 0 };
int enterHere = 0;
int sleep_time = 1;
int sleep_cnt=0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// 사용함수들
void reset();
void *thread_1();
void findWord(char *str);
nodePointer makeNode();
void makePlusOne();
void addQ(char *str, int col);
char * returnWord();
void draw(int row, int col, char* str);
void function(int);

// 메인 함수
void main() {
	pthread_t t1,t2;

	initscr();
	clear();

	srand(time(NULL));
	
	// signal 처리
	signal(SIGQUIT,function);

	// default setting
	move(0,70);
	addstr("*username");

	// score print
	move(1,1);
	addstr("score : ");
	sprintf(score_str,"%d",score);
	addstr(score_str);
	refresh();

	// hp print
	move(0,1);
	addstr("life  : ");
	sprintf(hp_str,"%d",hp);
	addstr(hp_str);
	refresh();

	// enter setting
	draw(20, 20, "	| Enter | : ");

	// 쓰레드 생성
	pthread_create(&t1, NULL, thread_1, NULL);
	
	// hp > 0 이상일 동안 반복
	while (hp > 0) {		

		for (enterHere = 0; enterHere < 20;) {
			int c = getch();

			// enter 들어오면 문자열 찾아서 삭제
			if (c == '\n') {
				enterText[enterHere] = '\0';
				findWord(enterText);

				for (i = 0; i < 20; i++)
					enterText[i] = '\0';

				draw(20, 20, "	| Enter | : ");
				move(20, 36);

				break;
			}
			// backspace 들어오면 문자 하나 삭제
			else if (c == 127) {
				// 남아있으면 삭제
				if (enterHere > 0) {
					enterText[--enterHere] = '\0';
					move(20, 36);
					addstr("		");
					move(20, 36);
					addstr(enterText);
				}
				// 없으면 빈문자열 출력
				else {
					move(20, 36);
					addstr("		");
				}
			}

			// 입력이 되면 하나추가해서 보여주기
			else {
				enterText[enterHere++] = c;
				move(20, 36);
				addstr(enterText);
			}

			refresh();
		}
	}

	// 쓰레드 조인
	pthread_join(t1, NULL);

	// 초기화	
	reset();
	clear();
}

// 강제 종료시 함수 종료 처리
void function(int signum){
	reset();
	curs_set(1);
	endwin();
	exit(1);
}

// node free 함수
void reset() {
	nodePointer temp = NULL;
	nodePointer temp2 = NULL;

	temp = ptr;

	if (temp != NULL) {
		// 개수 남아있으면 삭제하기
		while (length > 0) {
			temp = ptr;

			while (temp->right) {
				temp2 = temp;
				temp = temp->right;
			}

			free(temp); // 비워주기

			if (temp2 != NULL)
				temp2->right = NULL;

			length--; // 개수 감소
			temp2 = NULL;
		}
		ptr = NULL;
	}
}

// 단어 출력 쓰레드
void * thread_1()
{
	int t = sleep_time;
	nodePointer temp = NULL;
	nodePointer del=NULL;	
	while (hp > 0) {
		
		sleep_cnt++;

		// 3칸마다 문자열 생성
		switch(sleep_cnt){
			case 0:
				addQ("",0);
				break;
			case 1:
				addQ("",0);
				break;
			case 2:
				addQ(returnWord(), (rand() % 40) + 8);
				sleep_cnt=0;
		}

		temp = ptr;
		

		// 문자열 전부 출력
		while (temp) {
			switch(temp->mode)
			{
			case 1://hide
			if(strcmp(temp->str,"") && temp->row<9)
				draw(temp->row, temp->col, temp->str);			

			else
			{//empty str or temp->row>9 : hide

				draw(temp->row,temp->col, "                   "); 			
			}
			break;
;
			case 2: //blink
			if(strcmp(temp->str,"") )
			{	
				draw(temp->row, temp->col, temp->str);
				usleep(300000);
				draw(temp->row,temp->col,"                 ");
				usleep(300000);

			}			

			else
			{//empty str  
				draw(temp->row,temp->col,"                 ");
							
			}
			break;	

			default://usual mode
			if(strcmp(temp->str,""))
				draw(temp->row, temp->col, temp->str);			

			else{
				draw(temp->row,temp->col,"                 ");
			}
			break;

			}

			

			// row가 19 넘어가면 없애고 점수까기
			if(temp->row >=18 && strcmp(temp->str,"")){
				pthread_mutex_lock(&lock); // 제어
				// 삭제
				del =temp;				
				temp->left->right = NULL;
				temp = temp->right;
				free(del); 

				// hp print
				hp--;
				pthread_mutex_unlock(&lock); // 제어 unlock

				move(0,1);
				addstr("life  : ");
				sprintf(hp_str,"%d",hp);
				addstr(hp_str);
				refresh();
			}
			
			// row 19 넘어가고 빈문자열일 경우
			else if(temp->row >= 18){
				pthread_mutex_lock(&lock); // 제어
				// 삭제
				del =temp;				
				temp->left->right = NULL;
				temp = temp->right;
				free(del); 
				pthread_mutex_unlock(&lock); // 제어 unlock

			}

			else
				temp = temp->right;
		}

		move(20, 36); // 커서 이동
		sleep(t); // 대기
		
	}
}

// 해당 단어 찾기 함수
void findWord(char *str) {

	nodePointer temp = NULL;
	temp = ptr;
	
	// 맨끝으로 가기
	while(temp->right)
		temp=temp->right;
	// 밑에서부터 탐색
	while (temp) {

		if(!strcmp(str,""))
			return;

		// 같은 걸 찾으면 빈문자열로 바꿔준 후 종료
		if (!strcmp(temp->str, str)) {
			strcpy(temp->str,"");
			
			score++;

			// score print
			move(1,1);
			addstr("score : ");
			sprintf(score_str,"%d",score);
			addstr(score_str);
			refresh();
			return;
		}
		// 다음 노드
		else
			temp = temp->left;

		
	}

}

// 노드 만들기 함수
nodePointer makeNode() {	
	nodePointer temp;

	temp = (nodePointer)malloc(sizeof(*temp));
	temp->right = temp->left = NULL;
	return temp;
}

// 한줄 씩 내리는 함수
void makePlusOne() {

	nodePointer temp = ptr->right;

	// row 1 증가
	while (temp) {
		temp->row += 1;
		temp = temp->right;
	}

}

// 단어 추가하는 함수
void addQ(char *str, int col) {

	nodePointer temp, temp2;

	// 첫 노드 일 경우, 단어 추가
	if (ptr == NULL) {
		ptr = makeNode();
		strcpy(ptr->str, str); // 받은 문자열로 단어 만들기
		ptr->row = 2;
		ptr->col = col;
		ptr->mode=1+rand()%3;//mode : 1 to 3
	}

	// 단어 추가
	else {
		temp = makeNode();
		strcpy(temp->str, str); // 받은 문자열로 단어 만들기
		temp->row = 2;
		temp->col = col;
		ptr->mode=1+rand()%3;//mode : 1to 3
		temp->right = ptr; // 양방향 연결
		ptr->left = temp; // 양방향 연결
		ptr = temp;
		makePlusOne();
	}
	length++;
}

// 랜덤으로 문자열 반환하기
char * returnWord() {

	char * DB[] = {"grape","orange","potato","watermelon","melon","peach","tomato","onion","spinach",
"cabbage","carrot","strawberry","pepper","pear","persimmon","pumpkin","mandarin"
,"mushroom","apple","banana","cherry","plum","jujube","citron","pineapple" };

	return DB[rand() % 14];
}

// 그리기 함수
void draw(int row, int col, char* str) {

	move(row, 0);	addstr("														");
	move(row, col);
	addstr(str);
	refresh();
}


