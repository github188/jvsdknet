// MakeHoleC.h: interface for the CMakeHoleC class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAKEHOLEC_H__08EED94C_9502_4AF7_8ED8_C479BFBE0788__INCLUDED_)
#define AFX_MAKEHOLEC_H__08EED94C_9502_4AF7_8ED8_C479BFBE0788__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "CChannel.h"
#include "CVirtualChannel.h"

//A IP1 B IP2 ,X Y �˿�
#define CHECK_TYPE_AX_AX 1
#define CHECK_TYPE_AX_BY 2
#define CHECK_TYPE_BX_BX 3
#define CHECK_TYPE_BX_BY 4

BOOL AddrIsTheSame(sockaddr* addrRemote,sockaddr* addrRemote2);//�ж�2����ַ�Ƿ�����ͬ 4�ֽڵ�ַ 2�ֽڶ˿�

class CMakeHoleC;

#define JVN_CMD_YSTCHECK2     0xB1//??????????????????SDK?? ????NAT??

class CLocker
{
public:
	CLocker(pthread_mutex_t& lock,char* pName = "",int nLine = 0);
	~CLocker();
	
	static void enterCS(pthread_mutex_t& lock);
	static void leaveCS(pthread_mutex_t& lock);
	
private:
	pthread_mutex_t& m_Mutex;            // Alias name of the mutex to be protected
	int m_iLocked;                       // Locking status

	char m_strName[1000];
	int m_nLine;
};
#define JVN_REQ_EXCONNA2        0xB0
#define JVN_REQ_NAT				0x101


#ifdef _WIN32
#define MAX_NUM 1//���δ򿪱��صĶ˿�����������ͬʱ��������  �ο�teamviewer�޸Ķ�˿�һ������
#else
#define MAX_NUM 1//���δ򿪱��صĶ˿�����������ͬʱ�������� Ƕ��ʽϵͳֻ��һ������
#endif

#define JVN_CMD_NEW_TRYTOUCH	0xb2//�µĴ򶴰� ������
#define JVN_CMD_A_HANDSHAKE		0xb3//�µĴ򶴰� ���ط����ֿ� 
#define JVN_CMD_B_HANDSHAKE		0xb4//�µĴ򶴰� �ֿط�������
#define JVN_CMD_AB_HANDSHAKE	0xb5//���ط����ֿ� �ֿ��յ����������


//
//���� 0xb2�������յ�����0xb3,�ֿ��յ�����0xb4
//�����յ�0xb4 ���� 0xb5,��������
//�ֿ��յ�0xb3 ���� 0xb4
//�ֿ��յ�0xb4 ֱ������

typedef struct 
{
	SOCKET sSocket;//�����׽���
	
	SOCKADDR_IN addrRemote;//Զ�̶˿�
	int nChannel;//���ӵ�ͨ��
	int nTimeout;//��ʱʱ��ms

	void* pChannel;
	BOOL bConnected;//�Ƿ��Ѿ����ӹ�
	BOOL bIsHole;//�Ƿ��Ǵ򶴰�
}CONNECT_INFO;//UDT���ص����ݰ�

typedef ::std::vector<CONNECT_INFO >CONNECT_LIST;//����ʹ��UDT����

class CMakeHoleC  
{
public:
	void PrePunch(SOCKET s,SOCKADDR_IN addrA);

	BOOL AddConnect(CONNECT_INFO info,BOOL bFirst = FALSE);//���һ���������ӵ�Ŀ��

	BOOL ConnectA(SOCKET s,SOCKADDR_IN *remote,int nTimeOut,UDTSOCKET& uSocket);//�������أ��յ���������
	void CreatUDT(SOCKET s);//����UDT ���ڶಥ����
	CMakeHoleC(char *pGroup,int nYst,int nChannel,BOOL bLocalTry,ServerList ServerList,int nNatA,int nNatB,int nUpnp = 0,void* pChannel = NULL,BOOL bVirtual = FALSE);
	virtual ~CMakeHoleC();

	unsigned long m_dwStartTime;
	void Start();
	void End();

	SOCKET m_sWorkSocket;//9200�׽��� �̶��˿�
	int m_nNatA,m_nNatB;

	BOOL m_bRuning;

	BOOL m_bTryLocal;
	int m_nChannel;

	CONNECT_LIST m_UdtConnectList;//�������ӵ��б�
	pthread_mutex_t m_Connect;
	BOOL m_bConnectOK;

	void* m_pParent;
	BOOL m_bVirtual;//�����ӱ���

	char m_strGroup[4];
	int m_nYst;

	ServerList m_ServerSList;//�������б�

