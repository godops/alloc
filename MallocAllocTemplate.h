#pragma once
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include<new>
#define __THROW_BAD_ALLOC throw bad_alloc
#elif !define(_THROW_BAD_ALLOC)
#include<iostream>
#define __THROW_BAD_ALLOC std::cerr<<"out of memory"<<endl;exit(1)
#endif // !ALLOCATOR_H

//��һ������������������>128byte
template<int inst>
class __malloc_alloc_template
{
private:
	static void *oom_malloc(size_t);
	static void *oom_realloc(void *, size_t);
	static void(*__malloc_alloc_oom_handler)();

public:
	static void * allocate(size_t n)
	{
		void *result = malloc(n);		//ʹ��mallocֱ�ӷ����ڴ�ռ�
		if (result == 0)result = oom_malloc(n);
		return result;
	}

	static void deallocate(void *p, size_t)
	{
		free(p);						//ʹ��free�ͷ��ڴ�
	}

	//��������һ���ڴ�
	static void * reallocate(void *p, size_t, size_t new_size)
	{
		void * result = realloc(p, new_size);
		if (result == 0)result = oom_realloc(p, new_size);
		return result;
	}

	static void(*set_malloc_handler(void(*f)()))()
	{
		void(*old)() = __malloc_alloc_oom_handler;
		__malloc_alloc_oom_handler = f;
		return(old);
	}
};

//��ֵΪ0���д��Ͷ��趨
template<int inst>
void(*__malloc_alloc_template<inst>::__malloc_alloc_oom_handler)() = 0;

template<int inst>
void * __malloc_alloc_template<inst>::oom_malloc(size_t n)
{
	void(*my_malloc_handler)();
	void *result;
	for (;;)
	{
		my_malloc_handler = __malloc_alloc_oom_handler;
		if (my_malloc_handler == 0)
			__THROW_BAD_ALLOC;
		(*my_malloc_handler)();		//���ô������̣���ͼ�ͷ��ڴ�
		result = malloc(n);			//�ٴγ��������ڴ�
		if (result)
			return(result);
	}
}

template <int inst>
void * __malloc_alloc_template<inst>::oom_realloc(void *p, size_t n)
{
	void(*my_malloc_handler)();
	void *result;

	for (;;)
	{
		my_malloc_handler = __malloc_alloc_oom_handler;
		if (my_malloc_handler == 0)
			__THROW_BAD_ALLOC;
		(*my_malloc_handler)();		//���ô������̣���ͼ�ͷ��ڴ�
		result = realloc(p, n);		//�ٴγ��������ڴ�
		if (result)
			return(result);
	}
}

typedef __malloc_alloc_template<0>malloc_alloc;