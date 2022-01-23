#if defined(ESP8266)
#include "Log.h"
#include "Compatability.h"
#include "Types.h"

using namespace Sync;

#define log(what) LOG_MODULE("Udp", what)

ESPDatagramContext Sync::createDatagramContext(IPAddress address, IPAddress mask, int port, Backend* backend) {
	backend(backend), port(port), localEndpoint(address) {
		u32 remoteAddress = (address.v4() & mask.v4()) | (~mask.v4());
		remoteEndpoint = IPAddress(remoteAddress);
		udp.begin(port);
	}

	Error Sync::writeBroadcast(Buffer * buffer, uint length) {
		println("Sende Broadcast");
		udp.beginPacket(remoteEndpoint, port);
		udp.write(buffer->data, length);
		udp.endPacket();
		return SUCCESS;
	}

	Error Sync::writeUnicast(u32 address, Buffer * buffer, uint length) {
		println("Sende Unicast");
		IPAddress remoteAddress(address);
		udp.beginPacket(remoteEndpoint, port);
		udp.write(buffer->data, length);
		udp.endPacket();
		return SUCCESS;
	}

	uint Sync::readAvailable() {
		return udp.parsePacket();
	}

	Error Sync::read(Buffer * buffer, uint length) {
		return toErrorCode((uint)udp.read(buffer->data, length) == length);
	}
#endif