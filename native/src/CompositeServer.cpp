#include "CompositeServer.h"

CompositeServer::CompositeServer(int port, MsgBackend *backend, asio::io_service &service)
  : broadcastServer(port, backend, service), server(port, backend, service)
{
  backend->setMsgSendInterface(this);
}

bool CompositeServer::send(Session *session, BufferedMsg *msg) {
  
}

bool CompositeServer::sendBroadcast(BufferedMsg *msg) {
  broadcastServer.send(msg);
}
