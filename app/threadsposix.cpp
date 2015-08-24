#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#include "threads.h"
#include "results.h"
#include "lock.h"



//////////////////////////////////////////////////////////////////////////
//   PosixSystem
class PosixSystem
{
public:
    // class variables
    static PosixSystem System;
    
    // methods
    PosixSystem();
   ~PosixSystem();

    // members
    pthread_mutex_t m_SleepMutex;
    pthread_cond_t  m_SleepCondition;
};
PosixSystem PosixSystem::System;


PosixSystem::PosixSystem()
{
    pthread_mutex_init(&m_SleepMutex, NULL);
    pthread_cond_init(&m_SleepCondition, NULL);
}

PosixSystem::~PosixSystem()
{
    pthread_cond_destroy(&m_SleepCondition);
    pthread_mutex_destroy(&m_SleepMutex);
}


//////////////////////////////////////////////////////////////////////////
// HelperSystem
int HelperSystem::GetProcessID(unsigned int& id)
{
	id = getpid();
	return ZHD_SUCCESS;
}

int HelperSystem::GetMachineName(std::string& name)
{
	//return HelperEnvironment::Get("COMPUTERNAME", name);
	return ZHD_SUCCESS;
}

int HelperSystem::GetCurrentTimeStamp(TimeStamp& now)
{
    struct timeval now_tv;

    // get current time from system
    if (gettimeofday(&now_tv, NULL)) {
        now.SetNanos(0);
        return ZHD_FAILURE;
    }
    
    // convert format
    now.SetNanos((Int64)now_tv.tv_sec  * 1000000000 + 
                 (Int64)now_tv.tv_usec * 1000);

	return ZHD_SUCCESS;
}

int HelperSystem::Sleep(const TimeStamp& duration)
{
    struct timespec time_req;
    struct timespec time_rem;
    int             result;

    // setup the time value
    time_req.tv_sec  = duration.ToNanos()/1000000000;
    time_req.tv_nsec = duration.ToNanos()%1000000000;

    // sleep
    do {
        result = nanosleep(&time_req, &time_rem);
        time_req = time_rem;
    } while (result == -1 && 
             errno == EINTR && 
             (long)time_req.tv_sec >= 0 && 
             (long)time_req.tv_nsec >= 0);

	return ZHD_SUCCESS;
}

int HelperSystem::SleepUntil(const TimeStamp& when)
{
    struct timespec timeout;
    struct timeval  now;
    int             result;

    // get current time from system
    if (gettimeofday(&now, NULL)) {
        return ZHD_FAILURE;
    }

    // setup timeout
    Int64 limit = (Int64)now.tv_sec*1000000000 +
                       (Int64)now.tv_usec*1000 +
                       when.ToNanos();
    timeout.tv_sec  = limit/1000000000;
    timeout.tv_nsec = limit%1000000000;

    // sleep
    do {
        result = pthread_cond_timedwait(&PosixSystem::System.m_SleepCondition, 
                                        &PosixSystem::System.m_SleepMutex, 
                                        &timeout);
        if (result == ETIMEDOUT) {
            return ZHD_SUCCESS;
        }
    } while (result == EINTR);

    return ZHD_FAILURE;
}

int HelperSystem::SetRandomSeed(unsigned int seed)
{
	srand(seed);
	return ZHD_SUCCESS;
}

unsigned int HelperSystem::GetRandomInteger()
{
	static bool seeded = false;
    if (seeded == false) {
        TimeStamp now;
        GetCurrentTimeStamp(now);
        SetRandomSeed((unsigned int)now.ToNanos());
        seeded = true;
    }

	return rand();
}


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


//////////////////////////////////////////////////////////////////////////
// Win32SharedVariable
class PosixSharedVariable : public SharedVariableInterface
{
public:
	PosixSharedVariable(int value);
	~PosixSharedVariable();

