#include "stdafx.h"
#include "FTP_Client.h"

FTP_Client::FTP_Client() {
	if (AfxSocketInit() == FALSE) {
		cout << "Khong the khoi tao Socket Libraray";
		exit(1);
	}
	SocketCommand.Create();
	memset(buf, 0, sizeof buf);
	isConnected = false;
	isPassiveMode = false;
	flag = false;
}

FTP_Client::~FTP_Client() {
	SocketCommand.Close();
}

void FTP_Client::ReplyLogCode(int code) {
	switch (code) {
	case 200:
		cout << "Command okay";
		break;
	case 500:
		cout << "Syntax error, command unrecognized." << endl;
		cout << "This may include errors such as command line too long.";
		break;
	case 501:
		cout << "Syntax error in parameters or arguments.";
		break;
	case 202:
		cout << "Command not implemented, superfluous at this site.";
		break;
	case 502:
		cout << "Command not implemented.";
		break;
	case 503: 
		cout << "Bad sequence of commands.";
		break;
	case 530:
		cout << "Not logged in.";
		break;
	case 550:
		cout << "Directory not found.";
		break;
	}
	cout << endl;
}

void FTP_Client::CreateAdressServer(const char * addr, const unsigned int & port) {
	addrServer.sin_family = AF_INET;//IPv4
	addrServer.sin_port = htons(port);//gán port
	addrServer.sin_addr.s_addr = inet_addr(addr);//gán địa chỉ IP
	memset(addrServer.sin_zero, 0, sizeof addrServer.sin_zero);
}

void FTP_Client::SendCommand() {
	if (this->ConnectServer() == false) exit(1);
	if (this->LoginServer() == false) exit(1);// 1) Login
	cin.ignore();

	while (1) {
		string str = "";
		char cmd[10], info[INFO];
		int posSpace = -1;

		memset(cmd, 0, sizeof cmd);
		memset(info, 0, sizeof info);

		cout << "ftp>> ";
		getline(cin,str,'\n');

		for (int i = 0; i < str.length(); i++) {
			if (str[i] == ' ') {
				cmd[i] = '\0';
				posSpace = i;
				break;
			}
			cmd[i] = str[i];
		}

		if (posSpace > 0) {
			int size = 0;
			for (int i = posSpace + 1; i < str.length(); i++, size++)
				info[size] = str[i];

			info[size] = '\0';
		}

		memset(buf, 0, sizeof buf);

		//cmd = strlwr(cmd);

		// 2) Liet ke danh sach tap tin
		if (strcmp(cmd, "dir") == 0 || strcmp(cmd, "ls") == 0) {
			this->ShowListFolder(cmd, info);
			continue;
		}

		// 3) Upload 1 file
		if (strcmp(cmd, "put") == 0) {
			this->UploadOneFile(info);
			continue;
		}

		// 4) Download 1 file
		if (strcmp(cmd, "get") == 0) {
			this->DowloadOneFile(info);
			continue;
		}

		// 5) Upload nhiều file
		if (strcmp(cmd, "mput") == 0) {
			this->UploadFiles(info);
			continue;
		}

		// 6) Download nhiều file
		if (strcmp(cmd, "mget") == 0) {
			this->DowloadFiles(info);
			continue;
		}

		// 7) Thay doi duong dan tren server
		if (strcmp(cmd, "cd") == 0) {
			this->ChangeLinkServer(info);
			continue;
		}

		// 8) Thay đổi đường dẫn trên Client
		if (strcmp(cmd, "lcd") == 0) {
			this->ChangeLinkClient(info);
			continue;
		}

		// 9) Xóa 1 File
		if (strcmp(cmd, "dele") == 0 || strcmp(cmd, "delete") == 0) {
			this->DeleteOneFile(info);
			continue;
		}

		// 10) Xóa nhiều file trên Client
		if (strcmp(cmd, "mdele") == 0 || strcmp(cmd, "mdelete") == 0) {
			this->DeleteFiles(info);
			continue;
		}

		// 11)Tạo thư mục mới
		if (strcmp(cmd, "mkdir") == 0) {
			this->CreateFolder(info);
			continue;
		}

		// 12) Xóa thư mục rỗng trên Client
		if (strcmp(cmd, "rmdir") == 0) {
			this->DeleteEmptyFolders(info);
			continue;
		}

		// 13) Hien thi duong dan hien tai tren server
		if (strcmp(cmd, "pwd") == 0) {
			this->ShowLinkCurrent();
			continue;
		}

		// 14) Passive Mode
		if (strcmp(cmd, "passive") == 0 || strcmp(cmd, "active") == 0) {
			this->ChooseModeDataConnection(cmd);
			continue;
		}

		// 15) Thoát
		if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
			this->ExitServer();
			exit(1);
		}

		cout << "Invalid command." << endl;
	}
}

