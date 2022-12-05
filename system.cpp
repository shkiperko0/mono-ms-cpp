#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#include "./system.h"

class _WSA_DATA {
	WSADATA wsaData;

public:
	_WSA_DATA(uint16_t version) {
		auto res = WSAStartup(version, &wsaData);
		if (res) {
			std::cout << "WSAStartup error " << res << std::endl;
			exit(-1);
		}
	}

	~_WSA_DATA() {
		auto res = WSACleanup();
		if (res) {
			std::cout << "WSACleanup error " << res << std::endl;
			exit(-1);
		}
	}
};

_WSA_DATA _g_WSA_DATA(0x202);

namespace iku::net {
	std::string sockaddr_ipv4::toString() {
		ipv4_t ip = ntohl(addr);
		return std::format("{}.{}.{}.{}:{}", ip.d, ip.c, ip.b, ip.a, ntohs(port));
	}

	std::string ipv4_t::toString() {
		return std::format("{}.{}.{}.{}", d, c, b, a);
	}
}

namespace iku::net::socket {

	void bind(socket_t sock, port_t port) {
		sockaddr_ipv4 addr = {};
		addr.port = htons(port);
		addr.family = AF_INET;
		addr.addr = htonl(INADDR_LOOPBACK);

		auto result = ::bind(sock, (sockaddr*)&addr, sizeof addr);
		if (result == SOCKET_ERROR) {
			auto error = std::format("can`t bind socket to {}\nerror {}", addr.toString(), WSAGetLastError());
			throw std::exception(error.c_str());
		}

		std::cout << std::format("socket binded to {}", addr.toString()) << std::endl;
	}

	socket_t tcp(port_t port) {
		socket_t sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET) {
			auto error = std::format("can`t create tcp socket\nerror {}", port, WSAGetLastError());
			throw std::exception(error.c_str());
		}

		bind(sock, port);
		return sock;
	}

	size_t recv(socket_t sock, void* buff, size_t size, uint32_t flags = 0) {
		auto result = ::recv(sock, (char*)buff, size, flags);

		if (result == SOCKET_ERROR) {
			auto error = std::format("socket recv\nerror {}", WSAGetLastError());
			throw std::exception(error.c_str());
		}

		return result;
	}

	size_t send(socket_t sock, const void* buff, size_t size, uint32_t flags = 0) {
		auto result = ::send(sock, (char*)buff, size, flags);

		if (result == SOCKET_ERROR) {
			auto error = std::format("socket send\nerror {}", WSAGetLastError());
			throw std::exception(error.c_str());
		}

		return result;
	}

	socket_t udp(port_t port) {
		SOCKET raw_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_UDP);
		if (raw_sock == INVALID_SOCKET) {
			auto error = std::format("can`t create udp socket\nerror {}", port, WSAGetLastError());
			throw std::exception(error.c_str());
		}

		socket_t sock = (socket_t)raw_sock;
		bind(sock, port);
		return sock;
	}

	void listen(socket_t sock) {
		auto result = ::listen(sock, SOMAXCONN);
		if (result == SOCKET_ERROR) {
			auto error = std::format("listen socket\nerror {}", WSAGetLastError());
			throw std::exception(error.c_str());
		}
	}

	socket_t accept(socket_t sock, sockaddr_ipv4* addr = nullptr) {
		sockaddr_ipv4 maddr;
		int len = sizeof(sockaddr_ipv4);


		auto client = ::accept(sock, (sockaddr*)&maddr, &len);
		if (client == INVALID_SOCKET) {
			auto error = std::format("accept socket\nerror {}", WSAGetLastError());
			throw std::exception(error.c_str());
		}

		if (addr) {
			memcpy(addr, &maddr, sizeof(sockaddr_ipv4));
		}

		std::cout << std::format("socket {} accepted", maddr.toString()) << std::endl;
		return client;
	}

	std::string socket_ip_port(socket_t sock) {
		sockaddr_ipv4 addr = {};
		int len = {};
		auto result = ::getpeername(sock, (sockaddr*)&addr, &len);
		if (result == SOCKET_ERROR) {
			auto error = std::format("get peer name\nerror {}", WSAGetLastError());
			throw std::exception(error.c_str());
		}
		return addr.toString();
	}

	// iku::net::socket
}





namespace iku::net {
	using namespace socket;


}

std::vector<std::string_view> split(const std::string_view str, const std::string_view delim = ","){

	size_t offset = 0;
	size_t loffset = 0;
	size_t find = 0;
	offset = str.find(delim, offset);
	if (offset == std::string_view::npos) {
		return {str};
	}

	std::vector<std::string_view> result;
	result.reserve(30);

	do{
		std::string_view column(str.data() + loffset, str.data() + offset);
		if(column.length()) result.push_back(column);

		find = str.find(delim, offset + 2);
		loffset = offset + 2;
		offset = find;

	} while (find != std::string_view::npos);

	result.push_back({ str.data() + loffset, str.data() + str.length() });
	return result;
}

auto http_status_from_code(uint16_t code){
	switch (code) {
	case 200: return "Ok";
	case 404: return "Not found";
	}
	return "undefined";
}

