#pragma once
#include "Types.h"
#include "Pool.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "Backend.h"

using namespace boost;
using namespace boost::asio::ip;
using boost_err = boost::system::error_code;
#define boost_err_placeholder asio::placeholders::error
#define boost_bt_placeholder asio::placeholders::bytes_transferred

constexpr uint broadcastBufferSize = 1024;

class BroadcastServer : public PacketServerInterface // : public boost::enable_shared_from_this<BroadcastServer>
{
private:
	BufferMng bufferMng;
	Backend* backend;
	ExecutorPool* pool;
	int port;
	system::error_code error;
	udp::endpoint localEndpoint, remoteEndpoint;
	udp::socket socket;
	bool running;

	udp::endpoint createRemoteEndpoint(u32 address);
	bool writeBroadcast(Buffer* buffer, uint length) override;
	bool writeUnicast(u32 address, Buffer* buffer, uint length) override;
	bool read(Buffer* buffer, uint length) override;

public:
	BroadcastServer(int port, Backend* backend, ExecutorPool* pool, asio::io_context& io_context);
	bool initSocket();
	bool isRunning();
};
