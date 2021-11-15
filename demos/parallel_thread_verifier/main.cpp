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
		, m_localVerifier(true)
	{
	}

	~CTaskThread()
	{
	}

	bool GlobalVerify() const
	{
		bool globalVerifer = true;
		int n_tasks = (int)m_tasks.size();
		if (0 < n_tasks)
		{
			int task_id_i = m_tasks[0].first;
			int step_id_i = m_tasks[0].second;
			for (int i_task = 1; i_task < n_tasks && globalVerifer; i_task ++)
			{
				// TASKs: (i, 0), (i, 1), (i, 2), (i+k, 0), (i+k, 1), (i+k, 2), (i+k+p, 0) ... where i>=0, k>0, p>0
				int task_id_i_p = m_tasks[i_task].first;
				int step_id_i_p = m_tasks[i_task].second;
				bool valid_same_task = (task_id_i == task_id_i_p
										&& step_id_i + 1 == step_id_i_p);
				bool valid_diff_task = (task_id_i < task_id_i_p
										&& c_stepCompleteTask == step_id_i
										&& c_stepPrepareTask == step_id_i_p);
				globalVerifer = (valid_same_task || valid_diff_task);
				task_id_i = task_id_i_p;
				step_id_i = step_id_i_p;
			}
		}
		return globalVerifer;
	}

	bool LocalVerify() const
	{
		return m_localVerifier;
	}

	void PrepareTask_main(int task_id)
	{
		bool initialized = !(m_task_id < 0);
		bool increasing_taskid = (m_task_id < task_id);
		bool valid_step = (c_stepCompleteTask == m_task_step);
		m_localVerifier = m_localVerifier
						&& ((!initialized || increasing_taskid)		// initialized->increasing_taskid
							&& (!initialized || valid_step));		// initialized->valid_step
		m_task_id = task_id;
		m_task_step = c_stepPrepareTask;
		m_tasks.push_back(std::make_pair(m_task_id, m_task_step));
		Execute_main();
	}

	void CompleteTask_main()
	{
		bool valid_task = !(m_task_id < 0);
		bool valid_step = (c_stepExeTask == m_task_step);
		m_localVerifier = m_localVerifier
						&& (valid_task && valid_step);
		m_task_step = c_stepCompleteTask;
		m_tasks.push_back(std::make_pair(m_task_id, m_task_step));
	}

private:
	virtual void Run_worker()
	{
		bool valid_task = !(m_task_id < 0);
		bool valid_step = (c_stepPrepareTask == m_task_step);
		m_localVerifier = m_localVerifier
						 && (valid_task && valid_step);
		m_task_step = c_stepExeTask;
		m_tasks.push_back(std::make_pair(m_task_id, m_task_step));
	}
private:
	const int c_stepPrepareTask;
	const int c_stepExeTask;
	const int c_stepCompleteTask;
	volatile int m_task_id;
	volatile int m_task_step;
	typedef std::pair<int, int> TASK;
	std::vector<TASK> m_tasks;
	bool m_localVerifier;
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
			thread->PrepareTask_main(task_id++);

		while (task_id < n_tasks)
		{
			CTaskThread* thread_i = NULL;
			do
			{
				thread_i = pool.WaitForAReadyThread_main(10);
			} while (NULL == thread_i);

			thread_i->CompleteTask_main();
			thread_i->PrepareTask_main(task_id ++);
		}

		threads = pool.WaitForAllReadyThreads_main();
		for (auto thread : threads)
			thread->CompleteTask_main();

		bool verified_successful = true;
		for (auto thread : threads)
			verified_successful = verified_successful
								&& thread->LocalVerify()
								&& thread->GlobalVerify();
		if (verified_successful)
			std::cout << "verified successful!!!" << std::endl;
		else
			std::cout << "verified failed!!!!" << std::endl;
		return 0;
	}

}
