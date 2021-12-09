#ifndef _MEMORYMGR_HPP_
#define _MEMORYMGR_HPP_
#include <stdlib.h>
#include <assert.h>
#include<mutex>//��

#ifdef _DEBUG
#include <stdio.h>
	#define xPrintf(...) printf(__VA_ARGS__)
#else 
	#define xPrintf(...)
#endif //_DEBUG

#define MAX_MEMORY_SIZE 1024
class MemoryAlloc;

//�ڴ�� ��С��Ԫ
class MemoryBlock
{
public:
	MemoryAlloc* pAlloc;//�������ڴ�飨�أ�
	MemoryBlock* pNext;//��һ��λ��
	int nID; //�ڴ����
	int nRef; //���ô���
	bool bPool;//�Ƿ����ڴ����
private:
	char c1;//Ԥ��
	char c2;//Ԥ��
	char c3;//Ԥ��
};
//const int MemoryBlockSize = sizeof(MemoryBlock);

//�ڴ��
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
	//�����ڴ棨��ʼ��)
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuf)
		{
			initMemory();
		}

		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader) //�ڴ������Դ������ϵͳ�����ڴ�
		{
			pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock)); //������ڴ浥Ԫ��С+�ڴ浥Ԫ��������Ϣ��С
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
		}
		else//�ڴ������Դ��������ڴ�ص���Դ
		{
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;//�ڴ��ͷ���ƶ�����һ��Ԫ
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;		
		}
		xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}
	//�ͷ��ڴ�
	void freeMemory(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		
		if (pBlock->bPool) //���ڣ��ڴ����ջ����ʽ������Դ
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0) //���������
			{
				return;
			}
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else //����->��ϵͳ������ڴ�
		{
			free(pBlock);
		}
	}
	//��ʼ��
	void initMemory()
	{
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//�����ڴ�صĴ�С
		size_t realSize = _nSize + sizeof(MemoryBlock);
		size_t bufSize = realSize * _nBlockSize;
		//��ϵͳ����ص��ڴ�
		_pBuf = (char*)malloc(bufSize);
		//��ʼ���ڴ��
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;
		//�����ڴ����г�ʼ��
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
	char* _pBuf;//�ڴ�ص�ַ
	MemoryBlock* _pHeader;//ͷ���ڴ浥Ԫ
	size_t _nSize; //�ڴ浥Ԫ�Ĵ�С
	size_t _nBlockSize;//�ڴ浥Ԫ������
	std::mutex _mutex;
};


//�������������Ա����ʱ��ʼ��MemoryAlloc�ĳ�Ա����
template<size_t nSize, size_t nBlockSize>
class MemoryAlloctor : public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		//8 4   61/8=7 61%8=5
		const size_t n = sizeof(void*); //32λϵͳn=4��64λϵͳn=8
		//(7*8) + 8 = 64
		_nSize = (nSize/n)*n + (nSize%n ? n : 0);
		_nBlockSize = nBlockSize;
	}
};

//�ڴ��������
class MemoryMgr
{
private: //����ģʽ
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
	static MemoryMgr& Instance()//����ģʽ:���ҽ���һ������
	{//��̬����
		static MemoryMgr mgr;
		return mgr;
	}
	//�����ڴ�
	void* allocMem(size_t nSize)
	{
		if (nSize <= MAX_MEMORY_SIZE)
		{
			return _szAlloc[nSize]->allocMemory(nSize);
		}
		else
		{
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock)); //������ڴ浥Ԫ��С+�ڴ浥Ԫ��������Ϣ��С

			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
			
			return ((char*)pReturn + sizeof(MemoryBlock));;
		}
	}
	//�ͷ��ڴ�
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
			if (--pBlock->nRef == 0) //���������
				free(pBlock);
		}
	}

	//�����ڴ������ü���
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		++pBlock->nRef;
	}

private:
	//��ʼ���ڴ����������
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
