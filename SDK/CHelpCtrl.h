// CHelpCtrl.h: interface for the CCHelpCtrl class.


#if !defined(AFX_CHELPCTRL_H__CBF9E4AA_BD7A_4C61_A2E4_C8A04555789D__INCLUDED_)
#define AFX_CHELPCTRL_H__CBF9E4AA_BD7A_4C61_A2E4_C8A04555789D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

#include <map>


class CCWorker;
class CCChannel;

/*���ֺ�cvͨ������*/
#define JVC_HELP_KEEP		 0x01//����������
#define JVC_HELP_CONNECT	 0x03//�ֿ�����������������
#define JVC_HELP_DISCONN	 0x04//�ֿ��������ֶϿ�����

#define JVC_HELP_CVDATA      0x05//�ֿؾ�С���������ط��͵�����

#define JVC_HELP_CONNSTATUS  0x06//���ֵ�ʵ����״̬
#define JVC_HELP_DATA        0x07//������ֿ�ת������ͨ���� 

#define JVC_HELP_VSTATUS     0x08//������ֿط��͵����������
#define JVC_HELP_NEWYSTNO    0x09//�ֿع�ע���º��룬������Ҫ���ǽ���������


typedef struct STCONNECTINFO
{
	BOOL bYST;
	int nLocalChannel;
	int nChannel;
	int nLocalPort;
	char chServerIP[16];
	int nServerPort;
	int nYSTNO;
	char chGroup[4];
	BOOL bLocalTry;
	char chPassName[MAX_PATH];
	char chPassWord[MAX_PATH];
	
	int nTURNType;
	int nConnectType;
	BOOL bCache;//��Ϊ���ٻ��棬ȫ���ϴ�
	
	int nShow;//���ӷ�ʽ P2P/TURN �������ӳɹ���ʾ
	
	int nWhoAmI;//
	SOCKADDR_IN quickAddr;//������ַ
	
	///////////////////////////////////////////////////////////////������Ӳ���Ҫ����ͨ����
	unsigned short m_wLocalPort;//���ض˿� ���ƶ��豸��
	SOCKET sRandSocket;
	BOOL isBeRequestVedio;//�Ƿ������������ط�����Ƶ
    int nVIP ;
    int nOnlyTCP;
    
	STCONNECTINFO()
	{
        nOnlyTCP =0;
        isBeRequestVedio = TRUE;
		sRandSocket = 0;
		bYST = FALSE;
		nLocalChannel = 0;
		nChannel = 0;
		nLocalPort = 0;
		memset(chServerIP, 0, 16);
		nServerPort = 0;
		nYSTNO = 0;
        nVIP = 0;
		memset(chGroup, 0, 4);
		bLocalTry = FALSE;
		memset(chPassName, 0, MAX_PATH);
		memset(chPassWord, 0, MAX_PATH);
		
		nTURNType = 0;
		nConnectType = 0;
		bCache = FALSE;//��Ϊ���ٻ��棬ȫ���ϴ�
		
		nWhoAmI = 0;//
	}
}STCONNECTINFO;

class CCVirtualChannel;
typedef struct STVLINK
{
	char chGroup[4];
	int nYSTNO;
	BOOL bStatus;//������״̬ TRUE��Ч��FALSE��Ч
	BOOL bTURN;//�Ƿ�ת����ת���Ļ������Ӿ�û����ʵ�������ˣ�Ӧ��ֱ�ӶϿ���ÿ��5�����ٽ���һ�����ӳ��ԣ����Ƿ��ܽ���ֱ��

	int nChannel;//Ҫ���ӵ�Ŀ��ͨ��
	SOCKADDR_IN addrVirtual;//����豸�����Ӷ�Ӧ�ĵ�ַ

	int nConnectType;//�����ӵ�״̬
	char chUserName[32];
	char chPasswords[32];

	DWORD dwLastVConnect;

	CCVirtualChannel* pVirtualChannel;
	int nFailedCount;
//	unsigned short m_wLocalPort;//���ض˿� ���ƶ��豸��
	SOCKET sRandSocket;
	STVLINK()
	{
		memset(chGroup, 0, 4);
		nYSTNO = 0;
		bStatus = FALSE;
		bTURN = FALSE;
		nChannel = 0;
		nConnectType = 0;
		memset(chUserName, 0, 32);
		memset(chPasswords, 0, 32);
		dwLastVConnect = 0;
		pVirtualChannel = NULL;
		nFailedCount = 0;

//		m_wLocalPort = 0;
		sRandSocket = 0;
	}

}STVLINK;