bool FTP_Client::ConnectServer() {
	//Nhap IP Server
	cout << "IP Server: ";
	cin.get(addr, sizeof addr);

	CreateAdressServer(addr, 21);

	if (SocketCommand.Connect((sockaddr*)&addrServer, sizeof addrServer) != 0)
		cout << "Server connection successful." << endl;
	else {
		cout << "Server connection failed!!!" << endl << endl;
		return false;
	}
	/*
	Connection Establishment
	120
	220
	220
	421
	*/

	int tmpres, codeftp;
	char *str;

	printf("Connection established, waiting for welcome message...\n");
	while ((tmpres = SocketCommand.Receive(buf, BUFSIZ, 0)) > 0) {
		sscanf(buf, "%d", &codeftp);
		if (codeftp != 220) { //120, 240, 421: something wrong
			printf("%s", buf);
			SocketCommand.Close();
			return false;
		}
		printf("%s", buf);

		str = strstr(buf, "220");//khi nhận thông báo 220 thì thoát khỏi vòng lặp, tránh ngồi đợi nhận thông báo
		if (str != NULL) break;
		memset(buf, 0, tmpres);
	}
	return true;
}

bool FTP_Client::LoginServer() {
	//Login
	//Send Username 
	int tmpres, codeftp;
	char info[50];

	memset(buf, 0, sizeof buf);
	printf("User ((%s):none): ", addr);
	scanf("%s", info);

	sprintf(buf, "USER %s\r\n", info);
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
		/*
		Login
		USER
		230
		530
		500, 501, 421
		331, 332
		*/
		sscanf(buf, "%d", &codeftp);
		if (codeftp != 331) {
			printf("%s", buf);
			return false;
		}
		printf("%s", buf);
	}

	//--------------------------------------------------------------------------------------//

	//Send Password
	memset(info, 0, sizeof info);
	printf("Password: ");

	//phần ẩn đi password lúc nhập vào
	char c = '.';
	int i = 0;
	while (c != 13 || i == 0)	// 13 là enter 
	{
		if (kbhit()) //nhấn phím thì true
		{
			c = getch();
			if (c != 8) //8 là nút backspace
			{
				info[i] = c;
				i++;
			}
			else
			{
				if (i > 0) {
					i--;
					info[i] = '\0';
				}
			}
		}
	}
	
	cout << endl;

	memset(buf, 0, sizeof buf); 
	sprintf(buf, "PASS %s\r\n", info);
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
		/*
		PASS
		230
		202
		530
		500, 501, 503, 421
		332
		*/
		sscanf(buf, "%d", &codeftp);
		if (codeftp != 230) {
			printf("%s", buf);
			system("pause");
			return false;
		}
		printf("%s", buf);

	}
	isConnected = true;

	return true;
}

