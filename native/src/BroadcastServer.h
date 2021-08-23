#pragma once
#include "Types.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "MsgBackend.h"

using namespace boost;
using namespace boost::asio::ip;
using boost_err = boost::system::error_code;
#define boost_err_placeholder asio::placeholders::error
#define boost_bt_placeholder asio::placeholders::bytes_transferred

class BroadcastServer// : public boost::enable_shared_from_this<BroadcastServer>
{
 private:
  MsgBuffer buffer;
  MsgBackend *backend;
  system::error_code error;
  asio::ip::udp::endpoint endpoint;
  asio::ip::udp::socket socket;
  bool running;
  
  void startRead();
  void handleRead(const boost_err &err, size_t bytes_transferred);
  void handleWrite(const boost_err &err, size_t bytes_transferred);
 public: 
  BroadcastServer(int port, MsgBackend *backend, asio::io_service& io_service);
  bool initSocket();
  bool send(BufferedMsg *msg);
  bool isRunning();
};
