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
#include <functional>
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
using pSocket = std::shared_ptr<tcp::socket>;
using Req = http::request<http::string_body>;
using pReq = std::shared_ptr<http::request<http::string_body>>;
using Res = http::response<http::string_body>;

// Структура для описания задачи
struct Task {
	std::string id;
	std::string title;
	std::string description;
	std::string status;
	std::string created_by;
	std::vector<std::string> comments;
};

// Структура для описания пользователя
struct User {
	std::string username;
	std::string password_hash;
	std::string token;
};

//------------------------------------------------------------------------------

// Класс для управления аутентификацией
class AuthManager {
private:
	std::map<std::string, User> users; // Хранит пользователей
	std::map<std::string, std::string> tokens; // Хранит токены

public:
	AuthManager() {
		// Добавление тестового пользователя
		users["test_user"] = { "test_user", "hashed_password", "" };
		std::cout << "Тестовый пользователь добавлен: test_user" << std::endl; // Вывод сообщения о пользователе
	}

	std::string authenticate(const std::string& username, const std::string& password) {
		std::cout << "Попытка аутентификации: " << username << std::endl; // Вывод имени пользователя
		if (users.find(username) != users.end() && users[username].password_hash == hash_password(password)) {
			auto token = generate_token(); // Генерация токена
			users[username].token = token; // Сохранение токена для пользователя
			tokens[token] = username; // Сохранение ассоциации токена с пользователем
			std::cout << "Аутентификация успешна. Токен: " << token << std::endl; // Успех аутентификации
			return token;
		}
		std::cout << "Ошибка аутентификации: неверный логин или пароль." << std::endl; // Вывод сообщения об ошибке
		return "";
	}

	bool validate_token(const std::string& token) {
		return tokens.find(token) != tokens.end(); // Проверка на существование токена
	}

private:
	std::string generate_token() {
		boost::uuids::random_generator gen;
		return boost::uuids::to_string(gen()); // Генерация уникального токена
	}

	std::string hash_password(const std::string& password) {
		return password; // Упрощение. В реальном приложении используйте хеш-функцию.
	}
};

//------------------------------------------------------------------------------

// Класс для управления задачами
class TaskManager {
private:
	std::map<std::string, Task> tasks; // Хранит задачи
	AuthManager& auth_manager; // Ссылка на менеджер аутентификации

public:
	TaskManager(AuthManager& auth) : auth_manager(auth) {}

	// Метод для создания новой задачи
	Task create_task(const std::string& token, const Task& task) {
		if (!auth_manager.validate_token(token)) {
			throw std::runtime_error("Unauthorized"); // Ошибка, если токен недействителен
		}
		auto task_id = generate_uuid(); // Генерация уникального идентификатора задачи
		tasks[task_id] = task; // Сохранение задачи по ID
		tasks[task_id].id = task_id; // Присвоение ID
		std::cout << "Создана задача с ID: " << task_id << std::endl; // Вывод сообщения о создании задачи
		return tasks[task_id];
	}

	// Метод для получения конкретной задачи
	Task get_task(const std::string& token, const std::string& task_id) {
		if (!auth_manager.validate_token(token)) {
			throw std::runtime_error("Unauthorized"); // Ошибка, если токен недействителен
		}
		std::cout << "Получение задачи с ID: " << task_id << std::endl; // Вывод сообщения о получении задачи
		return tasks.at(task_id); // Возвращение задачи
	}

	// Метод для получения списка задач
	std::vector<Task> get_tasks(const std::string& token, const std::string& status = "", const std::string& sort_by = "") {
		if (!auth_manager.validate_token(token)) {
			throw std::runtime_error("Unauthorized"); // Ошибка, если токен недействителен
		}

		std::vector<Task> result;
		for (const auto& pair : tasks) {
			if (status.empty() || pair.second.status == status) {
				result.push_back(pair.second); // Добавление задачи в результирующий массив
			}
		}
		std::cout << "Количество задач найдено: " << result.size() << std::endl; // Вывод количества найденных задач
		sort_tasks(result, sort_by); // Сортировка задач
		return result; // Возвращение списка задач
	}

private:
	void sort_tasks(std::vector<Task>& tasks, const std::string& sort_by) {
		if (sort_by == "title") {
			std::sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) { return a.title < b.title; }); // Сортировка по заголовку
			std::cout << "Задачи отсортированы по заголовку." << std::endl; // Вывод сообщения о сортировке
		}
	}

	std::string generate_uuid() {
		boost::uuids::random_generator generator;
		return boost::uuids::to_string(generator()); // Генерация уникального идентификатора
	}
};

