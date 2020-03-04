#pragma once
#include <chrono>
using namespace std::chrono;
class HigeResolutionTimer
{
public:
	HigeResolutionTimer()
	{
		update();
	}
	~HigeResolutionTimer()
	{}

	// 更新
	void update()
	{
		_begin = high_resolution_clock::now();
	}

	// 获取秒
	double getSecond()
	{
		return this->getMicroSecond()* 0.001* 0.001;
	}

	// 获取毫秒
	double getMilliSecond()
	{
		return this->getMicroSecond()* 0.001;
	}

	// 获取微秒
	long long getMicroSecond()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
private:
	time_point<high_resolution_clock> _begin;
};	// end of class