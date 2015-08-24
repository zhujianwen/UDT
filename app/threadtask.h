#ifndef __ZHD_THREAD_TASK_H__
#define __ZHD_THREAD_TASK_H__


#include <list>
#include <queue>
#include "lock.h"
#include "times.h"
#include "threads.h"


class CThreadTask;
class CThreadManager;


class CThreadTask : public Runnable
{
public:
	friend class CThreadManager;

	int Kill();

protected:
	virtual bool IsAborting(unsigned long timeout) { return ZHD_SUCCEEDED(m_Abort.WaitUntilEquals(1, timeout)); }
	int Start(CThreadManager* manager = NULL, TimeStamp* delay = NULL, bool auto_destroy = true);
	int Stop(bool blocking = true);

	virtual void DoInit() {}
	virtual void DoAbort() {}
	virtual void DoRun() {}

	CThreadTask();
	virtual ~CThreadTask();

private:
	int StartThread();
	void Run();

protected:
	CThreadManager* m_Manager;

private:
	SharedVariable m_Started;
	SharedVariable m_Abort;
	Thread* m_Thread;
	TimeStamp m_Delay;
	bool m_AutoDestroy;
};


class CThreadManager
{
public:
	CThreadManager(unsigned int max_tasks = 0);
	virtual ~CThreadManager();

	virtual int StartTask(CThreadTask* task, TimeStamp* delay = NULL, bool auto_destroy = true);
	int StopAllTasks();
	int GetMaxTasks() { return m_MaxTasks; }

private:
	friend class CThreadTask;
	int AddTask(CThreadTask* task);
	int RemoveTask(CThreadTask* task);

private:
	std::list<CThreadTask*> m_Tasks;
	Mutex m_TasksLock;
	unsigned int m_MaxTasks;
	unsigned int m_RunningTasks;
	bool m_bStopping;
};

#endif	// __ZHD_THREAD_TASK_H__