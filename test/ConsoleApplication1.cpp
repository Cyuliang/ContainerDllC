
// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "ContainerDll.h"

#include <iostream>

void message(string meg)
{
	std::cout << meg.data() << std::endl;
}

void messagea(string a,string b,string c,string d,string e,string f,string g,string h,string j)
{
	std::cout << a.data() << std::endl;
	std::cout << b.data() << std::endl;
	std::cout << c.data() << std::endl;
	std::cout << d.data() << std::endl;
	std::cout << e.data() << std::endl;
	std::cout << f.data() << std::endl;
	std::cout << g.data() << std::endl;
	std::cout << h.data() << std::endl;
	std::cout << j.data() << std::endl;
}

int main()
{
	Container *con =new Container();
	//注册回调函数
	con->registerMessageCallBack(message);
	con->registerResultCallBack(message);
	con->registerContainerCallBack(messagea);
	con->setModel(false);

	con->init("192.168.1.99", 12011);

	while (1) {
		getchar();
		con->getResult();
	}
}