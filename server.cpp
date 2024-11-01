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
        if (req_.method() == http::verb::get) {
            if (req_.target() == "/" || req_.target() == "/login.html") {
                serve_html_file("login.html");
            } else if (req_.target() == "/register.html") {
                serve_html_file("register.html");
            } else if (req_.target() == "/login.css") {
                serve_css_file("login.css");
            } else if (req_.target() == "/register.css") {
                serve_css_file("register.css");
            } else if (req_.target() == "/getMessages") {
                handle_get_messages();
            } else {
                res_.result(http::status::not_found);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "404 Not Found";
                write_response();
                return;
            }
        } else if (req_.method() == http::verb::post) {
            if (req_.target() == "/login") {
                handle_login();
            } else if (req_.target() == "/register") {
                handle_registration();
            } else if (req_.target() == "/sendMessage") {
                handle_send_message();
            } else {
                res_.result(http::status::not_found);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "404 Not Found";
                write_response();
                return;
            }
        }
    }

    void handle_login() {
        std::string body = req_.body();
        auto username_pos = body.find("username=");
        auto password_pos = body.find("&password=");
        
        std::string username = body.substr(username_pos + 9, password_pos - (username_pos + 9));
        std::string password = body.substr(password_pos + 10);

        decode_url(username);
        decode_url(password);

        if (check_credentials(username, password)) {
            serve_html_file("chat.html");
        } else {
            serve_html_file("login.html", "Credenziali errate. Riprova.");
        }
    }

    void handle_registration() {
        std::string body = req_.body();
        auto username_pos = body.find("username=");
        auto password_pos = body.find("&password=");
        
        std::string username = body.substr(username_pos + 9, password_pos - (username_pos + 9));
        std::string password = body.substr(password_pos + 10);

        decode_url(username);
        decode_url(password);

        if (register_user(username, password)) {
            serve_html_file("login.html");
        } else {
            serve_html_file("register.html", "Errore nella registrazione. Riprova.");
        }
    }

    void handle_send_message() {
        std::string body = req_.body();
        auto sender_pos = body.find("sender=");
        auto receiver_pos = body.find("&receiver=");
        auto text_pos = body.find("&text=");

        std::string sender = body.substr(sender_pos + 7, receiver_pos - (sender_pos + 7));
        std::string receiver = body.substr(receiver_pos + 10, text_pos - (receiver_pos + 10));
        std::string text = body.substr(text_pos + 6);

        decode_url(sender);
        decode_url(receiver);
        decode_url(text);

        save_message(sender, receiver, text);
        serve_response("Message sent", "text/plain", http::status::ok);
    }

    void handle_get_messages() {
        // Assuming the username is passed as a query parameter
        auto query = req_.target().to_string();
        auto username_pos = query.find("username=");
        std::string username = query.substr(username_pos + 9);

        decode_url(username);
        json::array messages = get_all_messages(username);
        
        json::object response = { {"messages", messages} };
        serve_response(json::serialize(response), "application/json", http::status::ok);
    }

    json::array get_all_messages(const std::string& username) {
        json::array messages;
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "trida", "Mogg4356%#TRIDAPALI"));
            con->setSchema("DB_Capolavoro");

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "SELECT sender, receiver, timestamp, text FROM Messages WHERE sender = ? OR receiver = ?"));
            pstmt->setString(1, username);
            pstmt->setString(2, username);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                json::object message = {
                    {"sender", res->getString("sender")},
                    {"receiver", res->getString("receiver")},
                    {"timestamp", res->getString("timestamp")},
                    {"text", res->getString("text")}
                };
                messages.push_back(message);
            }
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nel recupero dei messaggi: " << e.what() << std::endl;
        }
        return messages;
    }

    void save_message(const std::string& sender, const std::string& receiver, const std::string& text) {
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "trida", "Mogg4356%#TRIDAPALI"));
            con->setSchema("DB_Capolavoro");

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "INSERT INTO Messages (sender, receiver, timestamp, text) VALUES (?, ?, NOW(), ?)"));
            pstmt->setString(1, sender);
            pstmt->setString(2, receiver);
            pstmt->setString(3, text);
            pstmt->executeUpdate();
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nel salvataggio del messaggio: " << e.what() << std::endl;
        }
    }

    bool check_credentials(const std::string& username, const std::string& password) {
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "trida", "Mogg4356%#TRIDAPALI"));
            con->setSchema("DB_Capolavoro");

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "SELECT COUNT(*) FROM Users WHERE username = ? AND password = ?"));
            pstmt->setString(1, username);
            pstmt->setString(2, password); // Assicurati che la password sia memorizzata in modo sicuro (hash)

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            res->next();
            return res->getInt(1) > 0; // Ritorna true se le credenziali sono corrette
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nel controllo delle credenziali: " << e.what() << std::endl;
            return false;
        }
    }

    bool register_user(const std::string& username, const std::string& password) {
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "trida", "Mogg4356%#TRIDAPALI"));
            con->setSchema("DB_Capolavoro");

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "INSERT INTO Users (username, password) VALUES (?, ?)"));
            pstmt->setString(1, username);
            pstmt->setString(2, password); // Assicurati di gestire la sicurezza delle password

            pstmt->executeUpdate();
            return true; // Registrazione avvenuta con successo
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nella registrazione: " << e.what() << std::endl;
            return false;
        }
    }

    void serve_html_file(const std::string& file_path, const std::string& error_message = "") {
        std::string content = read_html_file(file_path);
        if (content.empty() && !error_message.empty()) {
            content = "<html><body><h1>" + error_message + "</h1></body></html>";
        }
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
