#include "Types.h"
#include "Buffer.h"
#include "Log.h"
#include "HashTable.h"
#include "boost/asio.hpp"

using namespace boost;
using namespace boost::asio::ip;
using namespace Sync;

namespace Sync {
	void live_stream_init(address_v4::bytes_type& addr, u16 port, asio::io_context& io_context);
	u32 stream_accept();
	err stream_context(u32 address);
	err stream_open();
	err stream_close();
	err stream_write(Buffer* buffer, u32 length);
	err stream_read(Buffer* buffer, u32 length);
	u32 stream_read_available();
}

struct ConHandler
{
	tcp::socket socket;

	ConHandler(asio::io_context& io_context) : socket(io_context) {}

	err write(Buffer* buff, u32 length) {
		system::error_code err;
		socket.write_some(asio::buffer(buff->buff, length), err);
		DYNAMIC_ASSERT(!err);
		return err ? ERROR_UNKNOWN : SUCCESS;
	}

	err read(Buffer* buff, u32 length) {
		system::error_code err;
		boost::asio::read(socket, asio::buffer(buff->buff + buff->i, length), asio::transfer_exactly(length), err);
		//socket.read_some(asio::buffer(buff->buff + buff->i, length), asio::transfer_exactly(length), err);
		DYNAMIC_ASSERT(!err);
		return err ? ERROR_UNKNOWN : SUCCESS;
	}

	u32 read_available() {
		return socket.available();
	}

	err close() {
		system::error_code err;
		socket.close(err);
		DYNAMIC_ASSERT(!err);
		return err ? ERROR_UNKNOWN : SUCCESS;
	}
};

struct StreamServer {
	int port;

	asio::io_context& io_context;
	tcp::acceptor acceptor;

	u32 contextAddr;
	tcp::endpoint contextEndpoint;
	ConHandler* contextConnection;

	HashTableOpenAddress<u32, ConHandler*, 32> connections;

	StreamServer(address_v4::bytes_type address, int port, asio::io_context& io_context) : port(port), io_context(io_context), acceptor(io_context) {
		tcp::endpoint acceptor_endp = tcp::endpoint(address_v4::any(), port);
		acceptor = tcp::acceptor(io_context, acceptor_endp.protocol());
		acceptor.bind(acceptor_endp);
		acceptor.listen();
	}
};

StreamServer* server;

void Sync::live_stream_init(address_v4::bytes_type& addr, u16 port, asio::io_context& io_context) {
	server = new StreamServer(addr, port, io_context);
}

tcp::endpoint createRemoteEndpoint(u32 address, u32 port) {
	address_v4::bytes_type removeAddress;
	memcpy(removeAddress.data(), &address, sizeof(u32));
	return tcp::endpoint(address_v4(removeAddress), port);
}

u32 Sync::stream_accept() {
	system::error_code err;
	ConHandler* con = new ConHandler(server->io_context);
	server->acceptor.accept(con->socket, err);
	if (err) return 0;
	else {
		// save connection
		u32 addr = con->socket.remote_endpoint().address().to_v4().to_uint();
		server->connections.add(addr, con);
		// enable connection
		stream_context(addr);
		return addr;
	}
}

err Sync::stream_context(u32 addr) {
	server->contextAddr = addr;
	server->contextEndpoint = createRemoteEndpoint(addr, server->port);
	ConHandler** con = server->connections.find(addr);
	if (con) {
		server->contextConnection = *con;
		return SUCCESS;
	}
	else {
		return ERROR_UNKNOWN;
	}
}

err Sync::stream_open() {
	system::error_code err;
	ConHandler* con = new ConHandler(server->io_context);
	con->socket.connect(server->contextEndpoint, err);
	if (err) return ERROR_UNKNOWN;
	else {
		// save connection
		server->connections.add(server->contextAddr, con);
		return SUCCESS;
	}
}

err Sync::stream_close() {
	server->contextConnection->socket.close();
	delete server->contextConnection;
	return SUCCESS;
}

err Sync::stream_write(Buffer* buff, u32 len) {
	NULL_FALSE(server->contextConnection);
	return server->contextConnection->write(buff, len);
}

err Sync::stream_read(Buffer* buff, u32 len) {
	bool success = server->contextConnection->read(buff, len);
	return !success;
}

u32 Sync::stream_read_available() {
	DYNAMIC_ASSERT(server->contextConnection);
	return server->contextConnection->read_available();
}