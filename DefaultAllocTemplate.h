#pragma once
#include"MallocAllocTemplate.h"

enum {__ALIGN = 8};					//小型区块的上调边界
enum {__MAX_BYTES = 128};			//小型区块的上限
enum {__NFREELISTS = __MAX_BYTES/__ALIGN};		//free_lists的个数

template <bool threads,int inst>
class __default_alloc_template
{
private:
	static size_t ROUND_UP(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1)& ~(__ALIGN - 1));
	}
private:
	union obj {
		union obj * free_list_link;
		char client_data[1];
	};
private:
	//16个free_lists
	static obj * volatile free_list[__NFREELISTS] = {0};
	//以下函数根据区块大小，决定使用第n号free_list。n从0起算
	static size_t FREELIST_INDEX(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) / __ALIGN - 1);
	}
	static void *refill(size_t n);
	//配置一大块空间，可容纳nojbs个大小为“size”的区块
	//如果配置nobjs的区块有所不便，nobjs可能会降低
	static char *chunk_alloc(size_t size, int &nobjs);

	static char *start_free;
	static char *end_free;
	static size_t heap_size;

public:
	static void * allocate(size_t n);
	static void deallocate(void *p, size_t n);
//	static void * reallocate(void *p, size_t old_sz, size_t new_sz);
};

template<bool threads,int inst>
char *__default_alloc_template<threads, inst>::start_free = 0;
template<bool threads,int inst>
char *__default_alloc_template<threads, inst>::end_free = 0;
template<bool threads, int inst>
size_t __default_alloc_template<threads, inst>::heap_size = 0;

template<bool threads,int inst>
void * __default_alloc_template<threads, inst>::allocate(size_t t) 
{
	obj * volatile * my_free_list;
	obj * result;
	if (n > (size_t)__MAX_BYTES) {
		return (malloc_alloc::allocate(n));
	}
	my_free_list = free_list + FREELIST_INDEX(n);
	result = *my_free_list;
	if (result == 0)
	{
		void *r = refill(ROUND_UP(n));
		return r;
	}
	*my_free_list = result->free_list_link;
	return (result);
}

template<bool threads,int inst>
static void  __default_alloc_template<threads, inst>::deallocate(void *p, size_t n)
{
	obj *q = (obj *)p;
	obj * volatile * my_free_list;
	if (n > (size_t)__MAX_BYTES) {
		malloc_alloc::deallocate(p, n);
		return;
	}
	my_free_list = free_list + FREELIST_INDEX(n);
	q->free_list_link = *my_free_list;
	*my_free_list = q;
}

template <bool threads,int inst>
void * __default_alloc_template<threads, inst>::refill(size_t n)
{
	int nobjs = 20;
	char * chunk = chunk_alloc(n, nobjs);
	obj * volatile * my_free_list;
	obj * result;
	obj * current_obj, *next_obj;
	int i;
	//如果只获得一个区块，这个区块就分配给调用者使用，free list无新节点
	if (nobjs == 1)return (chunk);
	//否则准备调整free list，纳入新节点。
	my_free_list = free_list + FREELIST_INDEX(n);
	//以下在chunk空间建立free list
	result = (obj *)chunk;
	*my_free_list = next_obj = (obj*)(chunk + n);
	//以下将个点连接起来
	for (i = 1;; i++)
	{
		current_obj = next_obj;
		next_obj = (obj*)((char *)next_obj + n);
		if (nobjs - 1 == i) {
			current_obj->free_list_link = 0;
			break;
		}
		else {
			current_obj->free_list_link = next_obj;
		}
	}
	return (result);
}
template<bool threads,int inst>
char * __default_alloc_template<threads, inst>::chunk_alloc(size_t size, int& nobjs)
{
	char * result;
	size_t total_bytes = size*nobjs;
	size_t bytes_left = end_free - start_free;	//内存池剩余空间

	if (bytes_left >= total_bytes) {		//内存池完全满足需求量
		result = start_free;
		start_free += total_bytes;
		return(result);
	}
	else if (bytes_left >= size) {		//内存池不能完全满足需求量，只能供应一个以上的区块
		nobjs = bytes_left / size;
		total_bytes = size*nobjs;
		result = start_free;
		start_free += total_bytes;
		return (result);
	}
	else {
		//内存池一个区块都无法供应
		size_t bytes_to_get = 2 * total_bytes + ROUND_UP(heap_size >> 4);
		if (bytes_left > 0) {
			//内存池中还有一些零头，先配给适当的free list
			obj * volatile *my_free_list = free_list + FREELIST_INDEX(bytes_left);
			//调整free list，将内存池中的残余空间编入
			((obj *)start_free)->free_list_link = *my_free_list;
			*my_free_list = (obj *)start_free;
		}
		//配置heap空间，用来补充内存池
		start_free = (char *)malloc(bytes_to_get);
		if (0 == start_free) {
			//heap空间不足，malloc()失败
			int i;
			obj * volatile my_free_list, *p;

			for (i = size; i <= __MAX_BYTES; i += __ALIGN) {
				my_free_list = free_list + FREELIST_INDEX(i);
				p = *my_free_list;
				if (0 != p) {		//尚有未用区块*
					//调整free list以释放未用区块
					*my_free_list = p->free_list_link;
					start_free = (char *)p;
					end_free = start_free + i;
					//递归调用自己，为了修正nobjs
					return(chunk_alloc(size, nobjs));
					//注意，任何残余零头终将被编入适当的free-list中备用
				}
			}
			end_free = 0;//如果出现意外，调用第一级配置器
			start_free = (char *)malloc_alloc::allocate(bytes_to_get);
		}
		heap_size += bytes_to_get;
		end_free = start_free + bytes_to_get;
		//递归调用自己，为了修正nobjs
		return (chunk_alloc(size, nobjs));
	}
}