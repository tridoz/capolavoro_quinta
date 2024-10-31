#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <sstream>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

std::unordered_map<std::string, std::string> users = {
    {"utente1", "password1"},
    {"utente2", "password2"}
};

std::unordered_map<std::string, std::string> tokens;  // Memorizza i token generati

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
            token = std::string(auth_header->value());
        }

        if (req_.method() == http::verb::get) {
            if (req_.target() == "/" || req_.target() == "/login.html") {
                serve_html_file("login.html");  // Serve la pagina di login
            } else if (req_.target() == "/login.css") {
                serve_css_file("login.css");  // Serve il CSS della pagina di login
            } else if (req_.target() == "/chat.html") {
                serve_html_file("chat.html");  // Serve la pagina della chat
            } else if (req_.target() == "/chat.css") {
                serve_css_file("chat.css");  // Serve il CSS della chat
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

                // Restituisce chat.html
                res_.result(http::status::ok);
                res_.set(http::field::content_type, "text/html");
                res_.body() = read_html_file("chat.html");  // Serve il file HTML della chat
                res_.prepare_payload();
                write_response();

                // La richiesta per chat.css sarà gestita separatamente
                return;
            } else {
                serve_html_file("login.html", "Credenziali errate. Riprova.");
            }
        } else if (req_.method() == http::verb::put) {
            if (tokens.find(token) != tokens.end()) {
                // Operazione autorizzata, gestisci l'operazione qui
                res_.result(http::status::ok);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "Operazione autorizzata!";
            } else {
                res_.result(http::status::unauthorized);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "Token non valido.";
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
        // Genera un token unico (UUID)
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

int main() {
    try {
        boost::asio::io_context io_context;

        Server server(io_context, 8080);

        std::cout << "Server in ascolto su http://localhost:8080" << std::endl;

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Errore: " << e.what() << std::endl;
    }

    return 0;
}
