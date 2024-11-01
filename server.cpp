#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>

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

std::unordered_map<std::string, std::string> users = {
    {"utente1", "password1"},
    {"utente2", "password2"}
};

std::unordered_map<std::string, std::string> tokens;  

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
        std::string token;
        auto auth_header = req_.find(http::field::authorization);

        if (auth_header != req_.end()) {
            token = std::string(auth_header->value());
        }

        if (req_.method() == http::verb::get) {
            if (req_.target() == "/" || req_.target() == "/login.html") {
                serve_html_file("login.html");
            } else if (req_.target() == "/login.css") {
                serve_css_file("login.css");
            } else if (req_.target() == "/chat.html") {
                serve_html_file("chat.html");
            } else if (req_.target() == "/chat.css") {
                serve_css_file("chat.css");
            } else if (req_.target() == "/chat.js") {
                serve_js_file("chat.js", token);
            } else if (req_.target() == "/getAllMessages") {
                serve_all_messages(token);
            } else {
                res_.result(http::status::not_found);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "404 Not Found";
                write_response();
                return;
            }
        } else if (req_.method() == http::verb::post) {
            std::string body = req_.body();
            if (req_.target() == "/sendMessage") {
                handle_send_message(body);
                return;
            } else {
                auto username_pos = body.find("username=");
                auto password_pos = body.find("&password=");
                
                std::string username = body.substr(username_pos + 9, password_pos - (username_pos + 9));
                std::string password = body.substr(password_pos + 10);

                decode_url(username);
                decode_url(password);

                if (users.find(username) != users.end() && users[username] == password) {
                    token = generate_token(username);
                    tokens[token] = username;  // Memorizza il token con il relativo username
                    serve_html_file("chat.html");
                } else {
                    serve_html_file("login.html", "Credenziali errate. Riprova.");
                }
            }
        }
        write_response();
    }

    void handle_send_message(const std::string& body) {
        auto sender_pos = body.find("sender=");
        auto receiver_pos = body.find("&receiver=");
        auto timestamp_pos = body.find("&timestamp=");
        auto message_pos = body.find("&message=");

        std::string sender = body.substr(sender_pos + 7, receiver_pos - (sender_pos + 7));
        std::string receiver = body.substr(receiver_pos + 10, timestamp_pos - (receiver_pos + 10));
        std::string timestamp = body.substr(timestamp_pos + 11, message_pos - (timestamp_pos + 11));
        std::string message = body.substr(message_pos + 8);

        decode_url(sender);
        decode_url(receiver);
        decode_url(timestamp);
        decode_url(message);

        save_message(sender, receiver, timestamp, message);

        res_.result(http::status::ok);
        write_response();
    }

    void serve_all_messages(const std::string& token) {
        if (tokens.find(token) != tokens.end()) {
            std::string username = tokens[token];
            json::array messages_json = retrieve_user_messages(username);
            std::string json_response = json::serialize(messages_json);

            serve_response(json_response, "application/json", http::status::ok);
        } else {
            res_.result(http::status::unauthorized);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Token non valido.";
            write_response();
        }
    }

    void serve_html_file(const std::string& file_path, const std::string& error_message = "") {
        std::string content = read_html_file(file_path);
        if (content.empty() && !error_message.empty()) {
            content = "<html><body><h1>" + error_message + "</h1></body></html>";
        }
        serve_response(content, "text/html", http::status::ok);
    }

    void serve_css_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            res_.result(http::status::not_found);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "404 Not Found";
            write_response();
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        res_.result(http::status::ok);
        res_.set(http::field::content_type, "text/css");
        res_.body() = buffer.str();
        res_.prepare_payload();
        write_response();
    }

    void serve_js_file(const std::string& path, const std::string& token) {
        if (tokens.find(token) != tokens.end()) {
            std::string username = tokens[token];
            json::array messages_json = retrieve_user_messages(username);

            std::ifstream file(path);
            std::stringstream buffer;
            buffer << file.rdbuf();

            std::string js_code = "const messages = " + json::serialize(messages_json) + ";\n" + buffer.str();
            serve_response(js_code, "application/javascript", http::status::ok);
        } else {
            res_.result(http::status::unauthorized);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Token non valido.";
            write_response();
        }
    }

    json::array retrieve_user_messages(const std::string& username) {
        json::array messages_json;

        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "root", "password"));
            con->setSchema("DB_Capolavoro");

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "SELECT sender, receiver, message, timestamp FROM messages WHERE sender = ? OR receiver = ?"));
            pstmt->setString(1, username);
            pstmt->setString(2, username);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            while (res->next()) {
                json::object msg;
                msg["sender"] = json::value(std::string(res->getString("sender")));
                msg["receiver"] = json::value(std::string(res->getString("receiver")));
                msg["message"] = json::value(std::string(res->getString("message")));
                msg["timestamp"] = json::value(std::string(res->getString("timestamp")));
                messages_json.push_back(msg);
            }
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nel recupero dei messaggi: " << e.what() << std::endl;
        }

        return messages_json;
    }

    void save_message(const std::string& sender, const std::string& receiver, const std::string& timestamp, const std::string& message) {
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "root", "password"));
            con->setSchema("DB_Capolavoro");

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "INSERT INTO messages (sender, receiver, message, timestamp) VALUES (?, ?, ?, ?)"));
            pstmt->setString(1, sender);
            pstmt->setString(2, receiver);
            pstmt->setString(3, message);
            pstmt->setString(4, timestamp);
            pstmt->executeUpdate();
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nel salvataggio del messaggio: " << e.what() << std::endl;
        }
    }

    void serve_response(const std::string& content, const std::string& content_type, http::status status) {
        res_.result(status);
        res_.set(http::field::content_type, content_type);
        res_.body() = content;
        res_.prepare_payload();
        write_response();
    }

    std::string read_html_file(const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void write_response() {
        auto self = shared_from_this();
        http::async_write(socket_, res_,
            [self](boost::beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }

    std::string generate_token(const std::string& username) {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        return to_string(uuid);
    }

    void decode_url(std::string& value) {
        boost::algorithm::replace_all(value, "+", " "); // Sostituisci i "+" con spazi
        std::string decoded;
        decoded.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '%') {
                if (i + 2 < value.size()) {
                    std::string hex = value.substr(i + 1, 2);
                    char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                    decoded.push_back(decoded_char);
                    i += 2; 
                }
            } else {
                decoded.push_back(value[i]);
            }
        }
        value = decoded;
    }
};

class Server {
public:
    Server(boost::asio::io_context& ioc, tcp::endpoint endpoint) 
        : acceptor_(ioc, endpoint) {
        do_accept();
    }

private:
    tcp::acceptor acceptor_;

    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                }
                do_accept();
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