	void       SetValue(int value);
	int        GetValue();
	int WaitUntilEquals(int value, int timeout = ZHD_TIMEOUT_INFINITE);
	int WaitWhileEquals(int value, int timeout = ZHD_TIMEOUT_INFINITE);

private:
	// members
	volatile int          m_Value;
    pthread_mutex_t m_Mutex;
    pthread_cond_t  m_Condition;
};

PosixSharedVariable::PosixSharedVariable(int value)
	: m_Value(value)
{
	pthread_mutex_init(&m_Mutex, NULL);
    pthread_cond_init(&m_Condition, NULL);
}

PosixSharedVariable::~PosixSharedVariable()
{
	pthread_cond_destroy(&m_Condition);
    pthread_mutex_destroy(&m_Mutex);
}

void PosixSharedVariable::SetValue(int value)
{
    pthread_mutex_lock(&m_Mutex);
    m_Value = value;
    pthread_cond_broadcast(&m_Condition);
    pthread_mutex_unlock(&m_Mutex);
}

int PosixSharedVariable::GetValue()
{
	return m_Value;
}

int PosixSharedVariable::WaitUntilEquals(int value, int timeout /* = ZHD_TIMEOUT_INFINITE */)
{
    int result = ZHD_SUCCESS;
    struct     timespec timed;

    if (timeout != ZHD_TIMEOUT_INFINITE) {
        // get current time from system
        struct timeval now;
        if (gettimeofday(&now, NULL)) {
            return ZHD_FAILURE;
        }

        now.tv_usec += timeout * 1000;
        if (now.tv_usec >= 1000000) {
            now.tv_sec += now.tv_usec / 1000000;
            now.tv_usec = now.tv_usec % 1000000;
        }

        // setup timeout
        timed.tv_sec  = now.tv_sec;
        timed.tv_nsec = now.tv_usec * 1000;
    }
    
    pthread_mutex_lock(&m_Mutex);
    while (value != m_Value) {
        if (timeout == ZHD_TIMEOUT_INFINITE) {
            pthread_cond_wait(&m_Condition, &m_Mutex);
        } else {
            int wait_res = pthread_cond_timedwait(&m_Condition, &m_Mutex, &timed);
            if (wait_res == ETIMEDOUT) {
                result = ZHD_ERROR_TIMEOUT;
                break;
            }
        }
    }
    pthread_mutex_unlock(&m_Mutex);
    
    return result;
}

int PosixSharedVariable::WaitWhileEquals(int value, int timeout /* = ZHD_TIMEOUT_INFINITE */)
{
    int result = ZHD_SUCCESS;
    struct     timespec timed;

    if (timeout != ZHD_TIMEOUT_INFINITE) {
        // get current time from system
        struct timeval now;
        if (gettimeofday(&now, NULL)) {
            return ZHD_FAILURE;
        }

        now.tv_usec += timeout * 1000;
        if (now.tv_usec >= 1000000) {
            now.tv_sec += now.tv_usec / 1000000;
            now.tv_usec = now.tv_usec % 1000000;
        }

        // setup timeout
        timed.tv_sec  = now.tv_sec;
        timed.tv_nsec = now.tv_usec * 1000;
    }
    
    pthread_mutex_lock(&m_Mutex);
    while (value == m_Value) {
        if (timeout == ZHD_TIMEOUT_INFINITE) {
            pthread_cond_wait(&m_Condition, &m_Mutex);
        } else {
            int wait_res = pthread_cond_timedwait(&m_Condition, &m_Mutex, &timed);
            if (wait_res == ETIMEDOUT) {
                result = ZHD_ERROR_TIMEOUT;
                break;
            }
        }
    }
    pthread_mutex_unlock(&m_Mutex);
    
    return result;
}

SharedVariable::SharedVariable(int value /* = 0 */)
{
	m_Delegate = new PosixSharedVariable(value);
}


//////////////////////////////////////////////////////////////////////////
// Win32Thread class
class PosixThread : public ThreadInterface
{
public:
	PosixThread(Thread* delagator, Runnable& target, bool detached);
	~PosixThread();

