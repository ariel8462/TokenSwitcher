#include <iostream>
#include <Windows.h>
#include "../TokenSwitcher//TokenSwitcherCommon.h"

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cout << "./TokenSwitcherClient <pid>" << std::endl;
		return 0;
	}
    auto hFile = CreateFile(L"\\\\.\\TokenSwitcher", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cout << "Error in openning a handle to the device" << GetLastError() << std::endl;
		return 1;
	}
	ULONG pid = atoi(argv[1]);
	bool success;

	success = DeviceIoControl(hFile, IOCTL_TOKEN_SWITCHER_SWITCH_TOKEN,
		&pid, sizeof(pid),
		nullptr, 0,
		nullptr, 0
		);
	
	if (success)
	{
		std::cout << "Switched the token of process - " << pid << std::endl;
	}
	else
	{
		std::cout << "Failed switching the token of - " << pid << " - " << GetLastError << std::endl;
	}
	CloseHandle(hFile);
	
	return 0;
}