bool FTP_Client::UploadOneFile(const char* info){
	int tmpres, codeftp;

	//Bước 1: Chọn mode truyền data
	if (this->SetModeDataConnection() == false) return false;

	//Bước 2: Mở file chuẩn bị đọc file
	ifstream file_In(info, ios::binary);
	if (file_In.fail()) {
		cout << "Can not open this file on Client!!!" << endl;
		file_In.close();
		if (isPassiveMode == false) ClientListen.Close();
		return false;
	}

	//Tách lấy tên
	char fileName[INFO];
	int pos = -1, len = strlen(info);

	memset(fileName, 0, sizeof fileName);
	for (int i = len - 1; i >= 0; i--) {
		if (info[i] == '\\') {
			pos = i;
			break;
		}
	}

	if (pos != -1)
		strcpy(fileName, info + pos + 1);
	else
		strcpy(fileName, info);

	//Bước 3: Gửi câu lệnh
	//STOR <SP> <pathname> <CRLF>
	memset(buf, 0, sizeof buf);
	sprintf(buf, "STOR %s\r\n", fileName);
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);
	/*
	STOR
	125, 150
		(100)
		226, 250
		425, 426, 451, 551, 552
	532, 150, 452, 553
	500, 501, 421, 530
	*/

	//Bước 4: Khởi tạo kênh truyền dữ liệu
	if (this->CreateDataConnection() == false) {
		file_In.close();
		return false;
	}

	//Bước 5: Tiến hành gửi từng BOCK dữ liệu lên Server
	cout << "Please wait. Client is sending this file to server." << endl;

	char data[BLOCK];
	file_In.seekg(0, ios::end);//di chuyển vị trí đọc xuống cuối file
	streampos  size = file_In.tellg();//lấy kích thước file
	
	//Trường hợp kích thức file = 0
	if (!size) {
		file_In.close();
		SocketData.Close();
		//Bước 7: Nhận ACK từ Server
		memset(buf, 0, sizeof buf);
		if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
			sscanf(buf, "%d", &codeftp);
			if (codeftp != 226) {
				printf("%s", buf);
				SocketData.Close();
				return false;
			}
		}
		printf("%s", buf);
		return true;
	}
	file_In.seekg(0, ios::beg);//di chuyển vị trí đọc lên đầu file
	unsigned int n = size / BLOCK;//xác định số lượng BLOCK cần gửi

	for (int i = 0; i < n; i++){
		memset(data, 0, sizeof data);
		file_In.read(data, BLOCK);
		tmpres = SocketData.Send(data, BLOCK, 0);
	}

	//Gửi phần lẻ không đủ tạo BLOCK ở cuối file
	short tmp = size % BLOCK;
	if (tmp != 0){
		memset(data, 0, sizeof BLOCK);
		file_In.read(data, tmp);
		tmpres = SocketData.Send(data, tmp, 0);
	}

	//Bước 6: Đóng file
	file_In.close();
	SocketData.Close();

	//Bước 7: Nhận ACK từ Server
	if (!flag) {
		memset(buf, 0, sizeof buf);
		if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
			sscanf(buf, "%d", &codeftp);
			if (codeftp != 226) {
				printf("%s", buf);
				return false;
			}
		}
		printf("%s", buf);
	}
	return true;
}

bool FTP_Client::DowloadOneFile(const char* info){
	int tmpres, codeftp;

	//Bước 1: Chọn mode truyền data
	if (this->SetModeDataConnection() == false) return false;

	//Bước 2: Gửi câu lệnh
	//RETR <SP> <pathname> <CRLF>
	memset(buf, 0, sizeof buf);
	sprintf(buf, "RETR %s\r\n", info);
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);
	/*
	RETR
	125, 150
		(100)
		226, 250
		425, 426, 451
	450, 550
	500, 501, 421, 530
	*/

	//Bước 3: Khởi tạo kênh truyền dữ liệu
	if (this->CreateDataConnection() == false) return false;


	//Bước 4: Mở file chuẩn bị ghi file
	char fileName[INFO];
	int pos = -1, len = strlen(info);
	memset(fileName, 0, sizeof fileName);

	for (int i = len - 1; i >= 0; i--) {
		if (info[i] == '/') {
			pos = i;
			break;
		}
	}

	if (pos != -1)
		strcpy(fileName, info + pos + 1);
	else
		strcpy(fileName, info);

	ofstream file_Out(fileName, ios::binary);
	if (file_Out.fail()) {
		cout << "Can not create this file on Client!!!" << endl;
		file_Out.close();
		SocketData.Close();
		return false;
	}

	//Bước 5: Nhận từng BOCK dữ liệu từ Server
	char data[BLOCK];
	cout << "Please wait. Client is downloading from server." << endl;

	memset(data, 0, BLOCK);
	while ((tmpres = SocketData.Receive(data, BLOCK, 0)) > 0) {
		file_Out.write(data, tmpres);
		memset(data, 0, BLOCK);
		tmpres = 0;
	}

	//Bước 6: Đóng file và kênh dữ liệu
	file_Out.close();
	SocketData.Close();

	//Bước 7: Nhận ACK
	memset(buf, 0, sizeof buf);
	if (!flag) {
		if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
			sscanf(buf, "%d", &codeftp);
			if (codeftp != 226) {
				printf("%s", buf);
				return false;
			}
		}
		printf("%s", buf);
	}
	return true;
}

