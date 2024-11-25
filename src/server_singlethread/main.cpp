#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <ctime>
#include <locale>
#include <codecvt>

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::string timeStr = std::ctime(&time);
    timeStr.pop_back(); // Удаляем символ новой строки
    return timeStr;
}

std::string to_utf8(const std::wstring &wstr) {
    // Преобразование из wstring в utf8
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

std::wstring from_utf8(const std::string &str) {

    // Преобразование из utf8 в wstring

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    return converter.from_bytes(str);

}

int main() {

    const int PORT = 12345;

    const std::string AUTHOR = "Плоцкий Б.А. М3О-411Б-21";

    std::ofstream logFile("server.log", std::ios::app);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;

    address.sin_family = AF_INET;

    address.sin_addr.s_addr = INADDR_ANY;

    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 3);

    logFile << "Сервер запущен: " << getCurrentTime() << std::endl;

    while (true) {

        int client_socket = accept(server_fd, NULL, NULL);

        logFile << "Клиент подключен: " << getCurrentTime() << std::endl;

        char buffer[1024] = {0};

        read(client_socket, buffer, 1024);

        

        // Преобразование полученной строки в формат wstring

        std::wstring message = from_utf8(buffer);

        logFile << "Получено сообщение: " << getCurrentTime() << " - " << to_utf8(message) << std::endl;

        // Эмуляция работы сервера

        sleep(2);

        // Зеркальное отражение строки

        std::reverse(message.begin(), message.end());

        std::string response = to_utf8(message) + ". Сервер написан " + AUTHOR;

        send(client_socket, response.c_str(), response.length(), 0);

        logFile << "Отправлено сообщение: " << getCurrentTime() << " - " << response << std::endl;

        // Задержка перед отключением

        sleep(5);

        close(client_socket);

        logFile << "Клиент отключен: " << getCurrentTime() << std::endl;

    }

    return 0;

}