	int Start();
	int Wait(int timeout = ZHD_TIMEOUT_INFINITE);
	int GetPriority(int& priority);
	int SetPriority(int priority);
	
    // class methods
    static int GetPriority(unsigned long thread_id, int& priority);
    static int SetPriority(unsigned long thread_id, int priority);

private:
    // class methods
    static void* EntryPoint(void* argument);
	void Run();
	int Interrupt() { return ZHD_ERROR_NOT_IMPLEMENTED; }

    // members
    Thread*        m_Delegator;
    Runnable&      m_Target;
    bool               m_Detached;
    pthread_t          m_ThreadId;
    bool               m_Joined;
    PosixMutex     m_JoinLock;
    SharedVariable m_Done;
};

PosixThread::PosixThread(Thread* delagator, Runnable& target, bool detached)
	: m_Delegator(delagator)
	, m_Target(target)
	, m_Detached(detached)
	, m_ThreadId(0)
	, m_Joined(false)
{
	m_Done.SetValue(0);
}

PosixThread::~PosixThread()
{
	if (!m_Detached)
		Wait();
}

void* PosixThread::EntryPoint(void* argument)
{
	PosixThread* thread = reinterpret_cast<PosixThread*>(argument);

	printf("thread in =======================");

	thread->m_ThreadId = pthread_self();
	
	// set random seed per thread
	//NPT_TimeStamp now;
	//NPT_System::GetCurrentTimeStamp(now);
	//NPT_System::SetRandomSeed((NPT_UInt32)(now.ToNanos()) + ::GetCurrentThreadId());
	
	// run the thread 
	thread->Run();

	// if the thread is detached, delete it
    if (thread->m_Detached) {
        delete thread->m_Delegator;
    } else {
        // notify we're done
        thread->m_Done.SetValue(1);
    }

	return 0;
}

int PosixThread::Start()
{
    // reset values
    m_Joined = false;
    m_ThreadId = 0;
    m_Done.SetValue(0);

    pthread_attr_t *attributes = NULL;

#if defined(ZHD_CONFIG_THREAD_STACK_SIZE)
    pthread_attr_t stack_size_attributes;
    pthread_attr_init(&stack_size_attributes);
    pthread_attr_setstacksize(&stack_size_attributes, ZHD_CONFIG_THREAD_STACK_SIZE);
    attributes = &stack_size_attributes;
#endif

    // use local copies of some of the object's members, because for
    // detached threads, the object instance may have deleted itself
    // before the pthread_create() function returns
    bool detached = m_Detached;

    // create the native thread
    pthread_t thread_id;
    int result = pthread_create(&thread_id, attributes, EntryPoint, 
                                static_cast<PosixThread*>(this));
    printf("---- Thread Created: id = %d, res=%d", 
                   thread_id, result);
    if (result != 0) {
        // failed
        return ZHD_ERROR_ERRNO(result);
    } else {
        // detach the thread if we're not joinable
        if (detached) {
            pthread_detach(thread_id);
        } else {
            // store the thread ID (NOTE: this is also done by the thread Run() method
            // but it is necessary to do it from both contexts, because we don't know
            // which one will need it first.)
            m_ThreadId = thread_id;        
        } 
        return ZHD_SUCCESS;
    }
}

void PosixThread::Run()
{
	m_Target.Run();
}

