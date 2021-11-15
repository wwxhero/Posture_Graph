#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "filesystem_helper.hpp"
#include "posture_graph.h"
#include "parallel_thread_helper.hpp"

class CTaskThread : public CThread_W32
{
public:
	CTaskThread()
		: c_stepPrepareTask(0)
		, c_stepExeTask(1)
		, c_stepCompleteTask(2)
		, m_task_id(-1)
		, m_task_step(c_stepCompleteTask)
	{
	}

	~CTaskThread()
	{
	}

	void PrepareTask_main(int task_id)
	{
		bool initialized = !(m_task_id < 0);
		bool increasing_taskid = (m_task_id < task_id);
		bool valid_step = (c_stepCompleteTask == m_task_step);
		assert((!initialized || increasing_taskid)	// initialized->increasing_taskid
			&& (!initialized || valid_step));		// initialized->valid_step
		m_task_id = task_id;
		m_task_step = c_stepPrepareTask;
		Execute_main();
	}

	void CompleteTask_main()
	{
		bool valid_task = !(m_task_id < 0);
		bool valid_step = (c_stepExeTask == m_task_step);
		assert(valid_task && valid_step);
		m_task_step = c_stepCompleteTask;
	}

private:
	virtual void Run_worker()
	{
		bool valid_task = !(m_task_id < 0);
		bool valid_step = (c_stepPrepareTask == m_task_step);
		assert(valid_task && valid_step);
		m_task_step = c_stepExeTask;
	}
private:
	const int c_stepPrepareTask;
	const int c_stepExeTask;
	const int c_stepCompleteTask;
	volatile int m_task_id;
	volatile int m_task_step;
};


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (3 != argc)
	{
		std::cout << "Usage:\tparallel_thread_verifier <N_threads> <N_tasks>" << std::endl;
		return -1;
	}
	else
	{
		const int n_parallel = atoi(argv[1]);
		const int n_tasks = atoi(argv[2]);
		CThreadPool_W32<CTaskThread> pool;
		pool.Initialize_main(n_parallel);

		int task_id = 0;
		std::vector<CTaskThread*>& threads = pool.WaitForAllReadyThreads_main();
		for (auto thread : threads)
			thread->PrepareTask_main(task_id);

		for (task_id ++; task_id < n_tasks; task_id ++)
		{
			CTaskThread* thread_i = NULL;
			do
			{
				thread_i = pool.WaitForAReadyThread_main(10);
			} while (NULL == thread_i);

			thread_i->CompleteTask_main();
			thread_i->PrepareTask_main(task_id);
		}

		threads = pool.WaitForAllReadyThreads_main();
		for (auto thread : threads)
			thread->CompleteTask_main();

		std::cout << "verified successful!!!" << std::endl;
		return 0;
	}

}