bool FTP_Client::UploadFiles(const char * info) {
	string fileName = "";
	string numberFile = "";
	int len = strlen(info);

	stringstream ss(info);
	string text;
	vector<string> files;
	while (ss >> text) {
		CFileFind cff;
		if (cff.FindFile(text.c_str())) {
			bool nextFile;
			do {
				nextFile = cff.FindNextFileA();
				if (!cff.IsDots() && !cff.IsDirectory()) {
					files.push_back(cff.GetFileName().GetString());
				}
			} while (nextFile);
		}
		else {
			std::cout << text << " not found!" << endl;
		}
	}

	bool put = false;
	char choose;
	for (string& fileName : files) {
		std::cout << "put " << fileName << "?";
		choose = cin.get();
		if (choose == 'y' || choose == 'Y' || choose == '\n') {
			if (this->UploadOneFile(fileName.c_str()) == true)
				put = true;
		}
		if (choose != '\n')
			cin.ignore();
	}

	if (put == false) {
		cout << "Cannot find list in Client." << endl;
		return false;
	}
	return true;
}

bool FTP_Client::DowloadFiles(const char * info){
	int tmpres, codeftp;
	string fileName = "";
	int len = strlen(info);

	//Bước 1: TYPE A
	memset(buf, 0, sizeof buf);
	//TYPE <SP> <type-code> <CRLF>
	sprintf(buf, "TYPE A\r\n");
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = SocketCommand.Receive(buf, BUFSIZ, 0);

	/*
	TYPE
	200
	500, 501, 502, 421, 530
	*/
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 200) {
		printf("%s", buf);
		return false;
	}
	printf("%s", buf);

	//Bước 2: NLST fileNam
	infoFiles = "";
	for (int i = 0; i < len; i++) {
		if (info[i] != ' ') fileName.push_back(info[i]);
		if (info[i] == ' ' || (i + 1) == len) {
			cout << fileName << " in Server?" << endl;
			this->ShowListFolder("ls", fileName.c_str());
			fileName = "";
		}
	}
	if (infoFiles == "") {
		cout << "Cannot find list of remote files." << endl;
		return false;
	}
	//Bước 3: TYPE A 
	memset(buf, 0, sizeof buf);
	//TYPE <SP> <type-code> <CRLF>
	sprintf(buf, "TYPE A\r\n");
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = SocketCommand.Receive(buf, BUFSIZ, 0);

	/*
	TYPE
	200
	500, 501, 502, 421, 530
	*/
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 200) {
		printf("%s", buf);
		return false;
	}
	printf("%s", buf);

	//Bước 4: Download từng file
	if (infoFiles.length() == 0) return false;

	stringstream ss(infoFiles);
	
	bool got = false;
	char choose;
	while (getline(ss, fileName, '\r')) {
		ss.get(); // skip \n
		cout << "get " << fileName << "?";
		choose = cin.get();
		if (choose == 'y' || choose == 'Y' || choose == '\n') {
			if (this->DowloadOneFile(fileName.c_str()) == true)
				got = true;
		}
		if (choose != '\n')
			cin.ignore();
	}

	if (got == false) { 
		cout << "Cannot find list of remote files." << endl; 
		return false;
	}
	return true;
}

bool FTP_Client::ShowListFolder(const char* mode, const char* info){
	int tmpres, codeftp;
	//Bước 1: Chọn mode truyền data
	if (this->SetModeDataConnection() == false) return false;

	//Bước 2: Gửi lệnh
	memset(buf, 0, sizeof buf);
	if (strcmp(mode, "ls") == 0) {
		//NLST [<SP> <pathname>]<CRLF>
		sprintf(buf, "NLST %s\r\n", info);
		tmpres = SocketCommand.Send(buf, strlen(buf), 0);
	}
	/*
	NLST
	125, 150
	226, 250
	425, 426, 451
	450
	500, 501, 502, 421, 530
	*/

	//---------------------------------------------------
	if (strcmp(mode, "dir") == 0) {
		//LIST [<SP> <pathname>]<CRLF>
		sprintf(buf, "LIST %s\r\n", info);
		tmpres = SocketCommand.Send(buf, strlen(buf), 0);
	}
	/*
	LIST
	125, 150
		226, 250
		425, 426, 451
	450
	500, 501, 502, 421, 530
	*/
	
	//Bước 3: Khởi tạo kênh truyền dữ liệu
	if (this->CreateDataConnection() == false) return false;

	//Bước 4: Nhận từng BOCK dữ liệu từ Server
	char data[BLOCK];

	memset(data, 0, sizeof data);
	memset(buf, 0, sizeof buf);

	while (tmpres = SocketData.Receive(data, BLOCK, 0) > 0) {
		printf("%s", data);
		infoFiles += data;
		memset(data, 0, sizeof data);
	}

	//Bước 5: Nhận thông báo từ Server
	memset(buf, 0, sizeof buf);
	if (flag == false) {
		if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
			sscanf(buf, "%d", &codeftp);
			if (codeftp != 226) {
				printf("%s", buf);
				return false;
			}
		}
		printf("%s", buf);
	}

	memset(buf, 0, sizeof buf);
	//Bước 6: Đóng kênh dữ liệu
	SocketData.Close();
	return true;
}

