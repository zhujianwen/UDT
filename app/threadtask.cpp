#include "threadtask.h"


//////////////////////////////////////////////////////////////////////////
// class CThreadTask
CThreadTask::CThreadTask()
	: m_Manager(NULL)
	, m_Thread(NULL)
	, m_AutoDestroy(false)
{
}

CThreadTask::~CThreadTask()
{
	if (!m_AutoDestroy) delete m_Thread;
}


int CThreadTask::Start(CThreadManager* manager /* = NULL */, TimeStamp* delay /* = NULL */, bool auto_destroy /* = true */)
{
	m_Abort.SetValue(0);
	m_AutoDestroy = auto_destroy;
	m_Delay = delay ? *delay : TimeStamp(0.0);
	m_Manager = manager;

	if (m_Manager)
	{
		m_Manager->AddTask(this);
		return ZHD_SUCCESS;
	}
	else
	{
		int result = StartThread();
		if (ZHD_FAILED(result) && m_AutoDestroy) delete this;
		return result;
	}
}

int CThreadTask::StartThread()
{
	m_Started.SetValue(0);
	m_Thread = new Thread((Runnable &)*this, m_AutoDestroy);
	int result = m_Thread->Start();
	if (ZHD_FAILED(result))
	{
		if (m_AutoDestroy)
		{
			delete m_Thread;
			m_Thread = NULL;
		}
		//NPT_CHECK_FATAL(result);
	}

	return m_Started.WaitUntilEquals(1, ZHD_TIMEOUT_INFINITE);
}

int CThreadTask::Stop(bool blocking /* = true */)
{
	bool auto_destroy = m_AutoDestroy;
	m_Abort.SetValue(1);
	DoAbort();

	if (!blocking || !m_Thread) return ZHD_SUCCESS;

	return auto_destroy ? ZHD_FAILURE : m_Thread->Wait();
}

int CThreadTask::Kill()
{
	Stop();
	//ASSERT(m_AutoDestroy == false);
	if (!m_AutoDestroy) delete this;

	return ZHD_SUCCESS;
}

void CThreadTask::Run()
{
	m_Started.SetValue(1);

	if ((float)m_Delay > 0.f)
	{
		// more than 100ms, loop so we can abort it
		if ((float)m_Delay > 0.1f)
		{
			TimeStamp start, now, stamp;
			HelperSystem::GetCurrentTimeStamp(start);
			do 
			{
				HelperSystem::GetCurrentTimeStamp(now);
				stamp = start + m_Delay;
				if (now >= stamp) break;
			} while (!IsAborting(100));
		}
		else
		{
			HelperSystem::Sleep(m_Delay);
		}
	}

	// loop
	if (!IsAborting(0))
	{
		DoInit();
		DoRun();
	}

	if (m_Manager)
		m_Manager->RemoveTask(this);
	else if (m_AutoDestroy)
		delete this;
}


//////////////////////////////////////////////////////////////////////////
// class CThreadManager
CThreadManager::CThreadManager(unsigned int max_tasks /* = 0 */)
	: m_MaxTasks(max_tasks)
	, m_RunningTasks(0)
	, m_bStopping(false)
{

}

CThreadManager::~CThreadManager()
{
	StopAllTasks();
}

int CThreadManager::StartTask(CThreadTask* task, TimeStamp* delay /* = NULL */, bool auto_destroy /* = true */)
{
	return task->Start(this, delay, auto_destroy);
}

int CThreadManager::StopAllTasks()
{
	unsigned long num_running_tasks;

	do 
	{
		// abort all running tasks
		AutoLock lock(m_TasksLock);
		std::list<CThreadTask*>::iterator task = m_Tasks.begin();
		while (task != m_Tasks.end())
		{
			if (!(*task)->IsAborting(0))
				(*task)->Stop(false);
			++task;
		}
		num_running_tasks = m_Tasks.size();

		if (num_running_tasks == 0)
			break;

		HelperSystem::Sleep(TimeStamp(0.05));
	} while (1);

	return ZHD_SUCCESS;
}

int CThreadManager::AddTask(CThreadTask* task)
{
	int result = ZHD_SUCCESS;

	m_TasksLock.Lock();
	if (m_bStopping)
	{
		m_TasksLock.UnLock();
		if (task->m_AutoDestroy) delete task;
		return ZHD_FAILURE;
	}

	// start task now
	if (ZHD_FAILED(result = task->StartThread()))
	{
		m_TasksLock.UnLock();
		RemoveTask(task);
		return result;
	}

	m_Tasks.push_back(task);
	m_TasksLock.UnLock();
	return ZHD_SUCCESS;
}


int CThreadManager::RemoveTask(CThreadTask* task)
{
	int result = ZHD_SUCCESS;

	m_TasksLock.Lock();
	m_Tasks.remove(task);
	if (task->m_AutoDestroy) delete task;
	m_TasksLock.UnLock();

	return ZHD_SUCCESS;
}