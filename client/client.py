import socket
import time
import logging
import configparser

def client_program(host, port):
    client_socket = socket.socket()
    client_socket.connect((host, port))

    # Конфигурация логирования
    logging.basicConfig(
        filename="client.log",
        level=logging.INFO,
        format='%(asctime)s %(message)s',
        encoding='utf-8',  # Указываем кодировку UTF-8
        filemode='w'
    )
    logging.info(f"Подключен к серверу {host}:{port}")
    
    try:
        # Отправка сообщения через 5 секунд
        time.sleep(5)
        message = f"Плоцкий Богдан Андреевич, М3О-411Б-21"
        client_socket.send(message.encode())
        logging.info(f"Сообщение отправлено: {message}")

        # Получение ответа
        data = client_socket.recv(1024).decode()
        logging.info(f"Сообщение получено: {data}")

    except Exception as e:
        print(f"Произошла ошибка: {str(e)}")

    client_socket.close()
    logging.info("Отключен от сервера")

if __name__ == '__main__':
    config = configparser.ConfigParser()

    # Validate configuration file existence and structure
    try:
        config.read('config.ini')

        if not config.has_section('server'):
            raise ValueError("Missing '[server]' section in config.ini")
        if not config['server'].get('ip'):  # Use get() with default value for optional 'ip' key
            raise ValueError("Missing 'ip' key in '[server]' section of config.ini")

        host = config['server']['ip']  # Access 'ip' key if present
        port = int(config['server']['port'])

    except (FileNotFoundError, ValueError) as e:
        print(f"Configuration error: {str(e)}")
        exit(1)  # Indicate abnormal termination

    client_program(host, port)