bool FTP_Client::ChangeLinkServer(const char* info){
	int tmpres, codeftp;

	if (strcmp(info, "..") != 0) {
		//CWD <SP> <pathname> <CRLF>
		sprintf(buf, "CWD %s\r\n", info);
	}
	else {
		//CDUP <SP> <pathname> <CRLF>
		sprintf(buf, "CDUP %s\r\n", info);
	}

	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) != 0) {
		/*
		CWD
		250
		500, 501, 502, 421, 530, 550
		*/

		/*
		CDUP
		200
		500, 501, 502, 421, 530, 550
		*/

		sscanf(buf, "%d", &codeftp);
		if (codeftp != 250 && codeftp != 200) {
			printf("%s", buf);
			return false;
		}
	}
	printf("%s", buf);

	return true;
}

bool FTP_Client::ChangeLinkClient(const char* info) {
	TCHAR szDir[MAX_PATH];
	string folder(info);

	if (folder.size()>0) {
		if (folder.back() != '\\')
			folder.push_back('\\');

		if (SetCurrentDirectory(folder.c_str()) == 0) {
			GetCurrentDirectory(MAX_PATH, szDir);

			cout << "Local directory now " << szDir << endl;
			return false;
		}
	}

	GetCurrentDirectory(MAX_PATH, szDir);
	cout << "Local directory now " << szDir << endl;

	return true;
}

bool FTP_Client::DeleteOneFile(const char* info){
	int tmpres, codeftp;

	memset(buf, 0, sizeof buf);
	//DELE <SP> <pathname> <CRLF>
	sprintf(buf, "DELE %s\r\n", info);
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = SocketCommand.Receive(buf, BUFSIZ, 0);

	/*
	DELE
	250
	500, 501, 502, 421, 530
	*/
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 250) {
		printf("%s", buf);
		return false;
	}
	printf("%s", buf);

	return true;
}

bool FTP_Client::DeleteFiles(const char * info){
	int tmpres, codeftp;
	string fileName = "";
	int len = strlen(info);

	//Bước 1: TYPE A
	memset(buf, 0, sizeof buf);
	//TYPE <SP> <type-code> <CRLF>
	sprintf(buf, "TYPE A\r\n");
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = SocketCommand.Receive(buf, BUFSIZ, 0);

	/*
	TYPE
	200
	500, 501, 502, 421, 530
	*/
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 200) {
		printf("%s", buf);
		return false;
	}
	printf("%s", buf);

	//Bước 2: NLST fileNam
	infoFiles = "";
	for (int i = 0; i < len; i++) {
		if (info[i] != ' ') fileName.push_back(info[i]);
		if (info[i] == ' ' || (i + 1) == len) {
			cout << fileName << " in Server?" << endl;
			this->ShowListFolder("ls", fileName.c_str());
			fileName = "";
		}
	}

	//Bước 3: TYPE A
	memset(buf, 0, sizeof buf);
	//TYPE <SP> <type-code> <CRLF>
	sprintf(buf, "TYPE A\r\n");
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = SocketCommand.Receive(buf, BUFSIZ, 0);

	/*
	TYPE
	200
	500, 501, 502, 421, 530
	*/
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 200) {
		printf("%s", buf);
		return false;
	}
	printf("%s", buf);

	//Bước 4: Delete tung file
	if (infoFiles.length() == 0) return false;

	stringstream ss(infoFiles);

	bool del = false;
	char choose;
	while (getline(ss, fileName, '\r')) {
		ss.get(); // skip \n
		cout << "get " << fileName << " y/n? ";
		choose = cin.get();
		if (choose == 'y' || choose == 'Y' || choose == '\n') {
			if (this->DeleteOneFile(fileName.c_str()) == true)
				del = true;
		}
		if (choose != '\n')
			cin.ignore();
	}

	if (del == false) {
		cout << "Cannot find list of remote files." << endl;
		return false;
	}
	return true;
}

