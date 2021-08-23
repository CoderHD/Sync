#pragma once
#include "Types.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "Backend.h"

using namespace boost;
using namespace boost::asio::ip;
using boost_err = boost::system::error_code;
#define boost_err_placeholder asio::placeholders::error
#define boost_bt_placeholder asio::placeholders::bytes_transferred

constexpr uint msgBufferSize = 1024;

class BroadcastServer : public BroadcastServerInterface // : public boost::enable_shared_from_this<BroadcastServer>
{
 private:
  char bufferData[msgBufferSize];
  Buffer buffer;
  Backend *backend;
  system::error_code error;
  asio::ip::udp::endpoint endpoint;
  asio::ip::udp::socket socket;
  bool running;
  
  void startRead();
  void handleRead(const boost_err &err, size_t bytes_transferred);
  void handleWrite(const boost_err &err, size_t bytes_transferred);
  bool send(Buffer *buffer, uint length) override;
  
 public: 
  BroadcastServer(int port, Backend *backend, asio::io_context& io_context);
  bool initSocket();
  bool isRunning();
};
