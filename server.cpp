#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

void gestisci_richiesta(http::request<http::string_body>&& req, tcp::socket& socket) {
    http::response<http::string_body> res;

    if (req.method() == http::verb::get) {
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/html");
        res.body() = R"(
            <!DOCTYPE html>
            <html lang="it">
            <head><meta charset="UTF-8"><title>Server C++</title></head>
            <body>
                <h1>Invia dati al server</h1>
                <form action="/" method="post">
                    <label for="dati">Inserisci qualcosa:</label>
                    <input type="text" id="dati" name="dati">
                    <button type="submit">Invia</button>
                </form>
            </body>
            </html>
        )";
        res.prepare_payload();
    }
    else if (req.method() == http::verb::post) {
        std::string dati = req.body();  // Dati inviati dal modulo
        std::cout << "Dati ricevuti: " << dati << std::endl;

        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Dati ricevuti: " + dati;
        res.prepare_payload();
    }

    http::write(socket, res);
}

int main() {
    boost::asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

    std::cout << "Server in ascolto su http://localhost:8080" << std::endl;

    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        boost::beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        gestisci_richiesta(std::move(req), socket);

        socket.shutdown(tcp::socket::shutdown_send);
    }

    return 0;
}
