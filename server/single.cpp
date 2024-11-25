#include "gen.h"

std::ostream& log(std::ostream& out, int socket_fd, std::string msg, bool enter = 1)
{
	out << "[" << getCurrentTime() << "]" << "[fd: " << socket_fd << "]: " << msg;
	if (enter) out << std::endl;
	return out;
}

#define LOG(msg)  log(log_file, client_socket, msg)
#define LOGB(msg) log(log_file, client_socket, msg, 0)


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

	log(log_file, server_fd, "Сервер запущен на: ", 0) << getLocalIPAddress() << ":" << g_port << std::endl;

	while (true) {
		int client_socket = accept(server_fd, NULL, NULL);
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
