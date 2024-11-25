#include "gen.h"

std::ostream& log(std::ostream& out, std::string name, std::string msg, std::mutex& mx, bool enter = 1)
{
	std::unique_lock<std::mutex> lock(mx);
	out << "[" << getCurrentTime() << "]" << "[" << name << "]: " << msg;
	if (enter) out << std::endl;
	return out;
}

#define LOG(lock, msg)  log(log_file, name, msg, lock)
#define LOGB(lock, msg) log(log_file, name, msg, lock, 0)


int main() {
	std::ofstream log_file(g_log_file, std::ios::app);
	std::mutex log_file_mx;
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// настройка сокета
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(g_port);

	bind(server_fd, (sockaddr*)&address, sizeof(address));

	listen(server_fd, 3);

	auto serv_name = getLocalIPAddress() + ":" + std::to_string(g_port);
	log(log_file, serv_name, "Сервер запущен", log_file_mx);

	while (true) {
		// подключение клиента
		sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);
		int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

		// запуск работы клиента
		std::thread th([client_socket, &log_file, &log_file_mx, client_addr]()
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
				std::string response = toUtf8(message) + ". Сервер написал: " + g_author;
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