typedef std::map<std::string, STVLINK > VCONN_List;//���������������б�	����ͨ ->����

//����TCP���ӵĽṹ	ά�������ֵ����ӺͲ�ѯ ����������
class CCHelpCtrl;
typedef struct _LOCAL_TCP_CMD
{
	CCHelpCtrl* pCtrl;
	
	SOCKET sTcpCmdSocket;
	DWORD dwSendTime;//����ʱ�� s
	DWORD dwRecvTime;//����ʱ�� s
}LOCAL_TCP_CMD,*PLOCAL_TCP_CMD;

typedef std::map<SOCKET, PLOCAL_TCP_CMD>CMD_LIST;

//�ֿ��������֣����� ������������
typedef struct _LOCAL_TCP_DATA
{
	CCHelpCtrl* pCtrl;
	CCChannel* pChannel;
	
	DWORD dwLocalChannel;//���ر�־	
	SOCKET sTcpDataSocket;
	DWORD dwSendTime;//����ʱ�� ms
	DWORD dwRecvTime;//����ʱ�� ms

}LOCAL_TCP_DATA,*PLOCAL_TCP_DATA;

typedef std::map<int, PLOCAL_TCP_DATA>DATA_LIST;


typedef struct STCONNBACK
{
	int nRet;//��� 0û���� 1�ɹ� 2ʧ��
	BOOL bJVP2P;//�Ƿ��Ƕಥ��ʽ
	int nFYSTVER;
	BYTE pMsg[1024];
	int nSize;
	int  nTmp;//����
	STCONNBACK()
	{
		bJVP2P = FALSE;
	}
}STCONNBACK;

//������ ��CWorker�ж���һ��ʵ��
class CCHelpCtrl  
{
public:
	CCHelpCtrl();
	CCHelpCtrl(CCWorker *pWorker);
	virtual ~CCHelpCtrl();

public:
	void ClearCache(void){return;};
	virtual BOOL SetHelpYSTNO(BYTE *pBuffer, int nSize){return FALSE;};//设置�1�7�1�7��1�7�1�7衄1�7
	virtual int GetHelpYSTNO(BYTE *pBuffer, int &nSize){return 0;};//�1�7�1�7��1�7�1�7�1�7�1�7��1�7�1�7衄1�7

	virtual int SearchYSTNO(STVLINK *stVLink){return 0;};//��ѯĳ���������Ϣ

	DWORD JVGetTime();
	static void jvc_sleep(unsigned long time);//����

	virtual void VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg){};//�����ض�ͨ��״̬����(����״̬)
	virtual BOOL AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort){return FALSE;};//ʵ�����ӳɹ���Ҫ�����ְ��մ�������������
public:
	CCWorker* m_pWorker;
	int m_nHelpType;//��������1;2;3
	BOOL m_bExit;//�Ƿ��˳������߳�
private:
};

//����С���ֶ�ʹ��
class CCHelpCtrlH:public CCHelpCtrl  
{
public:
	CCHelpCtrlH();
	CCHelpCtrlH(CCWorker *pWorker);
	virtual ~CCHelpCtrlH();

	//��ͻ��˷�������	��channel����
	int SendDataToCV(SOCKET s,char* pBuff,int nLen);
	//�ӿͻ��˽�������	��channel����
	int ReceiveDataFromCV(SOCKET s,char* pBuff,int nMaxLen);
	
public:
	BOOL SetHelpYSTNO(BYTE *pBuffer, int nSize);//STBASEYSTNO
	int GetHelpYSTNO(BYTE *pBuffer, int &nSize);//STBASEYSTNO

