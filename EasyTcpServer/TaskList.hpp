#ifndef _TASKLIST_HPP_
#define _TASKLIST_HPP_

#include <list>
#include <mutex>
#include <thread>

class Task
{
public:
	Task()
	{}

	virtual ~Task()
	{}

	virtual void doTask()
	{}
};

class CellProc 
{
public:
	// 添加任务到缓冲区
	void AddTaskToBuffer(Task* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}

	/*       -------CellProc线程入口函数---------      */
	void ThrRoute()
	{
		while (g_TaskRuning)
		{
			// 1.将缓冲队列的客户端，取出来放到正式队列并监控，更新maxfd，清空缓冲队列
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto ptask : _tasksBuf)
				{
					_tasks.push_back(ptask);
				}
			}

			// 2.正式队列中没有客户就跳过
			if (_tasks.empty())
			{
				std::chrono::milliseconds millSec(1);
				std::this_thread::sleep_for(millSec);
				continue;
			}

			for (auto ptask : _tasks)
			{
				ptask->doTask();
				delete ptask;
			}

			_tasks.clear();
		}
	}

	// CellProc线程启动函数
	void CreateThread()
	{
		auto route = std::mem_fn(&CellProc::ThrRoute);
		std::thread proc(route, this);
		proc.detach();
	}
	

private:
	std::list<Task*> _tasks;		// 正式队列
	std::list<Task*> _tasksBuf;		// 缓冲队列
	std::mutex _mutex;
};

#endif // !_TASKLIST_HPP_

