#include "BroadcastServer.h"
#include "Error.h"
#include <iostream>

void BroadcastServer::startRead() {
  socket.async_receive_from(
			    asio::buffer(buffer.buffer, backend->getDiscMsgSize()), endpoint,
			    boost::bind(&BroadcastServer::handleRead, this, boost_err_placeholder, boost_bt_placeholder)
			    );
}

void BroadcastServer::handleRead(const boost_err& err, size_t bytes_transferred) {  
  if(err) {
    LOG_ERROR(err.message().c_str());
    socket.close();
    running = false;
  }   
  else {
    backend->handleDiscMsg(&buffer);
  }  
}  

void handleWrite(const boost_err &err, size_t bytes_transferred) {
  if(err) {
    LOG_ERROR(err.message().c_str());
    socket.close();
    running = false;
  }
}

BroadcastServer::BroadcastServer(int port, MsgBackend *backend, asio::io_context& io_context)
  : backend(backend), endpoint(asio::ip::address_v4::any(), port), socket(io_context) {
    std::cout << asio::ip::address_v4::any() << std::endl;
    std::cout << asio::ip::address_v4::broadcast() << std::endl;
  initSocket();
}

bool BroadcastServer::initSocket() {
  socket.open(endpoint.protocol(), error);

  if(error) {
    LOG_ERROR("BroadcastServer Socket wurde nicht geöffnet.");
    running = false;
    return false;
  } else {
    socket.set_option(asio::ip::udp::socket::reuse_address(true));
    socket.set_option(asio::socket_base::broadcast(true));
    socket.bind(endpoint);
    startRead();
    running = true;
    return true;
  }
}

bool send(BufferedMsg *msg) {
  socket.async_write_some(
			  asio::buffer(msg->buffer, msg->length),
			  bind(&BroadcastServer::handleWrite, this, boost_err_placeholder, boost_bt_placeholder)
			  );
}


bool BroadcastServer::isRunning() { return running; }
