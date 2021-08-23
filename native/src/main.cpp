#include "../src/Server.h"
#include "../src/BroadcastServer.h"
#include "boost/asio.hpp"

constexpr int port = 30001;

void getAddress(char *buff, asio::io_context &context) {
  asio::ip::tcp::resolver resolver(context);
  auto address = boost::asio::ip::address_v4::from_string(resolver.resolve(asio::ip::host_name(), "")->endpoint().address().to_string());
  auto bytes = address.to_bytes();
  memcpy(buff, bytes.data(), bytes.size());
}

int main() {
  asio::io_context context;

  char localIp[IP_SIZE];
  s32 localId = 8852613920269705449;
  std::string localName = "Test";
  Backend backend(localIp, localId, localName);
  Server server(port, &backend, context);
  BroadcastServer broadcast(port, &backend, context);
  backend.setInterfaces(&server, &broadcast);

  context.run();

  return 0;
}
