#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <conio.h>
#include <string>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#define PAUSE 1

// Attempt to connect to an address until one succeeds
SOCKET ConnectSocket = INVALID_SOCKET;

int direction = 0; // "0"

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

DWORD WINAPI Sender(void* param)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD position;
    position.X = 5;
    position.Y = 5;
    SetConsoleCursorPosition(h, position);
    cout << "+";

    CONSOLE_CURSOR_INFO cursor;
    cursor.bVisible = false;
    cursor.dwSize = 100;
    SetConsoleCursorInfo(h, &cursor);

    while (true)
    {
        int code = _getch();
        if (code == 224) code = _getch(); // если первый код 224 - то это стрелка (но пока не понятно, какая), нужно вызвать функцию ГЕЧ ещё раз для получения нормального кода 

        // стирание смайлика в его "старых" координатах
        eraseSmiley(position.X, position.Y);

        if (code == 75 && position.X > 0) {
            position.X--;
            direction = 1;
        }
        else if (code == 77 && position.X < 15) {
            position.X++;
            direction = 2;
        }
        else if (code == 72 && position.Y > 0) {
            position.Y--;
            direction = 3;
        }
        else if (code == 80 && position.Y < 15) {
            position.Y++;
            direction = 4;
        }

        // отрисова смайлика в его "новых" координатах
        drawSmiley(position.X, position.Y);

        char message[200];
        strcpy_s(message, 199, to_string(direction).c_str());

        int iResult = send(ConnectSocket, message, (int)strlen(message), 0);
        if (iResult == SOCKET_ERROR) {
            cout << "send failed with error: " << WSAGetLastError() << "\n";
            closesocket(ConnectSocket);
            WSACleanup();
            return 15;
        }
    }

    return 0;
}

DWORD WINAPI Receiver(void* param)
{
    while (true)
    {
        char answer[DEFAULT_BUFLEN];

        int iResult = recv(ConnectSocket, answer, DEFAULT_BUFLEN, 0);
        answer[iResult] = '\0';

        if (iResult > 0) {
            cout << answer << "\n";
        }
        else if (iResult == 0)
            cout << "соединение с сервером закрыто.\n";
        else
            cout << "recv failed with error: " << WSAGetLastError() << "\n";
    }
    return 0;
}

int main()
{
    setlocale(0, "");
    system("title CLIENT SIDE");
    system("mode con cols=50 lines=15");

    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup failed with error: " << iResult << "\n";
        return 11;
    }

    // Resolve the server address and port
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const char* ip = "localhost"; // по умолчанию, оба приложения, и клиент, и сервер, запускаются на одной и той же машине
    addrinfo* result = NULL;
    iResult = getaddrinfo(ip, DEFAULT_PORT, &hints, &result);

    if (iResult != 0) {
        cout << "getaddrinfo failed with error: " << iResult << "\n";
        WSACleanup();
        return 12;
    }
    else {
        Sleep(PAUSE);
    }

    for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (ConnectSocket == INVALID_SOCKET) {
            cout << "socket failed with error: " << WSAGetLastError() << "\n";
            WSACleanup();
            return 13;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        cout << "невозможно подключиться к серверу!\n";
        WSACleanup();
        return 14;
    }

    CreateThread(0, 0, Sender, 0, 0, 0);

    Sleep(INFINITE);
}
