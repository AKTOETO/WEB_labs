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


constexpr int g_port = 12345;
constexpr const char* g_author = "\"Плоцкий Б.А. М3О-411Б-21\"";
constexpr const char* g_log_file = "server.log";

inline std::string getCurrentTime() {
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
inline std::string toUtf8(const std::wstring& wstr) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

// Преобразование из utf8 в wstring
inline std::wstring fromUtf8(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(str);
}

inline std::string getLocalIPAddress() {
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