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
#include <iomanip>
#include <thread>
#include <mutex>

// std::string getCurrentTime() {
// 	auto now = std::chrono::system_clock::now();
// 	std::time_t time = std::chrono::system_clock::to_time_t(now);
// 	std::string timeStr = std::ctime(&time);
// 	timeStr.pop_back(); // Удаляем символ новой строки
// 	return timeStr;
// }

std::string getCurrentTime() {
	auto now = std::chrono::system_clock::now();
	std::time_t time = std::chrono::system_clock::to_time_t(now);
	std::tm tm_time = *std::localtime(&time);

	// Форматируем время в строку
	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(2) << tm_time.tm_hour << ":"
		<< std::setfill('0') << std::setw(2) << tm_time.tm_min << ":"
		<< std::setfill('0') << std::setw(2) << tm_time.tm_sec;

	return oss.str();
}

// Преобразование из wstring в utf8
std::string toUtf8(const std::wstring& wstr) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

// Преобразование из utf8 в wstring
std::wstring fromUtf8(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(str);
}

std::ostream& log(std::ostream& out, int socket_fd, std::string msg, std::mutex& mx, bool enter = 1)
{
	std::unique_lock<std::mutex> lock(mx);
	out << "[" << getCurrentTime() << "]" << "[fd: " << socket_fd << "]: " << msg;
	if (enter) out << std::endl;
	return out;
}

#define LOG(lock, msg)  log(log_file, client_socket, msg, lock)
#define LOGB(lock, msg) log(log_file, client_socket, msg, lock, 0)

int main() {
	const int PORT = 12345;
	const std::string author = "\"Плоцкий Б.А. М3О-411Б-21\"";
	std::ofstream log_file("server.log", std::ios::app);
	std::mutex log_file_mx;
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// настройка сокета
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	bind(server_fd, (struct sockaddr*)&address, sizeof(address));

	listen(server_fd, 3);

	log(log_file, 0, "Сервер запущен", log_file_mx);

	while (true) {
		int client_socket = accept(server_fd, NULL, NULL);
		std::thread th([client_socket, &log_file, &author, &log_file_mx]() {

			LOG(log_file_mx, "Клиент подключен");
			char buffer[1024] = { 0 };
			read(client_socket, buffer, 1024);

			// Преобразование полученной строки в формат wstring
			std::wstring message = fromUtf8(buffer);
			LOGB(log_file_mx, "Получено сообщение: ") << toUtf8(message) << std::endl;

			// Эмуляция работы сервера
			sleep(2);

			// Зеркальное отражение строки
			std::reverse(message.begin(), message.end());
			std::string response = toUtf8(message) + ". Сервер написал: " + author;
			send(client_socket, response.c_str(), response.length(), 0);
			LOGB(log_file_mx, "Отправлено сообщение: ") << response << std::endl;

			// Задержка перед отключением
			sleep(5);
			close(client_socket);
			LOG(log_file_mx, "Клиент отключен");
		});
		th.detach();
	}

	return 0;

}
