#include "Server.h"
#include <iostream>

ConHandler::pointer ConHandler::create(MsgBackend *backend, asio::io_service& io_service) 
{
  return pointer(new ConHandler(backend, io_service)); 
}  

ConHandler::ConHandler(MsgBackend *backend, asio::io_service& io_service): backend(backend), socket(io_service) 
{
}    

tcp::socket& ConHandler::getSocket() 
{ 
  return socket; 
}  

void ConHandler::start() {  
  socket.async_read_some(asio::buffer(buffer.buffer, backend->getMsgHeaderSize()), bind(&ConHandler::handleRead, shared_from_this(), boost_err_placeholder, boost_bt_placeholder));
  //socket.async_write_some(asio::buffer(message, length), bind(&ConHandler::handleWrite, shared_from_this(), boost_err_placeholder, boost_bt_placeholder));
}  
  
void ConHandler::handleRead(const boost_err& err, size_t bytes_transferred){  
  if (err) {  
    std::cerr << "error: " << err.message() << std::endl;
    socket.close();
  }   
  else {  
    std::cout << buffer.buffer << std::endl;
  }
}  
void ConHandler::handleWrite(const boost_err& err, size_t bytes_transferred){  
  if (!err) {  
    std::cout << "Server sent Hello message!"<< std::endl;  
  }   
  else {  
    std::cerr << "error: " << err.message() << std::endl;  
    socket.close();  
  }  
}  

void Server::startAccept() {  
  // socket  
  ConHandler::pointer connection = ConHandler::create(backend, *io_service);  
  // asynchronous accept operation and wait for a new connection.  
  acceptor.async_accept(connection->getSocket(), bind(&Server::handleAccept, this, connection, boost_err_placeholder));  
}  

Server::Server(MsgBackend *backend, asio::io_service& io_service): backend(backend), io_service(&io_service), acceptor(io_service, tcp::endpoint(tcp::v4(), serverPort)) {
  // Verbindungen sofort annehmen
  startAccept();
}           

void Server::handleAccept(ConHandler::pointer connection, const boost_err& err){  
  if (!err) {  
    connection->start();  
  }
  startAccept();  
}
