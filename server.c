#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#pragma comment(lib, "ws2_32")
#define MYPORT 9999
#define BACKLOG 5
#define BUFFER_SIZE 1000
int iserror = 0;

// 공백 커팅
void preprocess(char* expression) {
	// 공백 제거를 위한 임시 변수
	char temp[BUFFER_SIZE + 1];
	int j = 0;
	for (int i = 0; expression[i] != '\0'; i++) {
		if (!isspace(expression[i])) {
			temp[j++] = expression[i];
		}
	}
	temp[j] = '\0';
	strcpy(expression, temp);
}

int precedence(char op) {
	switch (op) {
	case '+':
	case '-':
		return 1;
	case '*':
	case '/':
		return 2;
	default:
		return 0;
	}
}

// 스택
typedef struct {
	char data[BUFFER_SIZE];
	int top;
} CharStack;

void initCharStack(CharStack* s) {
	s->top = -1;
}

int isCharStackEmpty(CharStack* s) {
	return s->top == -1;
}

void pushChar(CharStack* s, char value) {
	if (s->top < BUFFER_SIZE - 1) {
		s->data[++(s->top)] = value;
	}
}

char popChar(CharStack* s) {
	if (!isCharStackEmpty(s)) {
		return s->data[(s->top)--];
	}
	return '\0';
}

char peekChar(CharStack* s) {
	if (!isCharStackEmpty(s)) {
		return s->data[s->top];
	}
	return '\0';
}

// 더블 스택
typedef struct {
	double data[BUFFER_SIZE];
	int top;
} DoubleStack;

void initDoubleStack(DoubleStack* s) {
	s->top = -1;
}

int isDoubleStackEmpty(DoubleStack* s) {
	return s->top == -1;
}

void pushDouble(DoubleStack* s, double value) {
	if (s->top < BUFFER_SIZE - 1) {
		s->data[++(s->top)] = value;
	}
}

double popDouble(DoubleStack* s) {
	if (!isDoubleStackEmpty(s)) {
		return s->data[(s->top)--];
	}
	return 0;
}


// 중위 표기식 -> 후위 표기식 (사칙 연산에 필요)
void infixToPostfix(const char* infix, char* postfix) {
	CharStack stack;
	initCharStack(&stack);
	int j = 0;
	for (int i = 0; infix[i] != '\0'; i++) {
		char token = infix[i];

		if (isdigit(token)) {
			// 숫자인 경우
			postfix[j++] = token;
			// 다음 문자가 숫자가 아니면 공백 추가 (다자릿수 구분)
			if (!isdigit(infix[i + 1])) {
				postfix[j++] = ' ';
			}
		}
		else if (token == '+' || token == '-' || token == '*' || token == '/') {
			while (!isCharStackEmpty(&stack) && precedence(peekChar(&stack)) >= precedence(token)) {
				postfix[j++] = popChar(&stack);
				postfix[j++] = ' ';
			}
			pushChar(&stack, token);
		}
		else {
			printf("%c", token);
			iserror = 1; // 수식의 오류 감지
			return;
		}
	}

	// 스택에 남은 연산자 처리(스택 정리)
	while (!isCharStackEmpty(&stack)) {
		postfix[j++] = popChar(&stack);
		postfix[j++] = ' ';
	}
	postfix[j] = '\0';
}

// 만든 후위 표기식을 계산하고 연산 서버랑 통신도 함
double evaluatePostfix(const char* postfix) {
	DoubleStack stack;
	initDoubleStack(&stack);
	int i = 0;
	while (postfix[i] != '\0') {
		if (isdigit(postfix[i]) || postfix[i] == '.') {
			// 숫자 추출
			char number[32];
			int j = 0;
			while (isdigit(postfix[i]) || postfix[i] == '.') {
				number[j++] = postfix[i++];
			}
			number[j] = '\0';
			pushDouble(&stack, atof(number));
		}
		else if (postfix[i] == '+' || postfix[i] == '-' || postfix[i] == '*' || postfix[i] == '/') {
			// 연산자 처리
			double val1 = popDouble(&stack);
			double val2 = popDouble(&stack);
			double result = 0;

			// 서버에 전송할 데이터 구성
			char send_data[64];
			int operation_flag = 0; // 덧셈: 0, 뺄셈: 1
			int server_port = 0;

			if (postfix[i] == '+') {
				operation_flag = 0;
				server_port = 10000; // 덧셈 서버 포트
			}
			else if (postfix[i] == '-') {
				operation_flag = 1;
				server_port = 10000; // 뺄셈도 덧셈 서버에서 처리
			}
			else if (postfix[i] == '*') {
				operation_flag = 0;
				server_port = 10001; // 곱셈 서버 포트
			}
			else if (postfix[i] == '/') {
				operation_flag = 1;
				server_port = 10001; // 나눗셈도 곱셈 서버에서 처리
			}

			sprintf(send_data, "%.5lf\n%.5lf\n%d\n", val1,val2,operation_flag);

			// 서버에 연결
			SOCKET op_sock = socket(AF_INET, SOCK_STREAM, 0);
			if (op_sock == INVALID_SOCKET) {
				printf("소켓 생성 실패: %d\n", WSAGetLastError());
				// 에러 처리: 소켓 생성 실패 시 함수 종료
				return 0; // 적절한 에러 값을 반환
			}

			struct sockaddr_in op_addr;
			op_addr.sin_family = AF_INET;
			op_addr.sin_port = htons(server_port);
			op_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 로컬 호스트 IP 주소
			ZeroMemory(&(op_addr.sin_zero), 8);

			if (connect(op_sock, (struct sockaddr*)&op_addr, sizeof(op_addr)) == SOCKET_ERROR) {
				printf("연산 서버에 연결 실패: %d\n", WSAGetLastError());
				closesocket(op_sock);
				return 0;
			}

			// 서버에 데이터 전송
			int send_result = send(op_sock, send_data, strlen(send_data), 0);
			if (send_result == SOCKET_ERROR) {
				printf("데이터 전송 실패: %d\n", WSAGetLastError());
				closesocket(op_sock);
				return 0;
			}

			// 서버로부터 결과 수신
			char recv_data[64];
			int recv_bytes = recv(op_sock, recv_data, sizeof(recv_data) - 1, 0);
			if (recv_bytes > 0) {
				recv_data[recv_bytes] = '\0';
				result = atof(recv_data);
			}
			else {
				printf("서버로부터 데이터 수신 실패: %d\n", WSAGetLastError());
				closesocket(op_sock);
				// 에러 처리
				return 0;
			}
			if (result == 1234567891.00000) {
				return result;
			}

			closesocket(op_sock);

			// 결과를 스택에 푸시
			pushDouble(&stack, result);
			i++;
		}
		else {
			i++; // 공백 등 무시
		}
	}
	double result = popDouble(&stack);
	return result;
}

