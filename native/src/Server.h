#pragma once
#include "Types.h"
#include "Backend.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"
#include <unordered_map>

using namespace boost;
using namespace boost::asio::ip;
using boost_err = boost::system::error_code;
#define boost_err_placeholder asio::placeholders::error
#define boost_bt_placeholder asio::placeholders::bytes_transferred

constexpr uint serverBufferSize = 2048;

class ConHandler// : public enable_shared_from_this<ConHandler>  
{
private:
	u8 bufferData[serverBufferSize];
	Buffer buffer;
	Backend* backend;
	tcp::socket socket;
	bool socketBusy, socketClose;

	void handleRead(const boost_err& err, size_t bytes_transferred);
	void handleWrite(const boost_err& err, size_t bytes_transferred);
public:
	using pointer = boost::shared_ptr<ConHandler>;

	static pointer create(Backend* backend, asio::io_service& io_service);
	ConHandler(Backend* backend, asio::io_service& io_service);
	void read();
	void send(Buffer* buffer, uint length);
	void close();

	tcp::socket& getSocket();
	u32 getAddress();
};

class Server : public StreamServerInterface {
private:
	u8 bufferData[serverBufferSize];
	Buffer buffer;
	Backend* backend;
	int port;
	asio::io_context* io_context;
	tcp::acceptor acceptor;
	std::unordered_map<u32, ConHandler::pointer> connections;

	void handleAccept(ConHandler::pointer connection, const boost_err& err);
	void handleConnect(ConHandler::pointer connection, const boost_err& err);

	tcp::endpoint createRemoteEndpoint(u32 address);
	void accept();
	bool open(Client* client) override;
	bool close(Client* client) override;
	bool send(u32 address, Buffer* buffer, uint length) override;
public:
	// constructor for accepting connection from client  
	Server(address_v4::bytes_type address, int port, Backend* backend, asio::io_context& io_context);
};
