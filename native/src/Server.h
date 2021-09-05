#pragma once
#include "Types.h"
#include "Backend.h"
#include "Pool.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"
#include <unordered_map>
#include <vector>
#include <thread>

using namespace boost;
using namespace boost::asio::ip;
using boost_err = boost::system::error_code;
#define boost_err_placeholder asio::placeholders::error
#define boost_bt_placeholder asio::placeholders::bytes_transferred

constexpr uint serverBufferSize = 2048;

class ConHandler// : public enable_shared_from_this<ConHandler>  
{
	Backend* backend;
	tcp::socket socket;

public:
	using pointer = boost::shared_ptr<ConHandler>;

	static pointer create(Backend* backend, asio::io_service& io_service);
	ConHandler(Backend* backend, asio::io_service& io_service);
	void run(ExecutorPool* pool);
	bool read(Buffer* buffer, uint length);
	bool write(Buffer* buffer, uint length);
	bool close();

	tcp::socket& getSocket();
	u32 getAddress();
};

class Server : public StreamServerInterface {
	BufferMng bufferMng;
	Backend* backend;
	ExecutorPool* pool;
	int port;
	asio::io_context* io_context;
	tcp::acceptor acceptor;
	std::unordered_map<u32, ConHandler::pointer> connections;

	void handleAccept(ConHandler::pointer connection);
	void handleConnect(ConHandler::pointer connection);

	tcp::endpoint createRemoteEndpoint(u32 address);
	bool accept();
	bool open(Client* client) override;
	bool close(Client* client) override;
	bool write(u32 address, Buffer* buffer, uint length) override;
	bool read(u32 address, Buffer* buffer, uint length) override;

public:
	// constructor for accepting connection from client  
	Server(address_v4::bytes_type address, int port, Backend* backend, ExecutorPool* pool, asio::io_context& io_context);
};
