#include "BroadcastServer.h"
#include <iostream>

BroadcastServer::BroadcastServer(MsgBackend *backend, asio::io_service& io_service) : backend(backend), socket(io_service), endpoint(asio::ip::address_v4::broadcast(), serverBroadcastPort) {
  boost::system::error_code error;
  socket.open(asio::ip::udp::v4(), error);
  if(error) {
    running = true;
  } else {
    socket.set_option(asio::ip::udp::socket::reuse_address(true));
    socket.set_option(asio::socket_base::broadcast(true));
    running = false;
  }
  startRead();
}

void BroadcastServer::startRead() {
  socket.async_receive(
    asio::buffer(buffer.buffer, backend->getDiscMsgSize()), 
    boost::bind(&BroadcastServer::handleRead, shared_from_this(), boost_err_placeholder, boost_bt_placeholder)
  );
}

void BroadcastServer::handleRead(const boost_err& err, size_t bytes_transferred) {  
  if (!err) {
    backend->handleDiscMsg(&buffer); 
  }   
  else {  
    std::cerr << "error: " << err.message() << std::endl;  
    socket.close();  
  }  
}  
