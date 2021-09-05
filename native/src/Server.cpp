#include "Server.h"
#include "Log.h"
#include <iostream>

#define LOG(what) LOG_MODULE("Tcp", what)

ConHandler::pointer ConHandler::create(Backend* backend, asio::io_service& io_service)
{
	return pointer(new ConHandler(backend, io_service));
}

ConHandler::ConHandler(Backend* backend, asio::io_service& io_service) : backend(backend), socket(io_service)
{
}

void ConHandler::run(ExecutorPool* pool) {
	pool->execute([this]() {
		backend->executeConnection(getAddress());
	});
}

bool ConHandler::read(Buffer* buffer, uint length) {
	boost_err err;
	socket.read_some(asio::buffer(buffer->data, backend->getMsgHeaderSize()), err);
	LOG_ERROR_IF(err, err.message().c_str());
	return !err;
}

bool ConHandler::write(Buffer* buffer, uint length) {
	boost_err err;
	socket.write_some(asio::buffer(buffer->data, length), err);
	LOG_ERROR_IF(err, err.message().c_str());
	return !err;
}

bool ConHandler::close() {
	boost_err err;
	socket.close(err);
	LOG_ERROR_IF(err, err.message().c_str());
	return !err;
}

tcp::socket& ConHandler::getSocket() {
	return socket;
}

u32 ConHandler::getAddress() {
	auto address = socket.remote_endpoint().address().to_v4().to_bytes();
	return *reinterpret_cast<u32*>(address.data());
}

void Server::handleAccept(ConHandler::pointer connection) {
	LOG("Verbindung [remote -> local] wurde angenommen");
	u32 address = connection->getAddress();
	// Verbindung speichern
	connections.emplace(address, connection);
	// Verbindung starten
	connection->run(pool);
}

void Server::handleConnect(ConHandler::pointer connection) {
	LOG("Verbindung [local -> remote] wurde angenommen");
	u32 address = connection->getAddress();
	// Verbindung speichern
	connections.emplace(address, connection);
	// Verbindung starten
	connection->run(pool);
}

tcp::endpoint Server::createRemoteEndpoint(u32 address) {
	address_v4::bytes_type removeAddress;
	memcpy(removeAddress.data(), &address, IP_SIZE);
	auto endpoint = tcp::endpoint(address_v4(removeAddress), port);
	return endpoint;
}

bool Server::accept() {
	ConHandler::pointer connection = ConHandler::create(backend, *io_context);
	// Verbindung annehmen
	boost_err err;
	acceptor.accept(connection->getSocket(), err);

	if (err) {
		LOG("Verbindung [remote -> local] wurde abgelehnt");
		LOG_ERROR(err.message().c_str());
		return false;
	}
	else {
		handleAccept(connection);
		return true;
	}
}

bool Server::open(Client* client) {
	ConHandler::pointer connection = ConHandler::create(backend, *io_context);
	// Verbindung aufbauen
	boost_err err;
	connection->getSocket().connect(createRemoteEndpoint(client->address), err);

	if (err) {
		LOG("Verbindung [local -> remote] wurde abgelehnt");
		LOG_ERROR(err.message().c_str());
		return false;
	}
	else {
		handleConnect(connection);
		return true;
	}
}

bool Server::close(Client* client) {
	ConHandler::pointer connection = ConHandler::create(backend, *io_context);
	tcp::socket& socket = connection->getSocket();
	auto endpoint = createRemoteEndpoint(client->address);
	// Verbindung aufbauen
	boost_err err;
	socket.connect(endpoint, err);
	LOG_ERROR_IF(err, err.message().c_str());
	return !err;
}

bool Server::write(u32 address, Buffer* buffer, uint length) {
	auto found = connections.find(address);
	if (found == connections.end()) return false;
	else {
		ConHandler::pointer connection = found->second;
		return connection->write(buffer, length);
	}
}

bool Server::read(u32 address, Buffer* buffer, uint length) {
	auto found = connections.find(address);
	if (found == connections.end()) return false;
	else {
		ConHandler::pointer connection = found->second;
		return connection->read(buffer, length);
	}
}

Server::Server(address_v4::bytes_type address, int port, Backend* backend, ExecutorPool* pool, asio::io_context& io_context)
	: bufferMng(serverBufferSize), backend(backend), pool(pool), port(port), io_context(&io_context), acceptor(io_context) {
	acceptor = tcp::acceptor(io_context, tcp::endpoint(address_v4(address), port));
	// Verbindungen sofort annehmen
	accept();
}
