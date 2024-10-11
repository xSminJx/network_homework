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

// ���� Ŀ��
void preprocess(char* expression) {
	// ���� ���Ÿ� ���� �ӽ� ����
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

// ����
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

// ���� ����
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


// ���� ǥ��� -> ���� ǥ��� (��Ģ ���꿡 �ʿ�)
void infixToPostfix(const char* infix, char* postfix) {
	CharStack stack;
	initCharStack(&stack);
	int j = 0;
	for (int i = 0; infix[i] != '\0'; i++) {
		char token = infix[i];

		if (isdigit(token)) {
			// ������ ���
			postfix[j++] = token;
			// ���� ���ڰ� ���ڰ� �ƴϸ� ���� �߰� (���ڸ��� ����)
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
			iserror = 1; // ������ ���� ����
			return;
		}
	}

	// ���ÿ� ���� ������ ó��(���� ����)
	while (!isCharStackEmpty(&stack)) {
		postfix[j++] = popChar(&stack);
		postfix[j++] = ' ';
	}
	postfix[j] = '\0';
}

// ���� ���� ǥ����� ����ϰ� ���� ������ ��ŵ� ��
double evaluatePostfix(const char* postfix) {
	DoubleStack stack;
	initDoubleStack(&stack);
	int i = 0;
	while (postfix[i] != '\0') {
		if (isdigit(postfix[i]) || postfix[i] == '.') {
			// ���� ����
			char number[32];
			int j = 0;
			while (isdigit(postfix[i]) || postfix[i] == '.') {
				number[j++] = postfix[i++];
			}
			number[j] = '\0';
			pushDouble(&stack, atof(number));
		}
		else if (postfix[i] == '+' || postfix[i] == '-' || postfix[i] == '*' || postfix[i] == '/') {
			// ������ ó��
			double val1 = popDouble(&stack);
			double val2 = popDouble(&stack);
			double result = 0;

			// ������ ������ ������ ����
			char send_data[64];
			int operation_flag = 0; // ����: 0, ����: 1
			int server_port = 0;

			if (postfix[i] == '+') {
				operation_flag = 0;
				server_port = 10000; // ���� ���� ��Ʈ
			}
			else if (postfix[i] == '-') {
				operation_flag = 1;
				server_port = 10000; // ������ ���� �������� ó��
			}
			else if (postfix[i] == '*') {
				operation_flag = 0;
				server_port = 10001; // ���� ���� ��Ʈ
			}
			else if (postfix[i] == '/') {
				operation_flag = 1;
				server_port = 10001; // �������� ���� �������� ó��
			}

			sprintf(send_data, "%.5lf\n%.5lf\n%d\n", val1,val2,operation_flag);

			// ������ ����
			SOCKET op_sock = socket(AF_INET, SOCK_STREAM, 0);
			if (op_sock == INVALID_SOCKET) {
				printf("���� ���� ����: %d\n", WSAGetLastError());
				// ���� ó��: ���� ���� ���� �� �Լ� ����
				return 0; // ������ ���� ���� ��ȯ
			}

			struct sockaddr_in op_addr;
			op_addr.sin_family = AF_INET;
			op_addr.sin_port = htons(server_port);
			op_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // ���� ȣ��Ʈ IP �ּ�
			ZeroMemory(&(op_addr.sin_zero), 8);

			if (connect(op_sock, (struct sockaddr*)&op_addr, sizeof(op_addr)) == SOCKET_ERROR) {
				printf("���� ������ ���� ����: %d\n", WSAGetLastError());
				closesocket(op_sock);
				return 0;
			}

			// ������ ������ ����
			int send_result = send(op_sock, send_data, strlen(send_data), 0);
			if (send_result == SOCKET_ERROR) {
				printf("������ ���� ����: %d\n", WSAGetLastError());
				closesocket(op_sock);
				return 0;
			}

			// �����κ��� ��� ����
			char recv_data[64];
			int recv_bytes = recv(op_sock, recv_data, sizeof(recv_data) - 1, 0);
			if (recv_bytes > 0) {
				recv_data[recv_bytes] = '\0';
				result = atof(recv_data);
			}
			else {
				printf("�����κ��� ������ ���� ����: %d\n", WSAGetLastError());
				closesocket(op_sock);
				// ���� ó��
				return 0;
			}
			if (result == 1234567891.00000) {
				return result;
			}

			closesocket(op_sock);

			// ����� ���ÿ� Ǫ��
			pushDouble(&stack, result);
			i++;
		}
		else {
			i++; // ���� �� ����
		}
	}
	double result = popDouble(&stack);
	return result;
}

// Ŭ���̾�Ʈ�� ����ϴ� ����
unsigned __stdcall client_handler(void* arg) {
	SOCKET new_fd = (SOCKET)arg;
	char buffer[BUFFER_SIZE];
	char message[BUFFER_SIZE]; // ���� �Է� ����
	char postfix[BUFFER_SIZE]; // ���� ǥ���
	int buf_bytes, message_len = 0;
	double res = 0;

	const char* connect_msg = "������ ���� ����. ���� �Է�: ";

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
			message[message_len - 1] = '\0'; // '\n'�� '\0'���� ��ü
			printf("���ŵ� ����: %s\n", message);

			// ���� ó��
			preprocess(message);
			infixToPostfix(message, postfix);

			if (iserror) {
				const char* errormsg = "���� ���� �߻�";
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
					const char* errormsg = "0���� ������ ����";
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