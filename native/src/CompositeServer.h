#include "boost/asio.hpp"
#include "BroadcastServer.h"
#include "Server.h"

struct CompositeServer {
  BroadcastServer broadcastServer;
  Server server;
  CompositeServer(MsgBackend *backend, asio::io_service &service);
};