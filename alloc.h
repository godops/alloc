#pragma once

#include "MallocAllocTemplate.h"

template<class T,class Alloc>
class simple_alloc
{
public:
	static T *allocate(size_t n)
		return 0 == n ? 0 : (T*)Alloc::allocate(n * sizeof(T));
	static T *allocate(void)
		return (*T)Alloc::allocate(sizeof(T));
	static void deallocate(T *p, size_t n)
	{
		if (n != 0)
			Alloc::deallocate(p, n * sizeof(T));
	}
	static void deallocate(T *p)
		Alloc::deallocate(p, sizeof(T));
};