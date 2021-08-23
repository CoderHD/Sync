#include "Server.h"
#include "Error.h"
#include <iostream>
  
void ConHandler::handleRead(const boost_err& err, size_t bytes_transferred){  
  if (err) {  
    LOG_ERROR(err.message().c_str());
    socket.close();
  }   
  else {  
    std::cout << buffer.data << std::endl;
  }
}  
void ConHandler::handleWrite(const boost_err& err, size_t bytes_transferred){  
  if (err) {  
    LOG_ERROR(err.message().c_str());
    socket.close();
  }   
  else {  
    std::cout << "Test" << std::endl;
  }  
}  

ConHandler::pointer ConHandler::create(Backend *backend, asio::io_service& io_service) 
{
  return pointer(new ConHandler(backend, io_service));
}  

ConHandler::ConHandler(Backend *backend, asio::io_service& io_service): backend(backend), socket(io_service) 
{
}    

void ConHandler::start() {  
  socket.async_read_some(asio::buffer(buffer.data, backend->getMsgHeaderSize()), bind(&ConHandler::handleRead, this, boost_err_placeholder, boost_bt_placeholder));
  //socket.async_write_some(asio::buffer(message, length), bind(&ConHandler::handleWrite, shared_from_this(), boost_err_placeholder, boost_bt_placeholder));
}  

tcp::socket& ConHandler::getSocket() 
{ 
  return socket;
}  

void Server::startAccept() {  
  ConHandler::pointer connection = ConHandler::create(backend, *io_service);  
  acceptor.async_accept(connection->getSocket(), bind(&Server::handleAccept, this, connection, boost_err_placeholder));  
}  

void Server::handleAccept(ConHandler::pointer connection, const boost_err& err){  
  if (!err) {  
    connection->start();  
  }
  startAccept();  
}

bool Server::send(Session *session, Buffer *buffer, uint length) {
  return true;
}

Server::Server(int port, Backend *backend, asio::io_service& io_service): backend(backend), io_service(&io_service), acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
  // Verbindungen sofort annehmen
  startAccept();
}           
