#pragma warning(disable: 4996)

#include<stdio.h>
#include<stdlib.h>
#include<conio.h>
#include<string.h>
#include<time.h>
#include<windows.h>
#include<mmsystem.h>
#include<pthread.h>

#pragma comment(lib."winmm.lib")
#define HAVE_STRUCT_TIMESPEC
#define BGMPATH "AcidRain.wav"
#define SPEED 1000

char *words[35] = {
"오렌지",
"강아지",
"영화",
"컴퓨터",
"햄버거",
"자동차",
"축구",
"빅데이터",
"뉴욕",
"스마트폰",
"우유",
"초콜릿",
"캘리포니아",
"드론",
"자전거",
"커피",
"도쿄",
"역사",
"라면",
"스위스",
"경제학",
"코끼리",
"치즈",
"알고리즘",
"하늘",
"미술관",
"맥주",
"소파",
"초등학교",
"프랑스",
"생물학",
"네트워크",
"가방",
"인도",
"태양"};

typeder struct{
    int x; // x 좌표
    char[20]; //단어 저장
};

rain rains[21]; 
clock_t start, end // 게임 시작 종료 시간
double sec = 0.0; //플레이 시간
double ph = 7.0; // 산성비 농도
char buffer[50]; //사용자 입력창


void help(); //도움말
void viewlog(); // 점수 보기 함수
void initrains(); // 점수 초기화
void gamemain(); //메인 게임 로직
void prnscreen(); //게임 출력함수
void makerain(); //단어 생성 함수


static pthread p_thread; // 스레드 정의
static int thr_id //스레드 아이디
static int thr_exit =1; // 스레드 종료 여부

//스레드 시작 및 종료
void *t_function(void *data); // 스레드화 할 사용자 입력함수
void start_thread(); //스레드 시작함수
void end_thread(); // 스레드 중지함수

int main(void){
    

    
int choice = 0;
	
	
	printf("\n\n\n"); // main 화면
	printf("        ###    ##          ###      ##   ##        ##   ##  \n");
	printf("       ## ##   ##         ## ##     ##   ##        ##   ##  \n");
	printf("      ##   ##  ####      ##   ## #####   ############   ##  \n");
	printf("     ##     ## ##       ##     ##   ##   ##        ##   ##  \n");
	printf("                            ##      ##   ##        ##   ##  \n");
    printf("     ##                   ##  ##         ############   ##  \n");
	printf("     ##                  ##    ##                       ##  \n");
	printf("     ##                   ##  ##                        ##  \n");
    printf("     ############           ##                          ##  \n");
    printf("@@2024 Server Programming Project@@Lee__woo__young@@Kim__geum__young@@\n");
	printf("\n\n                      [ 시작하려면 아무키나 누르세요 ]\n");
	getch();

	while (1)
	{
		system("cls"); // 콘솔창 초기화
		printf("\n\n\n                            [ 메인메뉴 ]\n\n\n\n\n"); // 메인메뉴
		printf("                            1. 게임시작\n\n");
		printf("                            2. 기록보기\n\n");
		printf("                            3. 도 움 말\n\n");
		printf("                            4. 게임종료\n\n");
		printf("                       선택>");
		scanf("%d", &choice);

		switch (choice)
		{
		case 1:
			gamemain(); // 게임 함수 호출 (게임 시작)
			break;
		case 2:
			viewlog(); // 점수 기록 출력 함수 호출
			break;
		case 3:
			help(); // 도움말 출력 함수 호출
			break;
		case 4:
			system("cls"); // 콘솔창 초기화
			return 0; // 게임 종료
			break;
		default: // 그외 입력 무시
			break;
		}
	}
}
 
 void help() // 도움말
{
	system("cls"); //clear 명령어
	printf("타자연습보다 더 재밌었던 추억의 그 게임 '산성비'를 C언어로 구현한 게임입니다.\n");
	printf("하늘에서 내리는 단어들을 빨리 입력하여 없애주세요!\n");
	printf("하단의 수소 이온 농도(pH)가 0이 되면 게임이 종료됩니다.\n");
	printf("점수가 저장됩니다. 메인메뉴의 '2. 기록보기'에서 확인 가능합니다.\n\n");
	printf("입력이 없는 상태에서 엔터를 누르면 화면이 초기화되는 버그가 있습니다.\n\n\n"); // 버그
	printf("아무키나 누르면 메인 메뉴로 이동합니다.");
	getch();
}

