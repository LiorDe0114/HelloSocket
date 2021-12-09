#include <stdlib.h>
#include "Alloctor.h"

// int main()
// {
//  /*   //1
//     char* data1 = new char[128];
//     delete[] data1;
//     //2
//     char* data2 = new char;
//     delete data2;
//     //3
//     char* data3 = new char[64];
//     delete[] data3;
// */
//     char* data[1100];
//     for (size_t i = 0; i < 1100; i++)
//     {
//         data[i] = new char[1+i];
// 		//delete data[i];
//     }
// 	for (size_t i = 0; i < 1100; i++)
// 	{
// 		delete data[i];
// 	}
//     return 0;
// }


#include <iostream>
#include <thread>//C++11
#include <mutex> //锁
#include "CELLTimestamp.hpp"

using namespace std;
//原子操作 
mutex m;
const int tCount = 2;
const int mCount = 100000;
const int nCount = mCount/tCount;
void workFun(int index)
{
	char* data[nCount];
	for (size_t i = 0; i < nCount; i++)
	{
		data[i] = new char[1+(rand()%128)];
		//delete data[i];
	}
	for (size_t i = 0; i < nCount; i++)
	{
		delete data[i];
	}

/*
// 	for (int n = 0; n < nCount; n++)
// 	{
// 		//自解锁 在括号区域跳出时就自解锁
// 		//lock_guard<mutex> lg(m);
// 		//m.lock();
// 		//临界区域-开始
// 		//sum++;
// 		//临界区域-结束
// 		//m.unlock();
// 	}
*/
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
	cout << tTime.getElapsedTimeInMilliSec() << endl;

	cout << "Hello, main thread." << endl;
	return 0;
}