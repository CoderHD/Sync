#include "CompositeServer.h"

CompositeServer::CompositeServer(MsgBackend *backend, asio::io_service &service) : broadcastServer(backend, service), server(backend, service)
{
}
