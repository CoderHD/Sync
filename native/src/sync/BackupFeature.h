#include "Backend.h"

namespace Sync {
    constexpr u32 backupMsgSpace = 7871283;
    enum BackupMsgId : u16 { FILE_READ, FILE_WRITE, FILE_INFO, STORAGE_INFO, FILE_DATA };

    struct File {
        char* path;
        u32 pathLen;
    };

    struct FileRead {
        File file;
        u64 dataSize;
    };

    struct FileWrite {
        File file;
        u64 dataSize;
    };

    enum AccessMode : s8 { READ = 1, WRITE = 2, READ_WRITE = 3 };

    struct FileInfo {
        bool exists;
        u8 accessPermissions;
        u64 creationTime;
        u64 alterationTime;
        u64 size;
    };

    struct StorageInfo {
        u64 totalSpace;
        u64 occupiedSpace;
    };

    struct FileData {
        u64 dataSize;
    };

    class BackupFeature : public Feature {
        FeatureInfo info;
        Backend* back;
        Client* remote;
        Session* sess;

        FileInfo searchFileInfo(File& file);
        StorageInfo searchStorageInfo();

        void onInit(Backend* backend) override;
        void onClientCreated(Client* client) override;
        void onSessionCreated(Session* session) override;
        void onSessionClosed(Session* session) override;
        void onMsgRecieved(Session* session, MsgHeader* msg) override;
        FeatureInfo getInfo() override;

    public:
        BackupFeature();

        void read_remote_file(FileRead read, FILE* out);
        void write_remote_file(FileWrite write, FILE* in);
        FileInfo get_remote_file_info(File file);
        StorageInfo get_remote_storage_info();
    };
};