#ifndef _ALLOC_H_
#define _ALLOC_H_
void* operator new(size_t size);
void operator delete(void* p);
void* operator new[](size_t size);
void operator delete[](void* p);
void* mem_alloc(size_t size);
void mem_free(void* p);
#endif // _ALLOC_H_
