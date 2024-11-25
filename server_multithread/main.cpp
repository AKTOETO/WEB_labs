#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
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
#include <ifaddrs.h>

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

std::ostream& log(std::ostream& out, std::string name, std::string msg, std::mutex& mx, bool enter = 1)
{
	std::unique_lock<std::mutex> lock(mx);
	out << "[" << getCurrentTime() << "]" << "[" << name << "]: " << msg;
	if (enter) out << std::endl;
	return out;
}

#define LOG(lock, msg)  log(log_file, name, msg, lock)
#define LOGB(lock, msg) log(log_file, name, msg, lock, 0)

std::string getLocalIPAddress() {
	struct ifaddrs* interfaces = nullptr;
	struct ifaddrs* ifa = nullptr;
	std::string ipAddress;

	// Получаем список интерфейсов
	if (getifaddrs(&interfaces) == -1) {
		perror("getifaddrs");
		return ipAddress;
	}

	// Проходим по всем интерфейсам
	for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
		// Проверяем, является ли интерфейс IPv4
		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
			char addressBuffer[INET_ADDRSTRLEN];
			// Получаем IP-адрес
			inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, addressBuffer, sizeof(addressBuffer));
			// Проверяем, что это не loopback адрес
			if (strcmp(ifa->ifa_name, "lo") != 0) { // исключаем loopback интерфейс
				ipAddress = addressBuffer; // сохраняем первый найденный IP-адрес
				break; // выходим из цикла после нахождения первого подходящего адреса
			}
		}
	}

	// Освобождаем память
	freeifaddrs(interfaces);

	return ipAddress;
}

int main() {
	const int port = 12345;
	const std::string author = "\"Плоцкий Б.А. М3О-411Б-21\"";
	std::ofstream log_file("server.log", std::ios::app);
	std::mutex log_file_mx;
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// настройка сокета
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	bind(server_fd, (sockaddr*)&address, sizeof(address));

	listen(server_fd, 3);

	auto serv_name = getLocalIPAddress() + ":" + std::to_string(port);
	log(log_file, serv_name, "Сервер запущен", log_file_mx);

	while (true) {
		// подключение клиента
		sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);
		int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

		// запуск работы клиента
		std::thread th([client_socket, &log_file, &author, &log_file_mx, client_addr]()
			{
				// Получаем IP и порт клиента
				char ip_str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));

				unsigned int port = ntohs(client_addr.sin_port);
				std::string name{ ip_str };
				name += ":" + std::to_string(port);

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