namespace iku::net::http {
	using namespace socket;

	auto headers::clear() {
		return rows.clear();
	}

	auto headers::begin() {
		return rows.begin();
	}

	auto headers::end() {
		return rows.end();
	}

	void headers::set(std::string_view name, std::string_view value) {
		auto header = get(name);
		if (header) {
			header->assign(value.begin(), value.end());
		}
		else {
			rows.push_back({ 
				{ name.begin(), name.end() }, 
				{ value.begin(), value.end() }
			});
		}
	}

	string_pair_t* headers::find_by_name(std::string_view name) {
		auto c = rows.begin();
		auto f = rows.end();

		while (c != f) {
			auto& row = *c;
			if (row.first == name) {
				return &row;
			}
			c++;
		}

		return nullptr;
	}

	std::string* headers::get(std::string_view name) {
		auto row = find_by_name(name);
		if (row) {
			return &row->second;
		}
		return nullptr;
	}

	void headers::from(std::string_view buff) {
		auto rows = split(buff, "\r\n");
		this->rows.reserve(rows.size() + rows.size());
		for (auto& row : rows) {
			auto mid = row.find(": ");
			if (mid != std::string_view::npos) {
				this->rows.push_back({
					{row.data(), row.data() + mid},
					{row.data() + mid + 2, row.data() + row.length()}
				});
			}
		}
	}

	void request::from(std::string_view buff) {
		std::stringstream ss(buff.data());
		ss >> method >> url >> version;

		headers.from({ buff.data() + ss.tellg() + 2, buff.data() + buff.length()});
	}

	void server::setListener(socket_t sock) {
		this->sock = sock;
	}

	void server::GET(std::string_view url, const handler& handler) {
		routes.push_back({ url.data(), handler});
	}

	void server::OPTIONS(std::string_view url, const handler& handler) {

	}

	void server::POST(std::string_view url, const handler& handler) {

	}

	void onAccept(socket_t cl, server& server) {

		char buff[50];
		std::vector<char> header_buff;
		//std::vector<uint8_t> body_buff;
		header_buff.reserve(2048);
		size_t offset = 0;

		request req;

		for (;;) {
			auto len = recv(cl, buff, sizeof buff);
			std::string_view part{ buff, buff + len };
			offset = part.find("\r\n\r\n");
			if (offset == std::string_view::npos) {
				header_buff.insert(header_buff.end(), part.begin(), part.end());
				continue;
			}

			header_buff.insert(header_buff.end(), part.data(), part.data() + offset);
			break;
		}

		req.from({ header_buff.begin(), header_buff.end() });

		std::cout << '[' << req.method << ':' << req.version << "] << " << req.url << std::endl;

		for (auto& header : req.headers) {
			std::cout << '[' << header.first << "]: " << header.second << std::endl;
		}

		response res;
		handler hdl = nullptr;

		for (auto& route : server.routes) {
			if (route.first == req.url) {
				hdl = route.second;
				break;
			}
		}

		if (hdl) {
			try{
				hdl(req, res);
			}
			catch (const std::logic_error& ex) {
				std::string_view error = ex.what();
				res.status = 500;
				res.message = "Slu4ilos` vot eto vot";
				res.body.assign(error.begin(), error.end());
			}
			catch (const std::exception& ex) {
				std::string_view error = ex.what();
				res.status = 500;
				res.message = "Slu4ilos` vot eto vot";
				res.body.assign(error.begin(), error.end());
			}
			catch (...) {
				std::string_view error = "Nu tut sovsem pizda";
				res.status = 666;
				res.message = "Hell yeah";
				res.body.assign(error.begin(), error.end());
			}
		}

		else {
			res.status = 404;
			res.message = "Hz gde ono";
		}

		if (res.body.size()) {
			res.headers.set("Content-Length", std::format("{}", res.body.size()));
			auto header = res.headers.get("Content-Type");
			if (!header) {
				res.headers.set("Content-Type", "text/plain");
			}
		}

		if (!res.status) {
			res.status = 200;
		}

		if (!res.message.length()) {
			res.message = http_status_from_code(res.status);
		}

		if (!res.version.length()) {
			res.version = req.version;
		}
		

		std::string line;
		line.reserve(300);

		line = std::format("{} {} {}\r\n", res.version, res.status, res.message);
		send(cl, line.data(), line.length());

		for (auto& header : res.headers) {
			line = std::format("{}: {}\r\n", header.first, header.second);
			send(cl, line.data(), line.length());
		}

		send(cl, "\r\n", 2);

		if(res.body.size()){
			send(cl, res.body.data(), res.body.size());
		}

		closesocket(cl);
		return;
	}

	void server::start() {
		using namespace std;
		listen(sock);

	_accept:
		try {
			auto cl = accept(sock);
			onAccept(cl, *this);
		}
		catch (const exception& ex) {
			cout << ex.what() << endl;
		}
		catch (...) {
			cout << "accept err" << endl;
		}
		goto _accept;
	}

	// iku::net::http
}