void viewlog(){
    FILE *fp2 = NULL; // 점수 저장 파일
    double s;
    int cnt;
    System("cls");//clear 명령어
    printf("[점수(score)]\n\n\n");
    fp2 = fopen("score.txt","r");//스코어 파일 읽기
    if(fp2 == NULL) //파일 불러오기 실패했을때
    {
        printf("점수 파일 불러오기 실패");}

    else{
        cnt =1;
        while(EOF != fscanf(fp2, "%lf" , &s))//파일 끝까지 읽기
        {
            printf("%d.%.2lf초(sec)\n",cnt,s);

            cnt++;
        }

    }

    printf("\n\n 아무키나 누르면 메인메뉴로 이동합니다.");
    getch();}


    void gamemain() // 게임의 주 함수
{
	FILE *fp = NULL; // 점수 저장을 위한 파일 포인터
	ph = 7.0; // 수소 이온 농도 7.0으로 초기화
	system("cls"); // 콘솔창 초기화
	initrains(); // 단어 배열 초기화
	start_thread(); // 사용자로부터 입력을 받는 스레드 시작
	PlaySound(TEXT(BGMPATH), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP); // 배경음악 재생
	start = clock(); // 게임 시작 시간 기록
	while (1)
	{
		Sleep(SPEED); // 지정한 시간만큼 단어 생성 지연
		makerain(); // 새로운 단어 생성
		end = clock(); // 해당 단어가 생성된 시간 기록
		sec = (double)(end-start)/CLK_TCK; // 현재까지 진행한 시간
		prnscreen(); // 화면 출력
		if (ph <= 0) // 수소 이온 농도가 0 이하이면
			break; // 게임 오버
	}
	PlaySound(NULL, 0, 0); // 배경음악 중지
	end_thread(); // 입력 스레드 중지
	printf("\nGame Over!\n");
	fp = fopen("score.txt", "a"); // 점수 저장 파일 열기
	if (fp == NULL) // 파일 열기 오류
		printf("점수 기록 파일 작성 실패!\n\n");
	else
	{
		fprintf(fp, "%.2lf\n", sec); // 점수 기록
		fclose(fp);
	}
	printf("아무키나 누르면 메인메뉴로 이동합니다.\n");
	printf("메인메뉴가 나타나지 않으면 한번 더 입력해주세요.");
	getch();
}

void prnscreen() // 화면 출력 함수
{
	int i;
	system("cls"); // 콘솔창 초기화
	for (i = 0; i < 20; i++)
	{
		printf("%*s%s\n", rains[i].x,"",rains[i].word); // x좌표에 맞추어 가변적으로 단어 출력
	}
	printf("~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~"); // 판정선(rains[20].word)
	if (strcmp(rains[20].word, " ")) // 판정선에 단어가 남아있으면
		ph -= 0.5; // 0.5씩 산성화 시킴
	printf("\n[ pH ] %.1lf  [ 시간 ] %.2lf초\n\n", ph, sec); // 수소 이온 농도와 총 시간 출력
	printf("입력>"); // 사용자의 입력부
}

void makerain() // 새로운 단어(행)을 생성하는 함수
{
	int i;
	for (i = 20; i >= 0; i--)
	{
		strcpy(rains[i].word, rains[i - 1].word); // 기존 단어는 한 줄씩 밀고
		rains[i].x = rains[i - 1].x;
		rains[i - 1].x = 0;
		strcpy(rains[i - 1].word, " "); // 뒷 줄은 공백처리
	}
	rains[0].x = rand() % 53;
	srand(time(NULL));
	strcpy(rains[0].word, words[rand() % 35]); // 새로운 단어를 무작위로 배치
}

void initrains() // 단어 배열 초기화 함수 (모두 공백으로)
{
	int i;
	for (i = 0; i < 21; i++)
	{
		rains[i].x = 0;
		strcpy(rains[i].word, " ");
	}
}

void *t_function(void *data) // 스레드 처리할 단어 입력 함수
{
	while (!thr_exit) // 스레드가 중지될 때까지 입력을 계속 받음
	{
		int i;
		gets(buffer);
		for (i = 0; i < 20; i++)
		{
			if (strstr(rains[i].word, buffer)) // 입력 값과 일치하는 단어가 있으면
				strcpy(rains[i].word, " "); // 해당 단어 제거
		}
	}
}

void start_thread() // 스레드 시작 함수
{
	thr_exit = 0;
	thr_id = pthread_create(&p_thread, NULL, t_function, NULL); // 스레드 생성
}

void end_thread() // 스레드 중지 함수
{
	thr_exit = 1;
	pthread_cancel(p_thread); // 스레드 취소
}





