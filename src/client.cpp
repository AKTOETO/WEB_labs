#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <fstream>
#include <deque>
#include <atomic>

using namespace boost::asio;
using ip::tcp;

class ChatClient
{
private:
    io_service &io_service_;
    tcp::socket socket_;
    std::string nickname_;
    std::deque<std::string> local_cache_;
    std::atomic_bool m_working;

public:
    ChatClient(io_service &io_service)
        : io_service_(io_service),
          socket_(io_service), m_working(1)
    {
        load_cache();
    }

    ~ChatClient()
    {
        dump_cache();
    }

    void connect(const std::string &host, int port)
    {
        socket_.connect(tcp::endpoint(
            ip::address::from_string(host), port));

        std::array<char, 128> buffer;
        size_t len = socket_.read_some(boost::asio::buffer(buffer));
        std::cout << std::string(buffer.data(), len);

        std::getline(std::cin, nickname_);
        socket_.write_some(boost::asio::buffer(nickname_ + "\n"));

        std::thread t([this]()
                      { read_messages(); });
        t.detach();

        write_messages();
    }

private:
    // загрузка кэша чата с файла
    void load_cache()
    {
        std::ifstream file("client_cache.txt");
        std::string line;
        while (std::getline(file, line))
        {
            std::cout << line << std::endl;
            local_cache_.push_back(line);
            if (local_cache_.size() > 5)
                local_cache_.pop_front();
        }
        std::cout << "cache size: " << local_cache_.size() << std::endl;
    }

    void dump_cache()
    {
        std::cout << "dumping to 'client_cache.txt'\n";
        std::ofstream cache_file("client_cache.txt");
        for (auto &msg : local_cache_)
            cache_file << msg << "\n";
        cache_file.flush();
        cache_file.close();
    }

    void read_messages()
    {
        while (m_working)
        {
            std::string message;

            boost::system::error_code error;
            size_t len = boost::asio::read_until(
                socket_, boost::asio::dynamic_buffer(message),
                '\n', error);

            if (error)
            {
                std::cerr << "Error reading from socket: " << error.message() << std::endl;
                break; // Прерываем цикл в случае ошибки
            }

            // Удаляем символ новой строки в конце сообщения
            if (!message.empty() && message.back() == '\n')
            {
                message.pop_back();
            }

            std::cout << "[" << message << "]" << std::endl;

            local_cache_.push_back(message);
            while (local_cache_.size() > 5)
                local_cache_.pop_front();

            // cache_file_ << message;
            // cache_file_.flush();
        }
    }

    void write_messages()
    {
        while (m_working)
        {
            std::string message;
            std::getline(std::cin, message);
            if (message == "exit")
                m_working = 0;
            socket_.write_some(buffer(nickname_ + ": " + message + "\n"));
        }
    }
};

int main()
{
    try
    {
        boost::asio::io_service io_service;
        ChatClient client(io_service);

        std::string host;
        int port;

        std::cout << "Enter server IP: ";
        std::cin >> host;
        std::cout << "Enter port: ";
        std::cin >> port;
        std::cin.ignore();

        client.connect(host, port);
        io_service.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}