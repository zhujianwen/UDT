#ifndef __LOCK_H__
#define __LOCK_H__


class MutexInterface
{
public:
	virtual ~MutexInterface() {}
	virtual int Lock() = 0;
	virtual int UnLock() = 0;
};


class Mutex : public MutexInterface
{
public:
	Mutex();
	~Mutex() { delete m_Delegate; }
	int Lock() { return m_Delegate->Lock(); }
	int UnLock() { return m_Delegate->UnLock(); }

private:
	MutexInterface* m_Delegate;
};


class AutoLock
{
public:
	AutoLock(Mutex& mutex)
		: m_Mutex(mutex)
	{
		m_Mutex.Lock();
	}
	~AutoLock()
	{
		m_Mutex.UnLock();
	}

private:
	Mutex& m_Mutex;
};

#endif	// __LOCK_H__