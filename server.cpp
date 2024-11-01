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
        // Estrai il token dall'intestazione "Authorization"
        std::string token;
        auto auth_header = req_.find(http::field::authorization);

        if (auth_header != req_.end()) {
            token = std::string(auth_header->value());  // Converte direttamente in std::string
        }

        if (req_.method() == http::verb::get) {
            if (req_.target() == "/login.html") {
                serve_html_file("login.html");
            } else if (req_.target() == "/login.css") {
                serve_css_file("login.css");
            } else if (req_.target() == "/chat.html") {
                serve_html_file("chat.html");
            } else if (req_.target() == "/chat.css") {
                serve_css_file("chat.css");
            } else if (req_.target() == "/chat.js") {
                serve_js_file("chat.js", token);
            } else {
                res_.result(http::status::not_found);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "404 Not Found";
                write_response();
                return;
            }
        } else if (req_.method() == http::verb::post) {
            std::string body = req_.body();
            auto username_pos = body.find("username=");
            auto password_pos = body.find("&password=");
            
            std::string username = body.substr(username_pos + 9, password_pos - (username_pos + 9));
            std::string password = body.substr(password_pos + 10);

            decode_url(username);
            decode_url(password);

            // Verifica credenziali
            if (users.find(username) != users.end() && users[username] == password) {
                // Genera un token
                token = generate_token(username);
                tokens[token] = username;  // Memorizza il token con il relativo username

                // Invia la pagina chat
                serve_html_file("chat.html");
            } else {
                serve_html_file("login.html", "Credenziali errate. Riprova.");
            }
        }
        write_response();
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
        // Controlla se il token è valido
        if (tokens.find(token) != tokens.end()) {
            std::string username = tokens[token];
            json::array messages_json = retrieve_user_messages(username);

            // Leggi il contenuto di chat.js
            std::ifstream file(path);
            std::stringstream buffer;
            buffer << file.rdbuf();

            // Aggiungi i messaggi come variabile JavaScript
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

    std::string generate_token(const std::string& username) {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        return to_string(uuid);
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
