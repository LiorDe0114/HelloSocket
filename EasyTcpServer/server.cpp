#include "EasyTcpServer.hpp"
#include<thread>

bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("�˳�cmdThread�߳�\n");
			break;
		}
		else
		{
			printf("��֧�ֵ����\n");
		}
	}
}

class MyServer : public EasyTcpServer
{
public:

	//ֻ�ᱻһ���̴߳��� ��ȫ
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
		//printf("client<%d> join\n", pClient->sockfd());
	}
	//cellServer 4 ����̴߳��� ����ȫ
	//���ֻ����1��cellServer���ǰ�ȫ��
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
		//printf("client<%d> leave\n", pClient->sockfd());
	}
	//cellServer 4 ����̴߳��� ����ȫ
	//���ֻ����1��cellServer���ǰ�ȫ��
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_msgCount++;//��Ϣ����
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			//send recv
			Login* login = (Login*)header;
			//printf("�յ��ͻ���<Socket=%d>����CMD_LOGIN,���ݳ��ȣ�%d,userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->PassWord);
			//�����ж��û������Ƿ���ȷ�Ĺ���
			LoginResult ret;
			pClient->SendData(&ret);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//printf("�յ��ͻ���<Socket=%d>����CMD_LOGOUT,���ݳ��ȣ�%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
			//�����ж��û������Ƿ���ȷ�Ĺ���
			//LogoutResult ret;
			//SendData(cSock, &ret);
		}
		break;
		default:
		{
			printf("<socket=%d>�յ�δ������Ϣ,���ݳ��ȣ�%d\n", pClient->sockfd(), header->dataLength);
			//DataHeader ret;
			//SendData(cSock, &ret);
		}
		break;
		}
	}

	virtual void OnNetRecv(ClientSocket* pClient)
	{
		_recvCount++;
		//printf("client<%d> leave\n", pClient->sockfd());
	}
private:

};

int main()
{

	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);
	server.Start(4);

	//����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.OnRun();
		//printf("����ʱ�䴦������ҵ��..\n");
	}
	server.Close();
	printf("���˳���\n");
	getchar();
	return 0;
}