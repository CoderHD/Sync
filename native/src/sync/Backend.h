#pragma once
#include "Types.h"
#include "Buffer.h"
#include "Log.h"
#include "HashTable.h"

namespace Sync {
    constexpr u32 MAX_PURPOSES = 5;
    constexpr u32 MAX_CLIENTS = 2;
    constexpr u32 MAX_SESSIONS = 5;
    constexpr u32 backendBufferSize = 2048;

    err packet_write_broadcast(Buffer* buff, u32 len);
    err packet_write_unicast(u32 addr, Buffer* buff, u32 len);
    u32 packet_read_available();
    err packet_read(Buffer* buff, u32 len);
    u32 stream_accept();
    err stream_context(u32 address);
    err stream_open();
    err stream_close();
    err stream_write(Buffer* buffer, u32 length);
    err stream_read(Buffer* buffer, u32 length);
    u32 stream_read_available();

    struct Client {
        u32 addr;
        u32 id;
        u16 purposeCount;
        u32 purposes[MAX_PURPOSES];
        Client();
        Client(u32 ip, u32 id, u16 purposeCount, u32* purposes);
    };
    bool client_has_purpose(Client* client, u32 purpose);

    struct Session {
        Client* remote;
        u32 purpose;
    };
    u64 get_session_id(Client* client, u32 purpose);
    u32 get_client_id(u64 session_id);
    u64 get_session_id(Session* session);

    const u32 std_msg_id_count = 20;
    enum StdMsgId : u16 { DISCOVERY, OPEN_SESSION, OPEN_SESSION_ACCEPTED, CLOSE_SESSION };

    struct MsgHeader {
        u64 id;
        // relevantId might be clientid or sessionid
        u64 relevantId;
        u32 dataSize;
    };

    struct FeatureInfo {
        u32 purpose;
        u16 maxSessions;
        FeatureInfo(u32 purpose, u16 maxSessions);
    };
    class Backend;
    struct Feature {
        virtual void onInit(Backend* backend) = 0;
        virtual void onClientCreated(Client* client) = 0;
        virtual void onSessionCreated(Session* session) = 0;
        virtual void onSessionClosed(Session* session) = 0;
        virtual void onMsgRecieved(Session* session, MsgHeader* msg) = 0;
        virtual FeatureInfo getInfo() = 0;
    };

    u32 write_msg_header(Buffer* buff, MsgHeader& msg);
    MsgHeader stream_read_msg_header(Buffer* buffer);

    struct Backend {
        HashTableOpenAddress<u32, Client, MAX_CLIENTS * 2> clients;
        HashTableOpenAddress<u64, Session, MAX_SESSIONS * 2> sessions;
        HashTableOpenAddress<u32, Feature*, MAX_SESSIONS * 2> features;
        Client localClient;
        u32 discSize;
        Session* currSession;
        Buffer* buff;

        void context(Session* session);
        
        Backend(u32 localIp, u32 localId, const char* localName);
        err register_feature(Feature* feature);

        void update();
        
        Client* get_client(MsgHeader* msg);
    };

    err stream_handle_msg(Backend* back, u32 address);
    err stream_open_session(Backend* back, Client* client, Feature* feature);
    err stream_close_session(Backend* back, Session* session);
    err packet_handle_std_msg(Backend* back);
};