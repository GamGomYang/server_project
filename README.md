# 서프게임랜드

**조원**  
- **김금영** (소프트웨어학부)  
- **이우영** (소프트웨어학부)

**프로젝트 제목:** TCP/IP 소켓프로그래밍 및 멀티쓰레드를 활용한 멀티게임랜드

---

## 1. Background

수업 및 실습시간에 배웠던 **파일 입출력**, **프로세스**, **쓰레드**, **TCP/IP 소켓프로그래밍**을 최대한 활용하여 다양한 리눅스 함수 및 라이브러리를 이용한 여러 종류의 게임을 만들어보자는 목표로 시작된 프로젝트입니다.

---

## 2. Proposed Application

### 2.1 멀티플레이 타자게임 - 타짱 with chat

#### 전체 구성도 및 동작 방식
- 게임 실행 시, 클라이언트는 메인 소켓을 통해 서버에 접속 요청
- 서버는 클라이언트의 이름을 등록 후, **client_handler** 스레드를 생성하여 각 클라이언트 요청을 독립적으로 처리  
- 클라이언트는 `recv_handler`를 통해 서버로부터 전송된 메시지를 수신  
- 서버는 게임 시작, 게임 시간 측정, 종료 메시지 전송 등으로 게임을 제어

#### 사용 기술 및 특징
- **파일 입출력 (File IO):** 서버와 클라이언트의 에러 및 로그 기록 (예: server.log, client.log)
- **파이프 및 쓰레드 (Pipe & Threads):** 각 클라이언트 요청 독립 처리 및 공유자원 보호 (pthread, pthread_mutex)
- **소켓 (Socket):** TCP/IP 소켓을 통한 클라이언트-서버 연결 및 메시지 송수신

#### OSS 사용 여부
- **ncurses:** 오픈소스 Linux UI 라이브러리를 사용하여 게임창 UI 구현  
- **참고 OSS:** [typingGame](https://github.com/YoonShinWoong/typingGame)의 `word_check` 함수 참고

#### 응용의 강점
- **로그 파일 기록:** 에러 및 중요 이벤트를 기록하여 문제 파악 용이
- **멀티쓰레드 처리:** 독립적인 스레드 생성으로 클라이언트 요청의 병렬 처리 및 동기화 관리
- **TCP/IP 소켓:** 안정적인 메시지 송수신 및 명령어 처리

---

### 2.2 배틀쉽

#### 전체 구성도 및 동작 방식
- 단일 프로세스 기반의 서버-클라이언트 동작
- **client_handler**를 통해 각 클라이언트의 보드 배치, 공격 좌표, 공격 순서 등의 정보를 송수신
- 클라이언트는 공격 좌표를 선택하여 서버에 전송하고, 서버는 해당 좌표에 대해 **HIT/MISS** 판정을 수행 후 결과 전송

#### 사용 기술 및 특징
- **파일 입출력 (File IO):** 로그 파일을 통한 에러 및 문제점 기록
- **소켓 (Socket):** TCP/IP를 이용한 보드 배치, 공격 좌표 송수신 및 판정

#### OSS 사용 여부
- **ncurses:** 오픈소스 Linux UI 라이브러리로 게임창 UI 구현
- **참고 OSS:** [BattleshipClientServer](https://github.com/alexguillon/BattleshipClientServer)의 관련 함수 참고

#### 응용의 강점
- **그리드 배치 갱신 및 HIT/MISS 처리:** 실시간 공격 결과 반영 및 보드 갱신
- **안정적인 소켓 통신:** 공격 좌표 전송 및 결과 메시지 송수신으로 게임 진행 보장

---

### 2.3 다빈치코드 - Coda

#### 전체 구성도 및 동작 방식
- 서버에서 소켓 생성 및 바인딩 후 클라이언트 접속 대기  
- 클라이언트 접속 시, **mutex**를 사용하여 사용자 수 관리 및 동기화 처리  
- 사용자가 입력한 데이터를 서버가 받아 처리 후 결과를 클라이언트에 송신  
- 종료 조건 충족 시 게임 종료

#### 사용 기술 및 특징
- **TCP/IP 소켓:** 서버와 클라이언트 간 안정적인 데이터 전송
- **mutex:** 다중 클라이언트 접속 시 안전한 사용자 수 확인 및 턴 관리
- **ncurses:** 텍스트 기반의 직관적인 게임 UI 구현

#### OSS 사용 여부
- **ncurses:** 오픈소스 Linux UI 라이브러리를 사용하여 게임 UI 구현

#### 응용의 강점
- **안정적인 데이터 전송:** 데이터 손실 없이 서버와 클라이언트 간 통신
- **동기화 및 경쟁 조건 방지:** Mutex를 활용한 안전한 멀티 클라이언트 환경 관리
- **직관적인 게임 UI:** ncurses를 활용한 깔끔한 텍스트 기반 인터페이스

---

## 3. Usage Checklist

| 내용                                 | 사용 여부 |
| ------------------------------------ | --------- |
| Linux intro (week 2)                 | O         |
| Dir/File (week 4)                    | X         |
| File IO (week 5)                     | O         |
| Process (weeks 7 & 9)                | O         |
| Pipe & Threads (week 10)             | O         |
| Socket (weeks 11 & 12)               | O         |

---

## 4. Demonstration

### 멀티플레이 타자게임 - 타짱 with chat
- 게임 접속 시 채팅창 등장  
- 다양한 명령어를 통해 채팅, 방 생성, 방 리스트 확인, 모드 확인, 게임 시작 등 기능 제공  
- 게임은 소나기 게임 방식으로 90초간 진행되며, 한 유저가 목표점수(150점)에 도달하면 승리 후 게임 종료

### 배틀쉽
- **싱글플레이 모드:** 한 명의 유저가 다양한 난이도(easy, normal, hard)를 선택하여 컴퓨터와 게임 진행  
- **멀티플레이 모드 (배틀넷):** 두 명의 유저가 IP와 포트를 입력 후 차례대로 공격 및 상대 보드의 성공 여부 확인

### 다빈치코드 게임
- TCP/IP 접속 후, 상대와 타일을 나누어 턴제 방식으로 추리 진행  
- Mutex를 통한 한 턴에 한 명의 플레이어만 추리하도록 제어

---

## 5. Conclusions

### 보완사항

- **멀티플레이 타자게임 - 타짱 with chat**
  - 초기 설계 시 `exec` 함수군 활용 계획이었으나 커서 충돌 문제로 구현하지 못함
  - 게임 모드 설정 명령어 파싱 및 방 내 모든 유저 퇴장 시 처리 미흡

- **배틀쉽**
  - 멀티플레이 게임에서 ncurses 인터페이스 충돌로 인한 커서 사라짐 현상 개선 필요

- **다빈치코드 게임**
  - 최대 3~4명 접속 가능하지만, 상대 타일 보여주는 과정으로 인해 실제 게임은 2인용으로 제한
  - 남은 타일이 없는 상태에서의 추리 실패 시 랜덤 오픈 기능 추가 필요

