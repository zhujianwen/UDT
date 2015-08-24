// turbo.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include "transfer.h"


class TransferPorxy : public TurboListener
{
public:
	static TransferPorxy* GetInStance();
	TurboTransfer* core() const { return m_core; }

private:
	TransferPorxy()
		: m_core(new TurboTransfer(this))
	{}
	~TransferPorxy() 
	{}

	virtual void OnAccept(const char* filename);
	virtual void OnTransfer(const char* filename, long totalsize, long cursize, int type);
	virtual void OnFinished(const char* message, int result);

private:
	static TransferPorxy* m_InStance;
	TurboTransfer* m_core;
	int m_sock;
};

TransferPorxy* TransferPorxy::m_InStance = NULL;

TransferPorxy* TransferPorxy::GetInStance()
{
	if (m_InStance == NULL) m_InStance = new TransferPorxy();

	return m_InStance;
}

void TransferPorxy::OnAccept(const char* filename)
{

}

void TransferPorxy::OnTransfer(const char* filename, long totalsize, long cursize, int type)
{

}

void TransferPorxy::OnFinished(const char* message, int result)
{

}

int main(int argc, char* argv[])
{
	printf("\t/***********************************\n");
	printf("\t 1 - 发送文本消息   2 - 发送单多文件\n\t 3 - 停止发送文件   4 - 停止接收文件\n\t 5 - 是否接收文件   0 - 退出\n");
	printf("\t ***********************************/\n");

	TransferPorxy * pUdt = TransferPorxy::GetInStance();

	int sock;
	char ip[32] = {0};
	char buf[256] = {0};

	while (gets_s(buf))
	{
		if (strcmp(buf, "1") == 0)
		{
			pUdt->core()->StartServer("127.0.0.1", "6605");
		}
		else if (strcmp(buf, "2") == 0)
		{
			printf("输入目标IP：");
			gets_s(ip);
			printf("输入文件名全路径 , end 结束输入\n");
			std::vector<std::string> vecNames;
			char fileName[256];
			while (gets_s(fileName) != NULL)
			{
				if (strcmp("end", fileName) == 0)
					break;
				vecNames.push_back(fileName);
			}
			//sock = pUdt->core()->SendFiles(ip, vecNames, "HTC G18", "ANDROID", "zhujianwen", "WIN7", "GENIETURBO");
			pUdt->core()->FileTransfer(ip, 6605, vecNames[0].c_str(), 0, 0);
		}
		else if (strcmp(buf, "3") == 0)
		{
			//pUdt->core()->StopTransfer(sock, 2);
		}
		else if (strcmp(buf, "4") == 0)
		{
			//pUdt->core()->StopTransfer(sock, 1);
		}
		else if (strcmp(buf, "5") == 0)
		{
			printf("确定接收，输入文件存储路径:D:\\Genie\\；拒绝接收，输入:REJECT\n");
			char filePath[256];
			gets_s(filePath);
			pUdt->core()->ReplyAccept(sock, filePath);
		}
		else if (strcmp(buf, "0") == 0)
		{
			printf("quit.....\n");
			break;
		}
	}

	pUdt->core()->StopServer();

	return 0;
}

