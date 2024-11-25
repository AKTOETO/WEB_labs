#include "gen.h"

std::ostream& log(std::ostream& out, std::string name, std::string msg, bool enter = 1)
{
	out << "[" << getCurrentTime() << "]" << "[" << name << "]: " << msg;
	if (enter) out << std::endl;
	return out;
}

#define LOG(msg)  log(log_file, name, msg)
#define LOGB(msg) log(log_file, name, msg, 0)


int main() {
	std::ofstream log_file(g_log_file, std::ios::app);
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// настройка сокета
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(g_port);

	bind(server_fd, (struct sockaddr*)&address, sizeof(address));

	listen(server_fd, 3);

	auto serv_name = getLocalIPAddress() + ":" + std::to_string(g_port);
	log(log_file, serv_name, "Сервер запущен");

	while (true) {
		sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);
		int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

		// Получаем IP и порт клиента
		char ip_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));

		unsigned int port = ntohs(client_addr.sin_port);
		std::string name{ ip_str };
		name += ":" + std::to_string(port);

		LOG("Клиент подключен");
		char buffer[1024] = { 0 };
		read(client_socket, buffer, 1024);

		// Преобразование полученной строки в формат wstring
		std::wstring message = fromUtf8(buffer);
		LOGB("Получено сообщение: ") << toUtf8(message) << std::endl;

		// Эмуляция работы сервера
		sleep(2);

		// Зеркальное отражение строки
		std::reverse(message.begin(), message.end());
		std::string response = toUtf8(message) + ". Сервер написал: " + g_author;
		send(client_socket, response.c_str(), response.length(), 0);
		LOGB("Отправлено сообщение: ") << response << std::endl;

		// Задержка перед отключением
		sleep(5);
		close(client_socket);
		LOG("Клиент отключен");
	}

	return 0;

}
