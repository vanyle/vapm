// Process management for windows and linux. HEADER ONLY
// More features for windows though
// int exec(const char * command,std::string * result); available on both though.

#include <cassert>
#include <string>
#include <stdio.h>

#ifdef OUTDATED_WINDOWS_ONLY_CODE

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

// Header lib to start child processes on windows.
// Microsoft API wrapper.
struct ChildProcess
{

	std::string startCommand;

	SECURITY_ATTRIBUTES saAttr;
	HANDLE stdinHandleRead = NULL;
	HANDLE stdinHandleWrite = NULL;
	HANDLE stdoutHandleRead = NULL;
	HANDLE stdoutHandleWrite = NULL;

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;

	bool isOk = true; // add better errors ?
	bool isClosed = false;

	ChildProcess(std::string command);
	~ChildProcess();

	void start();
	void close();

	unsigned long wait(int timeout = 0x7f800000); // infinity by default. Returns the return value of the process.
	unsigned long write(char *buffer, int size);  // return number of bytes written
	unsigned long read(char *buffer, int size);	  // return number of bytes read (0 if fail)
};

ChildProcess::ChildProcess(std::string cmd)
{
	// Set the bInheritHandle flag so pipe handles are inherited.
	this->startCommand = cmd;
	this->saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	this->saAttr.bInheritHandle = true;
	this->saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT.
	if (!CreatePipe(&this->stdoutHandleRead, &this->stdoutHandleWrite, &this->saAttr, 0))
	{
		isOk = false;
		printf("Stdout CreatePipe error.\n");
		return;
	}
	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(this->stdoutHandleRead, HANDLE_FLAG_INHERIT, 0))
	{
		isOk = false;
		printf("Stdout SetHandleInformation error.\n");
		return;
	}

	// Create a pipe for the child process's STDIN.
	if (!CreatePipe(&this->stdinHandleRead, &this->stdinHandleWrite, &this->saAttr, 0))
	{
		isOk = false;
		printf("Stdin CreatePipe error.\n");
		return;
	}
	// Ensure the write handle to the pipe for STDIN is not inherited.
	if (!SetHandleInformation(this->stdinHandleWrite, HANDLE_FLAG_INHERIT, 0))
	{
		isOk = false;
		printf("Stdin SetHandleInformation error.\n");
		return;
	}

	// Set up members of the PROCESS_INFORMATION structure.
	ZeroMemory(&this->piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure.
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&this->siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = stdoutHandleWrite; // same stderr and stdout. (change this?)
	siStartInfo.hStdOutput = stdoutHandleWrite;
	siStartInfo.hStdInput = stdinHandleRead;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Ready to create process!
}
void ChildProcess::start()
{
	// Create the child process.

	char cmdCopy[this->startCommand.size()];
	strcpy(cmdCopy, this->startCommand.c_str());
	bool result = CreateProcess(NULL,
								cmdCopy,			// command line
								NULL,				// process security attributes
								NULL,				// primary thread security attributes
								TRUE,				// handles are inherited
								0,					// creation flags
								NULL,				// use parent's environment
								NULL,				// use parent's current directory
								&this->siStartInfo, // STARTUPINFO pointer
								&this->piProcInfo); // receives PROCESS_INFORMATION

	// If an error occurs, exit the application.
	if (!result)
	{
		printf("CreateProcess error:\n");
		char *messageBuffer;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&messageBuffer, 0, NULL);

		printf("[%lu] %s \n", GetLastError(), messageBuffer);
		this->isOk = false;
		return;
	}
}
ChildProcess::~ChildProcess()
{
	if (!this->isClosed)
	{
		this->close();
	}
}
void ChildProcess::close()
{
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);

	CloseHandle(this->stdinHandleWrite);
	CloseHandle(this->stdoutHandleRead);
	this->isClosed = true;
}
unsigned long ChildProcess::wait(int timeout)
{ // timeout in milliseconds. Default value is infinity.
	WaitForSingleObject(this->piProcInfo.hProcess, timeout);
	unsigned long exitCode;
	GetExitCodeProcess(this->piProcInfo.hProcess, &exitCode);
	return exitCode;
}
unsigned long ChildProcess::write(char *buffer, int size)
{
	unsigned long bytesWritten;
	bool result = WriteFile(stdinHandleWrite, buffer, size, &bytesWritten, NULL);
	if (!result)
		return 0;
	return bytesWritten;
}

unsigned long ChildProcess::read(char *buffer, int size)
{
	unsigned long bytesRead; // amount read
	printf("Read.\n");

	COMMTIMEOUTS timeoutInfo = {
		0,	//interval timeout. 0 = not used
		0,	// read multiplier
		10, // read constant (milliseconds)
		0,	// Write multiplier
		0	// Write Constant
	};
	SetCommTimeouts(this->stdoutHandleRead, &timeoutInfo);
	bool result = ReadFile(this->stdoutHandleRead, buffer, size, &bytesRead, NULL);
	if (!result)
		return 0;
	return bytesRead;
}

int exec(const char *cmd, std::string *result)
{
	ChildProcess cp = ChildProcess(std::string(cmd));
	cp.start();
	char buffer[128];
	unsigned long amountRead = 0;
	if (!cp.isOk)
	{
		printf("Not ok.\n");
		return 1;
	}

	printf("Read:\n");
	while ((amountRead = cp.read(buffer, sizeof buffer)) != 0)
	{
		result->append(buffer, amountRead);
		if (amountRead < sizeof buffer)
		{
			break;
		}
	}
	int exitCode = cp.wait();
	cp.close();
	return exitCode;
}

#else

// PORTABLE VERSION
// Seems to work for every os.

int exec(const char *cmd, std::string *result, FILE *outputStream)
{
	char buffer[128];
	buffer[127] = 0; // setup null terminated buffer.

	*result = "";
	FILE *pipe = _popen(cmd, "r");
	if (pipe == NULL)
	{
		assert("popen failed!");
	}
	try
	{
		long readCount = 0;
		while ((readCount = fread(buffer, sizeof(char), sizeof(buffer) - 1, pipe)))
		{
			*result += buffer;
			if (outputStream != 0)
			{
				fwrite(buffer, sizeof(char), readCount, outputStream);
			}

			if (feof(pipe) == EOF || ferror(pipe) != 0)
			{
				break;
			}
		}
	}
	catch (...)
	{
		return _pclose(pipe);
	}
	return _pclose(pipe);
}

#endif