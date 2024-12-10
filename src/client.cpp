#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

using Req = http::request<http::string_body>;
using pReq = std::shared_ptr<Req>;

using Res = http::response<http::string_body>;
using pRes = std::shared_ptr<Res>;
class RestClient {
public:
	RestClient(const std::string& host, const std::string& port)
		: host_(host), port_(port), resolver_(io_context_), socket_(io_context_) {
	}

	void authenticate(const std::string& username, const std::string& password) {
		std::string auth_endpoint = "/api/auth";
		boost::json::object auth_data = {
			{"username", username},
			{"password", password}
		};

		pReq req = std::make_shared<Req>(http::verb::post, auth_endpoint, 11);
		req->set(http::field::host, host_);
		req->set(http::field::content_type, "application/json");
		req->body() = boost::json::serialize(auth_data);
		req->prepare_payload();

		try {
			send_request(req);
		}
		catch (const std::exception& e) {
			std::cerr << "Ошибка при аутентификации: " << e.what() << std::endl;
		}
	}

	void get_tasks(const std::string& token) {
		std::string tasks_endpoint = "/api/tasks";
		pReq req = std::make_shared<Req>(http::verb::get, tasks_endpoint, 11);
		req->set(http::field::host, host_);
		req->set(http::field::authorization, "Bearer " + token);
		req->prepare_payload();

		try {
			send_request(req);
		}
		catch (const std::exception& e) {
			std::cerr << "Ошибка при получении задач: " << e.what() << std::endl;
		}
	}

private:
	void send_request(pReq req) {
		try {
			std::cout << "Разрешение хоста: " << host_ << ", порт: " << port_ << std::endl;

			// Resolve the host
			resolver_.async_resolve(host_, port_,
				[this, req](beast::error_code ec, tcp::resolver::results_type results) {
					if (ec) {
						std::cerr << "Ошибка разрешения: " << ec.message() << std::endl;
						return;
					}

					std::cout << "Хост разрешён. Попытка подключения..." << std::endl;

					// Connect to server
					net::async_connect(socket_, results,
						[this, req](beast::error_code ec, const tcp::endpoint& endpoint) {
							if (ec) {
								std::cerr << "Ошибка подключения: " << ec.message() << std::endl;
								return;
							}

							std::cout << "Подключено к: " << endpoint.address().to_string() << ":" << endpoint.port() << std::endl;

							// Send HTTP request
							std::cout << "Отправка HTTP-запроса на: " << req->target() << std::endl;
							http::async_write(socket_, *req,
								[this](beast::error_code ec, std::size_t) {
									if (ec) {
										std::cerr << "Ошибка при записи: " << ec.message() << std::endl;
										return;
									}

									std::cout << "Запрос отправлен. Ожидание ответа..." << std::endl;

									// Receive the response
									pRes res(new Res);
									http::async_read(socket_, buffer_, *res,
										[this, res](beast::error_code ec, std::size_t) mutable {
											if (ec) {
												std::cerr << "Ошибка при чтении: " << ec.message() << std::endl;
												return;
											}
											std::cout << "Ответ получен." << std::endl;
											handle_response(res);
										});
								});
						});
				});

			io_context_.run();
		}
		catch (const std::exception& e) {
			std::cerr << "Исключение в send_request: " << e.what() << std::endl;
		}
	}

	void handle_response(pRes res) {
		if (res->result() != http::status::ok) {
			std::cerr << "Error response: " << res->result_int() << " " << res->reason() << std::endl;
			std::cerr << "Response body: " << res->body() << std::endl;
		}
		else {
			std::cout << "Код ответа: " << res->result_int() << std::endl;
			std::cout << "Тело ответа: " << res->body() << std::endl;
		}
	}

	std::string host_;
	std::string port_;
	net::io_context io_context_;
	tcp::resolver resolver_;
	tcp::socket socket_;
	beast::flat_buffer buffer_;
};

int main() {
	std::string host = "localhost";
	std::string port = "8080";

	RestClient client(host, port);

	std::string username = "test_user";
	std::string password = "password"; // Укажите фактический пароль

	try {
		client.authenticate(username, password);

		std::string token = "received_token_here"; // Замените на фактический токен
		//client.get_tasks(token);
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка в main: " << e.what() << std::endl;
	}

	return 0;
}