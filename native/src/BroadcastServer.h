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

constexpr int serverBroadcastPort = 2000;

class BroadcastServer : public enable_shared_from_this<BroadcastServer>
{
private:
  MsgBuffer buffer;
  MsgBackend *backend;
  asio::ip::udp::socket socket;
  asio::ip::udp::endpoint endpoint;
  bool running;
  
public: 
  BroadcastServer(MsgBackend *backend, asio::io_service& io_service);
  void startRead();
  void handleRead(const boost_err& err, size_t bytes_transferred);
};
