#include "MsgBackend.h"
#include <string>

// Bitlogikfunktionen
// TODO: Big und Little Endian Support
s16 shortFromBuff(char *buff) { return (s16)buff[0] << 8 | (s16)buff[1] << 0; }
s32 intFromBuff(char *buff)   { return (s32)buff[0] << 24 | (s32)buff[1] << 16 | (s32)buff[2] << 8 | (s32)buff[3] << 0; }
s64 longFromBuff(char *buff)  {
  s64 val = 0;
  for(int i = 0; i < 8; i++) val |= (s64)buff[i] << (8 * (7-i));
  return val;
}
ushort shortToBuff(s16 val, char *buff) { buff[0] = (val >> 8) & 0xff; buff[1] = (val >> 0) & 0xff; return 2; }
ushort intToBuff(s32 val, char *buff) {
  buff[0] = (val >> 24) & 0xff; buff[1] = (val >> 16) & 0xff;
  buff[2] = (val >> 8)  & 0xff; buff[3] = (val >> 0)  & 0xff; return 4;
}
ushort longToBuff(s64 val, char *buff) {
  for(int i = 0; i < 8; i++) buff[i] = (val >> 8 * (7 - i)) & 0xff;
  return 8;
}

// Discovery-Nachrichten Bitlogik
ushort populateDiscBuffHeader(Client *client, char *buff) {
  ushort len = 0;
  len += intToBuff(client->id, buff + len);
  len += longToBuff(client->purposeId, buff + len);
  len += intToBuff(client->name.length(), buff + len);
  return len;
}
ushort populateDiscBuffData(Client *client, char *buff) {
  ushort nameLen = client->name.length();
  char *offBuff = buff + nameLen; for(ushort i = 0; i < nameLen; i++) offBuff[i] = client->name[i];
  return nameLen;
}
Client discMsgFromBuff(char *buff) {
  Client client;
  client.id = intFromBuff(buff + 0);
  client.purposeId = longFromBuff(buff + 4);
  //return {null, intFromBuff(buff + 0), longFromBuff(buff + 4), std::string(buff + 16, intFromBuff(buff + 12))};
  return client;
}

// Nachrichten Bitlogik
ushort populateBuffHeader(MsgInfo *header, char *buff) {
  ushort len = 0;
  len += shortToBuff(header->type, buff + len);
  len += intToBuff(header->id, buff + len);
  len += intToBuff(header->singlepacketDataSize, buff + len);
  return len;
}
MsgInfo msgHeaderFromBuff(char *buff) {
  return {shortFromBuff(buff + 0), intFromBuff(buff + 2), intFromBuff(buff + 6)};
}
ushort populateDiscMsg(Client *client, char *buff) {
  ushort len = 0; 
  len += populateDiscBuffHeader(client, buff); 
  len += populateDiscBuffData(client, buff);
  return len;
}

bool MsgBackend::createClient(Client client) {
  return clients.insert(client.id, client);
}

bool MsgBackend::createSession(Session session) {
  return sessions.insert(session.remote->id, session);
}

bool MsgBackend::registerClientCreated(s64 purposeId, ClientCreatedCallback callback) {
  return clCrCallbacks.insert(purposeId, callback);
}

bool MsgBackend::registerSessionCreated(s64 purposeId, SessionCreatedCallback callback) {
  return sessCrCallbacks.insert(purposeId, callback);
}

bool MsgBackend::registerMsgRecieved(s32 type, MsgRecievedCallback callback) {
  return msgRcCallbacks.insert(type, callback);
}

MsgBackend::MsgBackend() {
  char initBuff[100];
  MsgInfo initMsgInfo;
  msgHeaderSize = populateBuffHeader(&initMsgInfo, initBuff);
  Client initClient;
  discMsgSize   = populateDiscBuffHeader(&initClient, initBuff);
}

bool MsgBackend::handleDiscMsg(MsgBuffer *msgBuffer) {
  char *buff = msgBuffer->buffer;
  Client client = discMsgFromBuff(buff);
  if(createClient(client)) {
    // Client bisher noch nicht hinzugefügt
    populateDiscMsg(&client, buff);
    
    // Callback aufrufen (falls vorhanden)
    auto clCrCallback = clCrCallbacks.find(client.purposeId);
    if(clCrCallback) {
      if((*clCrCallback)(&client)) {
	return false;
      }
    }
    return true;
  } else {
    // Client bereits hinzugefügt gewesen
    return false;
  }
}

bool MsgBackend::handleMsg(MsgBuffer *msgBuffer) {
  char *buff = msgBuffer->buffer;
  MsgInfo msgInfo = msgHeaderFromBuff(buff);
  Msg msg = {msgInfo};

  // Callback aufrufen (falls vorhanden)
  auto msgRcCallback = msgRcCallbacks.find(msgInfo.type);
  if(msgRcCallback) {
    // Nachrichtencallback eingetragen
    return (*msgRcCallback)(&msg);
  } else {
    // Nachrichtencallback nicht eingetragen
    return false;
  }
}

uint MsgBackend::getMsgBuffSize() { return msgBufferSize; }
uint MsgBackend::getMsgHeaderSize() { return msgHeaderSize; }
uint MsgBackend::getDiscMsgSize() { return discMsgSize; }
