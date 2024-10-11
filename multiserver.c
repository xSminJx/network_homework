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
	char message[BUFFER_SIZE]; // ������ �����͸� ������ ����
	int buf_bytes, message_len = 0, i = 0;
	double val[3], res = 0; // ���ڿ� ������ �� �� �����

	message[0] = '\0'; // �޽��� ���� �ʱ�ȭ

	while (i < 3) {
		buf_bytes = recv(new_fd, buffer, BUFFER_SIZE - 1, 0);
		if (buf_bytes == SOCKET_ERROR) {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(new_fd);
			return 0;
		}
		else if (buf_bytes == 0) {
			// ���� ����
			printf("Client disconnected.\n");
			closesocket(new_fd);
			return 0;
		}

		buffer[buf_bytes] = '\0';
		// ������ �����͸� �޽��� ���ۿ� ����
		if (message_len + buf_bytes >= BUFFER_SIZE) {
			printf("Message buffer overflow.\n");
			closesocket(new_fd);
			return 0;
		}
		strcat(message, buffer);
		message_len += buf_bytes;

		// �޽��� ���ۿ��� ������ ���� ó��
		char* line_start = message;
		char* newline_pos;
		while ((newline_pos = strchr(line_start, '\n')) != NULL && i < 3) {
			*newline_pos = '\0'; // '\n'�� '\0'���� ��ü�Ͽ� �� �� ����
			// ������ ���� �Ľ��Ͽ� ���ڷ� ��ȯ
			char* endptr;
			val[i] = strtod(line_start, &endptr);
			if (endptr == line_start) {
				// ��ȯ ����
				printf("Invalid number format: %s\n", line_start);
				closesocket(new_fd);
				return 0;
			}
			printf("Received value[%d]: %lf\n", i, val[i]);
			i++;
			line_start = newline_pos + 1; // ���� �ٷ� �̵�
		}

		// ó������ ���� �����͸� �޽��� ������ �������� �̵�
		message_len = strlen(line_start);
		memmove(message, line_start, message_len + 1); // �� ���� �����Ͽ� �̵�
	}

	// ���� ����
	if (val[0] == 0 && val[2] == 1) res = 1234567891;
	else {
		if (val[2] == 1) val[0] = 1/val[0]; // ���� ó��
		res = val[1] * val[0];
	}


	// ��� ����
	char result_str[32];
	sprintf(result_str, "%.5lf\n", res); // '\n' �߰�
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

	SOCKET sockfd, new_fd;  /* ���� ���ϰ� ���ο� ���� ���� */
	struct sockaddr_in my_addr;    /* ���� �ּ� ���� */
	struct sockaddr_in their_addr; /* Ŭ���̾�Ʈ �ּ� ���� */
	int sin_size;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		perror("socket");
		WSACleanup();
		exit(1);
	}

	my_addr.sin_family = AF_INET;         /* ȣ��Ʈ ����Ʈ ���� */
	my_addr.sin_port = htons(MYPORT);     /* ��Ʈ��ũ ����Ʈ ������ ��Ʈ ��ȣ ���� */
	my_addr.sin_addr.s_addr = INADDR_ANY; /* �ڽ��� IP�� �ڵ� ���� */
	ZeroMemory(&(my_addr.sin_zero), 8);   /* ����ü�� ������ �κ��� 0���� �ʱ�ȭ */

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

	while (1) {  /* ���� accept() ���� */
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size)) == INVALID_SOCKET) {
			perror("accept");
			continue;
		}
		printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

		// �����带 �����Ͽ� Ŭ���̾�Ʈ ó��
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