//------------------------------------------------------------------------------

// Класс для экспорта данных
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
			tasks_array.push_back(std::make_pair("", task_node)); // Формирование JSON
		}
		root.add_child("tasks", tasks_array); // Добавление задач в корень
		std::stringstream ss;
		boost::property_tree::write_json(ss, root); // Запись в строку
		return ss.str(); // Возвращение JSON-строки
	}

	static std::string export_to_csv(const std::vector<Task>& tasks) {
		std::stringstream ss;
		ss << "id,title,description,status\n"; // Заголовки CSV
		for (const auto& task : tasks) {
			ss << task.id << "," << task.title << "," << task.description << "," << task.status << "\n"; // Формирование CSV
		}
		return ss.str(); // Возвращение CSV-строки
	}
};

//------------------------------------------------------------------------------
// Сессия, описывающая обработку запроса клиента на сервере
class Session : public std::enable_shared_from_this<Session>
{
private:
	beast::tcp_stream stream_;
	beast::flat_buffer buffer_;
	http::request<http::string_body> req_;
	std::function<Res(Req&&)> m_handler;

public:
	Session(tcp::socket&& socket, std::function<Res(Req&&)> handler
		= [](http::request<http::string_body>&&) {return Res();})
		:stream_(std::move(socket)), m_handler(handler)
	{
		std::cout << "Создание сессии\n";
	}

	// установить обработчик запроса
	void setHandler(std::function<Res(Req&&)> handler)
	{
		m_handler = handler;
	}

	// Запуск сессии
	void run()
	{
		doRead();
	}

	void doRead()
	{
		req_ = {};
		http::async_read(stream_, buffer_, req_,
			beast::bind_front_handler(&Session::onRead, shared_from_this())
		);
	}

	// обработка чтения
	void onRead(beast::error_code ec, std::size_t)
	{
		if (ec) {
			if (ec == boost::asio::error::operation_aborted) {
				std::cerr << "Операция чтения отменена." << std::endl;
			}
			else {
				std::cerr << "Ошибка при чтении запроса: " << ec.message() << std::endl;
			}
			// завершение сессии
			doClose();
			return;
		}
		std::cout << "Получен запрос: " << req_.method_string()
			<< " " << req_.target() << std::endl; // Вывод информации о запросе

		// обрабатываем запрос и пишем ответ
		doWrite(handleSession());
	}

	// пишем ответ 
	void doWrite(Res&& res)
	{
		bool keep = res.keep_alive();
		auto el = shared_from_this();
		http::async_write(stream_, std::move(res),
			//beast::bind_front_handler(&Session::onWrite, shared_from_this(), keep)
			[el, keep](beast::error_code ec, std::size_t len)
			{
				el->onWrite(ec, len, keep);
			}
		);
	}

	void onWrite(beast::error_code ec, std::size_t, bool keep_alive)
	{
		// если нужно закрыть соединение - закрываем
		if (!keep_alive)
		{
			return doClose();
		}

		// иначе продолжаем читать
		doRead();
	}

	// завершение работы сессии
	void doClose()
	{
		beast::error_code ec;
		std::cout<<"Уничтожение сессии\n";
		stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
	}

	// обработка запроса
	Res handleSession()
	{
		return m_handler(std::move(req_));
	}

};

