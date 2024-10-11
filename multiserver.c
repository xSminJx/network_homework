#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <process.h>
#pragma comment(lib, "Ws2_32.lib")
#define MYPORT 10001
#define BACKLOG 5
#define BUFFER_SIZE 1024

unsigned __stdcall client_handler(void* arg) {
	SOCKET new_fd = (SOCKET)arg;
	char buffer[BUFFER_SIZE];
	char message[BUFFER_SIZE]; // 누적된 데이터를 저장할 버퍼
	int buf_bytes, message_len = 0, i = 0;
	double val[3], res = 0; // 숫자와 연산자 값 및 결과값

	message[0] = '\0'; // 메시지 버퍼 초기화

	while (i < 3) {
		buf_bytes = recv(new_fd, buffer, BUFFER_SIZE - 1, 0);
		if (buf_bytes == SOCKET_ERROR) {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(new_fd);
			return 0;
		}
		else if (buf_bytes == 0) {
			// 연결 종료
			printf("Client disconnected.\n");
			closesocket(new_fd);
			return 0;
		}

		buffer[buf_bytes] = '\0';
		// 수신한 데이터를 메시지 버퍼에 누적
		if (message_len + buf_bytes >= BUFFER_SIZE) {
			printf("Message buffer overflow.\n");
			closesocket(new_fd);
			return 0;
		}
		strcat(message, buffer);
		message_len += buf_bytes;

		// 메시지 버퍼에서 완전한 줄을 처리
		char* line_start = message;
		char* newline_pos;
		while ((newline_pos = strchr(line_start, '\n')) != NULL && i < 3) {
			*newline_pos = '\0'; // '\n'을 '\0'으로 대체하여 한 줄 추출
			// 추출한 줄을 파싱하여 숫자로 변환
			char* endptr;
			val[i] = strtod(line_start, &endptr);
			if (endptr == line_start) {
				// 변환 실패
				printf("Invalid number format: %s\n", line_start);
				closesocket(new_fd);
				return 0;
			}
			printf("Received value[%d]: %lf\n", i, val[i]);
			i++;
			line_start = newline_pos + 1; // 다음 줄로 이동
		}

		// 처리되지 않은 데이터를 메시지 버퍼의 앞쪽으로 이동
		message_len = strlen(line_start);
		memmove(message, line_start, message_len + 1); // 널 문자 포함하여 이동
	}

	// 연산 수행
	if (val[0] == 0 && val[2] == 1) res = 1234567891;
	else {
		if (val[2] == 1) val[0] = 1/val[0]; // 뺄셈 처리
		res = val[1] * val[0];
	}


	// 결과 전송
	char result_str[32];
	sprintf(result_str, "%.5lf\n", res); // '\n' 추가
	if (send(new_fd, result_str, strlen(result_str), 0) == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(new_fd);
		return 0;
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
