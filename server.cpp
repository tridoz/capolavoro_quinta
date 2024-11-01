#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
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
    explicit Session(tcp::socket socket) : socket_(std::move(socket)) {}

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
            [self](boost::beast::error_code ec, std::size_t) {
                if (!ec) {
                    self->process_request();
                }
            });
    }

    void process_request() {
        if (req_.method() == http::verb::get) {
            handle_get_requests();
        } else if (req_.method() == http::verb::post) {
            handle_post_requests();
        } else {
            send_not_found();
        }
    }

    void handle_get_requests() {
        if (req_.target() == "/" || req_.target() == "/login.html") {
            serve_html_file("login.html");
        } else if (req_.target() == "/register.html") {
            serve_html_file("register.html");
        } else if (req_.target() == "/login.css") {
            serve_css_file("login.css");
        } else if (req_.target() == "/register.css") {
            serve_css_file("register.css");
        } else if (req_.target().starts_with("/getMessages")) {
            handle_get_messages();
        } else {
            send_not_found();
        }
    }

    void handle_post_requests() {
        if (req_.target() == "/login") {
            handle_login();
        } else if (req_.target() == "/register") {
            handle_registration();
        } else if (req_.target() == "/sendMessage") {
            handle_send_message();
        } else {
            send_not_found();
        }
    }

    void handle_login() {
        auto [username, password] = parse_credentials();
        if (check_credentials(username, password)) {
            serve_html_file("chat.html");
        } else {
            serve_html_file("login.html", "Credenziali errate. Riprova.");
        }
    }

    void handle_registration() {
        auto [username, password] = parse_credentials();
        if (register_user(username, password)) {
            serve_html_file("login.html");
        } else {
            serve_html_file("register.html", "Errore nella registrazione. Riprova.");
        }
    }

    void handle_send_message() {
        auto [sender, receiver, text] = parse_message();
        save_message(sender, receiver, text);
        serve_response("Message sent", "text/plain", http::status::ok);
    }

    void handle_get_messages() {
        auto query = std::string(req_.target());
        auto username = parse_query_parameter(query, "username");
        if (!username.empty()) {
            decode_url(username);
            json::array messages = get_all_messages(username);
            json::object response = { {"messages", messages} };
            serve_response(json::serialize(response), "application/json", http::status::ok);
        } else {
            serve_response("Username parameter missing", "text/plain", http::status::bad_request);
        }
    }

    std::pair<std::string, std::string> parse_credentials() {
        std::string body = req_.body();
        auto username_pos = body.find("username=");
        auto password_pos = body.find("&password=");

        std::string username = body.substr(username_pos + 9, password_pos - (username_pos + 9));
        std::string password = body.substr(password_pos + 10);

        decode_url(username);
        decode_url(password);
        return {username, password};
    }

    std::tuple<std::string, std::string, std::string> parse_message() {
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
        return {sender, receiver, text};
    }

    std::string parse_query_parameter(const std::string& query, const std::string& param) {
        auto pos = query.find(param + "=");
        if (pos == std::string::npos) return "";
        return query.substr(pos + param.length() + 1);
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
            pstmt->setString(2, password); // Assicurati che la password sia memorizzata in modo sicuro (hash)
            pstmt->executeUpdate();
            return true;
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nella registrazione: " << e.what() << std::endl;
            return false;
        }
    }

    void serve_html_file(const std::string& filename, const std::string& error_message = "") {
        std::ifstream file(filename);
        if (file) {
            std::ostringstream ss;
            ss << file.rdbuf();
            res_ = http::response<http::string_body>(http::status::ok, req_.version());
            res_.set(http::field::content_type, "text/html");
            res_.body() = ss.str();
            res_.prepare_payload();
        } else {
            send_not_found();
        }
        write_response();
    }

    void serve_css_file(const std::string& filename) {
        std::ifstream file(filename);
        if (file) {
            std::ostringstream ss;
            ss << file.rdbuf();
            res_ = http::response<http::string_body>(http::status::ok, req_.version());
            res_.set(http::field::content_type, "text/css");
            res_.body() = ss.str();
            res_.prepare_payload();
        } else {
            send_not_found();
        }
        write_response();
    }

    void serve_response(const std::string& body, const std::string& content_type, http::status status) {
        res_ = http::response<http::string_body>(status, req_.version());
        res_.set(http::field::content_type, content_type);
        res_.body() = body;
        res_.prepare_payload();
        write_response();
    }

    void send_not_found() {
        res_ = http::response<http::string_body>(http::status::not_found, req_.version());
        res_.set(http::field::content_type, "text/plain");
        res_.body() = "404 Not Found";
        res_.prepare_payload();
        write_response();
    }

    void write_response() {
        auto self = shared_from_this();
        http::async_write(socket_, res_,
            [self](boost::beast::error_code ec, std::size_t) {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }

    void decode_url(std::string& str) {
        // Decodifica le sequenze percentuali
        // Aggiungi qui la logica per decodificare i caratteri percentuali
    }
};

class Server {
public:
    Server(boost::asio::io_context& ioc, tcp::endpoint endpoint)
        : acceptor_(ioc, endpoint) {
        accept_connections();
    }

private:
    tcp::acceptor acceptor_;

    void accept_connections() {
        acceptor_.async_accept(
            [this](boost::beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                }
                accept_connections();
            });
    }
};

int main() {
    try {
        boost::asio::io_context ioc;
        tcp::endpoint endpoint(tcp::v4(), 8080);
        Server server(ioc, endpoint);
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Errore: " << e.what() << std::endl;
    }
    return 0;
}
