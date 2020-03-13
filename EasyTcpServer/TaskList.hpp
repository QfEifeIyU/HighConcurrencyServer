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
	// ������񵽻�����
	void AddTaskToBuffer(Task* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}

	/*       -------CellProc�߳���ں���---------      */
	void ThrRoute()
	{
		while (g_TaskRuning)
		{
			// 1.��������еĿͻ��ˣ�ȡ�����ŵ���ʽ���в���أ�����maxfd����ջ������
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto ptask : _tasksBuf)
				{
					_tasks.push_back(ptask);
				}
			}

			// 2.��ʽ������û�пͻ�������
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

	// CellProc�߳���������
	void CreateThread()
	{
		auto route = std::mem_fn(&CellProc::ThrRoute);
		std::thread proc(route, this);
		proc.detach();
	}
	

private:
	std::list<Task*> _tasks;		// ��ʽ����
	std::list<Task*> _tasksBuf;		// �������
	std::mutex _mutex;
};

#endif // !_TASKLIST_HPP_

