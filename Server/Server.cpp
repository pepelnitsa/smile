#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <string>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#define PAUSE 1

SOCKET ClientSocket = INVALID_SOCKET;

// Функція для переміщення курсора в консолі
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Функція для відображення смайлика
void drawSmiley(int x, int y) {
    gotoxy(x, y);
    cout << "+";
}

// Функція для стирання смайлика з поточних координат
void eraseSmiley(int x, int y) {
    gotoxy(x, y);
    cout << " ";
}

DWORD WINAPI Receiver(void* param)
{
    while (true)
    {
        char directionBuffer[DEFAULT_BUFLEN];
        int iResult = recv(ClientSocket, directionBuffer, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            directionBuffer[iResult] = '\0';
            int direction = atoi(directionBuffer);

            // Посилання напрямку руху до клієнта
            int iSendResult = send(ClientSocket, directionBuffer, iResult, 0);
            if (iSendResult == SOCKET_ERROR) {
                cout << "send failed with error: " << WSAGetLastError() << "\n";
                closesocket(ClientSocket);
                WSACleanup();
                return 15;
            }

            // Очищаємо попереднє положення смайлика
            eraseSmiley(5, 5);

            // Обробка отриманого напрямку руху та переміщення смайлика
            switch (direction) {
            case 1: // Вліво
                if (5 > 0)
                    drawSmiley(5--, 5);
                break;
            case 2: // Вправо
                if (5 < 15)
                    drawSmiley(5++, 5);
                break;
            case 3: // Вгору
                if (5 > 0)
                    drawSmiley(5, 5--);
                break;
            case 4: // Вниз
                if (5 < 15)
                    drawSmiley(5, 5++);
                break;
            }
        }
        else if (iResult == 0)
            cout << "Соединение с клиентом закрыто.\n";
        else
            cout << "recv failed with error: " << WSAGetLastError() << "\n";
    }
    return 0;
}


int main()
{
    setlocale(0, "");
    system("title SERVER SIDE");

    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup failed with error: " << iResult << "\n";
        return 1;
    }

    // Set up the server
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* result = NULL;
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo failed with error: " << iResult << "\n";
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections
    SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        cout << "socket failed with error: " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind the server socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cout << "bind failed with error: " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Listen for client connections
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "accept failed with error: " << WSAGetLastError() << "\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    closesocket(ListenSocket);

    // Create a thread for receiving data from the client
    HANDLE hReceiverThread = CreateThread(NULL, 0, Receiver, NULL, 0, NULL);
    if (hReceiverThread == NULL) {
        cout << "Failed to create receiver thread.\n";
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    WaitForSingleObject(hReceiverThread, INFINITE);

    // Clean up
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}
