// p2p_test.cpp : 定义控制台应用程序的入口点。
//

#include <iostream>
#include <stdio.h>
#include <tchar.h>

#include <udt.h>

#ifdef WIN32
	#include <Windows.h>
#endif


#ifndef WIN32
void* monitor(void*);
#else
DWORD WINAPI monitor(LPVOID);
#endif

#ifndef WIN32
void* monitor(void* usocket)
#else
DWORD WINAPI monitor(LPVOID usocket)
#endif
{
	//P2PSOCKET fhandle = *(P2PSOCKET*)usocket;
	//delete (P2PSOCKET*)usocket;
	//bool bConnect = false;

	//while(true)
	//{
	//	if (!bConnect)
	//	{
	//		lastick = ::GetTickCount();
	//		P2P::p2p_connect("android_test");
	//	}

	//	if (bConnect)
	//	{
	//		//icem = (struct icem *)list_ledata(list_head(ice_medialist(pconn->ice)));
	//		//icem_update(icem);
	//		//print_icem(icem);
	//		//p2p_send(pconn,"hello!", 6);
	//	}

	//	Sleep(5000);
	//}

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef WIN32
	char* filePath = argv[0];
	memset(argv, 0, sizeof(argv));
	argc = 2;
	argv[0] = filePath;
	argv[1] = "7777";
#endif

	//usage: sendfile [server_port]
	if ((2 < argc) || ((2 == argc) && (0 == atoi(argv[1]))))
	{
		std::cout << "usage: sendfile [server_port]" << std::endl;
		return 0;
	}

	int err;
	err = UDT::p2pInit("54.254.169.186", 3456, "54.254.169.186", 3478, "zhujianwen", "123456");
	if (err)
	{
		std::cout<< "p2p_init error"<<std::endl;
		goto out;
	}
	printf("\n\np2p init with username: zhujianwen\n");
	UDTSOCKET psock = 0;

	char buf[100] = {0};
	while (gets_s(buf))
	{
		if (strcmp("connect", buf) == 0)
		{
			printf("\n\np2p connect to: liuhong\n");
			psock = UDT::p2pConnect("liuhong");
		}
		else if (strcmp("send", buf) == 0)
		{
			char msg[1024] = {0};
			gets_s(msg);
			printf("\n\np2p send to: liuhong\n");
			err = UDT::p2pSend(psock, "liuhong", msg, strlen(msg));
		}
		else if (strcmp("exit", buf) == 0)
		{
			break;
		}
	}

#ifndef WIN32
	pthread_t filethread;
	pthread_create(&filethread, NULL, monitor, new P2PSOCKET(psock));
	pthread_detach(filethread);
#else
	//CreateThread(NULL, 0, monitor, new P2PSOCKET(psock), 0, NULL);
#endif
	//err = P2P::p2p_run();
	return 0;

out:
	UDT::p2pClose();
	return err;
}

