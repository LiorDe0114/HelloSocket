#include <iostream>
#include <thread>//C++11
#include <mutex> //锁
#include <atomic>//原子
#include "CELLTimestamp.hpp"

using namespace std;
//原子操作 
mutex m;
const int tCount = 4;
//atomic_int sum = 0;
atomic<int> sum = 0;
void workFun(int index)
{
    for (int n = 0; n < 2000000; n++)
    {
        //自解锁 在括号区域跳出时就自解锁
        //lock_guard<mutex> lg(m);
        //m.lock();
        //临界区域-开始
        sum++;
        //临界区域-结束
        //m.unlock();
    }
    //线程安全 线程不安全
    //原子操作 计算机处理命令时最小的操作单位
    //cout << index << "Hello, other thread." << n << endl;
}//抢占式

int main()
{
    thread t[tCount];
    //thread* t[3];
    for (int n = 0; n < tCount; n++)
    {
        t[n] = thread(workFun, n);
        //t[n] = new thread(workFun, n);
        //t[n].detach();
    }
    CELLTimestamp tTime;
	for (int n = 0; n < tCount; n++)
	{
        //t[n].detach();
		t[n].join();
	}
    cout << tTime.getElapsedTimeInMilliSec() << ",sum = " << sum << endl;
    sum = 0;
    tTime.update();

    for (int n = 0; n < 2000000; n++)
	{
		sum++;
	}
    cout << tTime.getElapsedTimeInMilliSec() << ",sum = " << sum << endl;
    cout << "Hello, main thread." << endl;
    return 0;
}