int PosixThread::Wait(int timeout /* = ZHD_TIMEOUT_INFINITE */)
{
    void* return_value;
    int   result;

    // Note: Logging here will cause a crash on exit because LogManager may already be destroyed
    // if this object is global or static as well
    //NPT_LOG_FINE_1("NPT_PosixThread::Wait - waiting for id %d", m_ThreadId);

    // check that we're not detached
    if (m_ThreadId == 0 || m_Detached) {
        return ZHD_FAILURE;
    }

    // wait for the thread to finish
    m_JoinLock.Lock();
    if (m_Joined) {
        result = 0;
    } else {
        if (timeout != ZHD_TIMEOUT_INFINITE) {
            if (ZHD_FAILED(m_Done.WaitUntilEquals(1, timeout))) {
                result = -1;
                goto timedout;
            }
        }

        result = pthread_join(m_ThreadId, &return_value);
        m_Joined = true;
    }

timedout:
    m_JoinLock.UnLock();
    if (result != 0) {
        return ZHD_FAILURE;
    } else {
        return ZHD_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::SetPriority
+---------------------------------------------------------------------*/
int PosixThread::SetPriority(int priority)
{
    // check that we're started
    if (m_ThreadId == 0) {
        return ZHD_FAILURE;
    }
    
    return PosixThread::SetPriority((unsigned long)m_ThreadId, priority);
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::SetPriority
+---------------------------------------------------------------------*/
int PosixThread::SetPriority(unsigned long thread_id, int priority)
{
    // check that we're started
    if (thread_id == 0) {
        return ZHD_FAILURE;
    }
    
    /* sched_priority will be the priority of the thread */
    struct sched_param sp;
    int policy;
    int result = pthread_getschedparam((pthread_t)thread_id, &policy, &sp);
    
    printf("Current thread policy: %d, priority: %d, new priority: %d", 
                   policy, sp.sched_priority, priority);
    printf("Thread max(SCHED_OTHER): %d, max(SCHED_RR): %d \
                   min(SCHED_OTHER): %d, min(SCHED_RR): %d",
                   sched_get_priority_max(SCHED_OTHER),
                   sched_get_priority_max(SCHED_RR),
                   sched_get_priority_min(SCHED_OTHER),
                   sched_get_priority_min(SCHED_RR));
    
    sp.sched_priority = priority;
    
    /*
    if (sp.sched_priority <= 0)
        sp.sched_priority += sched_get_priority_max (policy = SCHED_OTHER);
    else
        sp.sched_priority += sched_get_priority_min (policy = SCHED_RR);
     */
    
    /* scheduling parameters of target thread */
    result = pthread_setschedparam((pthread_t)thread_id, policy, &sp);
    
    return (result==0)?ZHD_SUCCESS:ZHD_ERROR_ERRNO(result);
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::GetPriority
+---------------------------------------------------------------------*/
int PosixThread::GetPriority(int& priority)
{
    // check that we're started
    if (m_ThreadId == 0) {
        return ZHD_FAILURE;
    }
    
    return PosixThread::GetPriority((unsigned long)m_ThreadId, priority);
}

/*----------------------------------------------------------------------
|       NPT_PosixThread::GetPriority
+---------------------------------------------------------------------*/
int PosixThread::GetPriority(unsigned long thread_id, int& priority)
{
    // check that we're started
    if (thread_id == 0) {
        return ZHD_FAILURE;
    }
 
    struct sched_param sp;
    int policy;

    int result = pthread_getschedparam((pthread_t)thread_id, &policy, &sp);
    printf("Current thread priority: %d", sp.sched_priority);
    
    priority = sp.sched_priority;
    return (result==0)?ZHD_SUCCESS:ZHD_ERROR_ERRNO(result);
}


//////////////////////////////////////////////////////////////////////////
// Thread class
Thread::Thread(bool detached /* = false */)
{
	m_Delegate = new PosixThread(this, *this, detached);
}

Thread::Thread(Runnable& target, bool detached /* = false */)
{
	m_Delegate = new PosixThread(this, target, detached);
}

unsigned long Thread::GetCurrentThreadId()
{
	pthread_t pid = pthread_self();
	return (unsigned long)((void*)pid);
}

int Thread::SetCurrentThreadPriority(int priority)
{
	return PosixThread::SetPriority(GetCurrentThreadId(), priority);
}

int Thread::GetCurrentThreadPriority(int& priority)
{
    return PosixThread::GetPriority(GetCurrentThreadId(), priority);
}