	SOCKADDR_IN m_addA;//���ص�ַ
	SOCKADDR_IN m_addLocalA;//���ص�ַ ����

	int m_nLocalPortWan;//upnp�˿�

	SOCKET m_sLocalSocket[MAX_NUM];//���������׽���

	SOCKET m_sLocalSocket2;//���������׽��� ���ӳɹ�����һ��

	UDTSOCKET m_udtSocket;//���ڶಥ������

	NATList m_NatList;//���ض�Ӧ������������NAT

	NATList m_NATListTEMP;//��ʱ����ʹ�õ����е�NAT �� ������ַ

	int GetLocalNat(int nMinTime);//�����˿ں�ip��������ֻ�����ϵĹ̶��˿�
	void GetNATADDR();

	int GetAddress();//��ȡ���ص�ַ
	
#ifndef WIN32
	static void* ConnectThread(void* pParam);//ֱ�������̣߳�����ȡ�õĵ�ַ����
#else
	static UINT WINAPI ConnectThread(LPVOID pParam);//ֱ�������̣߳�����ȡ�õĵ�ַ����
#endif

#ifndef WIN32
	static void* ConnectRemoteProc(void* pParam);//�����̣߳����ڴ���������
#else
	static UINT WINAPI ConnectRemoteProc(LPVOID pParam);//�����̣߳����ڴ���������
#endif
	
#ifndef WIN32
	pthread_t m_hConnThread;//�߳̾��
	BOOL m_bEndC;
	pthread_mutex_t m_ct;
#else
	HANDLE m_hConnThread;//�߳̾��
	HANDLE m_hStartEventC;
	HANDLE m_hEndEventC;
	
	CRITICAL_SECTION m_ct;
#endif

	CCWorker* m_pWorker;
	int m_nConnectNum;//�������������仯Ϊ0ʱ�ر��׽���

};

typedef std::map<std::string, CMakeHoleC* > CONN_List;//�������ӵ��б�

#define NAT_TYPE_0_UNKNOWN			0	//δ֪���� ��û��̽�����������
#define NAT_TYPE_0_PUBLIC_IP		1	//����,�ֿؿ���ֱ������
#define NAT_TYPE_1_FULL_CONE		2	//ȫ͸��,�ֿؿ���ֱ������
#define NAT_TYPE_2_IP_CONE			3	//ip�޶�
#define NAT_TYPE_3_PORT_CONE		4	//�˿��޶�
#define NAT_TYPE_4_SYMMETRIC		5	//�Գ�


class CMakeGroup//��¼�����е��������ӵ���Ϣ
{
public:
	CMakeGroup();
	~CMakeGroup();

	void Start(CCWorker* pWorker);
	void SetConnect(SOCKET s,int nType);//���������� nType = 1 ������ +1��nType = -1 �Ͽ� -1
	
	CCWorker* m_pWorker;
	pthread_mutex_t m_MakeLock;
	CONN_List m_ConnectList;//�������ӵĶ���

	BOOL CheckConnect(char *strGroup,int nYstNo);//����Ƿ����������ӵĶ���,һ������ֻ����һ���ڴ�͸

	BOOL AddConnect(void* pChannel,char *strGroup,int nYstNo,int nChannel,BOOL bLocalTry,ServerList ServerList,int nNatTypeA,int nUpnp,BOOL bVirtual);

	int GetNatType(char* strGroup);//NAT���ͼ�� 0 δ֪��1 ������ 2~5Ϊ4������
	BOOL IsThisLocal(SOCKADDR_IN);//��ַ�Ƿ��ڱ��ص�ַ�б��� ���ڹ����ж�

	ServerList m_ServerList;//���з������б�
	NATList m_LocalIPList;//����IP�б�

	int m_nNatTypeB;//���ͼ�� 0 δ֪��1 ������ 2~5Ϊ4������


#ifndef WIN32
	static void* CheckNatProc(void* pParam);//NAT����߳�
#else
	static UINT WINAPI CheckNatProc(LPVOID pParam);//NAT����߳�
#endif
	
#ifndef WIN32
	pthread_t m_hCheckThread;//�߳̾��
	BOOL m_bCheckEndC;
	pthread_mutex_t m_ctCheck;
#else
	HANDLE m_hCheckThread;//�߳̾��
	HANDLE m_hCheckStartEventC;
	HANDLE m_hCheckEndEventC;
	
	CRITICAL_SECTION m_ctCheck;
#endif

};

#endif // !defined(AFX_MAKEHOLEC_H__08EED94C_9502_4AF7_8ED8_C479BFBE0788__INCLUDED_)
