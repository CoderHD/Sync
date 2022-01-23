#include "boost/asio.hpp"
#include "TestFeature.h"
#include "Address.h"

using namespace boost;
using namespace boost::asio::ip;
using namespace Sync;

namespace Sync {
	void live_stream_init(address_v4::bytes_type& addr, u16 port, asio::io_context& io_context);
	void live_packet_init(address_v4& addr, address_v4& mask, u16 port, asio::io_context& io_context);
};

constexpr int port = 2003;

int main(int argc, char* argv[]) {
	bool server_mode = false;
	if (argc == 2) {
		if (strcmp(argv[1], "server") == 0) server_mode = true;
		else if (strcmp(argv[1], "client") == 0) server_mode = false;
	}
	
	asio::io_context context;

	// Erfasse lokale Addresse
	Interface localInterface;
	if (getLocalAddress(localInterface)) {
		log_info("Lokale Addresse: %s", localInterface.address.to_string().c_str());
		log_info("Lokale Subnetzmaske: %s", localInterface.mask.to_string().c_str());
	}
	else {
		log_info("Lokale Addresse nicht gefunden.");
		return -1;
	}
	if (!localInterface.address.is_v4()) {
		log_info("Lokale Addresse nicht IPv4.");
		return -1;
	}

	address_v4 localAddress = localInterface.address.to_v4();
	address_v4::bytes_type localAddressBytes = localInterface.address.to_v4().to_bytes();
	u32 localAddressNum; memcpy(&localAddressNum, localAddressBytes.data(), localAddressBytes.size());
	address_v4 localMask = localInterface.mask.to_v4();
	const char* localName = "Test";
	u32 localId = server_mode ? 1985821379482 : 689128403979;
	
	Backend back(localAddressNum, LOCAL_ID, localName);
	live_stream_init(localAddressBytes, port, context);
	live_packet_init(localAddress, localMask, port, context);

	auto feature = new TestFeature();
	back.register_feature(feature);

	if (server_mode) back.update();
	else {
		u32 remoteIp = address_v4::from_string("192...").to_uint();
		u32 remote_id = !server_mode ? 1985821379482 : 689128403979;
		Client c(remoteIp, remote_id, back.localClient.purposeCount, back.localClient.purposes);
		Client* client = back.clients.add(remoteIp, c);
		stream_open_session(&back, client, feature);
	}
	return 0;
}