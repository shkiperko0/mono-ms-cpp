#pragma once
#include "./pch.h"

namespace iku{

	namespace os {
	}

    namespace net {
        struct ipv4_t {
            ipv4_t(uint32_t ip){
                this->value = ip;
            }

            std::string toString();

            union{
                struct {
                    uint8_t a, b, c, d;
                };
                uint32_t value;
            };
        };

        struct sockaddr_ipv4 {
            sockaddr_ipv4(): zero{} {}

            std::string toString();

            uint16_t family;
            uint16_t port;
            union {
                uint32_t addr;
                ipv4_t ipv4;
            };
            char zero[8];
        };


        namespace socket {
            using socket_t = uint64_t;
            using port_t = uint16_t;

            socket_t tcp(port_t port);
            socket_t udp(port_t port);
        }


        namespace http {
            using namespace socket;

            using string_pair_t = std::pair<std::string, std::string>;

            struct headers {
                std::vector<string_pair_t> rows;

                string_pair_t* find_by_name(std::string_view name);

                void set(std::string_view name, std::string_view value);
                std::string* get(std::string_view name);

                auto clear();
                auto begin();
                auto end();

                void from(std::string_view buff);
            };

            struct response {
                headers headers;

                std::string version;
                uint16_t status = {};
                std::string message;

                std::vector<uint8_t> body;
            };

            struct request {
                headers headers;

                std::string method;
                std::string url;
                std::string version;

                std::vector<uint8_t> body;


                void from(std::string_view buff);
            };

            using handler = std::function<void(request&, response&)>;

            struct server {
                socket_t sock;

                std::vector<std::pair<std::string, handler>> routes;

                void GET(std::string_view url, const handler& handler);
                void OPTIONS(std::string_view url, const handler& handler);
                void POST(std::string_view url, const handler& handler);

                void start();
                void setListener(socket_t sock);
            };
        }
    }
}