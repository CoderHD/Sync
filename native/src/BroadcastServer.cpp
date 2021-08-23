#include "BroadcastServer.h"
#include "Error.h"
#include <iostream>

void BroadcastServer::startRead() {
  socket.async_receive_from(
			    asio::buffer(buffer.data, backend->getDiscMsgSize()), endpoint,
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

void BroadcastServer::handleWrite(const boost_err &err, size_t bytes_transferred) {
  if(err) {
    LOG_ERROR(err.message().c_str());
    socket.close();
    running = false;
  }
}

bool BroadcastServer::send(Buffer *buffer, uint length) {
  socket.async_send(asio::buffer(buffer, length), bind(&BroadcastServer::handleWrite, this, boost_err_placeholder, boost_bt_placeholder));
  return true;
}


BroadcastServer::BroadcastServer(int port, Backend *backend, asio::io_context& io_context) 
    : buffer(bufferData, msgBufferSize), backend(backend), endpoint(asio::ip::address_v4::any(), port), socket(io_context) {
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

bool BroadcastServer::isRunning() { return running; }
