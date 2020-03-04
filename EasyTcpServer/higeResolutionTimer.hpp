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

	// ����
	void update()
	{
		_begin = high_resolution_clock::now();
	}

	// ��ȡ��
	double getSecond()
	{
		return this->getMicroSecond()* 0.001* 0.001;
	}

	// ��ȡ����
	double getMilliSecond()
	{
		return this->getMicroSecond()* 0.001;
	}

	// ��ȡ΢��
	long long getMicroSecond()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
private:
	time_point<high_resolution_clock> _begin;
};	// end of class