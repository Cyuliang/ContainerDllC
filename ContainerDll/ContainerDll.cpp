// ContainerDll.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include "ContainerDll.h"
#include "string"

#pragma comment(lib,"WS2_32.Lib")

Container::Container()
{
	isLink = false;
	isRun = false;
	result = false;
	model = false;

	socketAddr = { 0 };

	socketClient = INVALID_SOCKET;
	socketEvent = WSA_INVALID_EVENT;
	createEvent = nullptr;
	threadEvent = nullptr;

	messageFunc = nullptr;
	resultFunc = nullptr;
	containerFunc = nullptr;
}

Container::~Container()
{
	isRun = false;
	WSACleanup();
	socketStop();
}

///设置模式
void  Container::setModel(bool model)
{
	this->model = model;
}

///注册结果回调函数
void  Container::registerContainerCallBack(containerCallBack func)
{
	this->containerFunc = func;
}

///注册信息流回调函数
void  Container::registerMessageCallBack(messageCallBack func)
{
	this->messageFunc = func;
}

///注册识别流回调函数
void  Container::registerResultCallBack(messageCallBack func)
{
	this->resultFunc = func;
}

///初始化SOCKET版本
void  Container::init(const char *serverIP, u_short serverPort)
{
	this->serverIP = serverIP;
	this->serverPort = serverPort;

	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIP, &socketAddr.sin_addr);

	WORD version = MAKEWORD(2, 2);
	WSADATA wsaData = { 0 };	

	if (WSAStartup(version, &wsaData) != 0) {

		if (messageFunc != nullptr) {
			messageFunc("WSAStartup Failed :" + std::to_string(WSAGetLastError()) + "\n");
		}
	}
	else {

		if (messageFunc != nullptr) {
			messageFunc("WSAStartup Successful\n");
		}
		if (socketLink()) {//判断连接
			isLink = true;
		}
		threadTime = CreateThread(nullptr, 0, ThreadTime, (void*)this, 0, 0);
	}
}

///主动获取结果
bool  Container::getResult()
{
	if (model) {
		if (send(socketClient, "[R|01]", 7, 0) == SOCKET_ERROR)
		{
			return false;
		}
		result = true;
		return true;
	}
	return false;
}

///断开连接，释放资源
void Container::socketStop()
{
	SetEvent(createEvent);
	WaitForSingleObject(threadEvent, INFINITE);
	closesocket(socketClient);
	WSACloseEvent(socketEvent);
	CloseHandle(createEvent);
}

///开始连接
bool Container::socketLink()
{
	socketClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	socketEvent = WSACreateEvent();
	createEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (INVALID_SOCKET == socketClient) {

		if (messageFunc != nullptr) {
			messageFunc("Socket Init Failed :" + std::to_string(WSAGetLastError()) + "\n");
		}
		return false;
	}
	else {

		if (messageFunc != nullptr) {
			messageFunc("Socket Init Successful\n");
		}
		if (connect(socketClient, (sockaddr*)&socketAddr, sizeof(sockaddr)) == SOCKET_ERROR) {

			if (messageFunc != nullptr) {
				messageFunc("Socket Link Failed\n");
			}
			closesocket(socketClient);
			return false;
		}
		else {

			if (messageFunc != nullptr) {
				messageFunc("Socket Link Successful\n");
			}
			if (WSAEventSelect(socketClient, socketEvent, FD_READ | FD_CLOSE | FD_CONNECT) != 0) {

				if (messageFunc != nullptr) {
					messageFunc("Event Set Failed\n");
				}
				return false;
			}
			else {

				if (messageFunc != nullptr) {
					messageFunc("Event Set Successful\n");
				}
				threadEvent = CreateThread(nullptr, 0, threadProc, (void*)this, 0, 0);
			}
		}
	}
	return true;
}

