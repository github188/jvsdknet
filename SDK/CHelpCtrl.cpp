// CHelpCtrl.cpp: implementation of the CCHelpCtrl class.
//
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
#pragma warning(disable:4786)
#endif

#include "CHelpCtrl.h"

#include "CChannel.h"

#include "CVirtualChannel.h"
#include "MakeHoleC.h"
#include "CWorker.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define TCP_TIME_OUT 1
char g_strConnectType[4][10] =
{
	"δ����","����","ת��","����"
};

#include <stdio.h>

#include <stdarg.h>
void OutputDebug(char* format, ...)
{
#ifdef WIN32
	#ifdef _DEBUG
		va_list arglist;
		char buffer[1024];

		va_start (arglist,format);
		vsprintf(buffer, format, arglist);
		va_end (arglist);

		SYSTEMTIME sys;
		GetLocalTime(&sys);

		char data[1024] = {0};
		sprintf(data,"%02d:%02d:%02d %s\n",sys.wHour,sys.wMinute,sys.wSecond,buffer);
		OutputDebugString(data);
	#endif
#else
//    char data[1024] = {0};
//    char szData[1024] = {0};
//    va_list va_args;
//    va_start(va_args, format);
//
//    size_t length = vsnprintf(NULL, 0,format, va_args);
//    int result = vsnprintf(szData, length + 1, format, va_args);
//
//	time_t now = time(0);
//	tm *tnow = localtime(&now);
//
//	sprintf(data,"%02d:%02d:%02d %s\n",tnow->tm_hour,tnow->tm_min,tnow->tm_sec,szData);
//
//    printf(data);
//
//    va_end(va_args);

#endif
}


//������
DWORD GetMyTimeCount()
{
	DWORD dwTime = 0;
#ifndef WIN32
	struct timeval start;
	gettimeofday(&start,NULL);

	dwTime = (DWORD)(start.tv_sec * 1000 + start.tv_usec/1000);
#else
	dwTime = GetTickCount();
#endif

	return dwTime;
}


CCHelpCtrl::CCHelpCtrl()
{
	m_bExit = FALSE;
}

CCHelpCtrl::CCHelpCtrl(CCWorker *pWorker)
{
	m_pWorker = pWorker;
	m_bExit = FALSE;
}

CCHelpCtrl::~CCHelpCtrl()
{
	m_bExit = TRUE;
}

DWORD CCHelpCtrl::JVGetTime()
{
	DWORD dwresult=0;
#ifndef WIN32
	struct timeval start;
	gettimeofday(&start,NULL);

	dwresult = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
#else
	dwresult = GetTickCount();
#endif

	return dwresult;
}
void CCHelpCtrl::jvc_sleep(unsigned long time)//����
{
#ifndef WIN32
	struct timeval start;
	start.tv_sec = 0;
	start.tv_usec = time*1000;
	select(0,NULL,NULL,NULL,&start);
#else
	Sleep(time);
#endif
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCHelpCtrlH::CCHelpCtrlH()
{
}

CCHelpCtrlH::CCHelpCtrlH(CCWorker *pWorker):CCHelpCtrl(pWorker)
{
#ifndef WIN32
	pthread_mutex_init(&m_criticalsectionVList, NULL); //��ʼ���ٽ���
	pthread_mutex_init(&m_criticalsectionHList, NULL); //��ʼ���ٽ���
#else
	InitializeCriticalSection(&m_criticalsectionVList); //��ʼ���ٽ���
	InitializeCriticalSection(&m_criticalsectionHList); //��ʼ���ٽ���
#endif
	m_nHelpType = JVN_WHO_H;

	m_hWorkCmdThread = 0;//�߳̾��
	m_hWorkDataThread = 0;//�߳̾��
	m_hListenCmdThread = 0;//�߳̾��
	m_hListenDataThread = 0;//�߳̾��
	m_ListenSockCmd = 0;//С���ֱ��ؼ����߳�	�����ͻ����� ���������
	m_ListenSockData = 0;//С���ֱ��ؼ����߳� ���ڴ�����ʵ����

	StartWorkThread();
}

CCHelpCtrlH::~CCHelpCtrlH()
{
	m_bExit = TRUE;

#ifndef WIN32
	if (0 != m_hWorkCmdThread)
	{
		pthread_join(m_hWorkCmdThread, NULL);
		m_hWorkCmdThread = 0;
	}
	if (0 != m_hWorkDataThread)
	{
		pthread_join(m_hWorkDataThread, NULL);
		m_hWorkDataThread = 0;
	}
	if (0 != m_hListenCmdThread)
	{
		pthread_join(m_hListenCmdThread, NULL);
		m_hListenCmdThread = 0;
	}
	if (0 != m_hListenDataThread)
	{
		pthread_join(m_hListenDataThread, NULL);
		m_hListenDataThread = 0;
	}
#else

	jvc_sleep(10);

	CCChannel::WaitThreadExit(m_hWorkDataThread);
	CCChannel::WaitThreadExit(m_hWorkCmdThread);
	CCChannel::WaitThreadExit(m_hListenCmdThread);
	CCChannel::WaitThreadExit(m_hListenDataThread);

	if(m_hWorkDataThread > 0)
	{
		CloseHandle(m_hWorkDataThread);
		m_hWorkDataThread = 0;
	}
	if(m_hWorkCmdThread > 0)
	{
		CloseHandle(m_hWorkCmdThread);
		m_hWorkCmdThread = 0;
	}
	if(m_hListenCmdThread > 0)
	{
		CloseHandle(m_hListenCmdThread);
		m_hListenCmdThread = 0;
	}
	if(m_hListenDataThread > 0)
	{
		CloseHandle(m_hListenDataThread);
		m_hListenDataThread = 0;
	}
#endif

	shutdown(m_ListenSockCmd,SD_BOTH);
	closesocket(m_ListenSockCmd);
	m_ListenSockCmd = 0;

	shutdown(m_ListenSockData,SD_BOTH);
	closesocket(m_ListenSockData);
	m_ListenSockData = 0;


	for(CMD_LIST::iterator i = m_TcpListCmd.begin(); i != m_TcpListCmd.end(); ++ i)
	{
		delete m_TcpListCmd[i->second->sTcpCmdSocket];
	}
	m_TcpListCmd.clear();

	for(DATA_LIST::iterator j = m_TcpListDataH.begin(); j != m_TcpListDataH.end(); ++ j)
	{
		shutdown(j->second->sTcpDataSocket,SD_BOTH);
		closesocket(j->second->sTcpDataSocket);
	}
	m_TcpListDataH.clear();

	for(VCONN_List::iterator k = m_VListH.begin(); k != m_VListH.end(); ++ k)
	{
		delete k->second.pVirtualChannel;
	}
	m_VListH.clear();

#ifndef WIN32
	pthread_mutex_destroy(&m_criticalsectionVList);
	pthread_mutex_destroy(&m_criticalsectionHList);
#else
	DeleteCriticalSection(&m_criticalsectionVList); //�ͷ��ٽ���
	DeleteCriticalSection(&m_criticalsectionHList); //�ͷ��ٽ���
#endif

}

//��ѯ����ͨ�б�״̬ ��������
int CCHelpCtrlH::GetHelpYSTNO(BYTE *pBuffer, int &nSize)
{
	if(m_ListenSockCmd == 0)
		return 0;

	int nFixSize = sizeof(STBASEYSTNO);

	int nNum = m_VListH.size();
	if(nNum * nFixSize > nSize)
		return -1;

	int i = 0;
	for(VCONN_List::iterator k = m_VListH.begin(); k != m_VListH.end(); ++ k,i ++)
	{
		STBASEYSTNO info;
		memset(&info,0,sizeof(STBASEYSTNO));
		strcpy(info.chGroup,k->second.chGroup);
		info.nYSTNO = k->second.nYSTNO;
		strcpy(info.chPName,k->second.chUserName);
		strcpy(info.chPWord,k->second.chPasswords);
		info.nChannel = k->second.nChannel;
		info.nConnectStatus = k->second.nConnectType;

		memcpy(pBuffer + i * nFixSize,&info,nFixSize);
	}
	nSize = nNum * nFixSize;

	return 1;
}

BOOL CCHelpCtrlH::SetHelpYSTNO(BYTE *pBuffer, int nSize)
{
#ifndef WIN32
	pthread_mutex_lock(&m_criticalsectionVList);
#else
	EnterCriticalSection(&m_criticalsectionVList);
#endif

	STBASEYSTNO info = {0};
	int num = nSize / sizeof(STBASEYSTNO);

	for(int j = 0;j < num;j ++)
	{
		memcpy(&info,&pBuffer[j*sizeof(STBASEYSTNO)],sizeof(STBASEYSTNO));
		char yst[20] = {0};
		sprintf(yst,"%s%d",info.chGroup,info.nYSTNO);
		VCONN_List ::iterator i = m_VListH.find(std::string(yst));

		//�����б����Ƿ��Ѿ����� ���������
		if(i == m_VListH.end())
		{
			STVLINK pLink;// = new STVLINK;
			memset(&pLink,0,sizeof(STVLINK));
			strcpy(pLink.chGroup,info.chGroup);
			pLink.nYSTNO = info.nYSTNO;
			pLink.bStatus = FALSE;
			pLink.nChannel = info.nChannel;
			strcpy(pLink.chUserName,info.chPName);
			strcpy(pLink.chPasswords,info.chPWord);

//			OutputDebug("���������%s  ͨ�� %d (%s,%s)",yst,pLink.nChannel,pLink.chUserName,pLink.chPasswords);
			STCONNECTINFO stConnectInfo;
			memset(&stConnectInfo,0,sizeof(STCONNECTINFO));
			stConnectInfo.nWhoAmI = JVN_WHO_H;
			stConnectInfo.bLocalTry = TRUE;
			stConnectInfo.nTURNType = JVN_TRYTURN;

			stConnectInfo.bYST = TRUE;
			strcpy(stConnectInfo.chGroup,pLink.chGroup);
			stConnectInfo.nYSTNO = pLink.nYSTNO;
			stConnectInfo.nChannel = pLink.nChannel;
			strcpy(stConnectInfo.chPassName,pLink.chUserName);
			strcpy(stConnectInfo.chPassWord,pLink.chPasswords);
			stConnectInfo.nLocalChannel = m_VListH.size() + 1;

			pLink.pVirtualChannel = new CCVirtualChannel(stConnectInfo,this,m_pWorker);

			std::string s(yst);
			//m_VListH.insert(yst,pLink);
			m_VListH.insert(std::map<std::string,STVLINK>::value_type(s, pLink));
		}
		else//�Ѿ�����
		{
			STVLINK pLink;// = {0};
			memcpy(&pLink,&i->second,sizeof(STVLINK));
			if(pLink.nConnectType == JVN_HELPRET_NULL)
			{
				pLink.pVirtualChannel->m_dwConnectTime = 0;
				pLink.nChannel = info.nChannel;
				strcpy(pLink.chUserName,info.chPName);
				strcpy(pLink.chPasswords,info.chPWord);
				std::string s(yst);
				m_VListH[s] = pLink;

//				OutputDebug("����������%s  ͨ�� %d (%s,%s)",yst,pLink.nChannel,pLink.chUserName,pLink.chPasswords);

			}
		}
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_criticalsectionVList);
#else
	LeaveCriticalSection(&m_criticalsectionVList);
#endif

	return FALSE;
}


BOOL CCHelpCtrlH::StartWorkThread(int nPortCmd/* = 9000*/,int nPortData/* = 8000*/)
{
	//�������
	{
		//��ʼ��������cvtcp����
		m_ListenSockCmd = socket(AF_INET, SOCK_STREAM, 0);
	#ifdef WIN32
		if (m_ListenSockCmd == INVALID_SOCKET)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(socket).%d",GetLastError());

			return FALSE;
		}
	#else
		if (m_ListenSockCmd <= 0)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(socket).%d",errno);

			return FALSE;
		}
	#endif

		BOOL bReuse = TRUE;
		setsockopt(m_ListenSockCmd, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));

		SOCKADDR_IN addr;
	#ifdef WIN32
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	#else
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	#endif

		addr.sin_family = AF_INET;
		addr.sin_port = htons(nPortCmd);//���ּ���9000�˿�
	#ifdef WIN32
		if (bind(m_ListenSockCmd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(bind).%d",GetLastError());

			shutdown(m_ListenSockCmd,SD_BOTH);
			closesocket(m_ListenSockCmd);
			return FALSE;
		}
	#else
		if (bind(m_ListenSockCmd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(bind).%d",errno);

			shutdown(m_ListenSockCmd,SD_BOTH);
			closesocket(m_ListenSockCmd);
			return FALSE;
		}
	#endif

		//////////////////////////////////////////////////////////////////////////
		int iSockStatus = 0;
		//���׽�����Ϊ������ģʽ
	#ifndef WIN32
		int flags;
		if ((flags = fcntl(m_ListenSockCmd, F_GETFL, 0)) < 0)
		{
			OutputDebug("TCP��������ʧ�� ioctlsocket err %d",errno);
			closesocket(m_ListenSockCmd);
			m_ListenSockCmd = 0;
			return FALSE;
		}

		if (fcntl(m_ListenSockCmd, F_SETFL, flags | O_NONBLOCK) < 0)
		{
			OutputDebug("TCP��������ʧ�� ioctlsocket err %d",errno);
			closesocket(m_ListenSockCmd);
			m_ListenSockCmd = 0;
			return FALSE;
		}
	#else
		unsigned long ulBlock = 1;
		iSockStatus = ioctlsocket(m_ListenSockCmd, FIONBIO, (unsigned long*)&ulBlock);
		if (SOCKET_ERROR == iSockStatus)
		{
			OutputDebug("TCP��������ʧ�� ioctlsocket err %d",GetLastError());
			closesocket(m_ListenSockCmd);
			m_ListenSockCmd = 0;
			return FALSE;
		}
	#endif

		//���׽�����Ϊ���ȴ�δ�����������
		LINGER linger;
		linger.l_onoff = 1;//0;
		linger.l_linger = 0;
		iSockStatus = setsockopt(m_ListenSockCmd, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
	#ifndef WIN32
		if (iSockStatus < 0)
		{
			OutputDebug("TCP��������ʧ�� setsockopt err %d",errno);
			closesocket(m_ListenSockCmd);
			m_ListenSockCmd = 0;
			return FALSE;
		}
	#else
		if (SOCKET_ERROR == iSockStatus)
		{
			OutputDebug("TCP��������ʧ�� setsockopt err %d",GetLastError());
			closesocket(m_ListenSockCmd);
			m_ListenSockCmd = 0;
			return FALSE;
		}
	#endif

	#ifdef WIN32
		if (listen(m_ListenSockCmd, 10) == SOCKET_ERROR)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(listen).%d",GetLastError());

			shutdown(m_ListenSockCmd,SD_BOTH);
			closesocket(m_ListenSockCmd);
			return FALSE;
		}
	#else
		if (listen(m_ListenSockCmd, 10) < 0)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(listen).%d",errno);

			shutdown(m_ListenSockCmd,SD_BOTH);
			closesocket(m_ListenSockCmd);
			return FALSE;
		}
	#endif
	}
	//�������Ӽ���
	{
		//��ʼ��������cvtcp����
		m_ListenSockData = socket(AF_INET, SOCK_STREAM, 0);

	#ifdef WIN32
		if (m_ListenSockData == INVALID_SOCKET)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(socket).%d",GetLastError());

			return FALSE;
		}
	#else
		if (m_ListenSockData <= 0)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(socket).%d",errno);

			return FALSE;
		}
	#endif

		BOOL bReuse = TRUE;
		setsockopt(m_ListenSockData, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));

		SOCKADDR_IN addr;
	#ifdef WIN32
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	#else
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	#endif

		addr.sin_family = AF_INET;
		addr.sin_port = htons(nPortData);//���ּ���8000�˿�
	#ifdef WIN32
		if (bind(m_ListenSockData, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(bind).%d",GetLastError());

			shutdown(m_ListenSockData,SD_BOTH);
			closesocket(m_ListenSockData);
			return FALSE;
		}
	#else
		if (bind(m_ListenSockData, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(bind).%d",errno);

			shutdown(m_ListenSockData,SD_BOTH);
			closesocket(m_ListenSockData);
			return FALSE;
		}
	#endif

		//////////////////////////////////////////////////////////////////////////
		int iSockStatus = 0;
		//���׽�����Ϊ������ģʽ
	#ifndef WIN32
		int flags;
		if ((flags = fcntl(m_ListenSockData, F_GETFL, 0)) < 0)
		{
			OutputDebug("TCP��������ʧ�� ioctlsocket err %d",errno);
			closesocket(m_ListenSockData);
			m_ListenSockData = 0;
			return FALSE;
		}

		if (fcntl(m_ListenSockData, F_SETFL, flags | O_NONBLOCK) < 0)
		{
			OutputDebug("TCP��������ʧ�� ioctlsocket err %d",errno);
			closesocket(m_ListenSockData);
			m_ListenSockData = 0;
			return FALSE;
		}
	#else
		unsigned long ulBlock = 1;
		iSockStatus = ioctlsocket(m_ListenSockData, FIONBIO, (unsigned long*)&ulBlock);
		if (SOCKET_ERROR == iSockStatus)
		{
			OutputDebug("TCP��������ʧ�� ioctlsocket err %d",GetLastError());
			closesocket(m_ListenSockData);
			m_ListenSockData = 0;
			return FALSE;
		}
	#endif

		//���׽�����Ϊ���ȴ�δ�����������
		LINGER linger;
		linger.l_onoff = 1;//0;
		linger.l_linger = 0;
		iSockStatus = setsockopt(m_ListenSockData, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
	#ifndef WIN32
		if (iSockStatus < 0)
		{
			OutputDebug("TCP��������ʧ�� setsockopt err %d",errno);
			closesocket(m_ListenSockData);
			m_ListenSockData = 0;
			return FALSE;
		}
	#else
		if (SOCKET_ERROR == iSockStatus)
		{
			OutputDebug("TCP��������ʧ�� setsockopt err %d",GetLastError());
			closesocket(m_ListenSockData);
			m_ListenSockData = 0;
			return FALSE;
		}
	#endif

		//////////////////////////////////////////////////////////////////////////

	#ifdef WIN32
		if (listen(m_ListenSockData, 10) == SOCKET_ERROR)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(listen).%d",GetLastError());

			shutdown(m_ListenSockData,SD_BOTH);
			closesocket(m_ListenSockData);
			return FALSE;
		}
	#else
		if (listen(m_ListenSockData, 10) < 0)
		{
			OutputDebug("TCP��������ʧ��.��ʼ��ʧ��(listen).%d",errno);

			shutdown(m_ListenSockData,SD_BOTH);
			closesocket(m_ListenSockData);
			return FALSE;
		}
	#endif
	}

#ifdef WIN32
	UINT unTheadID = 0;
	m_hListenCmdThread = (HANDLE)_beginthreadex(NULL, 0, ListenProcCmd, (void *)this, 0, &unTheadID);
	if (m_hListenCmdThread == 0)//�����߳�ʧ��
	{
		OutputDebug("TCP��������ʧ�� setsockopt err %d",GetLastError());
		closesocket(m_ListenSockData);
		m_ListenSockData = 0;
		return FALSE;
	}
	m_hListenDataThread = (HANDLE)_beginthreadex(NULL, 0, ListenProcData, (void *)this, 0, &unTheadID);
	if (m_hListenDataThread == 0)//�����߳�ʧ��
	{
		OutputDebug("TCP��������ʧ�� setsockopt err %d",GetLastError());
		closesocket(m_ListenSockData);
		m_ListenSockData = 0;
		return FALSE;
	}
	m_hWorkCmdThread = (HANDLE)_beginthreadex(NULL, 0, WorkCmdProc, (void *)this, 0, &unTheadID);
	if (m_hWorkCmdThread == 0)//�����߳�ʧ��
	{
		OutputDebug("TCP��������ʧ�� setsockopt err %d",GetLastError());
		closesocket(m_ListenSockData);
		m_ListenSockData = 0;
		return FALSE;
	}
#else
	pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
	if (0 != pthread_create(&m_hListenCmdThread, pAttr, ListenProcCmd, this))
	{
		m_hListenCmdThread = 0;

		OutputDebug("TCP��������ʧ�� setsockopt err %d",errno);
		closesocket(m_ListenSockData);
		m_ListenSockData = 0;
		return FALSE;
	}
	if (0 != pthread_create(&m_hListenDataThread, pAttr, ListenProcData, this))
	{
		m_hListenDataThread = 0;

		OutputDebug("TCP��������ʧ�� setsockopt err %d",errno);
		closesocket(m_ListenSockData);
		m_ListenSockData = 0;
		return FALSE;
	}
	if (0 != pthread_create(&m_hWorkCmdThread, pAttr, WorkCmdProc, this))
	{
		m_hWorkCmdThread = 0;

		OutputDebug("TCP��������ʧ�� setsockopt err %d",errno);
		closesocket(m_ListenSockData);
		m_ListenSockData = 0;
		return FALSE;
	}
#endif

	return TRUE;
}

