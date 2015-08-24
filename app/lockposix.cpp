#if defined(__SYMBIAN32__)
#include <stdio.h>
#endif
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include "Lock.h"


//////////////////////////////////////////////////////////////////////////
// PosixMutex
class PosixMutex : public MutexInterface
{
public:
	PosixMutex();
	virtual ~PosixMutex();

	virtual int Lock();
	virtual int UnLock();

private:
	pthread_mutex_t m_Mutex;
};

PosixMutex::PosixMutex()
{
	pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&m_Mutex, &attr);
}

PosixMutex::~PosixMutex()
{
	pthread_mutex_destroy(&m_Mutex);
}

int PosixMutex::Lock()
{
    pthread_mutex_lock(&m_Mutex);
    return 0;
}

int PosixMutex::UnLock()
{
	pthread_mutex_unlock(&m_Mutex);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Mutex
Mutex::Mutex()
{
	m_Delegate = new PosixMutex();
}