///轮询事件
DWORD Container::threadProc(LPVOID lpParam)
{
	if (lpParam == nullptr) {
		return 0;
	}
	
	DWORD ret = 0;
	int index = 0;

	Container *client =(Container*)lpParam;
	WSANETWORKEVENTS networkEvent;
	HANDLE events[2];
	events[0] = client->socketEvent;
	events[1] = client->createEvent;

	while (!client->isRun)
	{
		ret = WSAWaitForMultipleEvents(2, events, FALSE, INFINITE, FALSE);
		index = ret - WSA_WAIT_EVENT_0;

		if (index == WSA_WAIT_FAILED || index == WSA_WAIT_TIMEOUT) {

			if (client->messageFunc != nullptr) {
				client->messageFunc("Socket Failed or Timeout Error :" + std::to_string(WSAGetLastError()) + "\n");
			}
			continue;
		}

		WSAResetEvent(events);
	
		//读取事件
		WSAEnumNetworkEvents(client->socketClient, events[0], &networkEvent);
		if (networkEvent.lNetworkEvents & FD_READ){
			if (networkEvent.iErrorCode[FD_READ_BIT != 0]){

				if (client->messageFunc != nullptr) {
					client->messageFunc("Socket FD_READ Error :" + std::to_string(WSAGetLastError()) + "\n");
				}
				continue;
			}

			string container = "";
			while (true)//循环接收数据直到收到正确结果
			{
				Sleep(1);

				char* buff = new char[1024]{ 0 };
				ret = recv(client->socketClient, buff, 1024, 0);
				if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)) {

					if (client->messageFunc != nullptr) {
						client->messageFunc("Socket Read Data Error :" + std::to_string(WSAGetLastError()) + "\n");
					}
					break;
				}

				string tmp = buff;	

				delete[] buff;
				buff = NULL;

				if (tmp.length() == 4) {
					break;
				}	
				else {					
					while (true)//排除心跳包
					{
						size_t pos = tmp.find("[H]");
						if (pos !=tmp.npos) {							
							tmp.erase(pos,4);
						}
						else {
							break;
						}
					}
				}
				container += tmp;

				if (tmp.find("[C") != tmp.npos) {//收到完整结果跳出
					break;
				}
			}
			client->containerAnalysis(container);//分析结果
		}
		//关闭事件
		else if (networkEvent.lNetworkEvents & FD_CLOSE){
			if (networkEvent.iErrorCode[FD_CLOSE_BIT] != 0){

				if (client->messageFunc != nullptr) {
					client->messageFunc("Socket FD_CLOSE Error :" + std::to_string(WSAGetLastError()) + "\n");
				}
				continue;   
			}			

			if (client->messageFunc != nullptr) {
				client->messageFunc("Socket FD_CLOSE Successful\n");
			}
			break;    //关闭		
		}
		//连接事件
		else {
			if (networkEvent.lNetworkEvents & FD_CONNECT)
			{
				if (networkEvent.iErrorCode[FD_CONNECT_BIT] != 0)
				{
					break;
				}

				if (client->messageFunc != nullptr) {
					client->messageFunc("Socket FD_CONNECT Successful\n");
				}
			}
		}
		client->isLink = true;
	}
	client->isLink = false;
	return 1;
}

///线程循环判断连接状态
DWORD  Container::ThreadTime(LPVOID lpParam)
{
	if (lpParam == nullptr) {
		return 0;
	}

	Container *client = (Container*)lpParam;

	while (!client->isRun)
	{
		Sleep(8000);
		if (!client->isLink|| (send(client->socketClient, "0xFF", 5, 0) == SOCKET_ERROR))
		{
			client->socketStop();
			client->socketLink();
		}
	}
	return 1;
}

///分析结果
void Container::containerAnalysis(string con)
{	
	string str[20];
	size_t pos = con.rfind("[C|");

	//中间结果
	if (resultFunc != nullptr) {
		resultFunc(con.substr(0, pos));
	}

	string tmp= con.substr(pos + 3);

	int s = 0;
	for (size_t i = 0; i < tmp.length(); i++) { //将字符串分割成字符串数组
		if (tmp[i] == '|') {               
			s++;
			continue;
		}
		str[s] += tmp[i];
	}
	
	for (size_t j =0 ; j < str->length(); j++) {
		pos = str[j].rfind("]");
		if (pos != str[j].npos) {
			str[j]=str[j].substr(0, pos);
		}
	}

	if (result&&model) {//主动模式
		if (str[2] != "2") {

			if (containerFunc != nullptr) {
				containerFunc(str[0], str[1], str[2], str[3], "", "", "", str[5], "");
			}

		}
		else {//时间，通道号，箱型，前箱，校验，后箱，校验，前箱型，后箱型

			if (containerFunc != nullptr) {
				containerFunc(str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8]);
			}
		}
		result = false;
	}
	if (!model) {//被动模式
		if (str[2] != "2") {

			if (containerFunc != nullptr) {
				containerFunc(str[0], str[1], str[2], str[3], "", "", "", str[5], "");
			}
		}
		else {//时间，通道号，箱型，前箱，校验，后箱，校验，前箱型，后箱型

			if (containerFunc != nullptr) {
				containerFunc(str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8]);
			}
		}
	}
}