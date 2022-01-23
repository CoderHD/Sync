#if defined(LIVE)
#include "BackupFeature.h"
#include "Log.h"
#include "Types.h"
#include <string>
#include <cmath>
#if defined(WINDOWS)
#define stdmin(a, b) min(a, b)
#elif defined(LINUX)
#define stdmin(a, b) std::min(a, b)
#endif
#include "Compatability.h"
#if defined(WINDOWS)
#include <Windows.h>
#include <shlwapi.h>
#elif defined(LINUX)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h> 
#endif 

using namespace Sync;

constexpr u32 MAX_FILE_BUFF_SIZE = 32.768;

u64 combine(u32 low, u32 high) {
	u32 ret;
	memcpy(&ret, &low, sizeof(u32));
	memcpy(&ret + sizeof(u32), &high, sizeof(u32));
	return ret;
}

#if defined(WINDOWS)
FileInfo getFileInfo(File& file) {
	if (!PathFileExistsA(file.path)) {
		return { false };
	}

	WIN32_FILE_ATTRIBUTE_DATA fInfo;
	GetFileAttributesEx(file.path, GetFileExInfoStandard, &fInfo);
	u64 attribs = fInfo.dwFileAttributes;

	u64 size = combine(fInfo.nFileSizeLow, fInfo.nFileSizeHigh);
	u64 creationTime = combine(fInfo.ftCreationTime.dwLowDateTime, fInfo.ftCreationTime.dwHighDateTime);
	u64 alterationTime = combine(fInfo.ftLastWriteTime.dwLowDateTime, fInfo.ftLastWriteTime.dwHighDateTime);
	bool write = false, read = false;
	if (attribs & FILE_ATTRIBUTE_READONLY) {
		write = false;
	}
	if (attribs & FILE_ATTRIBUTE_SYSTEM || attribs & FILE_ATTRIBUTE_OFFLINE || attribs & FILE_ATTRIBUTE_HIDDEN) {
		read = false;
		write = false;
	}
	u8 accessPermissions = 0;
	if (read)  accessPermissions |= AccessMode::READ;
	if (write) accessPermissions |= AccessMode::WRITE;
	return { true, accessPermissions, creationTime, alterationTime, size };
}
#elif defined(LINUX)
FileInfo getFileInfo(File& file) {
	int handle = open("test.txt", O_RDONLY);
	if (handle == -1)
		return { false };

	struct stat st;
	if (fstat(handle, &st) == -1)
		return { false };

	u64 size = st.st_size;
	u64 creationTime = st.st_mtime;
	u64 alterationTime = st.st_mtime;
	bool read = false, write = false;
	if (st.st_mode & S_IRUSR) {
		read = true;
	}
	if (st.st_mode & S_IWUSR && st.st_mode & S_IXUSR) {
		write = true;
	}
	u8 accessPermissions = 0;
	if (read)  accessPermissions |= AccessMode::READ;
	if (write) accessPermissions |= AccessMode::WRITE;
	return { true, accessPermissions, creationTime, alterationTime, size };
}
#endif

#if defined(WINDOWS)
StorageInfo getStorageInfo() {
	PULARGE_INTEGER lpFreeBytesAvailableToCaller = null, lpTotalNumberOfBytes = null, lpTotalNumberOfFreeBytes = null;
	if (!GetDiskFreeSpaceExA(null, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes)) {
		LOG_ERROR("GetDiskFreeSpace Fehler");
	}
	DYNAMIC_ASSERT(lpFreeBytesAvailableToCaller && lpTotalNumberOfBytes && lpTotalNumberOfFreeBytes);
	return { (u64)lpTotalNumberOfBytes->QuadPart, (u64)lpTotalNumberOfBytes->QuadPart - (u64)lpFreeBytesAvailableToCaller->QuadPart };
}
#elif defined(LINUX)
StorageInfo getStorageInfo() {
	struct statvfs st;
	if (statvfs("/", &st) == -1) {
		ASSERT_ERROR("statvfs Fehler");
	}
	u64 freeBytes = st.f_bfree * st.f_bsize, totalBytes = st.f_blocks * st.f_bsize;
	return { totalBytes, totalBytes - freeBytes };
}
#endif

std::string get_real_path(Client* client, std::string path) {
	return std::to_string(client->id) + "/" + path;
}

void bendian_write_file(Buffer* buff, File& file) {
	bendian_write(buff, file.path, file.pathLen);
}

