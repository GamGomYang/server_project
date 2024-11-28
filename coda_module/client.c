// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // �������� ���
    while (1) {
        // �����κ��� �� ���� ����
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("%s", buffer); // �����κ��� ���� �޽��� ���

            // �ڽ��� ���� ���� �Է� �ޱ�
            if (strstr(buffer, "Your turn") != NULL) {
                char input[1024];
                printf("Enter your move (index color number): ");
                fgets(input, 1024, stdin);
                send(sock, input, strlen(input), 0);
            }

            // ���� ���� ���� üũ
            if (strstr(buffer, "Game Over") != NULL) {
                break;
            }
        }
    }

    close(sock);
    return 0;
}