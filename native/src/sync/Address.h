#pragma once

#if defined(LIVE)
#include "Types.h"
#include "boost/asio.hpp"

using namespace boost;
using namespace boost::asio::ip;

namespace Sync {
	using addr = boost::asio::ip::address;
	struct Interface {
		addr address;
		addr mask;
	};

	// TODO: REMOVE size 4 (IPv4)!!!!!!!!!!!!!!!!!!!!!!!
	inline void prefixLengthToByteArray(u8 prefixLength, u8* buff) {
		memset(buff, 0, 4);
		u32 bytes = prefixLength / 8;
		memset(buff, 255, bytes);

		u32 remBits = prefixLength - bytes * 8;
		u8& remByte  = buff[bytes + 1];
		for (u32 i = 0; i < remBits; i++) {
			remByte = 1 << (8 - i) | remByte;
		}
	}

	boost::asio::ip::address_v6 sinaddr_to_asio(sockaddr_in6* addr) {
		boost::asio::ip::address_v6::bytes_type buf;
		memcpy(buf.data(), addr->sin6_addr.s6_addr, sizeof(addr->sin6_addr));
		return boost::asio::ip::make_address_v6(buf, addr->sin6_scope_id);
	}

#if defined(WINDOWS)
#undef UNICODE
#include <winsock2.h>
	// Headers that need to be included after winsock2.h:
#include <iphlpapi.h>
#include <ws2ipdef.h>

	typedef IP_ADAPTER_UNICAST_ADDRESS_LH Addr;
	typedef IP_ADAPTER_ADDRESSES* AddrList;

	inline std::vector<Interface> get_local_interfaces() {
		// It's a windows machine, we assume it has 512KB free memory
		DWORD outBufLen = 1 << 19;
		AddrList ifaddrs = (AddrList) new char[outBufLen];

		std::vector<Interface> res;

		ULONG err = GetAdaptersAddresses(AF_INET,
			GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, ifaddrs,
			&outBufLen);

		if (err == NO_ERROR) {
			for (AddrList addr = ifaddrs; addr != 0; addr = addr->Next) {
				if (addr->OperStatus != IfOperStatusUp) continue;
				if (addr->NoMulticast) continue;

				// Find the first IPv4 address
				if (addr->Ipv4Enabled) {
					for (Addr* uaddr = addr->FirstUnicastAddress; uaddr != 0; uaddr = uaddr->Next) {
						if (uaddr->Address.lpSockaddr->sa_family != AF_INET) continue;
						address_v4::bytes_type maskBytes;
						prefixLengthToByteArray(addr->FirstUnicastAddress->OnLinkPrefixLength, reinterpret_cast<u8*>(maskBytes.data()));
						memcpy(maskBytes.data(), maskBytes.data(), 4);
						address_v4 address = make_address_v4(ntohl(reinterpret_cast<sockaddr_in*>(addr->FirstUnicastAddress->Address.lpSockaddr)->sin_addr.s_addr));
						address_v4 mask(maskBytes);
						Interface interf ={ address, mask };
						res.push_back(interf);
					}
				}

				/*
				if (addr->Ipv6Enabled) {
					for (Addr* uaddr = addr->FirstUnicastAddress; uaddr != 0; uaddr = uaddr->Next) {
						if (uaddr->Address.lpSockaddr->sa_family != AF_INET6) continue;
						res.push_back(
							sinaddr_to_asio(reinterpret_cast<sockaddr_in6*>(addr->FirstUnicastAddress->Address.lpSockaddr))
						);
					}
				}
				*/
			}
		}
		else {

		}
		delete[]((char*)ifaddrs);
		return res;
	}
#elif defined(APPLE) || defined(LINUX)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/types.h>

	inline std::vector<Interface> get_local_interfaces() {
		std::vector<Interface> res;
		ifaddrs* ifs;
		if (getifaddrs(&ifs)) {
			return res;
		}
		for (auto addr = ifs; addr != nullptr; addr = addr->ifa_next) {
			// No address? Skip.
			if (addr->ifa_addr == nullptr) continue;

			// Interface isn't active? Skip.
			if (!(addr->ifa_flags & IFF_UP)) continue;

			if (addr->ifa_addr->sa_family == AF_INET) {
				address_v4 address = make_address_v4(ntohl(reinterpret_cast<sockaddr_in*>(addr->ifa_addr)->sin_addr.s_addr));
				address_v4 mask    = make_address_v4(ntohl(reinterpret_cast<sockaddr_in*>(addr->ifa_netmask)->sin_addr.s_addr));
				Interface interf ={ address, mask };
				res.push_back(interf);
			}
			/*
			else if (addr->ifa_addr->sa_family == AF_INET6) {
				res.push_back(sinaddr_to_asio(reinterpret_cast<sockaddr_in6*>(addr->ifa_addr)));
			}
			*/
			else continue;
		}
		freeifaddrs(ifs);
		return res;
	}
#else
#error "..."
#endif

	inline bool getLocalAddress(Interface& outInterf) {
		auto interfaces = get_local_interfaces();
		for (const auto interf : interfaces) {
			if (!interf.address.is_v4()) continue;
			std::string te = interf.address.to_v4().to_string();
			if (interf.address.to_v4().to_string().rfind("192.168.2.", 0) == 0) {
				outInterf = interf;
				return true;
			}
		}
		return false;
	}
};
#else
#error "Address.h is only available on Live systems"
#endif