bool FTP_Client::CreateFolder(const char* info){
	int tmpres, codeftp;

	memset(buf, 0, sizeof buf);
	//MKD <SP> <pathname> <CRLF>
	sprintf(buf, "MKD %s\r\n", info);
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = SocketCommand.Receive(buf, BUFSIZ, 0);

	/*
	MKD
	257
	500, 501, 502, 421, 530, 550
	*/
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 257) {
		printf("%s", buf);
		return false;
	}
	printf("%s", buf);

	return true;
}

bool FTP_Client::DeleteEmptyFolders(const char * info){
	int tmpres, codeftp;

	memset(buf, 0, sizeof buf);
	//RMD <SP> <pathname> <CRLF>
	sprintf(buf, "RMD %s\r\n", info);
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
		/*
		RMD
		250
		500, 501, 502, 421, 530, 550
		*/
		sscanf(buf, "%d", &codeftp);
		if (codeftp != 250) {
			printf("%s", buf);
			return false;
		}
	}
	printf("%s", buf);

	return true;
}

bool FTP_Client::ShowLinkCurrent(){
	int tmpres, codeftp;

	memset(buf, 0, sizeof buf);
	//PWD <CRLF>
	sprintf(buf, "PWD\r\n");
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = SocketCommand.Receive(buf, BUFSIZ, 0);

	/*
	PWD
	257
	500, 501, 502, 421, 550
	*/
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 257) {
		printf("%s", buf);
		return false;
	}
	printf("%s", buf);

	return true;
}

bool FTP_Client::ActiveModeServer(){
	if (isConnected == false) {
		cout << "Please log - in before using!!!!";
		return false;
	}

	//Active mode: thì Server sẽ khởi tạo kênh dữ liệu tại port số 20
	//---------------------------------------------------------------------------//

	int tmpres, codeftp, port;

	//Bước 1: Client tạo Socket lắng nghe kết nối từ phía Server
	ClientListen.Create();

	//Bước 2: Lấy địa chỉ IP và port của Socket vừa tạo gửi cho Server biết để kết nôi đến đúng port bên Client
	//Lưu vào addrDataClient

	//Bước này chỉ lấy được port của ClientListen mà không lấy được ip máy mình
	int len = sizeof(addrDataClient);
	getsockname(ClientListen, (sockaddr*)&addrDataClient, &len);
	port = ntohs(addrDataClient.sin_port);//Lấy port

	//Nên ta lấy ip trung gian thông qua SocketCommand
	getsockname(SocketCommand, (sockaddr*)&addrDataClient, &len);//Lấy IP, port lưu vào addrDataClient
	addrDataClient.sin_family = AF_INET;//IPv4
	memset(addrDataClient.sin_zero, 0, sizeof addrDataClient.sin_zero);//không dùng
	addrDataClient.sin_port = htons(port);//gán lại port của SocketListen

	//Lắng nghe 1 kết nối từ Server
	if (ClientListen.Listen(1) == FALSE) {
		ClientListen.Close();
		return false;
	}


	//Bước 3: Gửi IP, port máy Client cho Server biết để nó kết nối
	memset(buf, 0, sizeof buf);

	sprintf(buf, "PORT %i,%i,%i,%i,%i,%i\r\n", addrDataClient.sin_addr.S_un.S_un_b.s_b1, addrDataClient.sin_addr.S_un.S_un_b.s_b2, addrDataClient.sin_addr.S_un.S_un_b.s_b3, addrDataClient.sin_addr.S_un.S_un_b.s_b4, port / 256, port % 256);
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	//Bước 4: Client nhận ACK sau lệnh PORT từ Server
	memset(buf, 0, sizeof buf);
	if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
		/*
		PORT
		200
		500, 501, 421, 530
		*/
		sscanf(buf, "%d", &codeftp);
		if (codeftp != 200) {
			printf("%s", buf);
			return false;
		}
	}
	printf("%s", buf);
	return true;
}

