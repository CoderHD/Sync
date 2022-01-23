#include "Backend.h"
#include "Compatability.h"
#include <string>

using namespace Sync;

// Discovery-Nachrichten
u32 write_discovery_msg(Buffer* buff, Client& c) {
	bendian_write(buff, c.addr);
	bendian_write(buff, c.id);
	bendian_write(buff, c.purposeCount);
	for (u32 i = 0; i < c.purposeCount; i++)
		bendian_write(buff, c.purposes[i]);
	return buff->i;
}

Client packet_read_discovery_msg(Buffer* buff) {
	Client c;
	packet_read(buff, 2 * sizeof(u32) + sizeof(u16));
	c.addr	= bendian_read_u32(buff);
	c.id	= bendian_read_u32(buff);
	c.purposeCount	= bendian_read_u16(buff);

	packet_read(buff, c.purposeCount * sizeof(u32));
	for (u32 i = 0; i < c.purposeCount; i++)
		c.purposes[i] = bendian_read_u32(buff);
	return c;
}

Client stream_read_discovery_msg(Buffer* buff) {
	Client c;
	stream_read(buff, 2 * sizeof(u32) + sizeof(u16));
	c.addr	= bendian_read_u32(buff);
	c.id	= bendian_read_u32(buff);
	c.purposeCount	= bendian_read_u16(buff);

	stream_read(buff, c.purposeCount * sizeof(u32));
	for (u32 i = 0; i < c.purposeCount; i++)
		c.purposes[i] = bendian_read_u32(buff);
	return c;
}

// Nachrichten
u32 Sync::write_msg_header(Buffer* buff, MsgHeader& msg) {
	bendian_write(buff, msg.id);
	bendian_write(buff, msg.relevantId);
	bendian_write(buff, msg.dataSize);
	return buff->i;
}

MsgHeader Sync::stream_read_msg_header(Buffer* buff) {
	MsgHeader header;
	stream_read(buff, 2 * sizeof(u32) + sizeof(u64));
	header.id         = bendian_read_u32(buff);
	header.relevantId = bendian_read_u64(buff);
	header.dataSize   = bendian_read_u32(buff);
	return header;
}

Sync::Client::Client() {}

Sync::Client::Client(u32 addr, u32 id, u16 purposeCount, u32* purposes) : addr(addr), id(id), purposeCount(purposeCount), purposes() {
	if (purposes) memcpy(this->purposes, purposes, MAX_PURPOSES * sizeof(u32));
}

bool Sync::client_has_purpose(Client* client, u32 purpose) {
	int ind = -1;
	for (int i = 0; i < client->purposeCount; i++) {
		if (purpose == client->purposes[i]) {
			ind = i;
			break;
		}
	}
	return ind != -1;
}

u64 Sync::get_session_id(Client* client, u32 purpose) {
	return ((u64)client->id << 32) | ((u64)purpose << 0);
}

u32 Sync::get_client_id(u64 session_id) {
	return (u32)(session_id >> 32);
}

u64 Sync::get_session_id(Session* session) {
	return get_session_id(session->remote, session->purpose);
}

Sync::FeatureInfo::FeatureInfo(u32 purpose, u16 maxSessions) : purpose(purpose), maxSessions(maxSessions) { }

void Sync::Backend::context(Session* session) {
	currSession = session;
}

Sync::Backend::Backend(u32 localIp, u32 localId, const char* localName) : localClient(localIp, localId, 0, nullptr) {
	buff = new Buffer(2048);
}

err Sync::Backend::register_feature(Feature* feature) {
	// dont add more purposes than possible
	if (localClient.purposeCount == MAX_PURPOSES) return ERROR_UNKNOWN;

	// store purpose in local client
	u64 purpose = feature->getInfo().purpose;
	localClient.purposes[localClient.purposeCount] = purpose;
	localClient.purposeCount++;
	// update disc size
	discSize = (2 + localClient.purposeCount) * sizeof(u32) + sizeof(u16);
	// store feature and initialize
	features.add(purpose, feature);
	feature->onInit(this);
	return SUCCESS;
}

void Sync::Backend::update() {
	// accept potential client
	u32 addr = stream_accept();
	if (!addr) return;

	// recieve discovery message
	MsgHeader header = stream_read_msg_header(buff);
	if (header.id != DISCOVERY) return;
	Client remote = stream_read_discovery_msg(buff);
	buff->finish();
	clients.add(remote.addr, remote);
	log_info("Discovered client after accept() at %d with id=%d, purposes=%d", remote.addr, remote.id, remote.purposeCount);
}

Client* Sync::Backend::get_client(MsgHeader* msg) {
	auto session = sessions.find(msg->relevantId);
	if (!session)
		return nullptr;
	else
		return session->remote;
}

