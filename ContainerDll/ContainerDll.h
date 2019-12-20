#pragma once

#ifdef CONTAINERDLL_EXPORTS
#define CONTAINERDLL_EXPORTS_API _declspec(dllexport)  
#else  
#define CONTAINERDLL_EXPORTS_API  _declspec(dllimport)  
#endif 

#include "stdio.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"
#include "Windows.h"
#include <iostream>

using namespace std;

class CONTAINERDLL_EXPORTS_API Container
{
public:

	Container();
	~Container();

	///信息流回调函数类型
	typedef void(*messageCallBack)(string);

	///结果回调函数类型
	///时间，通道号，箱型，前箱，校验，后箱，校验，前箱型，后箱型
	typedef void(*containerCallBack)(string,string, string,string, string,string,string, string,string);

	///注册结果回调函数
	void registerContainerCallBack(containerCallBack func);

	///注册日志回调函数
	void registerMessageCallBack(messageCallBack func);

	///注册信息流回调函数
	void registerResultCallBack(messageCallBack func);

	///初始化TCP
	void init(const char *serverIP, u_short serverPort);

	///主动获取结果
	bool getResult();

	///设置获取结果模式
	void setModel(bool model);

private:

	///关闭TCP，释放资源
	void socketStop();

	///连接TCP
	bool socketLink();

	void containerAnalysis(string con);

	///事件循环线程
	static DWORD WINAPI threadProc(LPVOID lpParam);

	///连接状态线程，断开重连
	static DWORD WINAPI ThreadTime(LPVOID lpParameter);

private:	

	///回调对象
	messageCallBack messageFunc,resultFunc;
	containerCallBack containerFunc;

	///主动取结果动作
	bool result;

	///取结果模式
	bool model;

	///是否连接
	bool isLink;

	///线程循环状态
	bool isRun;	

	///服务器地址
	const char *serverIP;

	///服务器端口
	u_short serverPort;

	///TCP对象
	SOCKET socketClient;

	///事件对象
	WSAEVENT socketEvent;

	///地址结构
	SOCKADDR_IN socketAddr;

	///创建事件对象
	HANDLE createEvent;

	///事件线程对象
	HANDLE threadEvent;

	///状态线程对象
	HANDLE threadTime;
};