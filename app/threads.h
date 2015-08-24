#ifndef __ZHD_THREADS_H__
#define __ZHD_THREADS_H__


#include "results.h"
#include "times.h"


//////////////////////////////////////////////////////////////////////////
// HelperEnvironment class
class HelperEnvironment
{
public:
	static int Get(const char* name, std::string& value);
	static int Set(const char* name, const char* value);
};


//////////////////////////////////////////////////////////////////////////
// HelperSystem class
class HelperSystem
{
public:
	static int GetProcessID(unsigned int& id);
	static int GetMachineName(std::string& name);
	static int GetCurrentTimeStamp(TimeStamp& now);
	static int Sleep(const TimeStamp& duration);
	static int SleepUntil(const TimeStamp& when);
	static int SetRandomSeed(unsigned int seed);
	static unsigned int GetRandomInteger();

protected:
	HelperSystem() {}
};


//////////////////////////////////////////////////////////////////////////
// SharedVariableInterface class
class SharedVariableInterface
{
public:
	// methods
	virtual ~SharedVariableInterface() {}
	virtual void SetValue(int value) = 0;
	virtual int GetValue() = 0;
	virtual int WaitUntilEquals(int value, int timeout = ZHD_TIMEOUT_INFINITE) = 0;
	virtual int WaitWhileEquals(int value, int timeout = ZHD_TIMEOUT_INFINITE) = 0;
};


//////////////////////////////////////////////////////////////////////////
// SharedVariable class
class SharedVariable : public SharedVariableInterface
{
public:
	// methods
	SharedVariable(int value = 0);
	~SharedVariable()
	{ delete m_Delegate; }
	void SetValue(int value)
	{ m_Delegate->SetValue(value); }
	int GetValue()
	{ return m_Delegate->GetValue(); }
	int WaitUntilEquals(int value, int timeout = ZHD_TIMEOUT_INFINITE)
	{ return m_Delegate->WaitUntilEquals(value, timeout); }
	int WaitWhileEquals(int value, int timeout = ZHD_TIMEOUT_INFINITE)
	{ return m_Delegate->WaitWhileEquals(value, timeout); }

private:
	SharedVariableInterface* m_Delegate;
};


//////////////////////////////////////////////////////////////////////////
// Interruptible class
class Interruptible
{
public:
	virtual ~Interruptible() {}
	virtual int Interrupt() = 0;
};


//////////////////////////////////////////////////////////////////////////
// Runnable class
class Runnable
{
public:
	virtual ~Runnable() {}
	virtual void Run() = 0;
};


//////////////////////////////////////////////////////////////////////////
// ThreadInterface class
class ThreadInterface : public Runnable, public Interruptible
{
public:
	virtual ~ThreadInterface() {}
	virtual int Start() = 0;
	virtual int Wait(int timeout = ZHD_TIMEOUT_INFINITE) = 0;
	virtual int SetPriority(int priority) { return ZHD_SUCCESS; }
	virtual int GetPriority(int& priority) = 0;
};


//////////////////////////////////////////////////////////////////////////
// Thread class
class Thread : public ThreadInterface
{
public:
	static unsigned long GetCurrentThreadId();
	static int SetCurrentThreadPriority(int priority);
	static int GetCurrentThreadPriority(int& priority);

	// methods
	explicit Thread(bool detached = false);
	explicit Thread(Runnable& target, bool detached = false);
	~Thread() { delete m_Delegate; }

	// ThreadInterface methods
	int Start() { return m_Delegate->Start(); }
	int Wait(int timeout = ZHD_TIMEOUT_INFINITE) { return m_Delegate->Wait(timeout); }
	int SetPriority(int priority) { return m_Delegate->SetPriority(priority); }
	int GetPriority(int& priority) { return m_Delegate->GetPriority(priority); }

	virtual int Interrupt() { return m_Delegate->Interrupt();}
	virtual void Run() {}

private:
	ThreadInterface* m_Delegate;
};

#endif	// __ZHD_THREADS_H__