BOOL CCHelpCtrlH::VirtualConnect()
{
	char yst[20] = {0};
	STVLINK pLink;// = {0};
	int i = 0;
	for(VCONN_List::iterator k = m_VListH.begin(); k != m_VListH.end(); ++ k,i ++)
	{
		DWORD dwTime = JVGetTime();

		memcpy(&pLink,&k->second,sizeof(STVLINK));
		sprintf(yst,"%s%d",k->second.chGroup,k->second.nYSTNO);

		std::string s(yst);
		if(pLink.pVirtualChannel->m_nStatus == FAILD || (pLink.pVirtualChannel->m_nStatus == OK && pLink.pVirtualChannel->m_ServerSocket <= 0)
			|| (pLink.pVirtualChannel->m_nStatus == OK && pLink.pVirtualChannel->m_bTURN))
		{//����������Ҫ����������
			if(pLink.pVirtualChannel->m_bTURN)
			{
				if(pLink.pVirtualChannel->m_nStatus == OK)
				{//ת���ѶϿ�����ת��������
					pLink.pVirtualChannel->DisConnect();
					pLink.nConnectType = JVN_HELPRET_TURN;
					m_VListH[s] = pLink;//��ס��ת��

					continue;
				}
				//ת������
				int ntm = 1800000;
				if(pLink.nFailedCount >= 0 && pLink.nFailedCount < 3)
				{
					ntm = 300000;
				}
				else if(pLink.nFailedCount >= 3 && pLink.nFailedCount <= 10)
				{
					ntm = 900000;
				}
				if((dwTime < pLink.pVirtualChannel->m_dwConnectTime || dwTime > pLink.pVirtualChannel->m_dwConnectTime + ntm))//90000
				{//ת��ÿ��1.5��������һ��
					STCONNECTINFO stConnectInfo;
					memset(&stConnectInfo,0,sizeof(STCONNECTINFO));
					stConnectInfo.nWhoAmI = JVN_WHO_H;
					stConnectInfo.bLocalTry = TRUE;
					stConnectInfo.nTURNType = JVN_TRYTURN;

					stConnectInfo.bYST = TRUE;
					strcpy(stConnectInfo.chGroup,pLink.chGroup);
					stConnectInfo.nYSTNO = pLink.nYSTNO;
					stConnectInfo.nChannel = pLink.nChannel;
					strcpy(stConnectInfo.chPassName,pLink.chUserName);
					strcpy(stConnectInfo.chPassWord,pLink.chPasswords);

					pLink.pVirtualChannel->ReConnect(stConnectInfo,this,m_pWorker);
					pLink.nFailedCount++;
					OutputDebug("ת�� ����..%s...������ ",yst);
					continue;
				}
				continue;
			}

			//ת������
			int ntm = 1800000;//30����
			if(pLink.nFailedCount >= 0 && pLink.nFailedCount < 3)
			{
				ntm = 300000;
			}
			else if(pLink.nFailedCount >= 3 && pLink.nFailedCount <= 10)
			{
				ntm = 900000;
			}
			if(dwTime < pLink.pVirtualChannel->m_dwConnectTime || dwTime > pLink.pVirtualChannel->m_dwConnectTime + ntm)//20000
			{//�������ÿ��10������һ��

				pLink.pVirtualChannel->DisConnect();
				pLink.nConnectType = JVN_HELPRET_NULL;
				m_VListH[s] = pLink;

				STCONNECTINFO stConnectInfo;
				memset(&stConnectInfo,0,sizeof(STCONNECTINFO));
				stConnectInfo.nWhoAmI = JVN_WHO_H;
				stConnectInfo.bLocalTry = TRUE;
				stConnectInfo.nTURNType = JVN_TRYTURN;

				stConnectInfo.bYST = TRUE;
				strcpy(stConnectInfo.chGroup,pLink.chGroup);
				stConnectInfo.nYSTNO = pLink.nYSTNO;
				stConnectInfo.nChannel = pLink.nChannel;
				strcpy(stConnectInfo.chPassName,pLink.chUserName);
				strcpy(stConnectInfo.chPassWord,pLink.chPasswords);

				pLink.pVirtualChannel->ReConnect(stConnectInfo,this,m_pWorker);
				pLink.nFailedCount++;
				OutputDebug("����...%s - %d[%s:%s]..������ ",yst,stConnectInfo.nChannel,stConnectInfo.chPassName,stConnectInfo.chPassWord);
			}
		}
		else if(pLink.pVirtualChannel->m_nStatus == OK)
		{
			if(pLink.pVirtualChannel->RecvKeep())
			{
				pLink.pVirtualChannel->m_dwRecvTime = JVGetTime();
			}
			if(dwTime  > pLink.pVirtualChannel->m_dwRecvTime + 30000)
			{
				pLink.pVirtualChannel->DisConnect();
				pLink.nConnectType = JVN_HELPRET_NULL ;
				m_VListH[s] = pLink;
				//pLink.pVirtualChannel->m_nStatus = FAILD;
			}
			if(dwTime - pLink.pVirtualChannel->m_dwSenTime > 5000)
			{
				pLink.pVirtualChannel->m_dwSenTime = dwTime;
				pLink.pVirtualChannel->SendKeep();
				m_VListH[s] = pLink;//��ס����

/*				if(pLink.pVirtualChannel->m_bTURN)
				{
//					OutputDebug("������%d %s  ת��   ok",i,yst);
//					pLink.nConnectType = JVN_HELPRET_TURN;
				}
				else if(pLink.pVirtualChannel->m_bLocal)
				{
//					OutputDebug("������%d %s  ����   ok",i,yst);
//					memcpy(&pLink.addrVirtual,&pLink.pVirtualChannel->m_addrAL,sizeof(SOCKADDR_IN));
//					pLink.nConnectType = JVN_HELPRET_LOCAL;
				}
				else
				{
//					OutputDebug("������%d %s  ����   ok",i,yst);
					memcpy(&pLink.addrVirtual,&pLink.pVirtualChannel->m_addrAN,sizeof(SOCKADDR_IN));
					pLink.nConnectType = JVN_HELPRET_WAN;
				}*/
			}
		}
		else
		{
		}
	}
	return TRUE;
}

