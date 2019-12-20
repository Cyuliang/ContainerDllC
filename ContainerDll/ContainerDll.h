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

	///��Ϣ���ص���������
	typedef void(*messageCallBack)(string);

	///����ص���������
	///ʱ�䣬ͨ���ţ����ͣ�ǰ�䣬У�飬���䣬У�飬ǰ���ͣ�������
	typedef void(*containerCallBack)(string,string, string,string, string,string,string, string,string);

	///ע�����ص�����
	void registerContainerCallBack(containerCallBack func);

	///ע����־�ص�����
	void registerMessageCallBack(messageCallBack func);

	///ע����Ϣ���ص�����
	void registerResultCallBack(messageCallBack func);

	///��ʼ��TCP
	void init(const char *serverIP, u_short serverPort);

	///������ȡ���
	bool getResult();

	///���û�ȡ���ģʽ
	void setModel(bool model);

private:

	///�ر�TCP���ͷ���Դ
	void socketStop();

	///����TCP
	bool socketLink();

	void containerAnalysis(string con);

	///�¼�ѭ���߳�
	static DWORD WINAPI threadProc(LPVOID lpParam);

	///����״̬�̣߳��Ͽ�����
	static DWORD WINAPI ThreadTime(LPVOID lpParameter);

private:	

	///�ص�����
	messageCallBack messageFunc,resultFunc;
	containerCallBack containerFunc;

	///����ȡ�������
	bool result;

	///ȡ���ģʽ
	bool model;

	///�Ƿ�����
	bool isLink;

	///�߳�ѭ��״̬
	bool isRun;	

	///��������ַ
	const char *serverIP;

	///�������˿�
	u_short serverPort;

	///TCP����
	SOCKET socketClient;

	///�¼�����
	WSAEVENT socketEvent;

	///��ַ�ṹ
	SOCKADDR_IN socketAddr;

	///�����¼�����
	HANDLE createEvent;

	///�¼��̶߳���
	HANDLE threadEvent;

	///״̬�̶߳���
	HANDLE threadTime;
};