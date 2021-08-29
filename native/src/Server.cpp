#include "Server.h"
#include "Log.h"
#include <iostream>

#define LOG(what) LOG_MODULE("Tcp", what)

void ConHandler::handleRead(const boost_err& err, size_t bytes_transferred) {
	if (err) {
		LOG_ERROR(err.message().c_str());
		socket.close();
	}
	else {
		backend->handleMsg(&buffer);
	}
}
void ConHandler::handleWrite(const boost_err& err, size_t bytes_transferred) {
	if (err) {
		LOG_ERROR(err.message().c_str());
		socket.close();
	}
	else {
		// Socket nach erfolgreichem Senden schließen
		socketBusy = false;
		if (socketClose)
			close();
	}
}

ConHandler::pointer ConHandler::create(Backend* backend, asio::io_service& io_service)
{
	return pointer(new ConHandler(backend, io_service));
}

ConHandler::ConHandler(Backend* backend, asio::io_service& io_service) : buffer(bufferData, serverBufferSize), backend(backend), socket(io_service)
{
}

void ConHandler::read() {
	socket.async_read_some(asio::buffer(buffer.data, backend->getMsgHeaderSize()), bind(&ConHandler::handleRead, this, boost_err_placeholder, boost_bt_placeholder));
}

void ConHandler::send(Buffer* buffer, uint length) {
	socketBusy = true;
	socket.async_write_some(asio::buffer(buffer->data, length), bind(&ConHandler::handleWrite, this, boost_err_placeholder, boost_bt_placeholder));
}

void ConHandler::close() {
	// Schließen evtl. bis zum erfolgreichen Senden der letzten Nachricht verzögern. 
	if (socketBusy)
		socketClose = true;
	else
		socket.close();
}

tcp::socket& ConHandler::getSocket() {
	return socket;
}

u32 ConHandler::getAddress() {
	auto address = socket.remote_endpoint().address().to_v4().to_bytes();
	return *reinterpret_cast<u32*>(address.data());
}

void Server::handleAccept(ConHandler::pointer connection, const boost_err& err) {
	if (err) {
		LOG("Verbindung [remote -> local] wurde abgelehnt");
		LOG_ERROR(err.message().c_str());
	} else {
		LOG("Verbindung [remote -> local] wurde angenommen");
		u32 address = connection->getAddress();
		Client* client = backend->handleConnection(address, &buffer);
		if (client != null) {
			// Verbindung speichern
			connections.emplace(address, connection);
			// Verbindung annehmen
			connection->read();
		}
	}
	accept();
}

void Server::handleConnect(ConHandler::pointer connection, const boost_err& err) {
	if (err) {
		LOG("Verbindung [local -> remote] wurde abgelehnt");
		LOG_ERROR(err.message().c_str());
	} else {
		LOG("Verbindung [local -> remote] wurde angenommen");
		u32 address = connection->getAddress();
		// Verbindung speichern
		connections.emplace(address, connection);
		// Verbindung annehmen
		connection->read();
	}
}

tcp::endpoint Server::createRemoteEndpoint(u32 address) {
	address_v4::bytes_type removeAddress;
	memcpy(removeAddress.data(), &address, IP_SIZE);
	auto endpoint = tcp::endpoint(address_v4(removeAddress), port);
	return endpoint;
}

void Server::accept() {
	LOG("Warte auf Verbindungen");
	ConHandler::pointer connection = ConHandler::create(backend, *io_context);
	acceptor.async_accept(connection->getSocket(), bind(&Server::handleAccept, this, connection, boost_err_placeholder));
}

bool Server::open(Client* client) {
	LOG("Baue Verbindung auf");
	ConHandler::pointer connection = ConHandler::create(backend, *io_context);
	tcp::socket& socket = connection->getSocket();
	auto endpoint = createRemoteEndpoint(client->ip);
	// Verbindung aufbauen
	socket.async_connect(endpoint, bind(&Server::handleConnect, this, connection, boost_err_placeholder));
	return true;
}

bool Server::close(Client* client) {
	LOG("Schließe Verbindung");
	ConHandler::pointer connection = ConHandler::create(backend, *io_context);
	tcp::socket& socket = connection->getSocket();
	auto endpoint = createRemoteEndpoint(client->ip);
	// Verbindung aufbauen
	socket.async_connect(endpoint, bind(&Server::handleConnect, this, connection, boost_err_placeholder));
	return true;
}

bool Server::send(u32 address, Buffer* buffer, uint length) {
	auto found = connections.find(address);
	if (found == connections.end()) {
		return false;
	}
	else {
		ConHandler::pointer connection = found->second;
		connection->send(buffer, length);
		return true;
	}
}

Server::Server(address_v4::bytes_type address, int port, Backend* backend, asio::io_context& io_context)
	: buffer(bufferData, serverBufferSize), backend(backend), port(port), io_context(&io_context), acceptor(io_context) {
	acceptor = tcp::acceptor(io_context, tcp::endpoint(address_v4(address), port));
	// Verbindungen sofort annehmen
	accept();
}
