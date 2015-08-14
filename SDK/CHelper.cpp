// CHelper.cpp: implementation of the CCHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "CHelper.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#include "CWorker.h"

#include "CChannel.h"


CCHelper::CCHelper()
{
	m_bExit = FALSE;
	m_dwLastBroadcastTime = 0;
	memset(m_nIpNum,0,sizeof(m_nIpNum));
	m_nDestPort = 6666;
	m_bisMobile = FALSE;
	memset(m_ucSearchData,0,sizeof(m_ucSearchData));
	m_nSearchLen = 21;
	m_dwLastKeepActive = 0;

#ifndef WIN32
	pthread_mutex_init(&m_LocalLock, NULL); //��ʼ���ٽ���
	pthread_mutex_init(&m_OuterLock, NULL); //��ʼ���ٽ���

	m_hReceivThread = 0;
	m_hSendThread = 0;
#else
	m_LocalLock = CreateMutex(NULL, false, NULL);
	m_OuterLock = CreateMutex(NULL, false, NULL);

	m_hReceivThread = 0;
	m_hSendThread = 0;
#endif

}

CCHelper::~CCHelper()
{
	m_bExit = TRUE;

#ifndef WIN32
	if (0 != m_hReceivThread)
	{
		pthread_join(m_hReceivThread, NULL);
		m_hReceivThread = 0;
	}
	if (0 != m_hSendThread)
	{
		pthread_join(m_hSendThread, NULL);
		m_hSendThread = 0;
	}
	pthread_mutex_destroy(&m_LocalLock);
	pthread_mutex_destroy(&m_OuterLock);

#else
	CCWorker::jvc_sleep(10);
	CCChannel::WaitThreadExit(m_hReceivThread);
	if(m_hReceivThread > 0)
	{
		CloseHandle(m_hReceivThread);
		m_hReceivThread = 0;
	}

	CCWorker::jvc_sleep(10);
	CCChannel::WaitThreadExit(m_hSendThread);
	if(m_hSendThread > 0)
	{
		CloseHandle(m_hSendThread);
		m_hSendThread = 0;
	}

	CloseHandle(m_LocalLock);
	CloseHandle(m_OuterLock);

#endif

	m_LocalList.clear();
	m_OuterList.clear();

}

