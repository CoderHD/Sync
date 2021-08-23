#include "boost/asio.hpp"
#include "BroadcastServer.h"
#include "Server.h"

struct CompositeServer : public MsgSendInterface {
  BroadcastServer broadcastServer;
  Server server;
  CompositeServer(int port, MsgBackend *backend, asio::io_service &service);
  bool send(Session *session, BufferedMsg *msg) override;
  bool sendBroadcast(BufferedMsg *msg) override;
};
