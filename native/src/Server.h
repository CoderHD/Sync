#pragma once
#include "Types.h"
#include "Backend.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"

using namespace boost;
using namespace boost::asio::ip;
using boost_err = boost::system::error_code;
#define boost_err_placeholder asio::placeholders::error
#define boost_bt_placeholder asio::placeholders::bytes_transferred

class ConHandler// : public enable_shared_from_this<ConHandler>  
{  
private:
  Buffer buffer;
  Backend *backend;
  tcp::socket socket;

  void handleRead(const boost_err& err, size_t bytes_transferred);
  void handleWrite(const boost_err& err, size_t bytes_transferred); 
public:  
  using pointer = boost::shared_ptr<ConHandler>;  
 
  static pointer create(Backend *backend, asio::io_service& io_service);  
  ConHandler(Backend *backend, asio::io_service& io_service);
  void start();

  tcp::socket& getSocket();
};

class Server : public ServerInterface {  
private:  
  Backend *backend;
  asio::io_service *io_service;
  tcp::acceptor acceptor;
  ConHandler::pointer connection;
  
  void startAccept();
  void handleAccept(ConHandler::pointer connection, const boost_err& err);
  bool send(Session *session, Buffer* buffer, uint length) override;
public:  
  // constructor for accepting connection from client  
  Server(int port, Backend *backend, asio::io_service& io_service);
};  
