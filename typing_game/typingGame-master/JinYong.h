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

#define diagonal_mode 1
#define reverse_mode 2
#define a_mode 3
#define b_mode 4
#define c_mode 5
#define d_mode 6

typedef struct node * nodePointer;
typedef struct node {
	nodePointer right, left;
	char str[MAX]; // 출력 문자
	int row, col; // 출력 행열
	int mode; // 출력 모드
	int modeset;
}node;

char *getWord();//단어 결정
void reverse(nodePointer *);
void diagonal(nodePointer *);