// Класс слушающий входящие подключения
class Listener : public std::enable_shared_from_this<Listener>
{
	net::io_context& ioc_;
	tcp::acceptor acceptor_;
	std::function<void(std::shared_ptr<Session>)> m_sessions;

public:
	Listener(net::io_context& ioc,
		tcp::endpoint endpoint,
		std::function<void(std::shared_ptr<Session>)> sessions
		= [](std::shared_ptr<Session>) {})
		:ioc_(ioc), acceptor_(ioc, endpoint), m_sessions(sessions)
	{
		std::cout << "Listen on: " << endpoint.address().to_string()
			<< ":" << endpoint.port() << std::endl;

		// делаем возможным перезапуск listen н атом же порту
		beast::error_code ec;
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if (ec)
		{
			std::cerr << "Ошибка при bind к порту: " << ec.message() << std::endl;
			return;
		}
	}

	// запуск прослушки
	void run()
	{
		doAccept();
	}

	// установить колбэк для получения сессий
	void setSessionGetter(std::function<void(std::shared_ptr<Session>)> getter)
	{
		m_sessions = getter;
	}

private:
	// начинаем принимать соединения
	void doAccept()
	{
		acceptor_.async_accept(
			beast::bind_front_handler(
				&Listener::onAccept,
				shared_from_this())
		);
	}

	void onAccept(beast::error_code ec, tcp::socket socket)
	{
		// если нет ошибок - запускаем новую сессию
		if (ec)
		{
			std::cerr << "Ошибка при подключении клиента: " << ec.message() << std::endl;
			return;
		}

		auto sess = std::make_shared<Session>(std::move(socket));
		m_sessions(sess);
		sess->run();

		// продолжаем принимать клиентов
		doAccept();
	}

};

// Класс для HTTP-сервера
class HttpServer {
private:
	net::io_context& ioc_;
	TaskManager& task_manager; // Ссылка на менеджер задач
	AuthManager& auth_manager; // Ссылка на менеджер аутентификации

public:
	HttpServer(net::io_context& ioc, unsigned short port, TaskManager& tm, AuthManager& am)
		: ioc_(ioc), task_manager(tm), auth_manager(am)
	{
		auto listener = std::make_shared<Listener>(ioc, tcp::endpoint{ tcp::v4(), port });
		listener->setSessionGetter([this](std::shared_ptr<Session> sess) {setupSession(sess);});
		listener->run();		
	}

private:

	void setupSession(std::shared_ptr<Session> session)
	{
		session->setHandler([this](Req&& req) {return processLogic(std::move(req));});
	}

	Res processLogic(Req&& req)
	{
		http::response<http::string_body> response; // Ответ на запрос

		// Обработка запроса аутентификации
		if (req.method() == http::verb::post && req.target() == "/api/auth")
		{
			std::string username = req.body(); // Упрощение для примера
			std::string token = auth_manager.authenticate(username, "password"); // Аутентификация пользователя с паролем
			response.result(token.empty() ? http::status::unauthorized : http::status::ok); // Установка кода ответа
			response.body() = token.empty() ? "Unauthorized" : token; // Установка тела ответа
			std::cout << "Аутентификация завершена. Код ответа: " << response.result() << std::endl; // Вывод информации о результате аутентификации
		}
		// Обработка запроса на получение задач
		else if (req.method() == http::verb::get && req.target() == "/api/tasks")
		{
			std::string token = "extracted_token"; // Здесь должна быть логика для извлечения токена
			auto tasks = task_manager.get_tasks(token); // Получение задач
			response.result(http::status::ok);
			response.body() = DataExporter::export_to_json(tasks); // Экспорт задач в JSON
			std::cout << "Получены задачи. Количество задач: " << tasks.size() << std::endl; // Вывод количества задач
		}
		else
		{
			response.result(http::status::method_not_allowed);
			response.set(http::field::allow, "POST, GET"); // Указать поддерживаемые методы
			response.body() = "Method Not Allowed";
		}

		//response.prepare_payload(); // Подготовка ответа
		return response;
	}

};

// Главная функция
int main() {
	try {
		net::io_context ioc; // Создание контекста ввода-вывода
		AuthManager auth_manager; // Создание менеджера аутентификации
		TaskManager task_manager(auth_manager); // Создание менеджера задач
		HttpServer server(ioc, 8080, task_manager, auth_manager); // Создание сервера
		ioc.run(); // Запуск сервера
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl; // Обработка ошибок
	}
	return 0; // Завершение программы
}