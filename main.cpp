#include "./main.h"

int main(){
    using namespace iku::net;
    using namespace std;

    try{
        {
            using namespace http;
            socket_t sock = socket::tcp(4000);
            server& server = *new http::server();

            server.GET("/error", [](request& req, response& res) {
                throw std::exception("Dver` mne zapili");
            });

            server.GET("/hell", [](request & req, response & res) {
                res.status = 418;
            });

            server.GET("/", [](request& req, response& res) {
                std::string_view text = "<h1>Powel Nahoy</h1>";
                res.headers.set("Content-Type", "text/html");
                res.status = 228;
                res.message = "Lejat sosat";
                res.body.assign(text.begin(), text.end());
            });

            server.setListener(sock);
            server.start();

        }
    }
    catch (const logic_error& ex) {
        cout << "logic_error "  << ex.what() << endl;
    }
    catch (const exception& ex) {
        cout << ex.what() << endl;
    }
    catch (...) {
        cout << "UNKNOWN PROBLEM" << endl;
    }

    return 0;
}