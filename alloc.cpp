// allocator.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "DefaultAllocTemplate.h"
#include "MallocAllocTemplate.h"
#include <vector>
#include <iostream>
int main()
{
	std::vector<int>vec;
	vec.push_back(1);
	std::cout << vec[0] << std::endl;
    return 0;
}

