#include "BroadcastServer.h"
#include "Server.h"
#include "Log.h"
#include <iostream>

#define LOG(what) LOG_MODULE("Udp", what)

udp::endpoint BroadcastServer::createRemoteEndpoint(u32 address) {
	address_v4::bytes_type removeAddress;
	memcpy(removeAddress.data(), &address, IP_SIZE);
	auto endpoint = udp::endpoint(address_v4(removeAddress), port);
	return endpoint;
}

bool BroadcastServer::writeBroadcast(Buffer* buffer, uint length) {
	LOG("Sende Broadcast");
	uint bytes = socket.send_to(asio::buffer(buffer->data, length), remoteEndpoint);
	return bytes == length;
}

bool BroadcastServer::writeUnicast(u32 address, Buffer* buffer, uint length) {
	LOG("Sende Unicast");
	udp::endpoint endpoint = createRemoteEndpoint(address);
	uint bytes = socket.send_to(asio::buffer(buffer->data, length), endpoint);
	return bytes == length;
}

bool BroadcastServer::read(Buffer* buffer, uint length) {
	uint bytes = socket.receive_from(asio::buffer(buffer->data, backend->getDiscMsgSize()), remoteEndpoint);
	return bytes == length;
}

BroadcastServer::BroadcastServer(int port, Backend* backend, ExecutorPool* pool, asio::io_context& io_context)
	: bufferMng(broadcastBufferSize), backend(backend), pool(pool), port(port), localEndpoint(address_v4::loopback(), port), remoteEndpoint(address_v4::broadcast(), port), socket(io_context) {
	initSocket();
}

bool BroadcastServer::initSocket() {
	socket.open(localEndpoint.protocol(), error);

	if (error) {
		LOG("Socket nicht initialisiert");
		LOG_ERROR(error.message().c_str());
		running = false;
		return false;
	}
	else {
		LOG("Socket initialisiert");
		socket.set_option(udp::socket::reuse_address(true));
		socket.bind(localEndpoint);
		socket.set_option(asio::socket_base::broadcast(true));
		pool->execute([this]() {
			backend->executeDiscovery();
		});
		running = true;
		return true;
	}
}

bool BroadcastServer::isRunning() { return running; }