	BOOL StartWorkThread(int nPortCmd = 9000,int nPortData = 8000);//���������̣߳������ճ�ҵ���߳�	9000	8000
	
	BOOL VirtualConnect();//��ĳ���뽨��������
	BOOL ConnectYST(STCONNECTINFO st,LOCAL_TCP_DATA* pTcp);//��ĳ���뽨��ʵ���ӣ��ڲ�����ʹ�������ӵ�ַ����ͨʱ��ԭ����
	
	void DealwithLocalClientCmd();//����cv������ ����������� �����������б�
	
	int CallbackFromChannel(SOCKET s,char* pBuff,int nLen);//channel�յ����ݺ����
	
	virtual BOOL AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort){return FALSE;};//ʵ�����ӳɹ���Ҫ�����ְ��մ�������������
	void VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg);//�����ض�ͨ��״̬����(����״̬)
#ifdef WIN32
	static UINT WINAPI WorkCmdProc(LPVOID pParam);//���ͱ��ص����������б� �����µ�����������
	static UINT WINAPI WorkDataProc(LPVOID pParam);//���տͻ������� �������� �������� ���͵��ͻ���
	static UINT WINAPI ListenProcCmd(LPVOID pParam);//�����̣߳��������ؿͻ�������
	static UINT WINAPI ListenProcData(LPVOID pParam);//�����̣߳��������ؿͻ�������
#else
	static void* WorkCmdProc(void* pParam);//���ͱ��ص����������б� �����µ�����������
	static void* WorkDataProc(void* pParam);//���տͻ������� �������� �������� ���͵��ͻ���
	static void* ListenProcCmd(void* pParam);//�����̣߳��������ؿͻ�������
	static void* ListenProcData(void* pParam);//�����̣߳��������ؿͻ�������
#endif

public:
	//��cv������֮�����������
#ifndef WIN32
	pthread_mutex_t m_criticalsectionHList;
#else
	CRITICAL_SECTION m_criticalsectionHList;
#endif
	DATA_LIST m_TcpListDataH;//����TCP���� �������ֺ�cv֮�����ݴ���

private:
	SOCKET m_ListenSockCmd;//С���ֱ��ؼ����߳�	�����ͻ����� ���������
	SOCKET m_ListenSockData;//С���ֱ��ؼ����߳� ���ڴ�����ʵ����
	
	VCONN_List m_VListH;//���������
	
	CMD_LIST m_TcpListCmd;//����TCP���� ����Ͳ�ѯ����

#ifndef WIN32
	pthread_t m_hWorkCmdThread;//�߳̾��
	pthread_t m_hWorkDataThread;//�߳̾��
	pthread_t m_hListenCmdThread;//�߳̾��
	pthread_t m_hListenDataThread;//�߳̾��
	
	pthread_mutex_t m_criticalsectionVList;
#else
	HANDLE m_hWorkCmdThread;//�߳̾��
	HANDLE m_hWorkDataThread;//�߳̾��
	HANDLE m_hListenCmdThread;//�߳̾��
	HANDLE m_hListenDataThread;//�߳̾��
	
	CRITICAL_SECTION m_criticalsectionVList;//
#endif

};

//����ͻ���ʹ��,��С�������
class CCHelpCtrlP:public CCHelpCtrl  
{
public:
	CCHelpCtrlP();
	CCHelpCtrlP(CCWorker *pWorker);
	virtual ~CCHelpCtrlP();

public:
	BOOL ConnectHelp(char* pHelperName = "127.0.0.1",int nHelpPort = 9000);
	int SearchYSTNO(STVLINK *stVLink);//��ѯĳ���������Ϣ
	int GetHelpYSTNO(BYTE *pBuffer, int &nSize);//STBASEYSTNO
	void VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg){};//�����ض�ͨ��״̬����(����״̬)

	virtual BOOL AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort){return FALSE;};//ʵ�����ӳɹ���Ҫ�����ְ��մ�������������
