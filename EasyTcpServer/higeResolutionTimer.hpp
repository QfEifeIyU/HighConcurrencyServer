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
		//high_resolution_clock::now()  ��ǰʱ���
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
private:
	time_point<high_resolution_clock> _begin;		// ��ʼ��ʱ-ʱ���
};

#endif  /* end of higeResolutionTimer.hpp */