BOOL CCHelpCtrlH::ConnectYST(STCONNECTINFO st ,LOCAL_TCP_DATA* pTcp)
{
	st.nLocalChannel = pTcp->sTcpDataSocket;//������������ʱ ����ͨ������������ֿ�֮���TCP
	st.bYST = FALSE;
	st.nWhoAmI = JVN_WHO_H;
	//����������ip
	char yst[20] = {0};
	sprintf(yst,"%s%d",st.chGroup,st.nYSTNO);
	VCONN_List ::iterator i = m_VListH.find(std::string(yst));

	//�����б����Ƿ��Ѿ����� ���������
	if(i != m_VListH.end())
	{
		CCVirtualChannel* pVChannel = i->second.pVirtualChannel;

		if(pVChannel->m_nStatus == OK && pVChannel->m_ServerSocket > 0)
		{
			char ip[20] = {0};
			int port = 0;
			strcpy(st.chServerIP,inet_ntoa(pVChannel->m_addressA.sin_addr));
			st.nServerPort = ntohs(pVChannel->m_addressA.sin_port);
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

//	strcpy(st.chServerIP,"192.168.9.52");
//	st.nServerPort = 9101;
	//�½�ͨ��
	pTcp->pChannel = new CCChannel(st,m_pWorker);

	return TRUE;
}

//����cv������ ����������� �����������б�
void CCHelpCtrlH::DealwithLocalClientCmd()
{
	int nRet = 0;
	char buff[JVN_BAPACKDEFLEN] = {0};

	int num = m_TcpListCmd.size();
	for(CMD_LIST::iterator i = m_TcpListCmd.begin(); i != m_TcpListCmd.end(); ++ i)
	{
		LOCAL_TCP_CMD* p = i->second;
		if(!p)
			continue;

		nRet = CCChannel::tcpreceive(p->sTcpCmdSocket,buff,1,TCP_TIME_OUT);
		if(nRet == 1)
		{
			int rsize=0, rs=0;
			int nLen = -1;
			BYTE uchType = buff[0];
			//�������ݳ���
			switch(uchType)
			{
			case JVC_HELP_KEEP:
			case JVC_HELP_NEWYSTNO:
				{
					while (rsize < 4)
					{
						if(m_bExit)
						{
							return;
						}

					#ifndef WIN32
						if ((rs = recv(p->sTcpCmdSocket, (char *)&buff[rsize], 4 - rsize, MSG_NOSIGNAL)) < 0)
						{
							if(m_bExit)
							{
								//�����߳�
								if(p->sTcpCmdSocket > 0)
								{
									closesocket(p->sTcpCmdSocket);
								}
								p->sTcpCmdSocket = 0;
								return;
							}
			    			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
					#else
						if (SOCKET_ERROR == (rs = recv(p->sTcpCmdSocket, (char *)&buff[rsize], 4 - rsize, 0)))
						{
							if(m_bExit)
							{
								//�����߳�
								if(p->sTcpCmdSocket > 0)
								{
									closesocket(p->sTcpCmdSocket);
								}
								p->sTcpCmdSocket = 0;
								return;
							}

							int kkk=WSAGetLastError();
							if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
					#endif
							{
								jvc_sleep(1);
								continue;
							}

							if(p->sTcpCmdSocket > 0)
							{
								closesocket(p->sTcpCmdSocket);
							}
							p->sTcpCmdSocket = 0;

							return;
						}

						rsize += rs;
					}

					memcpy(&nLen, buff, 4);

					if(nLen < 0 || nLen > JVN_BAPACKDEFLEN)
					{
						continue;
					}
				}
				break;
			default:
				continue;
				break;
			}

			switch(uchType)
			{
			case JVC_HELP_KEEP://�յ� ����
				{
					if(nLen == 0)
					{
						p->dwRecvTime = GetMyTimeCount();
//						OutputDebug("�յ��ֿ�����");

						p->dwSendTime = GetMyTimeCount();
						//			OutputDebug("����״̬ %d",m_VListH.size());
						for(VCONN_List::iterator j = m_VListH.begin();j != m_VListH.end(); ++ j)
						{
							STVLINK pLink;// = {0};
							memcpy(&pLink,&j->second,sizeof(STVLINK));
							if(pLink.nConnectType == JVN_HELPRET_TURN)
							{
								pLink.nConnectType = JVN_HELPRET_NULL;//��������ת������Ȼ��Ӧ�ò㳢��ȫ�����ӣ����ֱ�����ᣬȱ����ת������
							}

							buff[0] = JVC_HELP_VSTATUS;
							int nl = sizeof(STVLINK);
							memcpy(&buff[1], &nl, 4);
							memcpy(&buff[5], &pLink, nl);
							if(CCChannel::tcpsend(i->second->sTcpCmdSocket,(char* )buff,nl + 5,TCP_TIME_OUT) <= 0)
							{
								if(i->second->sTcpCmdSocket > 0)
								{
									closesocket(i->second->sTcpCmdSocket);
								}
								i->second->sTcpCmdSocket = 0;
								return;
							}

//							OutputDebug("����״̬ %s%d %d  len = %d",pLink.chGroup,pLink.nYSTNO,pLink.nConnectType,nl + 5);
						}

					}
				}
				break;
			case JVC_HELP_NEWYSTNO://�յ��µ���������Ϣ
				{
					if(nLen == sizeof(STVLINK))
					{
						rsize=0;
						rs=0;
						while (rsize < nLen)
						{
							if(m_bExit)
							{
								return;
							}

						#ifndef WIN32
							if ((rs = recv(p->sTcpCmdSocket, (char *)&buff[rsize], nLen - rsize, MSG_NOSIGNAL)) < 0)
							{
								if(m_bExit)
								{
									//�����߳�
									if(p->sTcpCmdSocket > 0)
									{
										closesocket(p->sTcpCmdSocket);
									}
									p->sTcpCmdSocket = 0;
									return;
								}
			    				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
						#else
							if (SOCKET_ERROR == (rs = recv(p->sTcpCmdSocket, (char *)&buff[rsize], nLen - rsize, 0)))
							{
								if(m_bExit)
								{
									//�����߳�
									if(p->sTcpCmdSocket > 0)
									{
										closesocket(p->sTcpCmdSocket);
									}
									p->sTcpCmdSocket = 0;
									return;
								}

								int kkk=WSAGetLastError();
								if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
						#endif
								{
									jvc_sleep(1);
									continue;
								}

								if(p->sTcpCmdSocket > 0)
								{
									closesocket(p->sTcpCmdSocket);
								}
								p->sTcpCmdSocket = 0;

								return;
							}

							rsize += rs;
						}

						STVLINK info;// = {0};
						memcpy(&info,buff,sizeof(STVLINK));
						OutputDebug("�յ������� %s%d",info.chGroup,info.nYSTNO);

						STBASEYSTNO st = {0};
						strcpy(st.chGroup,info.chGroup);
						st.nYSTNO = info.nYSTNO;
						st.nChannel = info.nChannel;
						strcpy(st.chPName,info.chUserName);
						strcpy(st.chPWord,info.chPasswords);

						SetHelpYSTNO((BYTE *)&st, sizeof(STBASEYSTNO));//STBASEYSTNO
					}
				}
				break;
			default:
				continue;
				break;
			}
		}
		//�Ͽ� ��ʱ��û����Ϣ
		if(GetMyTimeCount() - p->dwRecvTime > 30000)//�Ͽ����� ��ʱ��û���յ�����
		{
			OutputDebug("�Ͽ� ");
			SOCKET s = p->sTcpCmdSocket;
			m_TcpListCmd.erase(s);
			delete p;
			break;
		}

	}
}

//������շֿط������� �������� ���͵�ǰ�����������б�
#ifndef WIN32
	void* CCHelpCtrlH::WorkCmdProc(void* pParam)
#else
	UINT CCHelpCtrlH::WorkCmdProc(LPVOID pParam)
#endif
{
	CCHelpCtrlH* pHelp = (CCHelpCtrlH* )pParam;

	//ά�ֱ���TCP���� �� Զ�� ��������
	while(TRUE)
	{
		if(pHelp->m_bExit)
		{
			break;
		}
#ifndef WIN32
		pthread_mutex_lock(&pHelp->m_criticalsectionVList);
#else
		EnterCriticalSection(&pHelp->m_criticalsectionVList);
#endif
		pHelp->DealwithLocalClientCmd();

		pHelp->VirtualConnect();
#ifndef WIN32
		pthread_mutex_unlock(&pHelp->m_criticalsectionVList);
#else
		LeaveCriticalSection(&pHelp->m_criticalsectionVList);
#endif
		jvc_sleep(10);
	}

#ifndef WIN32
	return NULL;
#else
	return 1;
#endif
}

#ifdef WIN32
	UINT CCHelpCtrlH::ListenProcCmd(LPVOID pParam)
#else
	void* CCHelpCtrlH::ListenProcCmd(void* pParam)
#endif
{
	CCHelpCtrlH* pHelpH = (CCHelpCtrlH* )pParam;

	SOCKADDR_IN clientaddr;
	int clientaddrlen = sizeof(SOCKADDR_IN);
	//������9001������
	OutputDebug("cmd start ");
	while(TRUE)
	{
		if(pHelpH->m_bExit)
		{
			break;
		}

		SOCKET client;
	#ifdef WIN32
		if ((client = accept(pHelpH->m_ListenSockCmd, (sockaddr*)&clientaddr, &clientaddrlen)) == INVALID_SOCKET)
	#else
		if ((client = accept(pHelpH->m_ListenSockCmd, (sockaddr*)&clientaddr, (socklen_t*)&clientaddrlen)) <= 0)
	#endif
		{
			//������
			jvc_sleep(1);
			continue;
		}

		//////////////////////////////////////////////////////////////////////////
		int iSockStatus = 0;
		//���׽�����Ϊ������ģʽ
	#ifndef WIN32
		int flags;
		if ((flags = fcntl(client, F_GETFL, 0)) < 0)
		{
			closesocket(client);
			client = 0;
			continue;
		}

		if (fcntl(client, F_SETFL, flags | O_NONBLOCK) < 0)
	#else
		unsigned long ulBlock = 1;
		iSockStatus = ioctlsocket(client, FIONBIO, (unsigned long*)&ulBlock);
		if (SOCKET_ERROR == iSockStatus)
	#endif
		{
			closesocket(client);
			client = 0;
			continue;
		}

		//���׽�����Ϊ���ȴ�δ�����������
		LINGER linger;
		linger.l_onoff = 1;//0;
		linger.l_linger = 0;
		iSockStatus = setsockopt(client, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
	#ifndef WIN32
		if (iSockStatus < 0)
	#else
		if (SOCKET_ERROR == iSockStatus)
	#endif
		{
			closesocket(client);
			client = 0;
			continue;
		}
		//////////////////////////////////////////////////////////////////////////

		LOCAL_TCP_CMD *info = new LOCAL_TCP_CMD;
		memset(info,0,sizeof(LOCAL_TCP_CMD));
		info->sTcpCmdSocket = client;
		info->pCtrl = pHelpH;

		//ֻ��ӵ��б��м���
		info->dwRecvTime = GetMyTimeCount();
		pHelpH->m_TcpListCmd[info->sTcpCmdSocket] = info;
	}

#ifdef WIN32
	return 1;
#else
	return NULL;
#endif
}

//����cv���� ���͵�����
#ifdef WIN32
	UINT CCHelpCtrlH::WorkDataProc(LPVOID pParam)
#else
	void* CCHelpCtrlH::WorkDataProc(void* pParam)
#endif
{
	LOCAL_TCP_DATA* pTcp = (LOCAL_TCP_DATA* )pParam;
	CCHelpCtrlH* pHelpH = (CCHelpCtrlH*)pTcp->pCtrl;

	DWORD dwLast = GetMyTimeCount();
	DWORD dwCurr = GetMyTimeCount();

	//����cv���� ���͵�����
	int nRet = 0;
	char buff[JVN_BAPACKDEFLEN] = {0};
	pTcp->dwSendTime = GetMyTimeCount();
	pTcp->dwRecvTime = GetMyTimeCount();

	DWORD dwSendPartnerTime = 0;
	while(TRUE)
	{
		if(pHelpH->m_bExit)
		{
			break;
		}
		dwCurr = GetMyTimeCount();
		if((dwCurr - pTcp->dwRecvTime) > 30000)
		{
			OutputDebug("��ʱ�Ͽ�.CCHelpCtrlH::WorkDataProc.....");
			break;
		}

		if((dwCurr - dwSendPartnerTime) > 6000 && pTcp->pChannel && pTcp->pChannel->m_bJVP2P && pTcp->pChannel->m_pchPartnerInfo != NULL)
		{
			pTcp->pChannel->m_nPartnerLen = JVNC_PTINFO_LEN;
			pTcp->pChannel->GetPartnerInfo(&pTcp->pChannel->m_pchPartnerInfo[10],pTcp->pChannel->m_nPartnerLen);


			dwSendPartnerTime = dwCurr;
		}

		//////////////////////////////////////////////////////////////////////////
		nRet = CCChannel::tcpreceive(pTcp->sTcpDataSocket,buff,1,TCP_TIME_OUT);
		if(nRet == 1)
		{
			pTcp->dwRecvTime = GetMyTimeCount();

			int rsize=0, rs=0;
			int nLen = -1;
			BYTE uchType = buff[0];
			//�������ݳ���
			switch(uchType)
			{
			case JVC_HELP_KEEP:
			case JVC_HELP_CONNECT:
			case JVC_HELP_DISCONN:
			case JVC_HELP_CVDATA:
				{
					while (rsize < 4)
					{
						if(pHelpH->m_bExit)
						{
							break;
						}

					#ifndef WIN32
						if ((rs = recv(pTcp->sTcpDataSocket, (char *)&buff[rsize], 4 - rsize, MSG_NOSIGNAL)) < 0)
						{
							if(pHelpH->m_bExit)
							{
								//�����߳�
								break;
							}
					    	if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
					#else
						if (SOCKET_ERROR == (rs = recv(pTcp->sTcpDataSocket, (char *)&buff[rsize], 4 - rsize, 0)))
						{
							if(pHelpH->m_bExit)
							{
								break;
							}

							int kkk=WSAGetLastError();
							if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
					#endif
							{
								jvc_sleep(1);
								continue;
							}

							break;
						}

						rsize += rs;
					}

					memcpy(&nLen, buff, 4);

					if(nLen < 0 || nLen > JVN_BAPACKDEFLEN)
					{
						continue;
					}
				}
				break;
			default:
				continue;
				break;
			}

			switch(uchType)
			{
			case JVC_HELP_CONNECT:
				{
					if(nLen == sizeof(STCONNECTINFO) - 8)//Ϊ������ǰ�汾����ӵ�8�ֽڲ���Ҫͨ��
					{
						rsize=0;
						rs=0;
						while (rsize < nLen)
						{
							if(pHelpH->m_bExit)
							{
								break;
							}

						#ifndef WIN32
							if ((rs = recv(pTcp->sTcpDataSocket, (char *)&buff[rsize], nLen - rsize, MSG_NOSIGNAL)) < 0)
							{
								if(pHelpH->m_bExit)
								{
									break;
								}
				  				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
						#else
							if (SOCKET_ERROR == (rs = recv(pTcp->sTcpDataSocket, (char *)&buff[rsize], nLen - rsize, 0)))
							{
								if(pHelpH->m_bExit)
								{
								//�����߳�
									break;
								}

								int kkk=WSAGetLastError();
								if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
						#endif
								{
									jvc_sleep(1);
									continue;
								}

								break;
							}

							rsize += rs;
						}
						STCONNECTINFO st;
						memset(&st,0,sizeof(STCONNECTINFO));
						memcpy(&st,buff,sizeof(STCONNECTINFO));

						pHelpH->ConnectYST(st,pTcp);
					}
				}
				break;
			case JVC_HELP_DISCONN:
				{
					if(nLen == 0)
					{
						pTcp->pChannel->SendData(JVN_CMD_DISCONN,NULL,0);
						OutputDebug("�յ�cv�Ͽ�����");
						break;
					}
				}
				break;
			case JVC_HELP_KEEP:
				{
//					OutputDebug("�յ�cv����");
					if(pTcp->pChannel && pTcp->pChannel->m_bJVP2P && pTcp->pChannel->m_pchPartnerInfo != NULL)
					{
						BYTE uchType = JVC_HELP_KEEP;//������ֿط��ͻ����Ϣ
						memcpy(pTcp->pChannel->m_pchPartnerInfo,&uchType,1);
						int nDataLen = pTcp->pChannel->m_nPartnerLen + 5;
						memcpy(&pTcp->pChannel->m_pchPartnerInfo[1],&nDataLen,4);
						memcpy(&pTcp->pChannel->m_pchPartnerInfo[5],&uchType,1);
						memcpy(&pTcp->pChannel->m_pchPartnerInfo[6],&pTcp->pChannel->m_nPartnerLen,4);

						CCChannel::tcpsend(pTcp->sTcpDataSocket,pTcp->pChannel->m_pchPartnerInfo,pTcp->pChannel->m_nPartnerLen + 10,TCP_TIME_OUT);
					}
					else
					{
						BYTE uchType = JVC_HELP_KEEP;//������ֿط�������
						char buff[10] = {0};

						memcpy(buff,&uchType,1);
						int nDataLen = -1;
						memcpy(&buff[1],&nDataLen,4);

						CCChannel::tcpsend(pTcp->sTcpDataSocket,buff,5,TCP_TIME_OUT);

					}
				}
				break;
			case JVC_HELP_CVDATA:
				{
					if(nLen > 4 && nLen < JVN_BAPACKDEFLEN)
					{
						rsize=0;
						rs=0;
						while (rsize < nLen)
						{
							if(pHelpH->m_bExit)
							{
								break;
							}

						#ifndef WIN32
							if ((rs = recv(pTcp->sTcpDataSocket, (char *)&buff[rsize], nLen - rsize, MSG_NOSIGNAL)) < 0)
							{
								if(pHelpH->m_bExit)
								{
									//�����߳�
									break;
								}
				   				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
						#else
							if (SOCKET_ERROR == (rs = recv(pTcp->sTcpDataSocket, (char *)&buff[rsize], nLen - rsize, 0)))
							{
								if(pHelpH->m_bExit)
								{
									//�����߳�
									break;
								}

								int kkk=WSAGetLastError();
								if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
						#endif
								{
									jvc_sleep(1);
									continue;
								}

								break;
							}

							rsize += rs;
						}

						//���͵�ָ��������
						pTcp->pChannel->SendData(buff[0], (BYTE*)&buff[5],nLen-5);
					}
				}
				break;
			default:
				continue;
				break;
			}
		}
		//////////////////////////////////////////////////////////////////////////

		jvc_sleep(1);
	}
	closesocket(pTcp->sTcpDataSocket);

#ifndef WIN32
	pthread_mutex_lock(&pHelpH->m_criticalsectionHList);
#else
	EnterCriticalSection(&pHelpH->m_criticalsectionHList);
#endif
	if(pTcp->pChannel)
	{
		pHelpH->m_TcpListDataH[pTcp->sTcpDataSocket] = NULL;
		pTcp->pChannel->DisConnect();
		delete pTcp->pChannel;
	}
	delete pTcp;

#ifndef WIN32
	pthread_mutex_unlock(&pHelpH->m_criticalsectionHList);
#else
	LeaveCriticalSection(&pHelpH->m_criticalsectionHList);
#endif

#ifndef WIN32
	return NULL;
#else
	return 1;
#endif
}

//��ͻ��˷�������
int CCHelpCtrlH::SendDataToCV(SOCKET s,char* pBuff,int nLen)
{
	int n = CCChannel::tcpsend(s,pBuff,nLen,TCP_TIME_OUT);

	return n;
}

//�ӿͻ��˽�������
int CCHelpCtrlH::ReceiveDataFromCV(SOCKET s,char* pBuff,int nMaxLen)
{
	int nLen = CCChannel::tcpreceive(s,pBuff,nMaxLen,TCP_TIME_OUT);

	return nLen;
}


//channel�յ������������ݺ����  ���ظ�cv��
int CCHelpCtrlH::CallbackFromChannel(SOCKET s,char* pBuff,int nLen)
{
	int n = CCChannel::tcpsend(s,pBuff,nLen,TCP_TIME_OUT);

	return n;
}

#ifdef WIN32
	UINT CCHelpCtrlH::ListenProcData(LPVOID pParam)
#else
	void* CCHelpCtrlH::ListenProcData(void* pParam)
#endif
{
	CCHelpCtrlH* pHelpH = (CCHelpCtrlH* )pParam;

	SOCKADDR_IN clientaddr;
	int clientaddrlen = sizeof(SOCKADDR_IN);
	//������9001������
	OutputDebug("data  start ");
	while(TRUE)
	{
		if(pHelpH->m_bExit)
		{
			break;
		}
		SOCKET client;
	#ifdef WIN32
		if ((client = accept(pHelpH->m_ListenSockData, (sockaddr*)&clientaddr, &clientaddrlen)) == INVALID_SOCKET)
	#else
		if ((client = accept(pHelpH->m_ListenSockData, (sockaddr*)&clientaddr, (socklen_t*)&clientaddrlen)) <= 0)
	#endif
		{
			//������
			jvc_sleep(1);
			continue;
		}

		//////////////////////////////////////////////////////////////////////////
		int iSockStatus = 0;
		//���׽�����Ϊ������ģʽ
	#ifndef WIN32
		int flags;
		if ((flags = fcntl(client, F_GETFL, 0)) < 0)
		{
			closesocket(client);
			client = 0;
			continue;
		}

		if (fcntl(client, F_SETFL, flags | O_NONBLOCK) < 0)
	#else
		unsigned long ulBlock = 1;
		iSockStatus = ioctlsocket(client, FIONBIO, (unsigned long*)&ulBlock);
		if (SOCKET_ERROR == iSockStatus)
	#endif
		{
			closesocket(client);
			client = 0;
			continue;
		}

		//���׽�����Ϊ���ȴ�δ�����������
		LINGER linger;
		linger.l_onoff = 1;//0;
		linger.l_linger = 0;
		iSockStatus = setsockopt(client, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
	#ifndef WIN32
		if (iSockStatus < 0)
	#else
		if (SOCKET_ERROR == iSockStatus)
	#endif
		{
			closesocket(client);
			client = 0;
			continue;
		}
		//////////////////////////////////////////////////////////////////////////
		int nSetSize = 128 * 1024;
		setsockopt(client, SOL_SOCKET, SO_SNDBUF, (char *)&nSetSize, sizeof(int));
		nSetSize = 128 * 1024;
		setsockopt(client, SOL_SOCKET, SO_RCVBUF, (char *)&nSetSize, sizeof(int));

		LOCAL_TCP_DATA *pTcp = new LOCAL_TCP_DATA;
		memset(pTcp,0,sizeof(LOCAL_TCP_DATA));
		pTcp->pCtrl = pHelpH;
		pTcp->sTcpDataSocket = client;

#ifndef WIN32
		pthread_mutex_lock(&pHelpH->m_criticalsectionHList);
#else
		EnterCriticalSection(&pHelpH->m_criticalsectionHList);
#endif
		pHelpH->m_TcpListDataH[pTcp->sTcpDataSocket] = pTcp;
#ifndef WIN32
		pthread_mutex_unlock(&pHelpH->m_criticalsectionHList);
#else
		LeaveCriticalSection(&pHelpH->m_criticalsectionHList);
#endif

	#ifdef WIN32
		UINT unTheadID = 0;
		pHelpH->m_hWorkDataThread = (HANDLE)_beginthreadex(NULL, 0, WorkDataProc, (void *)pTcp, 0, &unTheadID);
		if (pHelpH->m_hWorkDataThread == 0)//�����߳�ʧ��
		{
			closesocket(client);
			client = 0;
			continue;
		}
	#else
		pthread_attr_t attr;
		pthread_attr_t *pAttr = &attr;
		unsigned long size = LINUX_THREAD_STACK_SIZE;
		size_t stacksize = size;
		pthread_attr_init(pAttr);
		if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
		{
			pAttr = NULL;
		}
		if (0 != pthread_create(&pHelpH->m_hWorkDataThread, pAttr, WorkDataProc, pTcp))
		{
			pHelpH->m_hWorkDataThread = 0;
			closesocket(client);
			client = 0;
			continue;
		}
	#endif
	}
#ifndef WIN32
	return NULL;
#else
	return 1;
#endif
}


void CCHelpCtrlH::VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg)//�����ض�ͨ��״̬����(����״̬)
{
	if(JVN_CCONNECTTYPE_CONNOK == uchType)
	{
		char yst[20] = {0};
		STVLINK pLink;// = {0};
		
		int i = 0;
		for(VCONN_List::iterator k = m_VListH.begin(); k != m_VListH.end(); ++ k,i ++)
		{			
			memcpy(&pLink,&k->second,sizeof(STVLINK));
			sprintf(yst,"%s%d",k->second.chGroup,k->second.nYSTNO);
			std::string s(yst);
			
			if(k->second.pVirtualChannel->m_stConnInfo.nLocalChannel == nLocalChannel)
			{
				if(pLink.pVirtualChannel->m_bTURN)
				{
					pLink.nConnectType = JVN_HELPRET_TURN;
//					OutputDebug("=========������%d %s  ת��   ok",i,yst);
				}
				else if(pLink.pVirtualChannel->m_bLocal)
				{
					memcpy(&pLink.addrVirtual,&pLink.pVirtualChannel->m_addressA,sizeof(SOCKADDR_IN));
					pLink.nConnectType = JVN_HELPRET_LOCAL;
//					OutputDebug("=========������%d %s  ����   ok  %s : %d  CostTime:%10d",i,yst,
//						inet_ntoa(pLink.addrVirtual.sin_addr),ntohs(pLink.addrVirtual.sin_port),JVGetTime() - pLink.pVirtualChannel->m_dwStartConnectTime);
				}
				else
				{
					memcpy(&pLink.addrVirtual,&pLink.pVirtualChannel->m_addressA,sizeof(SOCKADDR_IN));
					pLink.nConnectType = JVN_HELPRET_WAN;
//					OutputDebug("=========������%d %s  ����   ok %s : %d  CostTime:%10d",i,yst,
//						inet_ntoa(pLink.addrVirtual.sin_addr),ntohs(pLink.addrVirtual.sin_port),JVGetTime() - pLink.pVirtualChannel->m_dwStartConnectTime);
				}
				m_VListH[s] = pLink;
				return;
			}
		}
	}
	else
	{
//		OutputDebug("%s\n\n",pMsg);
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CCHelpCtrlP::CCHelpCtrlP()
{
	m_ConnectCmdSock = 0;

#ifdef WIN32
	m_hEndEventWH = 0;
#endif

	m_hCWHThread = 0;//�߳̾��
	m_hConnHelpThread = 0;

	StartWorkThread();
}

CCHelpCtrlP::CCHelpCtrlP(CCWorker *pWorker):CCHelpCtrl(pWorker)
{
	m_ConnectCmdSock = 0;

#ifdef WIN32
	m_hEndEventWH = 0;
#endif

	m_hCWHThread = 0;//�߳̾��
	m_hConnHelpThread = 0;

	m_nHelpType = JVN_WHO_P;
	StartWorkThread();
}

CCHelpCtrlP::~CCHelpCtrlP()
{
	m_bExit = TRUE;

#ifndef WIN32
	if (0 != m_hCWHThread)
	{
		pthread_join(m_hCWHThread, NULL);
		m_hCWHThread = 0;
	}
	if (0 != m_hConnHelpThread)
	{
		pthread_join(m_hConnHelpThread, NULL);
		m_hConnHelpThread = 0;
	}
#else
	if(m_hEndEventWH > 0)
	{
		SetEvent(m_hEndEventWH);
		jvc_sleep(10);

		CloseHandle(m_hEndEventWH);
		m_hEndEventWH = 0;
	}

	jvc_sleep(10);
	CCChannel::WaitThreadExit(m_hCWHThread);
	CCChannel::WaitThreadExit(m_hConnHelpThread);
	if(m_hCWHThread > 0)
	{
		CloseHandle(m_hCWHThread);
		m_hCWHThread = 0;
	}
	if(m_hConnHelpThread > 0)
	{
		CloseHandle(m_hConnHelpThread);
		m_hConnHelpThread = 0;
	}
#endif

	m_VListC.clear();
}

BOOL CCHelpCtrlP::StartWorkThread()
{
#ifdef WIN32
	//���������߳�
	UINT unTheadID;
	m_hConnHelpThread = (HANDLE)_beginthreadex(NULL, 0, ConnectHelpProc, (void *)this, 0, &unTheadID);
	if (m_hConnHelpThread == 0)//�����߳�ʧ��
	{
		return FALSE;
	}
#else
	pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
	if (0 != pthread_create(&m_hConnHelpThread, pAttr, ConnectHelpProc, this))
	{
		m_hConnHelpThread = 0;
		return FALSE;
	}
#endif

	return TRUE;
}

BOOL CCHelpCtrlP::ConnectHelp(char* pHelperName/* = "127.0.0.1"*/,int nHelpPort/* = 9000*/)
{
	m_ConnectCmdSock = socket(AF_INET, SOCK_STREAM, 0);

#ifdef WIN32
	if (m_ConnectCmdSock == INVALID_SOCKET)
	{
		OutputDebug("TCP����ʧ��.��ʼ��ʧ��(socket).%d",GetLastError());

		m_ConnectCmdSock = 0;
		return FALSE;
	}
#else
	if (m_ConnectCmdSock <= 0)
	{
		OutputDebug("TCP����ʧ��.��ʼ��ʧ��(socket).%d",errno);

		m_ConnectCmdSock = 0;
		return FALSE;
	}
#endif

	BOOL bReuse = TRUE;
	setsockopt(m_ConnectCmdSock, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));

	SOCKADDR_IN addr;
#ifdef WIN32
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif

	addr.sin_family = AF_INET;
	addr.sin_port = htons(0);
#ifdef WIN32
	if (bind(m_ConnectCmdSock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		OutputDebug("TCP ʧ��.��ʼ��ʧ��(bind).%d",GetLastError());

		shutdown(m_ConnectCmdSock,SD_BOTH);
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}
#else
	if (bind(m_ConnectCmdSock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		OutputDebug("TCP ʧ��.��ʼ��ʧ��(bind).%d",errno);

		shutdown(m_ConnectCmdSock,SD_BOTH);
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}
#endif


	//////////////////////////////////////////////////////////////////////////

	SOCKADDR_IN srv;
#ifdef WIN32
	srv.sin_addr.S_un.S_addr = inet_addr(pHelperName);
#else
	srv.sin_addr.s_addr = inet_addr(pHelperName);
#endif

	srv.sin_family = AF_INET;
	srv.sin_port = htons(nHelpPort);
	if(CCChannel::connectnb(m_ConnectCmdSock,(sockaddr *)&srv,sizeof(SOCKADDR_IN),3) != 0)
	{
	#ifdef WIN32
//		OutputDebug("TCP ����ʧ�� err %d",GetLastError());
	#else
//		OutputDebug("TCP ����ʧ�� err %d",errno);
	#endif

		shutdown(m_ConnectCmdSock,SD_BOTH);
       	closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////////
	int iSockStatus = 0;
	//���׽�����Ϊ������ģʽ
#ifndef WIN32
	int flags;
	if ((flags = fcntl(m_ConnectCmdSock, F_GETFL, 0)) < 0)
	{
		OutputDebug("TCP ʧ�� ioctlsocket err %d",errno);
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}

	if (fcntl(m_ConnectCmdSock, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		OutputDebug("TCP ʧ�� ioctlsocket err %d",errno);
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}
#else
	unsigned long ulBlock = 1;
	iSockStatus = ioctlsocket(m_ConnectCmdSock, FIONBIO, (unsigned long*)&ulBlock);
	if (SOCKET_ERROR == iSockStatus)
	{
		OutputDebug("TCP ʧ�� ioctlsocket err %d",GetLastError());
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}
#endif

	//���׽�����Ϊ���ȴ�δ�����������
	LINGER linger;
	linger.l_onoff = 1;//0;
	linger.l_linger = 0;
	iSockStatus = setsockopt(m_ConnectCmdSock, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(LINGER));
#ifndef WIN32
	if (iSockStatus < 0)
	{
		OutputDebug("TCP ʧ�� ioctlsocket err %d",errno);
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}
#else
	if (SOCKET_ERROR == iSockStatus)
	{
		OutputDebug("TCP ʧ�� ioctlsocket err %d",GetLastError());
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	//tcp���÷�����ʱ ������ ���ӳɹ�������

#ifdef WIN32
	//���������߳�
	UINT unTheadID;
	m_hEndEventWH = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hCWHThread = (HANDLE)_beginthreadex(NULL, 0, CommWithHelpProc, (void *)this, 0, &unTheadID);
	if (m_hCWHThread == 0)//�����߳�ʧ��
	{
		if(m_hEndEventWH > 0)
		{
			CloseHandle(m_hEndEventWH);
			m_hEndEventWH = 0;
		}

		OutputDebug("TCP ʧ�� ioctlsocket err %d",GetLastError());
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}
#else
	pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
	if (0 != pthread_create(&m_hCWHThread, pAttr, CommWithHelpProc, this))
	{
		m_hCWHThread = 0;
		OutputDebug("TCP ʧ�� ioctlsocket err %d",errno);
		closesocket(m_ConnectCmdSock);
		m_ConnectCmdSock = 0;
		return FALSE;
	}
#endif
//	OutputDebug("�������ֳɹ�");

	return TRUE;
}

#ifndef WIN32
	void* CCHelpCtrlP::ConnectHelpProc(void* pParam)
#else
	UINT CCHelpCtrlP::ConnectHelpProc(LPVOID pParam)
#endif
{
	CCHelpCtrlP* pHelp = (CCHelpCtrlP* )pParam;

	while(!pHelp->m_bExit)
	{
		if(pHelp->m_bExit)
		{
			break;
		}

		if(pHelp->m_ConnectCmdSock == 0)
		{
			pHelp->ConnectHelp("127.0.0.1",9000);
		}
		jvc_sleep(10);
	}

#ifndef WIN32
	return NULL;
#else
	return 1;
#endif
}

int CCHelpCtrlP::GetHelpYSTNO(BYTE *pBuffer, int &nSize)//STBASEYSTNO
{
	int nFixSize = sizeof(STBASEYSTNO);

	int nNum = m_VListC.size();
	if(nNum * nFixSize > nSize)
		return -1;

	int i = 0;
	for(VCONN_List::iterator k = m_VListC.begin(); k != m_VListC.end(); ++ k,i ++)
	{
		STBASEYSTNO info;
		memset(&info,0,sizeof(STBASEYSTNO));
		strcpy(info.chGroup,k->second.chGroup);
		info.nYSTNO = k->second.nYSTNO;
		strcpy(info.chPName,k->second.chUserName);
		strcpy(info.chPWord,k->second.chPasswords);
		info.nChannel = k->second.nChannel;
		info.nConnectStatus = k->second.nConnectType;

		memcpy(pBuffer + i * nFixSize,&info,nFixSize);
	}
	nSize = nNum * nFixSize;

	return 1;
}
//ͬ��help����������Ϣ
#ifdef WIN32
	UINT CCHelpCtrlP::CommWithHelpProc(LPVOID pParam)
#else
	void* CCHelpCtrlP::CommWithHelpProc(void* pParam)
#endif
{
	CCHelpCtrlP* pHelpC = (CCHelpCtrlP* )pParam;

	int nRet = 0;
	char buff[JVN_BAPACKDEFLEN] = {0};

	DWORD dwLastRecv = GetMyTimeCount();
	DWORD dwLastSend = GetMyTimeCount();
	DWORD dwCurr = GetMyTimeCount();;

	int nFixSize = sizeof(STVLINK);

	STVLINK info;// = {0};
	char yst[20] = {0};
	dwLastSend = dwCurr - 10;//

	while(TRUE)
	{
	#ifdef WIN32
		if(pHelpC->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pHelpC->m_hEndEventWH, 0))
		{
			break;
		}
	#else
		if(pHelpC->m_bExit)
		{
			break;
		}
	#endif

		dwCurr = GetMyTimeCount();;

		if(dwCurr - dwLastRecv > 30000)
		{
			break;
		}

		//10�뷢�������� ����Ҫ����
		if(dwCurr - dwLastSend > 2000)
		{
			dwLastSend = GetMyTimeCount();;
			BYTE uchdata[10]={0};
			uchdata[0] = JVC_HELP_KEEP;
			CCChannel::tcpsend(pHelpC->m_ConnectCmdSock,(char* )uchdata,5,TCP_TIME_OUT);

//			OutputDebug("cv��������");
		}

		//////////////////////////////////////////////////////////////////////////
		nRet = CCChannel::tcpreceive(pHelpC->m_ConnectCmdSock,buff,1,TCP_TIME_OUT);
		if(nRet == 1)
		{
			int rsize=0, rs=0;
			int nLen = -1;
			BYTE uchType = buff[0];
			//�������ݳ���
			switch(uchType)
			{
			case JVC_HELP_VSTATUS:
				{
					while (rsize < 4)
					{
						if(pHelpC->m_bExit)
						{
							return 0;
						}

					#ifndef WIN32
						if ((rs = recv(pHelpC->m_ConnectCmdSock, (char *)&buff[rsize], 4 - rsize, MSG_NOSIGNAL)) < 0)
						{
							if(pHelpC->m_bExit)
							{
								//�����߳�
								if(pHelpC->m_ConnectCmdSock > 0)
								{
									closesocket(pHelpC->m_ConnectCmdSock);
								}
								pHelpC->m_ConnectCmdSock = 0;
								return 0;
							}
			    			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
					#else
						if (SOCKET_ERROR == (rs = recv(pHelpC->m_ConnectCmdSock, (char *)&buff[rsize], 4 - rsize, 0)))
						{
							if(pHelpC->m_bExit)
							{
								//�����߳�
								if(pHelpC->m_ConnectCmdSock > 0)
								{
									closesocket(pHelpC->m_ConnectCmdSock);
								}
								pHelpC->m_ConnectCmdSock = 0;
								return 0;
							}

							int kkk=WSAGetLastError();
							if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
					#endif
							{
								jvc_sleep(1);
								continue;
							}

							if(pHelpC->m_ConnectCmdSock > 0)
							{
								closesocket(pHelpC->m_ConnectCmdSock);
							}
							pHelpC->m_ConnectCmdSock = 0;

							return 0;
						}

						rsize += rs;
					}

					memcpy(&nLen, buff, 4);

					if(nLen < 0 || nLen > JVN_BAPACKDEFLEN)
					{
						continue;
					}
				}
				break;
			default:
				continue;
				break;
			}

			switch(uchType)
			{
			case JVC_HELP_VSTATUS://�յ���������Ϣ
				{
					if(nLen == sizeof(STVLINK))
					{
						rsize=0;
						rs=0;
						while (rsize < nLen)
						{
							if(pHelpC->m_bExit)
							{
								return 0;
							}

						#ifndef WIN32
							if ((rs = recv(pHelpC->m_ConnectCmdSock, (char *)&buff[rsize], nLen - rsize, MSG_NOSIGNAL)) < 0)
							{
								if(pHelpC->m_bExit)
								{
									//�����߳�
									if(pHelpC->m_ConnectCmdSock > 0)
									{
										closesocket(pHelpC->m_ConnectCmdSock);
									}
									pHelpC->m_ConnectCmdSock = 0;
									return 0;
								}
			  				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
						#else
							if (SOCKET_ERROR == (rs = recv(pHelpC->m_ConnectCmdSock, (char *)&buff[rsize], nLen - rsize, 0)))
							{
								if(pHelpC->m_bExit)
								{
									//�����߳�
									if(pHelpC->m_ConnectCmdSock > 0)
									{
										closesocket(pHelpC->m_ConnectCmdSock);
									}
									pHelpC->m_ConnectCmdSock = 0;
									return 0;
								}

								int kkk=WSAGetLastError();
								if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK)
						#endif
								{
									jvc_sleep(1);
									continue;
								}

								if(pHelpC->m_ConnectCmdSock > 0)
								{
									closesocket(pHelpC->m_ConnectCmdSock);
								}
								pHelpC->m_ConnectCmdSock = 0;

								return 0;
							}

							rsize += rs;
						}

						dwLastRecv = GetMyTimeCount();;
						memcpy(&info,buff,nFixSize);

						sprintf(yst,"%s%d",info.chGroup,info.nYSTNO);
//						OutputDebug("cv�յ�״̬ %s  %s",yst,g_strConnectType[info.nConnectType]);

						pHelpC->m_VListC[std::string(yst)] = info;
					}
				}
				break;
			default:
				continue;
				break;
			}
		}
		//////////////////////////////////////////////////////////////////////////

		jvc_sleep(1);
	}

	//�Ͽ������ֵ����Ӻ� ��������Ч
	pHelpC->m_VListC.clear();

	closesocket(pHelpC->m_ConnectCmdSock);
	pHelpC->m_ConnectCmdSock = 0;

#ifdef WIN32
	if(pHelpC->m_hEndEventWH > 0)
	{
		SetEvent(pHelpC->m_hEndEventWH);
		jvc_sleep(10);

		CloseHandle(pHelpC->m_hEndEventWH);
		pHelpC->m_hEndEventWH = 0;
	}
#endif
//	OutputDebug("�����ֶϿ�����");

#ifndef WIN32
	return NULL;
#else
	return 1;
#endif
}

//�ڱ��� ��ѯ�������Ƿ����
int CCHelpCtrlP::SearchYSTNO(STVLINK *stVLink)
{
	if(m_ConnectCmdSock == 0)
		return JVN_HELPRET_NULL;

	char yst[20] = {0};
	sprintf(yst,"%s%d",stVLink->chGroup,stVLink->nYSTNO);
	VCONN_List::iterator i = m_VListC.find(std::string(yst));
	OutputDebug("��ѯ���� %s",yst);

	//�ҵ�����״̬
	if(i != m_VListC.end())
	{
		memcpy(&stVLink->addrVirtual, &i->second.addrVirtual, sizeof(SOCKADDR_IN));

		OutputDebug("��ѯ����  %d IP = %s Port = %d",i->second.nConnectType,inet_ntoa(stVLink->addrVirtual.sin_addr),ntohs(stVLink->addrVirtual.sin_port));
		return i->second.nConnectType;
	}

	STVLINK info;// = {0};

	strcpy(info.chGroup,stVLink->chGroup);
	info.nYSTNO = stVLink->nYSTNO;
	info.nChannel = stVLink->nChannel;
	strcpy(info.chUserName,stVLink->chUserName);
	strcpy(info.chPasswords,stVLink->chPasswords);

	BYTE uchdata[JVN_BAPACKDEFLEN]={0};
	uchdata[0]=JVC_HELP_NEWYSTNO;
	int nlen = sizeof(STVLINK);
	memcpy(&uchdata[1], &nlen, 4);
	memcpy(&uchdata[5], &info, sizeof(STVLINK));
	CCChannel::tcpsend(m_ConnectCmdSock,(char* )&uchdata,sizeof(STVLINK)+5,TCP_TIME_OUT);

	return JVN_HELPRET_NULL;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CCHelpConnCtrl::CCHelpConnCtrl()
{
}

CCHelpConnCtrl::~CCHelpConnCtrl()
{

}

BOOL CCHelpConnCtrl::ConnectYSTNO(STCONNECTINFO stConnInfo)
{
	OutputDebug("cv��ʼ��������   ......");
	//��channel���� ����֮ǰ��ȷ��һ��channelֻ��һ��������
	m_tcpsock = socket(AF_INET, SOCK_STREAM, 0);

#ifdef WIN32
	if (m_tcpsock == INVALID_SOCKET)
	{
		OutputDebug("TCP����ʧ��.��ʼ��ʧ��(socket).%d",GetLastError());

		m_tcpsock = 0;
		return FALSE;
	}
#else
	if (m_tcpsock <= 0)
	{
		OutputDebug("TCP����ʧ��.��ʼ��ʧ��(socket).%d",errno);

		m_tcpsock = 0;
		return FALSE;
	}
#endif

	SOCKADDR_IN srv;
#ifdef WIN32
	srv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
	srv.sin_addr.s_addr = inet_addr("127.0.0.1");
#endif

	srv.sin_family = AF_INET;
	srv.sin_port = htons(8000);//���ӱ���8000�˿�

	int nSetSize = 128 * 1024;
	setsockopt(m_tcpsock, SOL_SOCKET, SO_SNDBUF, (char *)&nSetSize, sizeof(int));
	nSetSize = 128 * 1024;
	setsockopt(m_tcpsock, SOL_SOCKET, SO_RCVBUF, (char *)&nSetSize, sizeof(int));

	if(CCChannel::connectnb(m_tcpsock,(sockaddr *)&srv,sizeof(SOCKADDR_IN),1) != 0)
	{
	#ifdef WIN32
//		OutputDebug("TCP ����ʧ�� err %d",GetLastError());
	#else
//		OutputDebug("TCP ����ʧ�� err %d",errno);
	#endif

		shutdown(m_tcpsock,SD_BOTH);
       	closesocket(m_tcpsock);
		return FALSE;
	}
	OutputDebug("cv�������ֳɹ�");

	//�����ַ���������������
	char buff[1024] = {0};
	buff[0] = JVC_HELP_CONNECT;
	int nlen = sizeof(STCONNECTINFO) - 8;//Ϊ������ǰ�汾,����ӵ�8�ֽڲ���Ҫͨ��
	memcpy(&buff[1], &nlen, 4);
	memcpy(&buff[5], &stConnInfo, nlen);

	//����	����	����ͨ��	����	����ͨ Զ��ͨ��
	CCChannel::tcpsend(m_tcpsock,buff,sizeof(STCONNECTINFO) + 5,TCP_TIME_OUT);

	return TRUE;
}

int CCHelpConnCtrl::RecvConnResult(STCONNBACK *stconnback)
{
	//���ӿ�ʼ�� �������ӽ��
	char buff[1024] = {0};
	int nLen = CCChannel::tcpreceive2(m_tcpsock,buff,14,TCP_TIME_OUT);

	if(nLen == 14 && buff[0] == JVC_HELP_CONNSTATUS)
	{
		stconnback->nRet = buff[5];

		memcpy(&stconnback->nSize,&buff[10],4);
		if(stconnback->nSize > 0)
		{
			CCChannel::tcpreceive(m_tcpsock,(char* )stconnback->pMsg,stconnback->nSize,TCP_TIME_OUT);
		}
		if(stconnback->nRet == 1)
		{
			OutputDebug("��������ʵ������ �ɹ�");
			return 1;
		}
		OutputDebug("��������ʧ��");
		return 2;
	}

	return 0;
}


//�����ַ������� ��cv��client��channel����
int CCHelpConnCtrl::SendToHelp(BYTE* pBuff,int nLen)
{
	int n = CCChannel::tcpsend(m_tcpsock,(char* )pBuff,nLen,TCP_TIME_OUT);
#ifdef WIN32
	int d = GetLastError();
	if(d == 10054)
		n = -1;
#endif
	return n;
}

int CCHelpConnCtrl::SendToHelpActive()
{
	char buff[1024] = {0};
	buff[0] = JVC_HELP_KEEP;
	int nlen = 0;
	memcpy(&buff[1], &nlen, 4);

	int n = CCChannel::tcpsend(m_tcpsock,buff,5,TCP_TIME_OUT);

	return n;

}

int CCHelpConnCtrl::RecvFromHelp(BYTE* pBuff,int nMaxLen)
{
	int nLen = CCChannel::tcpreceive(m_tcpsock,(char* )pBuff,nMaxLen,TCP_TIME_OUT);
	//ԭʼ���ݷ��� �ڷ��غ����

#ifdef WIN32
	int kkk = WSAGetLastError();
	if(kkk != 0 && kkk != WSAEINTR && kkk != WSAEWOULDBLOCK)
	{
		return -1;
	}
#else
	if(errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
	{
		return -1;
	}
#endif

	return nLen;
}

void CCHelpConnCtrl::DisConnectYSTNO()
{
	OutputDebug("�Ͽ���������");
	char strBuff[20] = {0};
	strBuff[0] = JVC_HELP_DISCONN;
	int n = CCChannel::tcpsend(m_tcpsock,strBuff,5,TCP_TIME_OUT);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCHelpCtrlM::CCHelpCtrlM()
{
	m_nHelpType = JVN_WHO_M;
	m_nLocalChannelIndex = 1;
	m_hWorkCmdThread = 0;
}

CCHelpCtrlM::CCHelpCtrlM(CCWorker *pWorker):CCHelpCtrl(pWorker)
{
    m_bExitSignal = FALSE;
	m_nHelpType = JVN_WHO_M;
	m_nLocalChannelIndex = 1;

	m_hWorkCmdThread = 0;
#ifndef WIN32
	pthread_mutex_init(&m_VListMLock, NULL);
#else
	m_VListMLock = CreateMutex(NULL, false, NULL);
#endif
	
	StartWorkThread();
}

CCHelpCtrlM::~CCHelpCtrlM()
{
	m_bExit = TRUE;

#ifndef WIN32
	if (0 != m_hWorkCmdThread)
	{
		pthread_join(m_hWorkCmdThread, NULL);
		m_hWorkCmdThread = 0;
	}
#else
	jvc_sleep(10);
	CCChannel::WaitThreadExit(m_hWorkCmdThread);
	if(m_hWorkCmdThread > 0)
	{
		CloseHandle(m_hWorkCmdThread);
		m_hWorkCmdThread = 0;
	}
#endif

	{

		CLocker lock(m_VListMLock);
		for(VCONN_List::iterator k = m_VListM.begin(); k != m_VListM.end(); ++ k)
		{
			k->second.pVirtualChannel->DisConnect();
			delete k->second.pVirtualChannel;
		}
		m_VListM.clear();
	}

#ifndef WIN32
	pthread_mutex_destroy(&m_VListMLock);
#else
	CloseHandle(m_VListMLock);
#endif

}

//��Ӻ���
BOOL CCHelpCtrlM::SetHelpYSTNO(BYTE *pBuffer, int nSize)//���ú����
{
	STBASEYSTNO info = {0};
	int num = nSize / sizeof(STBASEYSTNO);

	for(int j = 0;j < num;j ++)
	{
		memcpy(&info,&pBuffer[j*sizeof(STBASEYSTNO)],sizeof(STBASEYSTNO));
		if(info.nYSTNO > 0)
		{
			char yst[20] = {0};
			sprintf(yst,"%s%d",info.chGroup,info.nYSTNO);
			CLocker lock(m_VListMLock);

			VCONN_List ::iterator i = m_VListM.find(std::string(yst));

			//�����б����Ƿ��Ѿ����� ���������
			if(i == m_VListM.end())
			{
                if(!m_pWorker->YstIsDemo(info.chGroup,info.nYSTNO))
                {
                    STVLINK pLink;// = new STVLINK;
                    memset(&pLink,0,sizeof(STVLINK));
                    strcpy(pLink.chGroup,info.chGroup);
                    pLink.nYSTNO = info.nYSTNO;
                    pLink.bStatus = FALSE;
                    pLink.nChannel = info.nChannel;
                    strcpy(pLink.chUserName,info.chPName);
                    strcpy(pLink.chPasswords,info.chPWord);
                    
                    STCONNECTINFO stConnectInfo;
                    memset(&stConnectInfo,0,sizeof(STCONNECTINFO));
                    stConnectInfo.nWhoAmI = JVN_WHO_M;
                    stConnectInfo.bLocalTry = TRUE;
                    stConnectInfo.nTURNType = JVN_TRYTURN;
                    
                    stConnectInfo.bYST = TRUE;
                    strcpy(stConnectInfo.chGroup,pLink.chGroup);
                    stConnectInfo.nYSTNO = pLink.nYSTNO;
                    stConnectInfo.nChannel = pLink.nChannel;
                    strcpy(stConnectInfo.chPassName,pLink.chUserName);
                    strcpy(stConnectInfo.chPassWord,pLink.chPasswords);
                    
                    stConnectInfo.nLocalChannel = m_VListM.size() + 1;
                    
                    pLink.pVirtualChannel = new CCVirtualChannel(stConnectInfo,this,m_pWorker);
                    
                    //		pLink->vsock = UDT::socket(AF_INET, SOCK_STREAM, 0);
                    
                    std::string s(yst);
                    //m_VListH.insert(yst,pLink);
                    //				OutputDebug("%5d ���������%s",stConnectInfo.nLocalChannel,yst);
                    m_VListM.insert(std::map<std::string,STVLINK>::value_type(s, pLink));
                }
				
			}
			else//�Ѿ�����
			{
				STVLINK pLink;// = {0};
				memcpy(&pLink,&i->second,sizeof(STVLINK));
				if(pLink.nConnectType == JVN_HELPRET_NULL)
				{
					pLink.nChannel = info.nChannel;
					strcpy(pLink.chUserName,info.chPName);
					strcpy(pLink.chPasswords,info.chPWord);
					std::string s(yst);

					m_VListM[s] = pLink;
//					OutputDebug("����������%s",yst);
				}

			}
		}
	}
	return FALSE;

}

BOOL CCHelpCtrlM::StartWorkThread()
{
#ifndef WIN32
	pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
	if (0 != pthread_create(&m_hWorkCmdThread, pAttr, WorkCmdProc, this))
	{
		m_hWorkCmdThread = 0;
		return FALSE;
	}
#else
	//���������߳�
	UINT unTheadID;
	m_hWorkCmdThread = (HANDLE)_beginthreadex(NULL, 0, WorkCmdProc, (void *)this, 0, &unTheadID);
	if (m_hWorkCmdThread == 0)//�����߳�ʧ��
	{
		return FALSE;
	}
#endif

	return TRUE;
}

#ifdef WIN32
	UINT CCHelpCtrlM::WorkCmdProc(LPVOID pParam)
#else
	void*  CCHelpCtrlM::WorkCmdProc(void* pParam)//���������״̬
#endif
{
	CCHelpCtrlM* pHelpM = (CCHelpCtrlM* )pParam;
    pHelpM->m_bExitSignal = FALSE;
	while(TRUE)
	{
		if(pHelpM->m_bExit)
		{
			break;
		}

		pHelpM->VirtualConnect();

		pHelpM->VirtualGetConnect();

		jvc_sleep(10);
	}
    pHelpM->m_bExitSignal = TRUE;
#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}

void CCHelpCtrlM::VirtualConnect()
        {
            char yst[20] = {0};
            STVLINK pLink;// = {0};
            
            int i = 0;
            CLocker lock(m_VListMLock);

            for(VCONN_List::iterator k = m_VListM.begin(); k != m_VListM.end(); ++ k,i ++)
            {
                DWORD dwTime = JVGetTime();
                
                memcpy(&pLink,&k->second,sizeof(STVLINK));
                sprintf(yst,"%s%d",k->second.chGroup,k->second.nYSTNO);
                std::string s(yst);
                
                //if(k->second.pVirtualChannel->m_nConnectStatus == 2)//�Ͽ� ��������
                if(pLink.pVirtualChannel->m_nStatus == FAILD || (pLink.pVirtualChannel->m_nStatus == OK && pLink.pVirtualChannel->m_ServerSocket <= 0)
                   || (pLink.pVirtualChannel->m_nStatus == OK && pLink.pVirtualChannel->m_bTURN))
                {//����������Ҫ����������
                    if(pLink.pVirtualChannel->m_bTURN)
                    {
                        if(pLink.pVirtualChannel->m_nStatus == OK)
                        {//ת���ѶϿ�����ת��������
                            pLink.pVirtualChannel->DisConnect();
                            
                            pLink.nConnectType = JVN_HELPRET_TURN;
                            m_VListM[s] = pLink;//��ס��ת��
                        }
                        
                        if(dwTime > pLink.pVirtualChannel->m_dwAddTime + 30 * 1000)
                        {
                            if(dwTime < pLink.pVirtualChannel->m_dwConnectTime || dwTime > pLink.pVirtualChannel->m_dwConnectTime + 30000)//10000
                            {//�������ÿ��10������һ��
                                pLink.pVirtualChannel->DisConnect();
                                pLink.nConnectType = JVN_HELPRET_NULL;
                                m_VListM[s] = pLink;
                                
                                STCONNECTINFO stConnectInfo;
                                memset(&stConnectInfo,0,sizeof(STCONNECTINFO));
                                stConnectInfo.nWhoAmI = JVN_WHO_M;
                                stConnectInfo.bLocalTry = TRUE;
                                stConnectInfo.nTURNType = JVN_TRYTURN;
                                
                                stConnectInfo.bYST = TRUE;
                                strcpy(stConnectInfo.chGroup,pLink.chGroup);
                                stConnectInfo.nYSTNO = pLink.nYSTNO;
                                stConnectInfo.nChannel = pLink.nChannel;
                                strcpy(stConnectInfo.chPassName,pLink.chUserName);
                                strcpy(stConnectInfo.chPassWord,pLink.chPasswords);
                                
                                pLink.pVirtualChannel->ReConnect(stConnectInfo,this,m_pWorker);

                                OutputDebug("m����.....������ line: %d",__LINE__);
                            }
                            
                        }
                        else//��ʼ���ӵ�30����
                        {
                            if(dwTime < pLink.pVirtualChannel->m_dwConnectTime || dwTime > pLink.pVirtualChannel->m_dwConnectTime)//90000
                            {//ת��ÿ��1.5��������һ��
                                pLink.pVirtualChannel->DisConnect();
                                STCONNECTINFO stConnectInfo;
                                memset(&stConnectInfo,0,sizeof(STCONNECTINFO));
                                stConnectInfo.nWhoAmI = JVN_WHO_M;
                                stConnectInfo.bLocalTry = TRUE;
                                stConnectInfo.nTURNType = JVN_TRYTURN;
                                
                                stConnectInfo.bYST = TRUE;
                                strcpy(stConnectInfo.chGroup,pLink.chGroup);
                                stConnectInfo.nYSTNO = pLink.nYSTNO;
                                stConnectInfo.nChannel = pLink.nChannel;
                                strcpy(stConnectInfo.chPassName,pLink.chUserName);
                                strcpy(stConnectInfo.chPassWord,pLink.chPasswords);
                                
                                pLink.pVirtualChannel->ReConnect(stConnectInfo,this,m_pWorker);

                                OutputDebug("m����.....������ line: %d",__LINE__);

                                continue;
                            }
                        }
                        
                    }
                    else//δ���ӳɹ���
                    {
                        if(dwTime > pLink.pVirtualChannel->m_dwAddTime + 30 * 1000)
                        {
                            if(dwTime < pLink.pVirtualChannel->m_dwConnectTime || dwTime > pLink.pVirtualChannel->m_dwConnectTime + 30000)//10000
                            {//�������ÿ��10������һ��
                                pLink.pVirtualChannel->DisConnect();
                                pLink.nConnectType = JVN_HELPRET_NULL;
                                m_VListM[s] = pLink;
                                
                                STCONNECTINFO stConnectInfo;
                                memset(&stConnectInfo,0,sizeof(STCONNECTINFO)); 
                                stConnectInfo.nWhoAmI = JVN_WHO_M; 
                                stConnectInfo.bLocalTry = TRUE; 
                                stConnectInfo.nTURNType = JVN_TRYTURN; 
                                
                                stConnectInfo.bYST = TRUE; 
                                strcpy(stConnectInfo.chGroup,pLink.chGroup); 
                                stConnectInfo.nYSTNO = pLink.nYSTNO; 
                                stConnectInfo.nChannel = pLink.nChannel; 
                                strcpy(stConnectInfo.chPassName,pLink.chUserName); 
                                strcpy(stConnectInfo.chPassWord,pLink.chPasswords); 
                                
                                pLink.pVirtualChannel->ReConnect(stConnectInfo,this,m_pWorker); 
                                
                                OutputDebug("m����.....������ line: %d",__LINE__);
                            }
                            
                        } 
                        else//��ʼ���ӵ�30���� 
                        { 
                            if(dwTime < pLink.pVirtualChannel->m_dwConnectTime || dwTime > pLink.pVirtualChannel->m_dwConnectTime)//90000 
                            {//ת��ÿ��1.5��������һ�� 
                                pLink.pVirtualChannel->DisConnect(); 
                                STCONNECTINFO stConnectInfo; 
                                memset(&stConnectInfo,0,sizeof(STCONNECTINFO)); 
                                stConnectInfo.nWhoAmI = JVN_WHO_M; 
                                stConnectInfo.bLocalTry = TRUE; 
                                stConnectInfo.nTURNType = JVN_TRYTURN; 
                                
                                stConnectInfo.bYST = TRUE; 
                                strcpy(stConnectInfo.chGroup,pLink.chGroup); 
                                stConnectInfo.nYSTNO = pLink.nYSTNO; 
                                stConnectInfo.nChannel = pLink.nChannel; 
                                strcpy(stConnectInfo.chPassName,pLink.chUserName); 
                                strcpy(stConnectInfo.chPassWord,pLink.chPasswords); 
                                
                                pLink.pVirtualChannel->ReConnect(stConnectInfo,this,m_pWorker); 
                                
                                OutputDebug("m����.....������ line: %d",__LINE__);
                            } 
                        } 
                    } 
                    
                } 
                else if(pLink.pVirtualChannel->m_nStatus == OK) 
                { 
                    if(pLink.pVirtualChannel->RecvKeep()) 
                    { 
                        pLink.pVirtualChannel->m_dwRecvTime = JVGetTime(); 
                        m_VListM[s] = pLink; 
                    } 
                    if(dwTime > pLink.pVirtualChannel->m_dwRecvTime + 30000) 
                    { 
                        pLink.pVirtualChannel->DisConnect(); 
                        pLink.nConnectType = JVN_HELPRET_NULL; 
                        m_VListM[s] = pLink; 
                        //	OutputDebug("m������%d %10s �Ͽ�",i,yst); 
                    } 
                    if(dwTime - pLink.pVirtualChannel->m_dwSenTime > 5000) 
                    { 
                        pLink.pVirtualChannel->m_dwSenTime = dwTime; 
                        pLink.pVirtualChannel->SendKeep(); 
                        m_VListM[s] = pLink; 
                        
                        if(pLink.pVirtualChannel->m_bTURN) 
                        { 
                            OutputDebug("m������%d %10s ת�� ok",i,yst); 
                        }
                        else if(pLink.pVirtualChannel->m_bLocal) 
                        { 
                            OutputDebug("m������%d %10s ���� ok",i,yst); 
                        }
                        else 
                        { 
                            OutputDebug("m������%d %10s ���� ok socket = %d %s : %d %d",i,yst, 
                                        pLink.sRandSocket,inet_ntoa(pLink.addrVirtual.sin_addr),ntohs(pLink.addrVirtual.sin_port),
                                        (dwTime - pLink.pVirtualChannel->m_dwRecvTime));
                        }/**/
                    } 
                } 
                else 
                { 
                    //	if(pLink.pVirtualChannel && dwTime - pLink.pVirtualChannel->m_dwSenTime > 15000) 
                    //	{ 
                    //	pLink.pVirtualChannel->DisConnect(); 
                    //	pLink.nConnectType = JVN_HELPRET_NULL; 
                    //	m_VListM[s] = pLink; 
                    //	} 
                } 
            } 
        }

void CCHelpCtrlM::VirtualGetConnect()
{
	char yst[20] = {0};
	STVLINK pLink;// = {0};

	int i = 0;
	CLocker lock(m_VListMLock);

	for(VCONN_List::iterator k = m_VListM.begin(); k != m_VListM.end(); ++ k,i ++)
	{
		DWORD dwTime = JVGetTime();

		memcpy(&pLink,&k->second,sizeof(STVLINK));
		sprintf(yst,"%s%d",k->second.chGroup,k->second.nYSTNO);
		std::string s(yst);

		//if(k->second.pVirtualChannel->m_nConnectStatus == 2)//�Ͽ� ��������
		if(pLink.pVirtualChannel->m_nStatus == OK)
		{
			if(pLink.pVirtualChannel->m_bTURN)
			{
				pLink.nConnectType = JVN_HELPRET_TURN;
			}
			else if(pLink.pVirtualChannel->m_bLocal)
			{
				memcpy(&pLink.addrVirtual,&pLink.pVirtualChannel->m_addressA,sizeof(SOCKADDR_IN));
				pLink.nConnectType = JVN_HELPRET_LOCAL;
			}
			else
			{
				memcpy(&pLink.addrVirtual,&pLink.pVirtualChannel->m_addressA,sizeof(SOCKADDR_IN));
				pLink.nConnectType = JVN_HELPRET_LOCAL;
			}
			pLink.sRandSocket = pLink.pVirtualChannel->m_sSocket;
			m_VListM[s] = pLink;

		}
	}
}
void CCHelpCtrlM::ClearCache(void)
{
	CLocker lock(m_VListMLock);
	m_VListM.clear();
}
int CCHelpCtrlM::SearchYSTNO(STVLINK *stVLink)//??��?��????��????��??信�??
{
	char yst[20] = {0};
	sprintf(yst,"%s%d",stVLink->chGroup,stVLink->nYSTNO);
	CLocker lock(m_VListMLock);

	VCONN_List::iterator i = m_VListM.find(std::string(yst));
//	OutputDebug("��ѯ���� %s",yst);

	//�ҵ�����״̬
	if(i != m_VListM.end())
	{
		STVLINK pLink;// = {0};
		memcpy(&pLink,&i->second,sizeof(STVLINK));
		memcpy(&stVLink->addrVirtual, &pLink.addrVirtual, sizeof(SOCKADDR_IN));
		stVLink->sRandSocket = pLink.sRandSocket;

//		OutputDebug("��ѯ����  %d   %s : %d",pLink.nConnectType,inet_ntoa(stVLink->addrVirtual.sin_addr),ntohs(stVLink->addrVirtual.sin_port));
		return pLink.nConnectType;
	}
//	STBASEYSTNO info = {0};
//	strcpy(info.chGroup,stVLink->chGroup);
//	info.nYSTNO = stVLink->nYSTNO;
//	info.nChannel = stVLink->nChannel;
//	strcpy(info.chPName,stVLink->chUserName);
//	strcpy(info.chPWord,stVLink->chPasswords);

//	SetHelpYSTNO((BYTE* )&info,sizeof(STBASEYSTNO));

	STVLINK pLink;// = new STVLINK;
	memset(&pLink,0,sizeof(STVLINK));
	strcpy(pLink.chGroup,stVLink->chGroup);
	pLink.nYSTNO = stVLink->nYSTNO;
	pLink.bStatus = FALSE;
	pLink.nChannel = stVLink->nChannel;
	strcpy(pLink.chUserName,stVLink->chUserName);
	strcpy(pLink.chPasswords,stVLink->chPasswords);
	
	STCONNECTINFO stConnectInfo;
	memset(&stConnectInfo,0,sizeof(STCONNECTINFO));
	stConnectInfo.nWhoAmI = JVN_WHO_M;
	stConnectInfo.bLocalTry = TRUE;
	stConnectInfo.nTURNType = JVN_TRYTURN;
	
	stConnectInfo.bYST = TRUE;
	strcpy(stConnectInfo.chGroup,pLink.chGroup);
	stConnectInfo.nYSTNO = pLink.nYSTNO;
	stConnectInfo.nChannel = pLink.nChannel;
	strcpy(stConnectInfo.chPassName,pLink.chUserName);
	strcpy(stConnectInfo.chPassWord,pLink.chPasswords);
	
	stConnectInfo.nLocalChannel = m_VListM.size() + 1;
	
	pLink.pVirtualChannel = new CCVirtualChannel(stConnectInfo,this,m_pWorker);
	
	//		pLink->vsock = UDT::socket(AF_INET, SOCK_STREAM, 0);
	
	std::string s(yst);
	//m_VListH.insert(yst,pLink);
//	OutputDebug("%5d ���������%s",stConnectInfo.nLocalChannel,yst);
	m_VListM.insert(std::map<std::string,STVLINK>::value_type(s, pLink));
	return JVN_HELPRET_NULL;
}


//��ѯ����ͨ�б�״̬ ��������
int CCHelpCtrlM::GetHelpYSTNO(BYTE *pBuffer, int &nSize)
{
	int nFixSize = sizeof(STBASEYSTNO);

	int nNum = m_VListM.size();
	if(nNum * nFixSize > nSize)
		return -1;

	CLocker lock(m_VListMLock);
	int i = 0;
	for(VCONN_List::iterator k = m_VListM.begin(); k != m_VListM.end(); ++ k,i ++)
	{
		STBASEYSTNO info;
		memset(&info,0,sizeof(STBASEYSTNO));
		strcpy(info.chGroup,k->second.chGroup);
		info.nYSTNO = k->second.nYSTNO;
		strcpy(info.chPName,k->second.chUserName);
		strcpy(info.chPWord,k->second.chPasswords);
		info.nChannel = k->second.nChannel;
		info.nConnectStatus = k->second.nConnectType;

		memcpy(pBuffer + i * nFixSize,&info,nFixSize);
	}
	nSize = nNum * nFixSize;

	return 1;
}

BOOL CCHelpCtrlM::AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort)
{
	CLocker lock(m_VListMLock);
	char yst[20] = {0};
	sprintf(yst,"%s%d",pGroup,nYST);
	VCONN_List::iterator i = m_VListM.find(std::string(yst));

	if(i != m_VListM.end())
	{
		STVLINK pLink;// = {0};
		memcpy(&pLink,&i->second,sizeof(STVLINK));

		pLink.pVirtualChannel->AddRemoteConnect(s,pIP,nPort);

		return TRUE;
	}

	return FALSE;
}

void CCHelpCtrlM::VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg)//�����ض�ͨ��״̬����(����״̬)
{
    return;
	if(JVN_CCONNECTTYPE_CONNOK == uchType)
	{
		char yst[20] = {0};
		STVLINK pLink;// = {0};
		
		CLocker lock(m_VListMLock);
		int i = 0;
		for(VCONN_List::iterator k = m_VListM.begin(); k != m_VListM.end(); ++ k,i ++)
		{			
			memcpy(&pLink,&k->second,sizeof(STVLINK));
			sprintf(yst,"%s%d",k->second.chGroup,k->second.nYSTNO);
			std::string s(yst);

			if(k->second.pVirtualChannel->m_stConnInfo.nLocalChannel == nLocalChannel)
			{
				if(pLink.pVirtualChannel->m_bTURN)
				{
					pLink.nConnectType = JVN_HELPRET_TURN;
					OutputDebug("=========m������%d %s  ת��   ok",i,yst);
				}
				else if(pLink.pVirtualChannel->m_bLocal)
				{
					memcpy(&pLink.addrVirtual,&pLink.pVirtualChannel->m_addressA,sizeof(SOCKADDR_IN));
					pLink.nConnectType = JVN_HELPRET_LOCAL;
					OutputDebug("=========m������%d %s  ����   ok  %s : %d  CostTime:%10d",i,yst,
						inet_ntoa(pLink.addrVirtual.sin_addr),ntohs(pLink.addrVirtual.sin_port),JVGetTime() - pLink.pVirtualChannel->m_dwStartConnectTime);
				}
				else
				{
					memcpy(&pLink.addrVirtual,&pLink.pVirtualChannel->m_addressA,sizeof(SOCKADDR_IN));
					pLink.nConnectType = JVN_HELPRET_LOCAL;
					OutputDebug("=========m������%d %s  ����   ok %s : %d  CostTime:%10d",i,yst,
						inet_ntoa(pLink.addrVirtual.sin_addr),ntohs(pLink.addrVirtual.sin_port),JVGetTime() - pLink.pVirtualChannel->m_dwStartConnectTime);
				}
				pLink.sRandSocket = pLink.pVirtualChannel->m_sSocket;
				m_VListM[s] = pLink;
				return;
			}
		}
	}
	else
	{
//		OutputDebug("%s\n\n",pMsg);
	}
}


void CCHelpCtrlM::HelpRemove(char* pGroup,int nYST)
{
    CLocker lock(m_VListMLock);
    char yst[20] = {0};
    sprintf(yst,"%s%d",pGroup,nYST);

    VCONN_List ::iterator i = m_VListM.find(std::string(yst));

    if(i != m_VListM.end())
    {
        STVLINK pLink;
        memset(&pLink,0,sizeof(STVLINK));
        memcpy(&pLink,&i->second,sizeof(STVLINK));
        m_VListM.erase(i);

//        if(pLink.pVirtualChannel)
//            delete pLink.pVirtualChannel;
    }
}


int CCHelpCtrlM::GetHelper(char* pGroup,int nYST,int *nCount)
{
    CLocker lock(m_VListMLock);
    *nCount = m_VListM.size();

    char yst[20] = {0};
    sprintf(yst,"%s%d",pGroup,nYST);
    VCONN_List ::iterator i = m_VListM.find(std::string(yst));

    if(i != m_VListM.end())
    {
        STVLINK pLink;// = new STVLINK;
        memset(&pLink,0,sizeof(STVLINK));
        memcpy(&pLink,&i->second,sizeof(STVLINK));
        return pLink.nConnectType;
    }
    return -1;
}
#ifdef WIN32
void LOGE(char* format, ...)
{

}
#endif
        void CCHelpCtrlM::StopHelp()
        {
            m_bExit = TRUE;
            
            while(!m_bExitSignal)
            {
                jvc_sleep(100);
                printf("StopHelp....\n");
                
            }
            printf("StopEND1.... size: %d\n",sizeof(CCVirtualChannel));
            CLocker lock(m_VListMLock);
            
            //     VCONN_List ::iterator i = m_VListM.begin();
            //            for(;i != m_VListM.end(); i ++)
            //            {
            //                STVLINK pLink;
            //                memset(&pLink,0,sizeof(STVLINK));
            //                memcpy(&pLink,&i->second,sizeof(STVLINK));
            //                m_VListM.erase(i);
            //                printf("StopBegin  %d....\n",pLink.nYSTNO);
            //                if(pLink.pVirtualChannel)
            //                    delete pLink.pVirtualChannel;
            //                printf("StopEND....\n");
            //            }
            //            m_VListM.clear();
            for(VCONN_List::iterator k = m_VListM.begin(); k != m_VListM.end(); ++ k)
            {
                //k->second.pVirtualChannel->DisConnect();
                delete k->second.pVirtualChannel;
            }
            m_VListM.clear();
            printf("StopEND2....\n");
        }
        
