#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#if !defined(_WIN32_WCE)
#include <sys/timeb.h>
#include <process.h>
#endif
#endif

#include <stdio.h>
#include "threads.h"
#include "results.h"
#include "lock.h"


#if defined(_WIN32_WCE) || defined(_XBOX)
#define ZHD_WIN32_USE_CREATE_THREAD
#endif

#if defined(ZHD_WIN32_USE_CREATE_THREAD)
#define _beginthreadex(security, stack_size, start_proc, arg, flags, pid) \
	CreateThread(security, stack_size, (LPTHREAD_START_ROUTINE) start_proc,   \
	arg, flags, (LPDWORD)pid)
#define _endthreadex ExitThread
#endif



//////////////////////////////////////////////////////////////////////////
// HelperEnvironment
int HelperEnvironment::Get(const char* name, std::string& value)
{
	return ZHD_SUCCESS;
}

int HelperEnvironment::Set(const char* name, const char* value)
{
	return ZHD_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// HelperSystem
int HelperSystem::GetProcessID(unsigned int& id)
{
	id = 0;
	return ZHD_SUCCESS;
}

int HelperSystem::GetMachineName(std::string& name)
{
	return HelperEnvironment::Get("COMPUTERNAME", name);
}

int HelperSystem::GetCurrentTimeStamp(TimeStamp& now)
{
	struct _timeb time_stamp;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	_ftime_s(&time_stamp);
#else
	_ftime(&time_stamp);
#endif
	now.SetNanos(((__int64)time_stamp.time)     * 1000000000UL +
		((__int64)time_stamp.millitm) * 1000000);

	return ZHD_SUCCESS;
}

int HelperSystem::Sleep(const TimeStamp& duration)
{
	::Sleep((unsigned int)duration.ToMillis());

	return ZHD_SUCCESS;
}

int HelperSystem::SleepUntil(const TimeStamp& when)
{
	TimeStamp now;
	GetCurrentTimeStamp(now);
	if (when > now) {
		TimeStamp duration = when-now;
		return Sleep(duration);
	} else {
		return ZHD_SUCCESS;
	}
}

int HelperSystem::SetRandomSeed(unsigned int seed)
{
	srand(seed);
	return ZHD_SUCCESS;
}

unsigned int HelperSystem::GetRandomInteger()
{
	static bool seeded = false;
	if (seeded == false)
	{
		TimeStamp now;
		GetCurrentTimeStamp(now);
		srand((unsigned int)now.ToNanos());
		seeded = true;
	}

	return rand();
}


//////////////////////////////////////////////////////////////////////////
// Win32Event class
class Win32Event
{
public:
	Win32Event(bool manual = false, bool initial = false);
	virtual ~Win32Event();

	virtual int Wait(int timeout = -1);
	virtual void Signal();
	virtual void Reset();

private:
	HANDLE m_Event;
};

Win32Event::Win32Event(bool manual /* = false */, bool initial /* = false */)
{
	m_Event = CreateEvent(NULL, (manual == true)?TRUE:FALSE, (initial == true)?TRUE:FALSE, NULL);
}

Win32Event::~Win32Event()
{
	CloseHandle(m_Event);
}

int Win32Event::Wait(int timeout /* = -1 */)
{
	if (m_Event)
	{
		DWORD result = WaitForSingleObject(m_Event, (timeout==-1)?INFINITE:timeout);
		if (result == WAIT_TIMEOUT)
			return -1;
		if (result != WAIT_OBJECT_0 && result != WAIT_ABANDONED)
			return -1;
	}
	return 0;
}

void Win32Event::Signal()
{
	SetEvent(m_Event);
}

void Win32Event::Reset()
{
	ResetEvent(m_Event);
}


//////////////////////////////////////////////////////////////////////////
// Win32CriticalSection
class Win32CriticalSection
{
public:
	Win32CriticalSection();
	~Win32CriticalSection();

	int Lock();
	int Unlock();

private:
	CRITICAL_SECTION m_CriticalSection;
};

Win32CriticalSection::Win32CriticalSection()
{
	InitializeCriticalSection(&m_CriticalSection);
}

Win32CriticalSection::~Win32CriticalSection()
{
	DeleteCriticalSection(&m_CriticalSection);
}

int Win32CriticalSection::Lock()
{
	EnterCriticalSection(&m_CriticalSection);
	return 0;
}

int Win32CriticalSection::Unlock()
{
	LeaveCriticalSection(&m_CriticalSection);
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// Win32SharedVariable
class Win32SharedVariable : public SharedVariableInterface
{
public:
	Win32SharedVariable(int value);
	~Win32SharedVariable();

	void       SetValue(int value);
	int        GetValue();
	int WaitUntilEquals(int value, int timeout = ZHD_TIMEOUT_INFINITE);
	int WaitWhileEquals(int value, int timeout = ZHD_TIMEOUT_INFINITE);
	int WaitWhileOrUntilEquals(int value, int timeout, bool until);

private:
	// members
	volatile int          m_Value;
	volatile unsigned int m_Waiters;
	Mutex             m_Lock;
	Win32Event        m_Event;
};

Win32SharedVariable::Win32SharedVariable(int value)
	: m_Value(value)
	, m_Waiters(0)
	, m_Event(true)
{
}

Win32SharedVariable::~Win32SharedVariable()
{
}

void Win32SharedVariable::SetValue(int value)
{
	m_Lock.Lock();
	if (value != m_Value) {
		m_Value = value;
		if (m_Waiters) m_Event.Signal();
	}
	m_Lock.UnLock();
}

int Win32SharedVariable::GetValue()
{
	return m_Value;
}

int Win32SharedVariable::WaitWhileOrUntilEquals(int value, int timeout, bool until)
{
	do {
		m_Lock.Lock();
		if (until) {
			if (m_Value == value) break;
		} else {
			if (m_Value != value) break;
		}
		++m_Waiters;
		m_Lock.UnLock();

		int result = m_Event.Wait(timeout);
		bool last_waiter = true;
		m_Lock.Lock();
		if (--m_Waiters == 0) {
			m_Event.Reset();
		} else {
			last_waiter = false;
		}
		m_Lock.UnLock();

		if (ZHD_FAILED(result)) return result;

		// FIXME: this is suboptimal, but we need it to ensure we don't busy-loop
		if (!last_waiter) {
			Sleep(10);
		}
	} while (true);

	m_Lock.UnLock();

	return ZHD_SUCCESS;
}

int Win32SharedVariable::WaitUntilEquals(int value, int timeout /* = ZHD_TIMEOUT_INFINITE */)
{
	return WaitWhileOrUntilEquals(value, timeout, true);
}

int Win32SharedVariable::WaitWhileEquals(int value, int timeout /* = ZHD_TIMEOUT_INFINITE */)
{
	return WaitWhileOrUntilEquals(value, timeout, false);
}

SharedVariable::SharedVariable(int value /* = 0 */)
{
	m_Delegate = new Win32SharedVariable(value);
}


//////////////////////////////////////////////////////////////////////////
// Win32Thread class
class Win32Thread : public ThreadInterface
{
public:
	// class methods
	static int SetPriority(HANDLE thread, int priority);
	static int GetPriority(HANDLE thread, int& priority);

	Win32Thread(Thread* delagator, Runnable& target, bool detached);
	~Win32Thread();

	int Start();
	int Wait(int timeout = ZHD_TIMEOUT_INFINITE);
	int GetPriority(int& priority);
	int SetPriority(int priority);

private:
	static unsigned int __stdcall EntryPoint(void* argument);
	void Run();
	int Interrupt() { return ZHD_ERROR_NOT_IMPLEMENTED; }

	Thread* m_Delegator;
	Runnable& m_Target;
	bool m_Detached;
	HANDLE m_ThreadHandle;
};

Win32Thread::Win32Thread(Thread* delagator, Runnable& target, bool detached)
	: m_Delegator(delagator)
	, m_Target(target)
	, m_Detached(detached)
	, m_ThreadHandle(0)
{

}

Win32Thread::~Win32Thread()
{
	if (!m_Detached)
		Wait();
	
	if (m_ThreadHandle)
		CloseHandle(m_ThreadHandle);
}

int Win32Thread::SetPriority(HANDLE thread, int priority)
{
	int win32_priority;
	if (priority < ZHD_THREAD_PRIORITY_LOWEST) {
		win32_priority = THREAD_PRIORITY_IDLE;
	} else if (priority < ZHD_THREAD_PRIORITY_BELOW_NORMAL) {
		win32_priority = THREAD_PRIORITY_LOWEST;
	} else if (priority < ZHD_THREAD_PRIORITY_NORMAL) {
		win32_priority = THREAD_PRIORITY_BELOW_NORMAL;
	} else if (priority < ZHD_THREAD_PRIORITY_ABOVE_NORMAL) {
		win32_priority = THREAD_PRIORITY_NORMAL;
	} else if (priority < ZHD_THREAD_PRIORITY_HIGHEST) {
		win32_priority = THREAD_PRIORITY_ABOVE_NORMAL;
	} else if (priority < ZHD_THREAD_PRIORITY_TIME_CRITICAL) {
		win32_priority = THREAD_PRIORITY_HIGHEST;
	} else {
		win32_priority = THREAD_PRIORITY_TIME_CRITICAL;
	}
	BOOL result = ::SetThreadPriority(thread, win32_priority);
	if (!result) {
		printf("SetThreadPriority failed (%x)", GetLastError());
		return ZHD_FAILURE;
	}

	return ZHD_SUCCESS;
}

int Win32Thread::GetPriority(HANDLE thread, int& priority)
{
	int win32_priority = ::GetThreadPriority(thread);
	if (win32_priority == THREAD_PRIORITY_ERROR_RETURN) {
		printf("GetThreadPriority failed (%x)", GetLastError());
		return ZHD_FAILURE;
	}

	priority = win32_priority;

	return ZHD_SUCCESS;
}

unsigned int __stdcall Win32Thread::EntryPoint(void* argument)
{
	Win32Thread* thread = reinterpret_cast<Win32Thread*>(argument);

	printf("thread in =======================");

	// set random seed per thread
	//NPT_TimeStamp now;
	//NPT_System::GetCurrentTimeStamp(now);
	//NPT_System::SetRandomSeed((NPT_UInt32)(now.ToNanos()) + ::GetCurrentThreadId());

	// run the thread 
	thread->Run();

	// if the thread is detached, delete it
	if (thread->m_Detached) {
		delete thread->m_Delegator;
	}

	return 0;
}

int Win32Thread::Start()
{
	if (m_ThreadHandle > 0) {
		// failed
		printf("thread already started !");
		return ZHD_ERROR_INVALID_STATE;
	}

	printf("creating thread");

	// create the native thread
#if defined(_WIN32_WCE)
	DWORD thread_id;
#else
	unsigned int thread_id;
#endif
	// create a stack local variable 'detached', as this object
	// may already be deleted when _beginthreadex returns and
	// before we get to call detach on the given thread
	bool detached = m_Detached;

	HANDLE thread_handle = (HANDLE)
		_beginthreadex(NULL, 
		0, 
		EntryPoint, 
		reinterpret_cast<void*>(this), 
		0, 
		&thread_id);
	if (thread_handle == 0) {
		// failed
		return ZHD_FAILURE;
	}

	if (detached) {
		CloseHandle(thread_handle);
	} else {
		m_ThreadHandle = thread_handle;
	}

	return ZHD_SUCCESS;
}

void Win32Thread::Run()
{
	m_Target.Run();
}

int Win32Thread::SetPriority(int priority)
{
	if (m_ThreadHandle == 0) return ZHD_ERROR_INVALID_STATE;
	return Win32Thread::SetPriority(m_ThreadHandle, priority);
}

int Win32Thread::GetPriority(int& priority)
{
	if (m_ThreadHandle == 0) return ZHD_ERROR_INVALID_STATE;
	return Win32Thread::GetPriority(m_ThreadHandle, priority);
}

int Win32Thread::Wait(int timeout /* = ZHD_TIMEOUT_INFINITE */)
{
	// check that we're not detached
	if (m_ThreadHandle == 0 || m_Detached) {
		return ZHD_FAILURE;
	}

	// wait for the thread to finish
	// Logging here will cause a crash on exit because LogManager may already be destroyed
	DWORD result = WaitForSingleObject(m_ThreadHandle, 
		timeout==ZHD_TIMEOUT_INFINITE?INFINITE:timeout);
	if (result != WAIT_OBJECT_0) {
		return ZHD_FAILURE;
	} else {
		return ZHD_SUCCESS;
	}
}


//////////////////////////////////////////////////////////////////////////
// Thread class
Thread::Thread(bool detached /* = false */)
{
	m_Delegate = new Win32Thread(this, *this, detached);
}

Thread::Thread(Runnable& target, bool detached /* = false */)
{
	m_Delegate = new Win32Thread(this, target, detached);
}

unsigned long Thread::GetCurrentThreadId()
{
	return ::GetCurrentThreadId();
}

int Thread::SetCurrentThreadPriority(int priority)
{
	return Win32Thread::SetPriority(::GetCurrentThread(), priority);
}