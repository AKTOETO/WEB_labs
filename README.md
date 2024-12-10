# Лабораторная работа 21.
## Сервис с Rest API. 
1. Создать простой REST API сервис для управления списком задач (To-Do list). Реализовать методы для создания, чтения, обновления и удаления задач.
2. Добавить функционал аутентификации пользователей в REST API сервисе. Пользователь должен получить токен доступа для выполнения операций с задачами.
3. Реализовать возможность фильтрации и сортировки задач в REST API сервисе. Добавить параметры запроса для выборки задач по определенным критериям.
4. Создать метод для экспорта данных в различные форматы (например, JSON, CSV) в REST API сервисе. Пользователь может получить данные в нужном формате для дальнейшей обработки.
5. Добавить возможность комментирования задач в REST API сервисе. Пользователь может оставлять комментарии к задачам и видеть комментарии других пользователей.


#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

struct Task {
	std::string id;
	std::string title;
	std::string description;
	std::string status;
	std::string created_by;
	std::vector<std::string> comments;
};

struct User {
	std::string username;
	std::string password_hash;
	std::string token;
};

class AuthManager {
private:
	std::map<std::string, User> users;
	std::map<std::string, std::string> tokens;

public:
	AuthManager() {
		// Добавление тестового пользователя
		users["test_user"] = { "test_user", "hashed_password", "" };
	}

	std::string authenticate(const std::string& username, const std::string& password) {
		if (users.find(username) != users.end() && users[username].password_hash == hash_password(password)) {
			auto token = generate_token();
			users[username].token = token;
			tokens[token] = username;
			return token;
		}
		return "";
	}

	bool validate_token(const std::string& token) {
		return tokens.find(token) != tokens.end();
	}

private:
	std::string generate_token() {
		boost::uuids::random_generator gen;
		return boost::uuids::to_string(gen());
	}

	std::string hash_password(const std::string& password) {
		return password; // Упрощенно
	}
};

class TaskManager {
private:
	std::map<std::string, Task> tasks;
	AuthManager& auth_manager;

public:
	TaskManager(AuthManager& auth) : auth_manager(auth) {}

	Task create_task(const std::string& token, const Task& task) {
		if (!auth_manager.validate_token(token)) {
			throw std::runtime_error("Unauthorized");
		}
		auto task_id = generate_uuid();
		tasks[task_id] = task;
		tasks[task_id].id = task_id;
		return tasks[task_id];
	}

	Task get_task(const std::string& token, const std::string& task_id) {
		if (!auth_manager.validate_token(token)) {
			throw std::runtime_error("Unauthorized");
		}
		return tasks.at(task_id);
	}

	std::vector<Task> get_tasks(const std::string& token, const std::string& status = "", const std::string& sort_by = "") {
		if (!auth_manager.validate_token(token)) {
			throw std::runtime_error("Unauthorized");
		}

		std::vector<Task> result;
		for (const auto& pair : tasks) {
			if (status.empty() || pair.second.status == status) {
				result.push_back(pair.second);
			}
		}
		sort_tasks(result, sort_by);
		return result;
	}

private:
	void sort_tasks(std::vector<Task>& tasks, const std::string& sort_by) {
		if (sort_by == "title") {
			std::sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) { return a.title < b.title; });
		}
	}

	std::string generate_uuid() {
		boost::uuids::random_generator generator;
		return boost::uuids::to_string(generator());
	}
};

class DataExporter {
public:
	static std::string export_to_json(const std::vector<Task>& tasks) {
		boost::property_tree::ptree root;
		boost::property_tree::ptree tasks_array;

		for (const auto& task : tasks) {
			boost::property_tree::ptree task_node;
			task_node.put("id", task.id);
			task_node.put("title", task.title);
			task_node.put("description", task.description);
			task_node.put("status", task.status);
			tasks_array.push_back(std::make_pair("", task_node));
		}
		root.add_child("tasks", tasks_array);
		std::stringstream ss;
		boost::property_tree::write_json(ss, root);
		return ss.str();
	}

	static std::string export_to_csv(const std::vector<Task>& tasks) {
		std::stringstream ss;
		ss << "id,title,description,status\n";
		for (const auto& task : tasks) {
			ss << task.id << "," << task.title << "," << task.description << "," << task.status << "\n";
		}
		return ss.str();
	}
};

class HttpServer {
private:
	net::io_context& ioc;
	tcp::acceptor acceptor;
	TaskManager& task_manager;
	AuthManager& auth_manager;

public:
	HttpServer(net::io_context& ioc, unsigned short port, TaskManager& tm, AuthManager& am)
		: ioc(ioc), acceptor(ioc, { tcp::v4(), port }), task_manager(tm), auth_manager(am) {
		accept();
	}

private:
	void accept() {
		acceptor.async_accept(
			[this](beast::error_code ec, tcp::socket socket) {
				if (!ec) {
					process_request(std::move(socket));
				}
				accept();
			});
	}

	void process_request(tcp::socket socket) {
		auto buffer = std::make_shared<beast::flat_buffer>();
		auto request = std::make_shared<http::request<http::string_body>>();

		http::async_read(socket, *buffer, *request,
			[this, request, socket = std::move(socket)](beast::error_code ec, std::size_t) mutable {
				if (ec) return;
				handle_request(socket, *request);
			});
	}

	void handle_request(tcp::socket& socket, const http::request<http::string_body>& request) {
		http::response<http::string_body> response;

		if (request.method() == http::verb::post && request.target() == "/api/auth") {
			std::string username = request.body(); // Упрощение для примера
			std::string token = auth_manager.authenticate(username, "password");
			response.result(token.empty() ? http::status::unauthorized : http::status::ok);
			response.body() = token.empty() ? "Unauthorized" : token;
		}
		else if (request.method() == http::verb::get && request.target() == "/api/tasks") {
			std::string token = "extracted_token"; // Обработка токена
			auto tasks = task_manager.get_tasks(token);
			response.result(http::status::ok);
			response.body() = DataExporter::export_to_json(tasks);
		}
		else {
			response.result(http::status::not_found);
			response.body() = "Not Found";
		}

		response.prepare_payload();
		http::async_write(socket, response,
			[this, &socket](beast::error_code ec, std::size_t) {
				socket.shutdown(tcp::socket::shutdown_send, ec);
			});
	}
};

int main() {
	try {
		net::io_context ioc;
		AuthManager auth_manager;
		TaskManager task_manager(auth_manager);
		HttpServer server(ioc, 8080, task_manager, auth_manager);
		ioc.run();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	return 0;
}