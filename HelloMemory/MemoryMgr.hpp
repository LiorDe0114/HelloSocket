#ifndef _MEMORYMGR_HPP_
#define _MEMORYMGR_HPP_
#include <stdlib.h>
#include <assert.h>
#include<mutex>//锁

#ifdef _DEBUG
#include <stdio.h>
	#define xPrintf(...) printf(__VA_ARGS__)
#else 
	#define xPrintf(...)
#endif //_DEBUG

#define MAX_MEMORY_SIZE 1024
class MemoryAlloc;

//内存块 最小单元
class MemoryBlock
{
public:
	MemoryAlloc* pAlloc;//所属大内存块（池）
	MemoryBlock* pNext;//下一块位置
	int nID; //内存块编号
	int nRef; //引用次数
	bool bPool;//是否在内存池中
private:
	char c1;//预留
	char c2;//预留
	char c3;//预留
};
//const int MemoryBlockSize = sizeof(MemoryBlock);

//内存池
class MemoryAlloc
{
public:
	MemoryAlloc()
	{
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nSize = 0;
		_nBlockSize = 0;
		xPrintf("MemoryAlloc\n");
	}

	~MemoryAlloc()
	{
		if (_pBuf)
		{
			free(_pBuf);
		}
	}
	//申请内存（初始化)
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuf)
		{
			initMemory();
		}

		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader) //内存池无资源，则向系统申请内存
		{
			pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock)); //分配的内存单元大小+内存单元的描述信息大小
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
		}
		else//内存池有资源，则分配内存池的资源
		{
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;//内存池头部移动到下一单元
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;		
		}
		xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}
	//释放内存
	void freeMemory(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		
		if (pBlock->bPool) //池内，内存池以栈的形式回收资源
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0) //被多次引用
			{
				return;
			}
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else //池外->向系统申请的内存
		{
			free(pBlock);
		}
	}
	//初始化
	void initMemory()
	{
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//计算内存池的大小
		size_t realSize = _nSize + sizeof(MemoryBlock);
		size_t bufSize = realSize * _nBlockSize;
		//向系统申请池的内存
		_pBuf = (char*)malloc(bufSize);
		//初始化内存池
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;
		//遍历内存块进行初始化
		MemoryBlock* pTemp1 = _pHeader;
		for (size_t n = 1; n < _nBlockSize; n++)
		{
			MemoryBlock* pTemp2 = (MemoryBlock*)(_pBuf + (n* realSize));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pAlloc = this;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
protected:
	char* _pBuf;//内存池地址
	MemoryBlock* _pHeader;//头部内存单元
	size_t _nSize; //内存单元的大小
	size_t _nBlockSize;//内存单元的数量
	std::mutex _mutex;
};


//便于在声明类成员变量时初始化MemoryAlloc的成员函数
template<size_t nSize, size_t nBlockSize>
class MemoryAlloctor : public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		//8 4   61/8=7 61%8=5
		const size_t n = sizeof(void*); //32位系统n=4，64位系统n=8
		//(7*8) + 8 = 64
		_nSize = (nSize/n)*n + (nSize%n ? n : 0);
		_nBlockSize = nBlockSize;
	}
};

//内存管理工具类
class MemoryMgr
{
private: //单例模式
	MemoryMgr()
	{
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		init_szAlloc(129, 256, &_mem256);
		init_szAlloc(257, 512, &_mem512);
		init_szAlloc(513, 1024, &_mem1024);
		xPrintf("MemoryMgr\n");
	}

	~MemoryMgr()
	{

	}

public:
	static MemoryMgr& Instance()//单例模式:有且仅有一个对象
	{//静态对象
		static MemoryMgr mgr;
		return mgr;
	}
	//申请内存
	void* allocMem(size_t nSize)
	{
		if (nSize <= MAX_MEMORY_SIZE)
		{
			return _szAlloc[nSize]->allocMemory(nSize);
		}
		else
		{
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock)); //分配的内存单元大小+内存单元的描述信息大小

			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
			
			return ((char*)pReturn + sizeof(MemoryBlock));;
		}
	}
	//释放内存
	void freeMem(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		xPrintf("freeMem: %llx, id=%d\n", pBlock, pBlock->nID);
		if (pBlock->bPool)
		{
			pBlock->pAlloc->freeMemory(pMem);
		}
		else
		{
			if (--pBlock->nRef == 0) //被多次引用
				free(pBlock);
		}
	}

	//增加内存块的引用计数
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		++pBlock->nRef;
	}

private:
	//初始化内存池隐射数组
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA)
	{
		for (int n = nBegin; n <= nEnd; n++)
		{
			_szAlloc[n] = pMemA;
		}
	}

private:
	MemoryAlloctor<64, 100000> _mem64;
	MemoryAlloctor<128, 100000> _mem128;
	MemoryAlloctor<256, 100000> _mem256;
	MemoryAlloctor<512, 100000> _mem512;
	MemoryAlloctor<1024, 100000> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1];
};

#endif // _MEMORYMGR_H_
