#pragma once
#include"MallocAllocTemplate.h"

enum {__ALIGN = 8};					//С��������ϵ��߽�
enum {__MAX_BYTES = 128};			//С�����������
enum {__NFREELISTS = __MAX_BYTES/__ALIGN};		//free_lists�ĸ���

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
	//16��free_lists
	static obj * volatile free_list[__NFREELISTS] = {0};
	//���º������������С������ʹ�õ�n��free_list��n��0����
	static size_t FREELIST_INDEX(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) / __ALIGN - 1);
	}
	static void *refill(size_t n);
	//����һ���ռ䣬������nojbs����СΪ��size��������
	//�������nobjs�������������㣬nobjs���ܻή��
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
	//���ֻ���һ�����飬�������ͷ����������ʹ�ã�free list���½ڵ�
	if (nobjs == 1)return (chunk);
	//����׼������free list�������½ڵ㡣
	my_free_list = free_list + FREELIST_INDEX(n);
	//������chunk�ռ佨��free list
	result = (obj *)chunk;
	*my_free_list = next_obj = (obj*)(chunk + n);
	//���½�������������
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
	size_t bytes_left = end_free - start_free;	//�ڴ��ʣ��ռ�

	if (bytes_left >= total_bytes) {		//�ڴ����ȫ����������
		result = start_free;
		start_free += total_bytes;
		return(result);
	}
	else if (bytes_left >= size) {		//�ڴ�ز�����ȫ������������ֻ�ܹ�Ӧһ�����ϵ�����
		nobjs = bytes_left / size;
		total_bytes = size*nobjs;
		result = start_free;
		start_free += total_bytes;
		return (result);
	}
	else {
		//�ڴ��һ�����鶼�޷���Ӧ
		size_t bytes_to_get = 2 * total_bytes + ROUND_UP(heap_size >> 4);
		if (bytes_left > 0) {
			//�ڴ���л���һЩ��ͷ��������ʵ���free list
			obj * volatile *my_free_list = free_list + FREELIST_INDEX(bytes_left);
			//����free list�����ڴ���еĲ���ռ����
			((obj *)start_free)->free_list_link = *my_free_list;
			*my_free_list = (obj *)start_free;
		}
		//����heap�ռ䣬���������ڴ��
		start_free = (char *)malloc(bytes_to_get);
		if (0 == start_free) {
			//heap�ռ䲻�㣬malloc()ʧ��
			int i;
			obj * volatile my_free_list, *p;

			for (i = size; i <= __MAX_BYTES; i += __ALIGN) {
				my_free_list = free_list + FREELIST_INDEX(i);
				p = *my_free_list;
				if (0 != p) {		//����δ������*
					//����free list���ͷ�δ������
					*my_free_list = p->free_list_link;
					start_free = (char *)p;
					end_free = start_free + i;
					//�ݹ�����Լ���Ϊ������nobjs
					return(chunk_alloc(size, nobjs));
					//ע�⣬�κβ�����ͷ�ս��������ʵ���free-list�б���
				}
			}
			end_free = 0;//����������⣬���õ�һ��������
			start_free = (char *)malloc_alloc::allocate(bytes_to_get);
		}
		heap_size += bytes_to_get;
		end_free = start_free + bytes_to_get;
		//�ݹ�����Լ���Ϊ������nobjs
		return (chunk_alloc(size, nobjs));
	}
}