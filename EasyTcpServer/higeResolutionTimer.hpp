#ifndef _highResolutionTimer_hpp_
#define	_highResolutionTimer_hpp_

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
		//high_resolution_clock::now()  当前时间戳
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
private:
	time_point<high_resolution_clock> _begin;		// 开始计时-时间戳
};

#endif  /* end of higeResolutionTimer.hpp */