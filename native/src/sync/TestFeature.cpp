#include "TestFeature.h"

using namespace Sync;

void write_data(Buffer* buff, TestData& data) {
	bendian_write(buff, data.a);
	bendian_write(buff, data.b);
	bendian_write(buff, data.c);
	bendian_write(buff, data.d);
	bendian_write(buff, data.fp);
	bendian_write(buff, data.dp);
}

TestData stream_read_data(Buffer* buff) {
	TestData test;
	test.a = bendian_read_u32(buff);
	test.b = bendian_read_u32(buff);
	test.c = bendian_read_u32(buff);
	test.d = bendian_read_u64(buff);
	test.fp = bendian_read_f32(buff);
	test.dp = bendian_read_f64(buff);
	return test;
}

Sync::TestFeature::TestFeature() : info(1923198, 1) {
	data.a = 1;
	data.b = 22;
	data.c = 991;
	data.d = 9918391283981193;
	data.fp = 99122.13393;
	data.dp = 12128923.1293123392;
}

void Sync::TestFeature::onInit(Backend* backend) {
	this->backend = backend;
}

void Sync::TestFeature::onClientCreated(Client* client) {
	bool has_purpose = client_has_purpose(client, info.purpose);
	log_info("client created: address: %d, id: %d, hasPurpose: %d", client->addr, client->id, has_purpose);
	if (has_purpose) {
		err err = stream_open_session(backend, client, this);
		DYNAMIC_ASSERT(!err);
	}
}

void Sync::TestFeature::onSessionCreated(Session* session) {
	log_info("session created!");
}

void Sync::TestFeature::onSessionClosed(Session* session) {
	log_info("session closed!");
}

void Sync::TestFeature::onMsgRecieved(Session* session, MsgHeader* msg) {
	Buffer* buff = backend->buff;
	
	Client* client = backend->get_client(msg);
	if (!client) return;

	switch (msg->id) {
	case TestMsgId::TEST:
	{
		// read command
		TestData testData = stream_read_data(buff);
		buff->finish();
		// execute command
		MsgHeader header;
		header.id = TestMsgId::TEST_RESPONSE;
		header.relevantId = get_session_id(session);
		header.dataSize = sizeof(TestData);
		write_msg_header(buff, header);
		write_data(buff, testData);
		stream_write(buff, buff->i);
		buff->finish();
	}
	break;
	case TestMsgId::TEST_RESPONSE:
	{
		TestData testData = stream_read_data(buff);
		buff->finish();
		log_info("a: %d; b: %d; c: %d; d: %d; fp: %f; dp: %f", testData.a, testData.b, testData.c, testData.d, testData.fp, testData.dp);
	}
	break;
	break;
	default:
	{
		LOG_ERROR("invalid backup msg id");
	}
	break;
	}
}

FeatureInfo Sync::TestFeature::getInfo() {
	return info;
}