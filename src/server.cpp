#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <deque>
#include <fstream>
#include <string>
#include <map>

using namespace boost::asio;
using ip::tcp;

class ChatServer
{
private:
    io_service &io_service_;
    tcp::acceptor acceptor_;
    std::map<std::string, std::shared_ptr<tcp::socket>> m_clients;
    std::deque<std::string> message_history_;
    std::ofstream log_file_;

public:
    ChatServer(io_service &io_service, int port)
        : io_service_(io_service),
          acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        log_file_.open("chat_history.txt", std::ios::app);
        start_accept();
        load_history();
    }

private:
    void load_history()
    {
        std::ifstream file("chat_history.txt");
        std::string line;
        while (std::getline(file, line))
        {
            message_history_.push_back(line);
            if (message_history_.size() > 10)
                message_history_.pop_front();
        }
    }

    void start_accept()
    {
        auto socket = std::make_shared<tcp::socket>(io_service_);
        acceptor_.async_accept(
            *socket,
            [this, socket](const boost::system::error_code &error)
            {
                if (!error)
                {
                    std::cout << "[INFO] Клиент подключается\n";
                    handle_new_client(socket);
                }
                start_accept();
            });
    }

    void handle_new_client(std::shared_ptr<tcp::socket> socket)
    {
        // Запрос nickname
        async_write(*socket, buffer("Enter nickname: "),
                    [this, socket](const boost::system::error_code &error, size_t)
                    {
                        if (!error)
                        {
                            std::cout << "[INFO] Читаем имя клиента\n";
                            read_nickname(socket);
                        }
                    });
    }

    void read_nickname(std::shared_ptr<tcp::socket> socket)
    {
        auto buffer = std::make_shared<boost::asio::streambuf>();
        async_read_until(
            *socket, *buffer, '\n',
            [this, socket, buffer](const boost::system::error_code &error, size_t)
            {
                if (!error)
                {
                    std::string nickname{std::istreambuf_iterator<char>(&*buffer),
                                         std::istreambuf_iterator<char>()};
                    nickname = nickname.substr(0, nickname.size() - 1);
                    std::cout << "[INFO] Прочитан никнейм: [" << nickname << "]\n";

                    // добавляем пользователя в список пользователей
                    auto pair =
                        std::make_shared<std::pair<std::string, std::shared_ptr<tcp::socket>>>(std::move(nickname), socket);
                    // m_clients[nickname] = socket;
                    m_clients.insert(*pair);

                    // Отправка истории сообщений
                    send_history(pair);
                    start_read_messages(pair);
                }
                else
                {
                    socket->close();
                }
            });
    }

    void send_history(std::shared_ptr<std::pair<std::string, std::shared_ptr<tcp::socket>>> pair)
    {
        for (const auto &msg : message_history_)
        {
            async_write(
                *pair->second, boost::asio::buffer(msg + "\n"),
                [msg, pair](const boost::system::error_code &, size_t)
                {
                    std::cout << "Пользователю [" << pair->first << "] отправлено: [" << msg << "]\n";
                });
        }
    }

    void start_read_messages(std::shared_ptr<std::pair<std::string, std::shared_ptr<tcp::socket>>> pair)
    {
        auto buffer = std::make_shared<boost::asio::streambuf>();
        async_read_until(
            *((*pair).second), *buffer, '\n',
            [this, pair, buffer](const boost::system::error_code &error, size_t len)
            {
                if (!error)
                {
                    std::string message(boost::asio::buffers_begin(buffer->data()),
                                        boost::asio::buffers_begin(buffer->data()) + len - 1);
                    std::cout << message << std::endl;
                    broadcast_message(message, pair->second);
                    start_read_messages(pair);
                }
                // закрылся удаленный сокет
                else if (error == boost::asio::error::eof)
                {
                    std::cout << "[INFO] Клиент отключен " << pair->first << std::endl;
                    m_clients.erase(m_clients.find(pair->first));
                };
            });
    }

    void broadcast_message(const std::string &message, std::shared_ptr<tcp::socket> sender)
    {
        // запись в файл
        log_file_ << (message + "\n");
        log_file_.flush();

        // запись истории сообщений
        message_history_.push_back(message);
        if (message_history_.size() > 10)
            message_history_.pop_front();

        // передача нового сообщения всем, кроме отправителя
        for (auto &[_, client] : m_clients)
        {
            if (client != sender)
            {
                async_write(*client, buffer(message + "\n"),
                            [](const boost::system::error_code &, size_t) {});
            }
        }
    }
};

int main()
{
    try
    {
        // Создаем объект io_service для обработки ввода-вывода
        io_service io_service;

        // Создаем экземпляр ChatServer, прослушивающий на порту 8001
        ChatServer server(io_service, 8001);

        // Запускаем цикл обработки событий
        io_service.run();
    }
    catch (std::exception &e)
    {
        // Обработка исключений, если что-то пойдет не так
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}