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

void BroadcastServer::read() {
	LOG("Warte auf Packet");
	socket.async_receive_from(
				  asio::buffer(buffer.data, backend->getDiscMsgSize()), remoteEndpoint,
				  boost::bind(&BroadcastServer::handleRead, this, boost_err_placeholder, boost_bt_placeholder)
				  );
}

void BroadcastServer::handleRead(const boost_err& err, size_t bytes_transferred) {
	if (err) {
		LOG("Packet wurden nicht gelesen.");
		LOG_ERROR(err.message().c_str());
		socket.close();
		running = false;
	}
	else {
		LOG("Packet wurden gelesen");
		backend->handleDiscMsg(&buffer);
	}
}

void BroadcastServer::handleWrite(const boost_err& err, size_t bytes_transferred) {
	if (err) {
		LOG("Packet wurde nicht gesendet.");
		LOG_ERROR(err.message().c_str());
		socket.close();
		running = false;
	}
	else {
		LOG("Packet wurde gesendet.");
	}
}

bool BroadcastServer::send(Buffer* buffer, uint length) {
	LOG("Sende Broadcast");
	socket.async_send_to(asio::buffer(buffer->data, length), remoteEndpoint, bind(&BroadcastServer::handleWrite, this, boost_err_placeholder, boost_bt_placeholder));
	return true;
}

bool BroadcastServer::send(Buffer* buffer, uint length, u32 address) {
	LOG("Sende Unicast");
	udp::endpoint endpoint = createRemoteEndpoint(address);
	socket.async_send_to(asio::buffer(buffer->data, length), endpoint, bind(&BroadcastServer::handleWrite, this, boost_err_placeholder, boost_bt_placeholder));
	return true;
}

BroadcastServer::BroadcastServer(int port, Backend* backend, asio::io_context& io_context)
	: buffer(bufferData, broadcastBufferSize), backend(backend), port(port), localEndpoint(address_v4::loopback(), port), remoteEndpoint(address_v4::broadcast(), port), socket(io_context) {
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
		read();
		running = true;
		return true;
	}
}

bool BroadcastServer::isRunning() { return running; }

Buffer* BroadcastServer::getBuffer() {
	return &buffer;
}
