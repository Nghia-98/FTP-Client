#ifndef _CLIENT_FTP_H_
#define _CLIENT_FTP_H_

#include "stdafx.h"
using namespace std;

#define LEN_IP 15 //xxx.xxx.xxx.xxx
#define BLOCK 4096
#define INFO 100

class FTP_Client {
private:
	CSocket SocketCommand;
	CSocket SocketData;
	CSocket ClientListen;//Socket này dùng tạm cho trường hợp ActiveMode

	bool isConnected;
	bool isPassiveMode;//cờ nhận biết chế độ PassiveMode
	bool flag;//cờ thông báo đã nhận 226 trong truyền data

	SOCKADDR_IN addrServer;//lưu địa địa chỉ IP, port phía Server dùng cho kênh điều khiển
	SOCKADDR_IN addrDataClient;//lưu địa địa chỉ IP, port phía Client dùng cho kênh dữ liệu <ActiveMode>
	SOCKADDR_IN addrDataServer;//lưu địa địa chỉ IP, port phía Server dùng cho kênh dữ liệu <PassiveMode>
	char addr[LEN_IP + 1];//IP server
	char buf[BUFSIZ + 1];
	string infoFiles;//lưu lại các file có tồn tại trên Server hay Client (tùy vào Down hay Load)

	void ReplyLogCode(int code);
	void CreateAdressServer(const char* addr, const unsigned int &port);

	bool SetModeDataConnection();//thiết lập ActiveMode hay PassiveMode
	bool CreateDataConnection();//tạo kết nối kênh dữ liệu
	bool ChooseModeDataConnection(const char *mode);//chọn ActiveMode hay PassiveMode

public:
	FTP_Client();
	~FTP_Client();

	void SendCommand();

	bool ConnectServer();
	bool LoginServer();
	bool ShowListFolder(const char* mode, const char* info);
	bool UploadOneFile(const char* info);
	bool DowloadOneFile(const char* info);
	bool UploadFiles(const char* info);
	bool DowloadFiles(const char* info);
	bool ChangeLinkServer(const char* info);
	bool ChangeLinkClient(const char* info);
	bool DeleteOneFile(const char* info);
	bool DeleteFiles(const char* info);
	bool CreateFolder(const char* info);
	bool DeleteEmptyFolders(const char* info);
	bool ShowLinkCurrent();
	bool ActiveModeServer();
	bool PassiveModeServer();
	bool ExitServer();
};

#endif