private:
	BOOL StartWorkThread();//�����ճ�ҵ���߳� ����С�������ݽ����߳�
	
	#ifdef WIN32
		static UINT WINAPI CommWithHelpProc(LPVOID pParam);//ͬ��help������������Ϣ
        static UINT WINAPI ConnectHelpProc(LPVOID pParam);//ͬ��help������������Ϣ
	#else
		static void* CommWithHelpProc(void* pParam);//ͬ��help������������Ϣ
        static void* ConnectHelpProc(void* pParam);//ͬ��help������������Ϣ
	#endif
	
private:
	SOCKET m_ConnectCmdSock;//�������ֵ�TCP����
	VCONN_List m_VListC;//cv �������������״̬ ��������Help

	#ifndef WIN32
		pthread_t m_hCWHThread;//�߳̾��
		pthread_t m_hConnHelpThread;
	#else
		HANDLE m_hCWHThread;//�߳̾��
		HANDLE m_hEndEventWH;

		HANDLE m_hConnHelpThread;
	#endif
};

//����
class CCHelpConnCtrl
{
public:
	CCHelpConnCtrl();
	virtual ~CCHelpConnCtrl();
	
public:
	BOOL ConnectYSTNO(STCONNECTINFO stConnInfo);
	int RecvConnResult(STCONNBACK *tconnback);
	
	int SendToHelp(BYTE *pBuffer, int nSize);
	int RecvFromHelp(BYTE *pBuffer, int nSize);
	int SendToHelpActive();//�����ַ�������

	void DisConnectYSTNO();

public:
	SOCKET m_tcpsock;//��������	�����ַ��ͽ�������
private:
	
};

//����ͻ���ʹ�ã��Լ����٣�����С���֣������ֻ��ȿͻ���
class CCHelpCtrlM:public CCHelpCtrl  
{
public:
	CCHelpCtrlM();
	CCHelpCtrlM(CCWorker *pWorker);
	virtual ~CCHelpCtrlM();
	
public:
	void ClearCache(void);
	int SearchYSTNO(STVLINK *stVLink);//�1�7�1�7��1�7��1�7�1�7�1�7�1�7��1�7�1�7�1�7�1�7��1�7�1�7信�1�7�1�7
	void VConnectChange(int nLocalChannel, BYTE uchType, char *pMsg);//丄1�7主�1�7��1�7�1�7�1�7�1�7�1�7信�1�7��1�7�1�7�1�7�1�7��1�7�1�7(迄1�7�1�7�1�7��1�7��1�7�1�7)
	virtual BOOL AddRemoteConnect(char* pGroup,int nYST,SOCKET s,char* pIP,int nPort);//宄1�7�1�7�1�7�1�7迄1�7�1�7�1�7��1�7�1�7�1�7�1�7�1�7�1�7�1�7�1�7覄1�7汄1�7�1�7�1�7��1�7�1�7�1�7�1�7�1�7�1�7�1�7��1�7��1�7�1�7�1�7�1�7��1�7�1�7�1�7�1�7�主�1�7�1�7�1�7
    
    void HelpRemove(char* pGroup,int nYST);
    
    
    int GetHelper(char* pGroup,int nYST,int *nCount);
    void StopHelp();
private:
	//��Ӻ���
	virtual BOOL SetHelpYSTNO(BYTE *pBuffer, int nSize);//���ú����
	int GetHelpYSTNO(BYTE *pBuffer, int &nSize);//STBASEYSTNO
	BOOL StartWorkThread();
	
	void VirtualConnect();
	void VirtualGetConnect();//��ȡ������Ϣ���б���
	#ifdef WIN32	
		static UINT WINAPI WorkCmdProc(LPVOID pParam);
	#else
		static void*  WorkCmdProc(void* pParam);//���������״̬
	#endif	

private:
	VCONN_List m_VListM;//���������
	pthread_mutex_t m_VListMLock;
	int m_nLocalChannelIndex;//+1����

	#ifndef WIN32
		pthread_t m_hWorkCmdThread;//�߳̾��
	#else
		HANDLE m_hWorkCmdThread;//�߳̾��
	#endif
    BOOL m_bExitSignal;
};


#endif // !defined(AFX_CHELPCTRL_H__CBF9E4AA_BD7A_4C61_A2E4_C8A04555789D__INCLUDED_)
