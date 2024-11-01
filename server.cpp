#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <sstream>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace json = boost::json;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        read_request();
    }

private:
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;

    void read_request() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_,
            [self](boost::beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    self->process_request();
                }
            });
    }

    void process_request() {
        if (req_.method() == http::verb::post) {
            std::string body = req_.body();
            if (req_.target() == "/register") {
                handle_registration(body);
            } else if (req_.target() == "/login") {
                handle_login(body);
            }
        } else if (req_.method() == http::verb::get) {
            if (req_.target() == "/chat.html") {
                serve_html_file("chat.html");
            } else {
                res_.result(http::status::not_found);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "404 Not Found";
                write_response();
                return;
            }
        }
    }

    void handle_registration(const std::string& body) {
        auto username_pos = body.find("username=");
        auto password_pos = body.find("&password=");
        
        std::string username = body.substr(username_pos + 9, password_pos - (username_pos + 9));
        std::string password = body.substr(password_pos + 10);

        decode_url(username);
        decode_url(password);

        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "trida", "Mogg4356%#TRIDAPALI"));
            con->setSchema("DB_Capolavoro");

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "INSERT INTO users (username, password) VALUES (?, ?)"));
            pstmt->setString(1, username);
            pstmt->setString(2, password);
            pstmt->execute();

            res_.result(http::status::ok);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Registrazione avvenuta con successo!";
        } catch (sql::SQLException& e) {
            res_.result(http::status::bad_request);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Errore nella registrazione: " + std::string(e.what());
        }
        write_response();
    }

    void handle_login(const std::string& body) {
        auto username_pos = body.find("username=");
        auto password_pos = body.find("&password=");
        
        std::string username = body.substr(username_pos + 9, password_pos - (username_pos + 9));
        std::string password = body.substr(password_pos + 10);

        decode_url(username);
        decode_url(password);

        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "trida", "Mogg4356%#TRIDAPALI"));
            con->setSchema("DB_Capolavoro");

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "SELECT * FROM users WHERE username = ? AND password = ?"));
            pstmt->setString(1, username);
            pstmt->setString(2, password);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            if (res->next()) {
                res_.result(http::status::ok);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "Login effettuato con successo!";
            } else {
                res_.result(http::status::unauthorized);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "Credenziali errate.";
            }
        } catch (sql::SQLException& e) {
            res_.result(http::status::internal_server_error);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Errore durante il login: " + std::string(e.what());
        }
        write_response();
    }

    void serve_html_file(const std::string& file_path) {
        std::string content = read_html_file(file_path);
        serve_response(content, "text/html", http::status::ok);
    }

    void serve_response(const std::string& body, const std::string& content_type, http::status status) {
        res_.result(status);
        res_.set(http::field::content_type, content_type);
        res_.body() = body;
        res_.prepare_payload();
        write_response();
    }

    void write_response() {
        auto self = shared_from_this();
        http::async_write(socket_, res_,
            [self](boost::beast::error_code ec, std::size_t bytes_transferred) {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }

    std::string read_html_file(const std::string& file_path) {
        std::ifstream file(file_path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void decode_url(std::string& str) {
        for (size_t pos = 0; (pos = str.find('+', pos)) != std::string::npos; pos++) {
            str.replace(pos, 1, " ");
        }
        for (size_t pos = 0; (pos = str.find("%20", pos)) != std::string::npos; pos++) {
            str.replace(pos, 3, " ");
        }
    }
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port) 
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    tcp::acceptor acceptor_;

    void start_accept() {
        acceptor_.async_accept(
            [this](boost::beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                }
                start_accept();
            });
    }
};

int main(int argc, char* argv[]) {
    try {
        boost::asio::io_context io_context;
        Server server(io_context, 8080);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Eccezione: " << e.what() << std::endl;
    }

    return 0;
}
