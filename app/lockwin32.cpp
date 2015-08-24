#include "Lock.h"


#ifndef WIN32
#include <sys/time.h>
#include <sys/uio.h>
#include <pthread.h>
#else
#include <Windows.h>
#endif

#include <cstdlib>


//////////////////////////////////////////////////////////////////////////
// Win32Mutex
class Win32Mutex : public MutexInterface
{
public:
	Win32Mutex();
	virtual ~Win32Mutex();

	virtual int Lock();
	virtual int UnLock();

private:
	HANDLE m_Handle;
};

Win32Mutex::Win32Mutex()
{
	m_Handle = CreateMutex(NULL, FALSE, NULL);
}

Win32Mutex::~Win32Mutex()
{
	CloseHandle(m_Handle);
}

int Win32Mutex::Lock()
{
	DWORD result = WaitForSingleObject(m_Handle, INFINITE);
	if (result == WAIT_OBJECT_0)
		return 0;
	else
		return -1;
}

int Win32Mutex::UnLock()
{
	ReleaseMutex(m_Handle);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Mutex
Mutex::Mutex()
{
	m_Delegate = new Win32Mutex();
}