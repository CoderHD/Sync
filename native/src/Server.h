#pragma once
#include "Types.h"
#include "MsgBackend.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"

using namespace boost;
using namespace boost::asio::ip;
using boost_err = boost::system::error_code;
#define boost_err_placeholder asio::placeholders::error
#define boost_bt_placeholder asio::placeholders::bytes_transferred

constexpr int serverPort = 2000;

class ConHandler : public enable_shared_from_this<ConHandler>  
{  
private:
  MsgBuffer buffer;
  MsgBackend *backend;
  tcp::socket socket;

public:  
  using pointer = boost::shared_ptr<ConHandler>;  
  
  // creating the pointer  
  static pointer create(MsgBackend *backend, asio::io_service& io_service);
  //socket creation  
  tcp::socket& getSocket();
  
  ConHandler(MsgBackend *backend, asio::io_service& io_service);
  
  void start();
  void handleRead(const boost_err& err, size_t bytes_transferred);
  void handleWrite(const boost_err& err, size_t bytes_transferred); 
};

class Server {  
private:  
  MsgBackend *backend;
  asio::io_service *io_service;
  tcp::acceptor acceptor;
  
  void startAccept();
public:  
  // constructor for accepting connection from client  
  Server(MsgBackend *backend, asio::io_service& io_service);
  void handleAccept(ConHandler::pointer connection, const boost_err& err);
};  
