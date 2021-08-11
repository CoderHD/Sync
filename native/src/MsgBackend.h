#pragma once
#include "Types.h"
#include "HashMap.h"
#include <string>

constexpr uint IP_SIZE = 4;

// Zustandsspeicherung
enum StdPurposeId : u64 { STORAGE = 1, ROBOT = 2, USER = 4, OTHER = 8 };
struct Client {
  char ip[IP_SIZE];
  s32  id;
  s64  purposeId;
  std::string name;
};
struct Session {
  Client *remote;
  // Weitere Informationen...
};

// Zustandsveränderung
struct MsgInfo {
  short type;
  s32 id;
  s32 singlepacketDataSize;
  // u32 multipacketDataSize; std::iostream multipacketStream;
};
struct Msg {
  MsgInfo header;
};

// Schnittstelle für Callbacks
using ClientCreatedCallback  = bool(*)(Client *);
using SessionCreatedCallback = bool(*)(Session*);
using MsgRecievedCallback    = bool(*)(Msg*);

constexpr uint msgBufferSize = 2048;

struct MsgBuffer {
  char buffer[msgBufferSize];
};

class MsgBackend {
  HashMap<Client> clients;
  HashMap<Session> sessions;
  HashMap<ClientCreatedCallback>  clCrCallbacks;
  HashMap<SessionCreatedCallback> sessCrCallbacks;
  HashMap<MsgRecievedCallback>    msgRcCallbacks;
  uint msgHeaderSize, discMsgSize;

  bool createClient(Client client);
  bool createSession(Session session);

public:
  MsgBackend();

  bool registerClientCreated(s64 purposeId, ClientCreatedCallback callback);
  bool registerSessionCreated(s64 purposeId, SessionCreatedCallback callback);
  bool registerMsgRecieved(s32 type, MsgRecievedCallback callback);

  bool handleDiscMsg(MsgBuffer *msgBuffer);
  bool handleMsg(MsgBuffer *msgBuffer);

  uint getMsgBuffSize();
  uint getMsgHeaderSize();
  uint getDiscMsgSize();
};