err stream_handle_std_msg(Backend* back, u32 addr, MsgHeader& header, Buffer* buff) {
	switch (header.id) {
	case StdMsgId::DISCOVERY: 
	{
		Client remote = stream_read_discovery_msg(buff);
		back->clients.add(remote.addr, remote);
		log_info("Discovered client at %d with id=%d, purposes=%d", remote.addr, remote.id, remote.purposeCount);
		return SUCCESS;
	}
	case StdMsgId::OPEN_SESSION:
	{
		// read requested purpose
		stream_read(buff, sizeof(u32));
		u32 purpose = bendian_read_u32(buff);
		buff->finish();
		// find corresponding feature
		Feature** found = back->features.find(purpose);
		NULL_FALSE(found);

		// relevant_id represents the client id in this context
		Client* client = back->clients.find(header.relevantId);
		NULL_FALSE(client);

		// create session
		Session session ={ client, purpose };
		u64 sessionId = get_session_id(&session);

		// respond with accept message
		MsgHeader opensess_accepted;
		opensess_accepted.id = StdMsgId::OPEN_SESSION_ACCEPTED;
		opensess_accepted.relevantId = sessionId;
		opensess_accepted.dataSize = 0;
		write_msg_header(buff, opensess_accepted);
		err err = stream_write(buff, buff->i);
		buff->finish();
		if (err) return err;

		// save session for further use
		back->sessions.add(sessionId, session);
		return SUCCESS;
	}
	break;

	case StdMsgId::OPEN_SESSION_ACCEPTED:
	{
		u64 session_id = header.relevantId;
		u32 purpose = (u32)session_id;
		// find corresponding feature and client
		Feature** found = back->features.find(purpose);
		NULL_FALSE(found);
		Client* client = back->clients.find(get_client_id(session_id));
		NULL_FALSE(client);
		// add corresponding session
		Session sess;
		sess.purpose = purpose;
		sess.remote = client;
		Session* ptr = back->sessions.add(session_id, sess);

		// signal it to the feature
		(*found)->onSessionCreated(ptr);
	}
	break;

	case StdMsgId::CLOSE_SESSION:
	{
		u64 session_id = header.relevantId;
		Session* session = back->sessions.find(session_id);
		NULL_FALSE(session);
		u32 purpose = session_id;
		Feature** feature = back->features.find(purpose);
		NULL_FALSE(feature);

		// remove session and signal it to the feature
		back->sessions.remove(session_id);
		(*feature)->onSessionClosed(session);
	}
	break;
	}

	return SUCCESS;
}

err Sync::stream_handle_msg(Backend* back, u32 addr) {
	Buffer* buff = back->buff;

	// Header im Stream lesen
	MsgHeader header = stream_read_msg_header(buff);
	buff->finish();

	// Kontext setzen
	stream_context(addr);

	if (header.id < std_msg_id_count)
		return stream_handle_std_msg(back, addr, header, buff);
	else {
		// find session
		u64 sessionId = header.relevantId;
		Session* session = back->sessions.find(sessionId);
		NULL_FALSE(session);

		// delegate to feature
		Feature** feature = back->features.find(sessionId);
		if (feature) {
			(*feature)->onMsgRecieved(session, &header);
			return SUCCESS;
		}
		else {
			return ERROR_UNKNOWN;
		}
	}
}

err Sync::stream_open_session(Backend* back, Client* client, Feature* feature) {
	NULL_FALSE(client);
	NULL_FALSE(feature);

	FeatureInfo info = feature->getInfo();
	// does the remote client even support this purpose?
	if (!client_has_purpose(client, info.purpose))
		return ERROR_UNKNOWN;

	// open connection if necessary
	err err = stream_open();
	if (err) return err;

	// open session if necessary
	Session* sess = back->sessions.find(info.purpose);
	if (!sess) {
		Buffer* buff = back->buff;

		// send discovery message
		MsgHeader discovery;
		discovery.id = StdMsgId::DISCOVERY;
		discovery.relevantId = back->localClient.id;
		discovery.dataSize = back->discSize;
		write_msg_header(buff, discovery);
		write_discovery_msg(buff, back->localClient);
		err = stream_write(buff, buff->i);
		buff->finish();
		if (err) return err;

		// send open session message
		MsgHeader open_session;
		open_session.id = StdMsgId::OPEN_SESSION;
		open_session.relevantId = back->localClient.addr;
		open_session.dataSize = sizeof(u32);
		write_msg_header(buff, open_session);
		bendian_write(buff, info.purpose);
		err = stream_write(buff, buff->i);
		buff->finish();
		return err;
	}
	return SUCCESS;
}

err Sync::stream_close_session(Backend* back, Session* session) {
	NULL_FALSE(session);
	Buffer* buff = back->buff;

	MsgHeader close_session;
	close_session.id = StdMsgId::CLOSE_SESSION;
	close_session.relevantId = get_session_id(session);
	close_session.dataSize = 0;
	write_msg_header(buff, close_session);
	err err = stream_write(buff, buff->i);
	buff->finish();
	return err;
}

err Sync::packet_handle_std_msg(Backend* back) {
	Buffer* buff = back->buff;

	Client client = packet_read_discovery_msg(buff);
	buff->finish();

	if (client.id == client.id || back->clients.find(client.addr)) return SUCCESS;

	// save client
	back->clients.add(client.addr, client);
	Client* clientPtr = back->clients.find(client.addr);

	// inform all features
	for (u32 i = 0; i < 2 * MAX_SESSIONS; i++) {
		u32* purpose = back->features.ptrs[i];
		if (purpose && purpose != TOMBSTONE) {
			back->features.vals[i]->onClientCreated(clientPtr);
		}
	}
	return SUCCESS;
}