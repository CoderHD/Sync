#include "../src/Server.h"
#include "../src/BroadcastServer.h"
#include "../src/CompositeServer.h"

int main() {
  MsgBackend backend;
  asio::io_context context;
  Server server(30001, &backend, context);
  //BroadcastServer server(30001, &backend, context);
  //CompositeServer server(&backend, context);
  context.run();
  return 0;
}