bool FTP_Client::PassiveModeServer(){
		if (isConnected == false) {
			cout << "Please log - in before using!!!!";
			return false;
		}

		//Pasive mode: thì Server sẽ lắng nghe kết nối từ Client
		//---------------------------------------------------------------------------//

		int tmpres, codeftp, port;

		//Bước 1: Gửi lệnh yêu cầu Server chuyển sang chế độ Passive
		memset(buf, 0, sizeof buf);
		sprintf(buf, "PASV\r\n");
		tmpres = SocketCommand.Send(buf, strlen(buf), 0);

		/*
		PASV
		227
		500, 501, 502, 421, 530
		*/

		memset(buf, 0, sizeof buf);
		if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
			sscanf(buf, "%d", &codeftp);
			if (codeftp != 227) {
				printf("%s", buf);
				return false;
			}
		}

		printf("%s", buf);

		int b1, b2, b3, b4, p1, p2;
		char tmp;
		string str = buf;
		str.erase(0, 27);

		sscanf(str.c_str(), "%d%c%d%c%d%c%d%c%d%c%d", &b1, &tmp, &b2, &tmp, &b3, &tmp, &b4, &tmp, &p1, &tmp, &p2);
		
		//Bước 2: Lưu lại IP và port mà Server thông báo
		memset(buf, 0, sizeof buf);
		sprintf(buf, "%d.%d.%d.%d", b1, b2, b3, b4);

		addrDataServer.sin_family = AF_INET;//IPv4
		addrDataServer.sin_port = htons(p1 * 256 + p2);//gán port
		addrDataServer.sin_addr.s_addr = inet_addr(buf);//gán địa chỉ IP
		memset(addrDataServer.sin_zero, 0, sizeof addrServer.sin_zero);//không dùng

		return true;
}

bool FTP_Client::SetModeDataConnection(){
	if (isPassiveMode == true)
		if (this->PassiveModeServer() == false) return false;
	if (isPassiveMode == false)
		if (this->ActiveModeServer() == false) return false;
	return true;
}

bool FTP_Client::CreateDataConnection(){
	int tmpres, codeftp;
	flag = false;

	//Passive mode
	if (isPassiveMode == true) {
		SocketData.Create();

		if (SocketData.Connect((sockaddr*)&addrDataServer, sizeof addrDataServer) == 0) {
			cout << "Creating data connection fail!!!" << endl;
			SocketData.Close();
			return false;
		}
		memset(buf, 0, sizeof buf);
		if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
			sscanf(buf, "%d", &codeftp);
			if (codeftp != 150) {
				printf("%s", buf);
				SocketData.Close();
				return false;
			}
			cout << buf;
		}

		char *str;
		str = strstr(buf, "226");//Why ???
		if (str != NULL) flag = true;

		memset(buf, 0, sizeof buf);
		return true;
	}

	//Active mode
	//Server thực hiện kết nối đến Client trên SocketData
	memset(buf, 0, sizeof buf);
	if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
		sscanf(buf, "%d", &codeftp);
		if (codeftp != 150) {
			printf("%s", buf);
			ClientListen.Close();
			SocketData.Close();
			return false;
		}
		printf("%s", buf);
	}

	char *str;
	str = strstr(buf, "226");//Why ???
	if (str != NULL) flag = true;

	if (ClientListen.Accept(SocketData) == 0) {
		cout << "Creating data connection fail!!!" << endl;
		ClientListen.Close();
		return false;
	}
	memset(buf, 0, sizeof buf);

	ClientListen.Close();
	return true;
}

bool FTP_Client::ChooseModeDataConnection(const char *mode){
	if (strcmp(mode, "passive") == 0) {
		isPassiveMode = true;
		cout << "Passive mode is enabled." << endl;
	}
	if (strcmp(mode, "active") == 0) {
		isPassiveMode = false;
		cout << "Active mode is enabled." << endl;
	}
	return true;
}

bool FTP_Client::ExitServer() {
	int tmpres, codeftp;

	memset(buf, 0, sizeof buf);
	//QUIT <CRLF>
	sprintf(buf, "QUIT\r\n");
	tmpres = SocketCommand.Send(buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	if (tmpres = SocketCommand.Receive(buf, BUFSIZ, 0) > 0) {
		/*
		QUIT
		221
		500
		*/
		sscanf(buf, "%d", &codeftp);
		if (codeftp != 221) {
			printf("%s", buf);
			SocketCommand.Close();
			return false;
		}
		printf("%s", buf);
	}
	SocketCommand.Close();
	return true;
}