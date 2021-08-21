/*
THÔNG TIN NHÓM - CQ2016/31
1612406 - Đặng Phương Nam
1612426 - Hoàng Nghĩa
1612427 - Nguyễn Xuân Nghiêm
*/
#include "stdafx.h"
#include "FTP_Client.h"

int main()
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);
	if (hModule != nullptr)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			wprintf(L"Fatal Error: MFC initialization failed\n");
			nRetCode = 1;
		}
		else
		{
			// TODO: code your application's behavior here.

			FTP_Client Host;
			Host.SendCommand();
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		wprintf(L"Fatal Error: GetModuleHandle failed\n");
		nRetCode = 1;
	}

	return nRetCode;
}