// 클라이언트와 통신하는 서버
unsigned __stdcall client_handler(void* arg) {
	SOCKET new_fd = (SOCKET)arg;
	char buffer[BUFFER_SIZE];
	char message[BUFFER_SIZE]; // 실제 입력 수식
	char postfix[BUFFER_SIZE]; // 후위 표기식
	int buf_bytes, message_len = 0;
	double res = 0;

	const char* connect_msg = "서버와 연결 성공. 수식 입력: ";

	if (send(new_fd, connect_msg, strlen(connect_msg), 0) == SOCKET_ERROR) {
		perror("send");
		closesocket(new_fd);
		return 0;
	}

	while ((buf_bytes = recv(new_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
		buffer[buf_bytes] = '\0';
		strcpy(&message[message_len], buffer);
		message_len += buf_bytes;

		if (strchr(buffer, '\n') != NULL) {
			message[message_len - 1] = '\0'; // '\n'을 '\0'으로 대체
			printf("수신된 수식: %s\n", message);

			// 수식 처리
			preprocess(message);
			infixToPostfix(message, postfix);

			if (iserror) {
				const char* errormsg = "수식 에러 발생";
				printf("%s", errormsg);
				if (send(new_fd, errormsg, strlen(errormsg), 0) == SOCKET_ERROR) {
					perror("send");
					closesocket(new_fd);
					return 0;
				}
			}
			else {
				res = evaluatePostfix(postfix);
				if (res == 1234567891.00000) {
					const char* errormsg = "0으로 나누기 존재";
					printf("%s\n", errormsg);
					if (send(new_fd, errormsg, strlen(errormsg), 0) == SOCKET_ERROR) {
						perror("send");
						closesocket(new_fd);
						return 0;
					}
				}
				else {
					char result_str[32];
					sprintf(result_str, "%.5lf", res);
					send(new_fd, result_str, strlen(result_str), 0);
				}
			}
			break;
		}
	}

	if (buf_bytes == SOCKET_ERROR) {
		perror("recv");
	}
	closesocket(new_fd);
	return 0;
}

int main() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup failed.\n");
		return 1;
	}

	SOCKET sockfd, new_fd;  /* 리슨 소켓과 새로운 연결 소켓 */
	struct sockaddr_in my_addr;    /* 서버 주소 정보 */
	struct sockaddr_in their_addr; /* 클라이언트 주소 정보 */
	int sin_size;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		perror("socket");
		WSACleanup();
		exit(1);
	}

	my_addr.sin_family = AF_INET;         /* 호스트 바이트 순서 */
	my_addr.sin_port = htons(MYPORT);     /* 네트워크 바이트 순서로 포트 번호 설정 */
	my_addr.sin_addr.s_addr = INADDR_ANY; /* 자신의 IP로 자동 설정 */
	ZeroMemory(&(my_addr.sin_zero), 8);   /* 구조체의 나머지 부분을 0으로 초기화 */

	if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		perror("bind");
		closesocket(sockfd);
		WSACleanup();
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == SOCKET_ERROR) {
		perror("listen");
		closesocket(sockfd);
		WSACleanup();
		exit(1);
	}

	while (1) {  /* 메인 accept() 루프 */
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size)) == INVALID_SOCKET) {
			perror("accept");
			continue;
		}
		printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

		// 스레드를 생성하여 클라이언트 처리
		uintptr_t hThread = _beginthreadex(NULL, 0, client_handler, (void*)new_fd, 0, NULL);
		if (hThread == 0) {
			perror("Thread creation failed");
			closesocket(new_fd);
			continue;
		}
		CloseHandle((HANDLE)hThread);
	}

	closesocket(sockfd);
	WSACleanup();
	return 0;
}