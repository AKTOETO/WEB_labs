#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <deque>
#include <fstream>
#include <string>

using namespace boost::asio;
using ip::tcp;

class ChatServer {
private:
    io_service& io_service_;
    tcp::acceptor acceptor_;
    std::vector<std::shared_ptr<tcp::socket>> clients_;
    std::vector<std::string> nicknames_;
    std::deque<std::string> message_history_;
    std::ofstream log_file_;

public:
    ChatServer(io_service& io_service, int port)
        : io_service_(io_service),
          acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
        log_file_.open("chat_history.txt", std::ios::app);
        start_accept();
        load_history();
    }

private:
    void load_history() {
        std::ifstream file("chat_history.txt");
        std::string line;
        while (std::getline(file, line)) {
            message_history_.push_back(line);
            if (message_history_.size() > 10)
                message_history_.pop_front();
        }
    }

    void start_accept() {
        auto socket = std::make_shared<tcp::socket>(io_service_);
        acceptor_.async_accept(*socket,
            [this, socket](const boost::system::error_code& error) {
                if (!error) {
                    handle_new_client(socket);
                }
                start_accept();
            });
    }

    void handle_new_client(std::shared_ptr<tcp::socket> socket) {
        clients_.push_back(socket);
        
        // Запрос nickname
        async_write(*socket, buffer("Enter nickname: "),
            [this, socket](const boost::system::error_code& error, size_t) {
                if (!error) {
                    read_nickname(socket);
                }
            });
    }

    void read_nickname(std::shared_ptr<tcp::socket> socket) {
        auto buffer = std::make_shared<boost::asio::streambuf>();
        async_read_until(*socket, *buffer, '\n',
            [this, socket, buffer](const boost::system::error_code& error, size_t) {
                if (!error) {
                    std::string nickname{std::istreambuf_iterator<char>(&*buffer),
                                      std::istreambuf_iterator<char>()};
                    nickname = nickname.substr(0, nickname.size() - 1);
                    nicknames_.push_back(nickname);
                    
                    // Отправка истории сообщений
                    for (const auto& msg : message_history_) {
                        async_write(*socket, boost::asio::buffer(msg + "\n"),
                            [](const boost::system::error_code&, size_t){});
                    }
                    
                    start_read_messages(socket);
                }
            });
    }

    void start_read_messages(std::shared_ptr<tcp::socket> socket) {
        auto buffer = std::make_shared<boost::asio::streambuf>();
        async_read_until(*socket, *buffer, '\n',
            [this, socket, buffer](const boost::system::error_code& error, size_t) {
                if (!error) {
                    std::string message{std::istreambuf_iterator<char>(&*buffer),
                                     std::istreambuf_iterator<char>()};
                    broadcast_message(message, socket);
                    start_read_messages(socket);
                }
            });
    }

    void broadcast_message(const std::string& message, std::shared_ptr<tcp::socket> sender) {
        log_file_ << message;
        log_file_.flush();
        
        message_history_.push_back(message);
        if (message_history_.size() > 10)
            message_history_.pop_front();

        for (auto& client : clients_) {
            if (client != sender) {
                async_write(*client, buffer(message),
                    [](const boost::system::error_code&, size_t){});
            }
        }
    }
};


int main() {
    try {
        // Создаем объект io_service для обработки ввода-вывода
        io_service io_service;
        
        // Создаем экземпляр ChatServer, прослушивающий на порту 8001
        ChatServer server(io_service, 8001);
        
        // Запускаем цикл обработки событий
        io_service.run();
    }
    catch (std::exception& e) {
        // Обработка исключений, если что-то пойдет не так
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}