File stream_read_file_from_buff(Buffer* buff) {
	stream_read(buff, sizeof(u32));
	u32 len = bendian_read_u32(buff);
	stream_read(buff, len);
	char* path = (char*)bendian_read_ptr(buff, len);
	return { path, len };
}

void write_file_read(Buffer* buff, FileRead& read) {
	bendian_write_file(buff, read.file);
	bendian_write(buff, read.dataSize);
}

void write_file_write(Buffer* buff, FileWrite& write) {
	bendian_write_file(buff, write.file);
	bendian_write(buff, write.dataSize);
}

void write_file_info(Buffer* buff, FileInfo& info) {
	bendian_write(buff, info.exists);
	bendian_write(buff, info.accessPermissions);
	bendian_write(buff, info.creationTime);
	bendian_write(buff, info.alterationTime);
	bendian_write(buff, info.size);
}

void write_storage_info(Buffer* buff, StorageInfo& info) {
	bendian_write(buff, info.totalSpace);
	bendian_write(buff, info.occupiedSpace);
}

void write_file_data(Buffer* buff, FileData& data) {
	bendian_write(buff, data.dataSize);
}

FileRead stream_read_file_read(Buffer* buff) {
	FileRead read;
	read.file = stream_read_file_from_buff(buff);
	stream_read(buff, sizeof(u64));
	read.dataSize = bendian_read_u64(buff);
	return read;
}

FileWrite stream_read_file_write(Buffer* buff) {
	FileWrite write;
	write.file = stream_read_file_from_buff(buff);
	stream_read(buff, sizeof(u64));
	write.dataSize = bendian_read_u64(buff);
	return write;
}

FileInfo read_file_info(Buffer* buff) {
	bool exists        = bendian_read_bool(buff);
	u8 accessMode      = bendian_read_u8(buff);
	u64 creationTime   = bendian_read_u64(buff);
	u64 alterationTime = bendian_read_u64(buff);
	u64 size           = bendian_read_u64(buff);
	return { exists, accessMode, creationTime, alterationTime, size };
}

StorageInfo read_storage_info(Buffer* buff) {
	u64 totalSpace    = bendian_read_u64(buff);
	u64 occupiedSpace = bendian_read_u64(buff);
	return { totalSpace, occupiedSpace };
}

FileData read_file_data(Buffer* buff) {
	u64 dataSize = bendian_read_u64(buff);
	return { dataSize };
}

FileInfo Sync::BackupFeature::searchFileInfo(File& file) {
	return get_remote_file_info(file);
}

StorageInfo Sync::BackupFeature::searchStorageInfo() {
	return get_remote_storage_info();
}

Sync::BackupFeature::BackupFeature() : info(6912389, 1) {}

void Sync::BackupFeature::onInit(Backend* backend) {
	this->back = backend;
}

void Sync::BackupFeature::onClientCreated(Client* client) {
	if (client_has_purpose(client, info.purpose)) {
		err err = stream_open_session(back, client, this);
		DYNAMIC_ASSERT(!err);
	}
}

void Sync::BackupFeature::onSessionCreated(Session* sess) {
	this->sess = sess;
	log_info("backup feature sess created");
}

void Sync::BackupFeature::onSessionClosed(Session* sess) {
	this->sess = nullptr;
	log_info("backup feature sess closed");
}

void stream_read_and_file_write(Backend* back, FILE* f) {
	// read size
	stream_read(back->buff, sizeof(u32));
	u32 file_size = bendian_read_u32(back->buff);
	back->buff->finish();
	// read data
	u32 file_buff_size = stdmin(MAX_FILE_BUFF_SIZE, file_size);
	Buffer file_buff(file_buff_size);
	u64 i = 0;
	while (i < file_size) {
		u32 write_size = stdmin(file_buff_size, file_size - i);
		u32 write = stream_read(&file_buff, write_size);
		DYNAMIC_ASSERT(write == write_size);
		i += write;
		fwrite(file_buff.buff, 1, write_size, f);
	}
}

void file_read_and_stream_write(FILE* f, u32 file_size) {
	u32 file_buff_size = stdmin(MAX_FILE_BUFF_SIZE, file_size);
	Buffer file_buff(file_buff_size);
	// write size
	bendian_write(&file_buff, file_size);
	stream_write(&file_buff, file_buff.i);
	file_buff.finish();
	// write data	
	u64 i = 0;
	while (i < file_size) {
		u32 read_size = stdmin(file_buff_size, file_size - i);
		u32 read = fread(file_buff.buff, 1, read_size, f);
		DYNAMIC_ASSERT(read == read_size);
		i += read;
		stream_write(&file_buff, file_buff.i);
	}
}