BOOL CCHelper::Start(CCWorker* pWorker)
{
	m_pWorker = pWorker;

	m_sBroadcast = socket(AF_INET, SOCK_DGRAM,0);

	SOCKADDR_IN addrSrv;
#ifndef WIN32
	addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(0);

#ifdef MOBILE_CLIENT
	int len1=1500*1024;
    UDT::setsockopt(m_sBroadcast, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
	
    len1=1000*1024;
    UDT::setsockopt(m_sBroadcast, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif

	bind(m_sBroadcast, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));

    int nBroadcast = 1;
    ::setsockopt(m_sBroadcast, SOL_SOCKET, SO_BROADCAST, (char*)&nBroadcast, sizeof(int));

	struct sockaddr_in sin;
	int len = sizeof(sin);
#ifdef WIN32
	if(getsockname(m_sBroadcast, (struct sockaddr *)&sin, &len) != 0)
#else
	if(getsockname(m_sBroadcast, (struct sockaddr *)&sin, (socklen_t* )&len) != 0)
#endif
	{
		printf("getsockname() error:%s\n", strerror(errno));
	}

	m_nLocalPort = ntohs(sin.sin_port);
//	OutputDebug("Search local port = %d",m_nLocalPort);

	int nSLen = 0;
	int nSType = 0;
	int nType = JVN_REQ_LANSERCH;
	memcpy(&m_ucSearchData[0], &nType, 4);
	nSType = JVN_CMD_LANSALL;
	nSLen = 13;
	memcpy(&m_ucSearchData[4], &nSLen, 4);
	memcpy(&m_ucSearchData[8], &m_nLocalPort, 4);
	unsigned int m_unLANSID = 0;//
	memcpy(&m_ucSearchData[12], &m_unLANSID, 4);
	memcpy(&m_ucSearchData[16], &nSType, 4);
	m_ucSearchData[20] = 0;
	m_nSearchLen = 21;

/*
	char cG[5] = {'A','B','S','Z','C'};
	char strG[5][4] = {"A","B","S","Z","C"};

	for(int i = 0;i < 4; i ++)//��ǰ�ĸ��飬�Ժ��ٲ������
	{
		if(!m_pWorker->GetIndexServerList(strG[i], m_ISList[cG[i] - 'A'], 1, 0))
		{
			if(!m_pWorker->GetIndexServerList(strG[i], m_ISList[cG[i] - 'A'], 2, 0))
			{
				;
			}
		}
		if(!m_pWorker->GetSerList(strG[i], m_YstServerList[cG[i] - 'A'], 1, 0))
		{
			if(!m_pWorker->GetSerList(strG[i], m_YstServerList[cG[i] - 'A'], 2, 0))
			{
				;
			}
		}
	}
	*/
	//	m_pWorker->
	//�����߳�
	{
#ifndef WIN32
	pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE * 4;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
	if (0 != pthread_create(&m_hReceivThread, pAttr, LanRecvProc, this))
	{
		m_hReceivThread = 0;
		return FALSE;
	}
#else
	//���������߳�
	UINT unTheadID;
	m_hReceivThread = (HANDLE)_beginthreadex(NULL, 0, LanRecvProc, (void *)this, 0, &unTheadID);
	if (m_hReceivThread == 0)//�����߳�ʧ��
	{
		return FALSE;
	}
#endif
	}


	return TRUE;
}

#ifdef WIN32
UINT CCHelper::LanRecvProc(LPVOID pParam)
#else
void*  CCHelper::LanRecvProc(void* pParam)//���������״̬
#endif
{
	CCHelper* pHelp = (CCHelper* )pParam;

//	pHelp->Broadcast();//��ʼ���ȹ㲥һ��

	while(!pHelp->m_bExit)
	{
		if(pHelp->m_bExit)
		{
			break;
		}
		pHelp->ReceiveBroadcast();//��������

		pHelp->KeepActive();//ά������

		CCWorker::jvc_sleep(10);
	}
#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}


void CCHelper::AddOkYST(char* pGroup,int nYST,char* pIP,int nPort)
{
	char strTemp[4] = {0};
	memcpy(strTemp,pGroup,4);
	if(strTemp[0] >= 'a')
		strTemp[0] -= 32;

	char yst[20] = {0};
	sprintf(yst,"%s%d",strTemp,nYST);

	//�����ǲ����Ѿ���������
	std::string s(yst);
	CLocker lock(m_LocalLock,__FILE__,__LINE__);
	LOCAL_LIST::iterator i = m_LocalList.find(s);
	if(i != m_LocalList.end())
	{
		return;
	}

	DATA_LOCAL info = {0};
	memcpy(info.strGroup,strTemp, 4);
	info.nYST = nYST;
	info.nPort = m_nDestPort;

	sprintf(info.strRemoteIP,"%s",pIP);
	info.nRemotePort = nPort;
	info.nChannelNum = 1;
	info.dwLastRecvTime = CCWorker::JVGetTime();
	info.bActive = TRUE;

	m_LocalList.insert(std::map<std::string,DATA_LOCAL>::value_type(s, info));
}

void CCHelper::AddYST(char* pGroup,int nYST,int* pIP,BOOL bIsMobile)
{
	char strTemp[4] = {0};
	memcpy(strTemp,pGroup,4);
	if(strTemp[0] >= 'a')
		strTemp[0] -= 32;

	//���޲鿴�������Ƿ���ڣ���������������ӵ����������ӣ���������������������ӣ������ڼ�����
	if(pIP != NULL)
		memcpy(m_nIpNum,pIP,255 * 4);
	m_bisMobile = bIsMobile;

	char yst[20] = {0};
	sprintf(yst,"%s%d",strTemp,nYST);

	//�����ǲ����Ѿ���������
	std::string s(yst);
	CLocker lock(m_LocalLock,__FILE__,__LINE__);
	LOCAL_LIST::iterator i = m_LocalList.find(s);
	if(i != m_LocalList.end())
	{
		return;
	}
}

#ifndef WIN32
	void* CCHelper::LANSSndProc(void* pParam)
#else
	UINT CCHelper::LANSSndProc(LPVOID pParam)
#endif
{
	CCHelper *pHelp = (CCHelper *)pParam;

//	OutputDebug("Searching...");
	DWORD dwend = 0;

	SOCKADDR_IN addRemote;
	addRemote.sin_family = AF_INET ;
	addRemote.sin_addr.s_addr = INADDR_BROADCAST; //::inet_addr("255.255.255.255");
	addRemote.sin_port = htons(pHelp->m_nDestPort);

	int ntmp = CCChannel::sendtoclientm(pHelp->m_sBroadcast,(char *)pHelp->m_ucSearchData,pHelp->m_nSearchLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

	return 0;//������������ȱ�ݣ���ʱ����

#ifndef MOBILE_CLIENT
//	pHelp->SearchFSIpSection();//���������ֶ����ӵ�IP��
	char ip[30] = {0};
	pHelp->GetAdapterInfo();//���»�ȡ��ip;
	BOOL bstop=FALSE;
	int ncount = pHelp->m_IpList.size();
	for(int k = 0; k < ncount;k ++)
	{
		//ֻ����һ������������Ϊ�յ������ʱ��������

		char ip_head[30] = {0};
		strcpy(ip_head,pHelp->m_IpList[k].IpHead);
		//��ʼ
		if (pHelp->m_bisMobile)//�ֻ���������;
		{
			//�ӵ�ǰ����ip�ο�ʼ�㲥�����Լ�������Ķγɹ���Խ��
			if(pHelp->m_IpList[k].nIP3 >= 0 && pHelp->m_IpList[k].nIP3 < 256)
			{
				int j=0;
				//�ӵ�ǰipֵ��ǰ����10����
				int n = pHelp->m_IpList[k].nIP3 - 10;
				n = ((n>=0)?n:0);
				for(j=n; j<pHelp->m_IpList[k].nIP3; j++)
				{
					for(int i = 1;i < 254;i ++)
					{
						sprintf(ip,"%s%d.%d",ip_head,j,i);
//						OutputDebug("ip = %s",ip);
						addRemote.sin_addr.s_addr = ::inet_addr(ip);
						CCChannel::sendtoclientm(pHelp->m_sBroadcast,(char *)pHelp->m_ucSearchData,pHelp->m_nSearchLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

						if (0 == i%200)//sleep(1)
						{
							CCWorker::jvc_sleep(5);
						}
					}
				}

				//�ӵ�ǰipֵ�����10����
				n = pHelp->m_IpList[k].nIP3 + 10;
				n = ((n<=255)?n:255);
				for(j = pHelp->m_IpList[k].nIP3; j<=n; j++)
				{
				    int i = 0;
					for(i = 1;i < 254;i ++)
					{
						sprintf(ip,"%s%d.%d",ip_head,j,i);
						addRemote.sin_addr.s_addr = ::inet_addr(ip);
//						OutputDebug("ip = %s",ip);

						CCChannel::sendtoclientm(pHelp->m_sBroadcast,(char *)pHelp->m_ucSearchData,pHelp->m_nSearchLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
						if (0 == i % 200)//sleep(1)
						{
							CCWorker::jvc_sleep(5);
						}
					}
				}


				//��ǰ��ʣ��η���
				n = pHelp->m_IpList[k].nIP3 - 10;
				n = ((n>=0)?n:0);
				for(j=0; j<n; j++)
				{
					for(int i = 1;i < 254;i ++)
					{
						sprintf(ip,"%s%d.%d",ip_head,j,i);
						addRemote.sin_addr.s_addr = ::inet_addr(ip);

						CCChannel::sendtoclientm(pHelp->m_sBroadcast,(char *)pHelp->m_ucSearchData,pHelp->m_nSearchLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

						if (0 == i % 200)//sleep(5)
						{
							CCWorker::jvc_sleep(5);
						}
					}
				}
				//�����ʣ��η���
				n = pHelp->m_IpList[k].nIP3 + 10;
				n = ((n<=255)?n:255);
				for(j = n; j < 256; j ++)
				{
					for(int i = 1;i < 254; i++)
					{
						sprintf(ip,"%s%d.%d",ip_head,j,i);
						addRemote.sin_addr.s_addr = ::inet_addr(ip);
//						OutputDebug("ip = %s",ip);

						CCChannel::sendtoclientm(pHelp->m_sBroadcast,(char *)pHelp->m_ucSearchData,pHelp->m_nSearchLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

						if (0 == i % 200)//sleep(5)
						{
							CCWorker::jvc_sleep(5);
						}
					}
				}
			}
		}
		else
		{//�������� ;
			for(int j = 0;j < 255;j ++)
			{
				if(pHelp->m_nIpNum[j] == 1)//Ҳ�������Լ�IP�б�������. && j!=pWorker->m_IpList[k].nIP3)//�����Լ����ڵ����η��͹㲥����Ϊ�Ѿ�����
				{
//					OutputDebug("search %s.%d.1",ip_head,j);
					int n =0;
					for(int k = 1;k < 254;k ++)
					{
						sprintf(ip,"%s%d.%d",ip_head,j,k);
						addRemote.sin_addr.s_addr = ::inet_addr(ip);
//						OutputDebug("ip = %s",ip);
						CCChannel::sendtoclientm(pHelp->m_sBroadcast,(char *)pHelp->m_ucSearchData,pHelp->m_nSearchLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

						if(k - n >= 6)//ÿ����6��IP֮��sleep 1ms;Sleep�Ƿǳ���ʱ���- -
						{
							CCWorker::jvc_sleep(1);
							n = k;
						}

					}//for(int k ...
				}//if(pWorker ...
				if(bstop)
				{
					break;
				}
			}//for (j =0...
		}//else ;
		//����
	}//for (k =0...
#endif
	return 0;
}

void CCHelper::SearchDevice(int* pIP,BOOL bIsMobile)
{
	if(pIP != NULL)
	{
		memcpy(m_nIpNum,pIP,255 * 4);
		m_bisMobile = bIsMobile;
	}

//�����߳�
#ifndef WIN32
	pthread_attr_t attr;
	pthread_attr_t *pAttr = &attr;
	unsigned long size = LINUX_THREAD_STACK_SIZE * 10;
	size_t stacksize = size;
	pthread_attr_init(pAttr);
	if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
	{
		pAttr = NULL;
	}
	if (0 != pthread_create(&m_hSendThread, pAttr, LANSSndProc, this))
	{
		m_hSendThread = 0;
		return ;
	}
#else
	//���������߳�
	UINT unTheadID;
	m_hSendThread = (HANDLE)_beginthreadex(NULL, 0, LANSSndProc, (void *)this, 0, &unTheadID);
	if (m_hSendThread == 0)//�����߳�ʧ��
	{
		return ;
	}
#endif
}

void CCHelper::Broadcast()
{
	DWORD dwTime = CCWorker::JVGetTime();
	if(m_dwLastBroadcastTime + 1000 * 30 < dwTime)
	{
		m_dwLastBroadcastTime = dwTime;

		SearchDevice();
	}
}

void CCHelper::ReceiveBroadcast()
{
	SOCKADDR_IN clientaddr = {0};
	int addrlen = sizeof(SOCKADDR_IN);

	BYTE recBuf[RC_DATA_SIZE] = {0};
	int nRecvLen = 0;
	int nType = 0;

	nRecvLen = CCChannel::receivefrom(m_sBroadcast,(char *)&recBuf, RC_DATA_SIZE, 0, (SOCKADDR*)&clientaddr,&addrlen,1);
	//�豸����
	if( nRecvLen > 0)
	{//���գ�����(4)+����(4)+����ID(4)+��ϵ(4)+����ͨ��(4)+��ͨ����(4)+����˿ں�(4)+�����(4)+��Ʒ����(1)+��������(1)+����(?) + NetMod(4) + CurMod(4) + size(4) + info(?)
		memcpy(&nType, &recBuf[0], 4);
		if(nType == JVN_RSP_LANSERCH)
		{//��SDKЭ�� ����������
			DATA_LOCAL info = {0};
			memcpy(info.strGroup,&recBuf[28], 4);
			memcpy(&info.nYST,&recBuf[16], 4);
			sprintf(info.strRemoteIP,"%s",inet_ntoa(clientaddr.sin_addr));
			info.nPort = ntohs(clientaddr.sin_port);
			memcpy(&info.nRemotePort,&recBuf[24], 4);//����˿ں�(4)
			memcpy(&info.nChannelNum,&recBuf[20], 4);//��ͨ����(4)
			info.dwLastRecvTime = CCWorker::JVGetTime();
			info.bActive = TRUE;
			char yst[20] = {0};
			sprintf(yst,"%s%d",info.strGroup,info.nYST);
			CLocker lock(m_LocalLock,__FILE__,__LINE__);
			std::string s(yst);
			LOCAL_LIST::iterator i = m_LocalList.find(s);

			if(i == m_LocalList.end())
			{
				m_LocalList.insert(std::map<std::string,DATA_LOCAL>::value_type(s, info));

//				OutputDebug("search %s %s : %d   %d",yst,info.strRemoteIP,info.nRemotePort,m_LocalList.size());
			}
			else
			{
				memcpy(&info,&i->second,sizeof(DATA_LOCAL));

				info.nPort = ntohs(clientaddr.sin_port);
				sprintf(info.strRemoteIP,"%s",inet_ntoa(clientaddr.sin_addr));
				memcpy(&info.nRemotePort,&recBuf[24], 4);//����˿ں�(4)
				memcpy(&info.nChannelNum,&recBuf[20], 4);//��ͨ����(4)

				info.dwLastRecvTime = CCWorker::JVGetTime();
				info.bActive = TRUE;
				m_LocalList[std::string(yst)] = info;

				//OutputDebug("recv active %s %s : %d",yst,info.strRemoteIP,info.nRemotePort);
			}
		}
		else
		{//��DVR֧��,����Ǳ��ذ�����������������dvr�����ڲ����ж���һ�࣬���϶���
			PACKET stPacket;
			memset(&stPacket, 0, sizeof(PACKET));
			memcpy(&stPacket, &recBuf, sizeof(PACKET));

			if(stPacket.nPacketType == JVN_REQ_LANSERCH)
			{
			}
		}
	}
}

void CCHelper::GetAdapterInfo()
{
	m_IpList.clear();

	_Adapter info;
	memset(&info,0,sizeof(_Adapter));
	//���ص�ַ
	NATList LocalIPList;
	m_pWorker->GetLocalIP(&LocalIPList);

	for(int i = 0;i < LocalIPList.size();i ++)
	{
		sprintf(info.IP,"%d.%d.%d.%d",LocalIPList[i].ip[0],LocalIPList[i].ip[1],LocalIPList[i].ip[2],LocalIPList[i].ip[3]);
		//BYTE n[4] = {0};
		unsigned int n[4] = {0};
		sscanf(info.IP,"%d.%d.%d.%d",&n[0],&n[1],&n[2],&n[3]);
		sprintf(info.IpHead,"%d.%d.",n[0],n[1]);
		info.nIP3 = n[2];
		m_nIpNum[info.nIP3] = 1;
		m_IpList.push_back(info);
	}
	int n = m_IpList.size();

	return;
}

void CCHelper::KeepActive()
{
	DWORD dwTime = CCWorker::JVGetTime();
	if(m_dwLastKeepActive + 1000 * 10 < dwTime)
	{
		m_dwLastKeepActive = dwTime;

		//����
		{
			char yst[20] = {0};
			DATA_LOCAL info = {0};

			SOCKADDR_IN addRemote;
			addRemote.sin_family = AF_INET ;
			CLocker lock(m_LocalLock,__FILE__,__LINE__);
			for(LOCAL_LIST::iterator k = m_LocalList.begin(); k != m_LocalList.end(); ++ k)
			{
				memcpy(&info,&k->second,sizeof(DATA_LOCAL));

				sprintf(yst,"%s%d",info.strGroup,info.nYST);
				std::string s(yst);

				addRemote.sin_addr.s_addr = ::inet_addr(info.strRemoteIP);
				addRemote.sin_port = htons(info.nPort);
				//CCChannel::sendtoclientm(m_sKeepActive,(char *)m_ucSearchData,m_nSearchLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
				CCChannel::sendtoclientm(m_sBroadcast,(char *)m_ucSearchData,m_nSearchLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);

				if(dwTime > info.dwLastRecvTime + 1000 * 30)//30s ��Ϊ����
					info.bActive = FALSE;
				m_LocalList[s] = info;

	//			OutputDebug("%s \t\t\t%d",yst,info.bActive);
			}
		}
		//���� udp
		//������ ��ʱȥ�������Ǻú��ټ���
/*		{
			char strActiveData[100] = {0};//�������ݰ� 4(Head)4(total_len)4(type)....(data)
			int nHead = UDP_HEAD_VALUE;
			memcpy(&strActiveData[0],&nHead,4);
			int nLen = 20;
			memcpy(&strActiveData[4],&nLen,4);
			int nType = JVN_KEEP_ACTIVE;
			memcpy(&strActiveData[8],&nType,4);
			char yst[20] = {0};
			DATA_OUTER info = {0};
			
			SOCKADDR_IN addRemote;
			addRemote.sin_family = AF_INET ;
			CLocker lock(m_OuterLock,__FILE__,__LINE__);
			for(OUTER_LIST::iterator k = m_OuterList.begin(); k != m_OuterList.end(); ++ k)
			{
				memcpy(&info,&k->second,sizeof(DATA_OUTER));
				
				sprintf(yst,"%s%d",info.strGroup,info.nYST);
				std::string s(yst);
				
				addRemote.sin_addr.s_addr = ::inet_addr(info.strRemoteIP);
				addRemote.sin_port = htons(info.nRemotePort);
				
				memcpy(&strActiveData[12],info.strGroup,4);
				memcpy(&strActiveData[16],&info.nYST,4);
				int dd = CCChannel::sendtoclientm(info.sLocal,(char *)strActiveData,nLen,0,(SOCKADDR *)&addRemote, sizeof(SOCKADDR),1);
				
				if(dwTime > info.dwLastRecvTime + 1000 * 30)//30s ��Ϊ����
					info.bActive = FALSE;
				else
					info.bActive = TRUE;
				
				m_OuterList[s] = info;
				
				OutputDebug("%s \t\t\tactive:%d  %s : %d",yst,info.bActive,info.strRemoteIP,info.nRemotePort);
			}
		}*/
	}

}


BOOL CCHelper::GetLocalInfo(char* pGroup,int nYST,char* pIP,int &nPort)
{
    return FALSE;
    
	char strTemp[4] = {0};
	memcpy(strTemp,pGroup,4);
	if(strTemp[0] >= 'a')
		strTemp[0] -= 32;

	char yst[20] = {0};
	sprintf(yst,"%s%d",strTemp,nYST);
	CLocker lock(m_LocalLock,__FILE__,__LINE__);
	std::string s(yst);
	LOCAL_LIST::iterator i = m_LocalList.find(s);

	if(i != m_LocalList.end())
	{
		DATA_LOCAL info = {0};
		memcpy(&info,&i->second,sizeof(DATA_LOCAL));

		if(info.bActive)
		{
			strcpy(pIP,info.strRemoteIP);
			nPort = info.nRemotePort;

			OutputDebug("===========================Find %s%d  %s : %d",pGroup,nYST,pIP,nPort);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CCHelper::GetOuterInfo(char* pGroup,int nYST,SOCKET &sLocal,char* pIP,int &nPort)
{
	return FALSE;
	//������ ��ʱȥ�������Ǻú��ټ���
	char strTemp[4] = {0};
	memcpy(strTemp,pGroup,4);
	if(strTemp[0] >= 'a')
		strTemp[0] -= 32;
	
	char yst[20] = {0};
	sprintf(yst,"%s%d",strTemp,nYST);
	CLocker lock(m_OuterLock,__FILE__,__LINE__);
	std::string s(yst);
	OUTER_LIST::iterator i = m_OuterList.find(s);
	
	if(i != m_OuterList.end())
	{
		DATA_OUTER info = {0};
		memcpy(&info,&i->second,sizeof(DATA_OUTER));
		
		if(info.bActive)
		{
			sLocal = info.sLocal;
			strcpy(pIP,info.strRemoteIP);
			nPort = info.nRemotePort;
			
			OutputDebug("========================Find outer %s%d  %s : %d",pGroup,nYST,pIP,nPort);
			return TRUE;
		}
	}
	return FALSE;
}


void CCHelper::UpdateTime(char* pGroup,int nYST,SOCKET sLocal,SOCKADDR_IN* addrRemote)
{
	return;
	//������ ��ʱȥ�������Ǻú��ټ���
	if(sLocal == 0)
		sLocal = m_pWorker->m_WorkerUDPSocket;
	char strTemp[4] = {0};
	memcpy(strTemp,pGroup,4);
	if(strTemp[0] >= 'a')
		strTemp[0] -= 32;
	
	char yst[20] = {0};
	sprintf(yst,"%s%d",strTemp,nYST);
	CLocker lock(m_OuterLock,__FILE__,__LINE__);
	std::string s(yst);
	OUTER_LIST::iterator i = m_OuterList.find(s);
	
	if(i != m_OuterList.end())
	{
		DATA_OUTER info = {0};
		memcpy(&info,&i->second,sizeof(DATA_OUTER));

		info.sLocal = sLocal;
		strcpy(info.strRemoteIP,inet_ntoa(addrRemote->sin_addr));
		info.nRemotePort = ntohs(addrRemote->sin_port);

		info.dwLastRecvTime = CCWorker::JVGetTime();
		info.bActive = TRUE;

		m_OuterList[s] = info;
//		OutputDebug("update a ok connect to helper.(%s%d)",pGroup,nYST);
	}
	else
	{
		DATA_OUTER info = {0};
		memcpy(info.strGroup,pGroup,4);
		info.nYST = nYST;
		info.sLocal = sLocal;
		strcpy(info.strRemoteIP,inet_ntoa(addrRemote->sin_addr));
		info.nRemotePort = ntohs(addrRemote->sin_port);
		info.dwLastRecvTime = CCWorker::JVGetTime();
		info.bActive = TRUE;
		m_OuterList.insert(std::map<std::string,DATA_OUTER>::value_type(s, info));

		//OutputDebug("=======================================Add a ok connect to helper.(%s%d)",pGroup,nYST);
	}
}
