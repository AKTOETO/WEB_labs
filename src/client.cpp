#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <fstream>
#include <deque>

using namespace boost::asio;
using ip::tcp;

class ChatClient {
private:
    io_service& io_service_;
    tcp::socket socket_;
    std::string nickname_;
    std::deque<std::string> local_cache_;
    std::ofstream cache_file_;

public:
    ChatClient(io_service& io_service)
        : io_service_(io_service),
          socket_(io_service) {
        cache_file_.open("client_cache.txt", std::ios::app);
        load_cache();
    }

    void connect(const std::string& host, int port) {
        socket_.connect(tcp::endpoint(
            ip::address::from_string(host), port));
        
        std::array<char, 128> buffer;
        size_t len = socket_.read_some(boost::asio::buffer(buffer));
        std::cout << std::string(buffer.data(), len);

        std::getline(std::cin, nickname_);
        socket_.write_some(boost::asio::buffer(nickname_ + "\n"));

        std::thread t([this]() { read_messages(); });
        t.detach();

        write_messages();
    }

private:
    void load_cache() {
        std::ifstream file("client_cache.txt");
        std::string line;
        while (std::getline(file, line)) {
            std::cout << line << std::endl;
            local_cache_.push_back(line);
            if (local_cache_.size() > 5)
                local_cache_.pop_front();
        }
    }

    void read_messages() {
        try {
            while (true) {
                std::array<char, 1024> buffer;
                size_t len = socket_.read_some(boost::asio::buffer(buffer));
                std::string message(buffer.data(), len);
                
                std::cout << message;
                
                local_cache_.push_back(message);
                if (local_cache_.size() > 5)
                    local_cache_.pop_front();
                
                cache_file_ << message;
                cache_file_.flush();
            }
        }
        catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void write_messages() {
        try {
            while (true) {
                std::string message;
                std::getline(std::cin, message);
                socket_.write_some(buffer(nickname_ + ": " + message + "\n"));
            }
        }
        catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
};

int main() {
    try {
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
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}