void execute_file_read(FileRead& read, Client* client) {
	File& file = read.file;
	std::string path = get_real_path(client, std::string(file.path, file.pathLen));

	// open file
	FILE* f;
	auto err = fopen_s(&f, path.c_str(), "rb");
	if (err) {
		//todo: send back failure message
	}
	fseek(f, SEEK_END, 0);
	u32 file_size = ftell(f);
	rewind(f);
	// send data
	file_read_and_stream_write(f, file_size);
	// close file
	fclose(f);
}

void execute_file_write(Backend* back, FileWrite& write, Client* client) {
	File& file = write.file;
	std::string path = get_real_path(client, std::string(file.path, file.pathLen));

	// open file
	FILE* f;
	auto err = fopen_s(&f, path.c_str(), "ab+");
	if (err) {
		//todo: send back failure message
	}
	// recv data
	stream_read_and_file_write(back, f);
	// close file
	fclose(f);
}

void Sync::BackupFeature::onMsgRecieved(Session* sess, MsgHeader* msg) {
	Buffer* buff = back->buff;
	Client* client = back->get_client(msg);
	if (!client) return;

	switch (msg->id) {
	case BackupMsgId::FILE_READ:
	{
		// read file read command
		FileRead read = stream_read_file_read(buff);
		buff->finish();
		// execute command
		execute_file_read(read, sess->remote);
	}
	break;
	case BackupMsgId::FILE_WRITE:
	{
		// read command
		FileWrite write = stream_read_file_write(buff);
		buff->finish();
		// execute command
		execute_file_write(back, write, sess->remote);
	}
	break;
	case BackupMsgId::FILE_INFO:
	{
		// read command
		File file = stream_read_file_from_buff(buff);
		buff->finish();
		// execute command
		FileInfo info = searchFileInfo(file);
		write_file_info(buff, info);
		buff->finish();
		stream_write(buff, buff->i);
	}
	break;
	case BackupMsgId::STORAGE_INFO:
	{
		// execute command
		StorageInfo info = searchStorageInfo();
		write_storage_info(buff, info);
		stream_write(buff, buff->i);
		buff->finish();
	}
	break;
	case BackupMsgId::FILE_DATA:
	{
		// todo: incomplete
	}
	break;
	default:
	{
		LOG_ERROR("invalid backup msg id");
	}
	break;
	}
}

FeatureInfo Sync::BackupFeature::getInfo() {
	return info;
}

void Sync::BackupFeature::read_remote_file(FileRead read, FILE* out) {
	Buffer* buff = back->buff;
	MsgHeader header;
	header.id = BackupMsgId::FILE_READ;
	header.relevantId = get_session_id(sess);
	header.dataSize = sizeof(FileRead);
	write_msg_header(buff, header);
	write_file_read(buff, read);
	stream_write(buff, buff->i);
	stream_read_and_file_write(back, out);
}

void Sync::BackupFeature::write_remote_file(FileWrite write, FILE* in) {
	Buffer* buff = back->buff;
	MsgHeader header;
	header.id = BackupMsgId::FILE_WRITE;
	header.relevantId = get_session_id(sess);
	header.dataSize = sizeof(FileWrite);
	write_msg_header(buff, header);
	write_file_write(buff, write);
	stream_write(buff, buff->i);
	file_read_and_stream_write(in, write.dataSize);
}

FileInfo Sync::BackupFeature::get_remote_file_info(File file) {
	Buffer* buff = back->buff;
	MsgHeader request;
	request.id = BackupMsgId::FILE_INFO;
	request.relevantId = get_session_id(sess);
	request.dataSize = sizeof(File);
	u32 header_size = write_msg_header(buff, request);
	bendian_write_file(buff, file);
	stream_write(buff, buff->i);
	buff->finish();

	stream_read(buff, header_size + sizeof(FileInfo));
	MsgHeader resp = stream_read_msg_header(buff);
	FileInfo info = read_file_info(buff);
	buff->finish();
	return info;
}

StorageInfo Sync::BackupFeature::get_remote_storage_info() {
	Buffer* buff = back->buff;
	MsgHeader request;
	request.id = BackupMsgId::STORAGE_INFO;
	request.relevantId = get_session_id(sess);
	request.dataSize = 0;
	write_msg_header(buff, request);
	stream_write(buff, buff->i);

	MsgHeader response = stream_read_msg_header(buff);
	StorageInfo info = read_storage_info(buff);
	buff->finish();
	return info;
}
#endif