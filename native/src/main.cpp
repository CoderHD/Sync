 #include "../src/CompositeServer.h"

int main() {
  MsgBackend backend;
  asio::io_service service;
  CompositeServer server(&backend, service);
  service.run();
  return 0;
}