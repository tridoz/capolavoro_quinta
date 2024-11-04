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
#include <stdexcept>
#include <iomanip>

#define DB_User "trida"
#define DB_Password "Mogg4356%#TRIDAPALI"
#define DB_Name "DB_Capolavoro"

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
    std::string logged_username;

    void read_request() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_,
            [self](boost::beast::error_code ec, std::size_t) {
                if (!ec) {
                    self->process_request();
                } else {
                    std::cerr << "Error reading request: " << ec.message() << std::endl;
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
        }else if(req_.target() == "/chat.css"){
            serve_css_file("chat.css");
        } else if (req_.target() == "/chat.js") {
            serve_js_file("chat.js");
        } else if(req_.target() == "/background.svg"){
            serve_image_file("background.svg");
        } else if(req_.target() == "/chat_background.svg"){
            serve_image_file("chat_background.svg");
        }else if (req_.target() == "/getUsers") {
            handle_get_users();  // Handle the new endpoint for getting users
        } else if (req_.target().starts_with("/getMessages")) {
            handle_get_messages();
        } else {
            send_not_found();
        }
    }

    void handle_get_users() {
        json::array users = get_all_users();
        json::object response = { {"users", users} };
        serve_response(json::serialize(response), "application/json", http::status::ok);
    }

    boost::json::array get_all_users() {
        sql::mysql::MySQL_Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", DB_User, DB_Password);
        con->setSchema(DB_Name);

        // Query per ottenere tutti gli utenti
        pstmt = con->prepareStatement("SELECT username FROM Users");
        res = pstmt->executeQuery();

        boost::json::array jsonArray;

        while (res->next()) {
            // Recupera il nome utente
            std::string username = res->getString("username");

            // Crea un oggetto JSON
            boost::json::object jsonObject;
            jsonObject["username"] = username;

            // Aggiungi l'oggetto all'array
            jsonArray.push_back(std::move(jsonObject));
        }

        // Clean up
        delete res;
        delete pstmt;
        delete con;

        return jsonArray;
    }

    void handle_post_requests() {
        if (req_.target() == "/login") {
            handle_login();
        } else if (req_.target() == "/register") {
            handle_registration();
        } else if (req_.target() == "/sendMessage") {
            handle_send_message();
        } else if(req_.target() == "/getAllMessages"){
            handle_get_messages();
        }else {
            send_not_found();
        }
    }

    void handle_login() {
        auto [username, password] = parse_credentials();
        if (check_credentials(username, password)) {
            logged_username = username;  
            std::cout << "User logged in: " << logged_username << std::endl;
            serve_html_file("chat.html", "", logged_username);
        } else {
            std::cout << "Login failed for username: " << username << std::endl;
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
        if (save_message(sender, receiver, text)) {
            serve_response("Message sent", "text/plain", http::status::ok);
        } else {
            serve_response("Error sending message", "text/plain", http::status::internal_server_error);
        }
    }

    void handle_get_messages() {

        std::pair<std::string, std::string> request = parse_username_receiver( req_.body() );
        json::array response = get_all_messages(request.first, request.second);
        json::object resp = {{"messages", response}};
        serve_response(json::serialize(resp), "application/json", http::status::ok);        
    }
    
    std::pair<std::string, std::string> parse_username_receiver(std::string body){
        auto username_pos = body.find("username=");
        auto receiver_pos = body.find("&receiver=");

        std::string username = body.substr(username_pos + 9, receiver_pos - (username_pos + 9));
        std::string receiver = body.substr(receiver_pos + 10);

        decode_url(username);
        decode_url(receiver);
        return {username, receiver};   
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
        auto text_pos = body.find("&message=");


        std::string sender = body.substr(sender_pos + 7, receiver_pos - (sender_pos + 7));
        std::string receiver = body.substr(receiver_pos + 10, text_pos-(receiver_pos+10)  );
        std::string text = body.substr(text_pos + 9);

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

    json::array get_all_messages(std::string search_username, std::string search_receiver) {
    
    sql::mysql::MySQL_Driver *driver;
    sql::Connection *con;
    sql::PreparedStatement *pstmt;
    sql::ResultSet *res;

    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect("tcp://127.0.0.1:3306", DB_User, DB_Password);
    con->setSchema(DB_Name);

    pstmt = con->prepareStatement(
        "SELECT sender, receiver, timestamp, message "
        "FROM Messages "
        "WHERE (sender = ? AND receiver = ?) OR (sender = ? AND receiver = ?)"
    );

    pstmt->setString(1, search_username);
    pstmt->setString(2, search_receiver);
    pstmt->setString(3, search_receiver);
    pstmt->setString(4, search_username);

    res = pstmt->executeQuery();

    boost::json::array jsonArray;

    while (res->next()) {

        std::string sender = res->getString("sender").c_str();
        std::string receiver = res->getString("receiver").c_str();
        std::string message = res->getString("message").c_str();
        std::string timestamp = res->getString("timestamp").c_str();

        boost::json::object jsonObject;
        jsonObject["sender"] = sender;
        jsonObject["receiver"] = receiver;
        jsonObject["message"] = message;
        jsonObject["timestamp"] = timestamp;

        // Add the object to the array
        jsonArray.push_back(jsonObject);
    }


    // Clean up
    delete res;
    delete pstmt;
    delete con;

    return jsonArray;
}

    bool save_message(const std::string& sender, const std::string& receiver, const std::string& text) {
        try {
            auto con = create_connection();
            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "INSERT INTO Messages (sender, receiver, timestamp, message) VALUES (?, ?, NOW(), ?)"));

            
            pstmt->setString(1, sender);
            pstmt->setString(2, receiver);
            pstmt->setString(3, text);
            pstmt->executeUpdate();
            std::cout<<"messaggio inserito correttamente nel DB:\nSender -> "<<sender<<"\nreceiver-> "<<receiver<<"\nmessage->"<<text<<std::endl;
            return true;
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nel salvataggio del messaggio: " << e.what() << std::endl;
            return false;
        }
    }

    bool check_credentials(const std::string& username, const std::string& password) {
        try {
            auto con = create_connection();
            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "SELECT COUNT(*) FROM Users WHERE username = ? AND password = ?"));
            pstmt->setString(1, username);
            pstmt->setString(2, password); // Hash password in a real implementation

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            res->next();
            return res->getInt(1) > 0;
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nel controllo delle credenziali: " << e.what() << std::endl;
            return false;
        }
    }

    bool register_user(const std::string& username, const std::string& password) {
        try {
            auto con = create_connection();
            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                "INSERT INTO Users (username, password) VALUES (?, ?)"));
            pstmt->setString(1, username);
            pstmt->setString(2, password); // Hash password in a real implementation
            pstmt->executeUpdate();
            std::cout<<"utente registrato correttamente"<<std::endl;
            return true;
        } catch (sql::SQLException& e) {
            std::cerr << "Errore nella registrazione: " << e.what() << std::endl;
            return false;
        }
    }

    void serve_html_file(const std::string& filename, const std::string& message = "", const std::string& username = "") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            send_not_found();
            return;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();

        if (!message.empty()) {
            content += "<p>" + message + "</p>";
        }

        if(!username.empty()){
            content += "<script> let username = \"" + username + "\";</script>";
        }

        serve_response(content, "text/html", http::status::ok);
    }

    void serve_css_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            send_not_found();
            return;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        serve_response(ss.str(), "text/css", http::status::ok);
    }

    void serve_js_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            send_not_found();
            return;
        }

        // Leggi il contenuto del file in una stringa
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        // Invia la risposta con il contenuto del file JavaScript
        serve_response(content, "application/javascript", http::status::ok);
    }

    void serve_image_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            send_not_found();
            return;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        serve_response(ss.str(), "image/svg+xml", http::status::ok);
    }
    
    void serve_response(const std::string& body, const std::string& content_type, http::status status) {
        res_.result(status);
        res_.set(http::field::content_type, content_type);
        res_.body() = body;
        res_.prepare_payload();  // Ensure the payload is prepared
        auto self = shared_from_this();
        http::async_write(socket_, res_,
            [self](boost::beast::error_code ec, std::size_t) {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }

    void send_not_found() {
        serve_response("404 Not Found", "text/plain", http::status::not_found);
    }

    void decode_url(std::string& str) {
        std::string decoded;
        char ch;
        int h;
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%') {
                sscanf(str.substr(i + 1, 2).c_str(), "%x", &h);
                decoded += static_cast<char>(h);
                i += 2;
            } else {
                decoded += str[i];
            }
        }
        str = decoded;
    }

    std::unique_ptr<sql::Connection> create_connection() {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", DB_User, DB_Password));
        con->setSchema("DB_Capolavoro");
        return con;
    }

};

class Server {
public:
    Server(boost::asio::io_context& ioc, tcp::endpoint endpoint) 
        : acceptor_(ioc, endpoint) {
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
        boost::asio::io_context ioc;
        tcp::endpoint endpoint(tcp::v4(), 8080);
        Server server(ioc, endpoint);
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
