#include "Types.h"
#include "Buffer.h"
#include "Log.h"
#include "boost/asio.hpp"

using namespace boost;
using namespace boost::asio::ip;
using namespace Sync;

namespace Sync {
	void live_packet_init(address_v4& addr, address_v4& mask, u16 port, asio::io_context& io_context);
	err packet_write_broadcast(Buffer* buff, u32 len);
	err packet_write_unicast(u32 addr, Buffer* buff, u32 len);
	u32 packet_read_available();
	err packet_read(Buffer* buff, u32 len);
};

struct DatagramServer
{
	int port;
	udp::endpoint localEndpoint, remoteEndpoint;
	udp::socket socket, broadcastSocket;
	bool running;

	DatagramServer(int port, address_v4 address, address_v4 mask, asio::io_context& io_context)
		: port(port), localEndpoint(address, port), remoteEndpoint(address_v4::broadcast(address, mask), port),
		socket(io_context), broadcastSocket(io_context) {
		initSocket();
	}

	bool initSocket() {
		system::error_code err;

		// open udp socket for unicasting
		socket.open(udp::v4(), err);
		DYNAMIC_ASSERT(!err);
		if (err) {
			LOG_ERROR(err.message().c_str());
			running = false;
			return false;
		}

		// open udp socket for broadcasting
		broadcastSocket.open(udp::v4(), err);
		DYNAMIC_ASSERT(!err);
		if (err) {
			LOG_ERROR(err.message().c_str());
			running = false;
			return false;
		}

		// set socket options
		socket.set_option(udp::socket::reuse_address(true));
		socket.bind(localEndpoint);
		broadcastSocket.set_option(udp::socket::reuse_address(true));
		broadcastSocket.set_option(asio::socket_base::broadcast(true));
		running = true;
		return true;
	}
};

DatagramServer* server;

void Sync::live_packet_init(address_v4& addr, address_v4& mask, u16 port, asio::io_context& io_context) {
	server = new DatagramServer(port, addr, mask, io_context);
}

udp::endpoint createRemoteEndpoint(u32 address, u32 port) {
	address_v4::bytes_type removeAddress;
	memcpy(removeAddress.data(), &address, sizeof(u32));
	return udp::endpoint(address_v4(removeAddress), port);
}

err Sync::packet_write_broadcast(Buffer* buff, u32 len) {
	log_info("wrote broadcast to %s", server->remoteEndpoint.address().to_string().c_str());
	system::error_code err;
	u32 bytes = server->broadcastSocket.send_to(asio::buffer(buff->buff, len), server->remoteEndpoint, 0, err);
	if (err) return ERROR_UNKNOWN;
	else     return bytes != len;
}

err Sync::packet_write_unicast(u32 address, Buffer* buff, u32 len) {
	udp::endpoint endpoint = createRemoteEndpoint(address, server->port);
	log_info("wrote unicast to %s", endpoint.address().to_string().c_str());
	system::error_code err;
	u32 bytes = server->socket.send_to(asio::buffer(buff->buff, len), endpoint, 0, err);
	if (err) return ERROR_UNKNOWN;
	else     return bytes != len;
}

u32 Sync::packet_read_available() {
	return server->socket.available();
}

err Sync::packet_read(Buffer* buff, u32 len) {
	system::error_code err;
	u32 bytes = server->socket.receive_from(asio::buffer(buff->buff + buff->i, len), server->localEndpoint, 0, err);
	return err ? ERROR_UNKNOWN : SUCCESS;
}