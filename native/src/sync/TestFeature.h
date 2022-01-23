#include "Backend.h"

namespace Sync {
    constexpr u32 testMsgSpace = 7871283;
    enum TestMsgId : u16 { TEST, TEST_RESPONSE };

    struct TestData {
        u32 a, b, c;
        u64 d;
        float fp;
        double dp;
    };

    class TestFeature : public Feature {
        FeatureInfo info;
        Backend* backend;
        TestData data;

        void onInit(Backend* backend) override;
        void onClientCreated(Client* client) override;
        void onSessionCreated(Session* session) override;
        void onSessionClosed(Session* session) override;
        void onMsgRecieved(Session* session, MsgHeader* msg) override;
        FeatureInfo getInfo() override;

    public:
        TestFeature();
    };
};