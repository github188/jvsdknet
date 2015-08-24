// CVirtualChannel.h: interface for the CCVirtualChannel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CVIRTUALCHANNEL_H__7DA71EED_A3E4_4CBD_BE53_4F1CFA4944C5__INCLUDED_)
#define AFX_CVIRTUALCHANNEL_H__7DA71EED_A3E4_4CBD_BE53_4F1CFA4944C5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "CChannel.h"

class CCHelpCtrl;
class CCVirtualChannel  
{
public:
	CCVirtualChannel();
	CCVirtualChannel(STCONNECTINFO stConnectInfo, CCHelpCtrl *pHelp, CCWorker *pWorker,BOOL bMobile = TRUE);
	virtual ~CCVirtualChannel();
	void ReConnect(STCONNECTINFO stConnectInfo, CCHelpCtrl *pHelp, CCWorker *pWorker);

	DWORD m_dwSenTime,m_dwRecvTime;
	BOOL SendKeep();//���������ɹ�
	BOOL RecvKeep();//
	BOOL DisConnect();
	BOOL ConnectStatus(SOCKET s,SOCKADDR_IN* addr,int nTimeOut,BOOL bFinish,UDTSOCKET uSocket);

	BOOL DealConnectIP(STCONNPROCP *pstConn);

public:
	BOOL m_bMobile;//�ֻ�����
	int m_nLocalStartPort;
	UDTSOCKET m_ServerSocket;//��Ӧ�����socket

	DWORD m_dwConnectTime;//��¼���η������ӵ�ʱ�䣬����С���ֿ�������Ƶ��


	SOCKET m_SocketSTmp;//��ʱ�׽��֣�TCP���Ӽ�ת������������ʹ��
	SOCKADDR_IN m_addrAN;//����������ַ
	SOCKADDR_IN m_addrAL;//����������ַ
	SOCKADDR_IN m_addrTS;//ת����������ַ
	int m_nStatus;//�����ӵ�ǰ״̬
	DWORD m_dwStartTime;//�����ӹؼ���ʱ
	SOCKADDR_IN m_addressA;
	BOOL m_bPassWordErr;//�Ƿ��⵽�����֤�ظ�

	SOCKADDR_IN m_addrLastLTryAL;//�ϴγ��Թ�������̽���ַ

	BOOL m_bJVP2P;//�Ƿ�ಥ��ʽ(����������)

	BOOL m_bTURN;//�Ƿ����ת��
	BOOL m_bLocal;//�Ƿ�����̽��

	int m_nFYSTVER;//Զ������ͨЭ��汾
	
	STCONNECTINFO m_stConnInfo;
	ServerList m_SList;//P2P����ʱʹ��
	ServerList m_SListTurn;//TURN����ʱʹ��
	ServerList m_SListBak;//����������
	
	ServerList m_ISList;//�����������б�

	BOOL m_bShowInfo;//�Ƿ��Ѿ�������ʾ��������Ϣ
	//ʵ�����ӳɹ���Ҫ��������Ĳ���ֱ������
	BOOL m_bDirectConnect;
	SOCKET m_sSocket;
	char m_strIP[20];
	int m_nPort;

	int m_nNatTypeA;//���ص��������ͣ�ͨ����һ����ѯ�������õ�

	DWORD m_dwStartConnectTime;
    DWORD m_dwAddTime;
private:
	BOOL SendData(BYTE uchType, BYTE *pBuffer, int nSize);

	//
	BOOL SendPWCheck();
	int RecvPWCheck(int &nPWData);
	BOOL SendSP2P(SOCKADDR_IN addrs, int nIndex, char *pchError);
	BOOL SendSP2PTCP(SOCKADDR_IN addrs, int nIndex, char *pchError);
	int RecvS(SOCKADDR_IN addrs, int nIndex, char *pchError);
	BOOL ConnectLocalTry(int nIndex, char *pchError);
	BOOL ConnectNet(int nIndex, char *pchError);
	BOOL ConnectNetTry(SOCKADDR_IN addrs, int nIndex, char *pchError);
	BOOL SendSTURN(SOCKADDR_IN addrs, int nIndex, char *pchError);
	int RecvSTURN(int nIndex, char *pchError);
	BOOL ConnectTURN(int nIndex, char *pchError);

	//�򶴺���
	BOOL Punch(int nYSTNO,int sock,SOCKADDR_IN *addrA);
	//Ԥ�򶴺���
	void PrePunch(int sock,SOCKADDR_IN addrA);

	/*���Ӵ�����*/
	void DealNewYST(STCONNPROCP *pstConn);

	void DealWaitSerREQ(STCONNPROCP *pstConn);
	void DealWaitSerRSP(STCONNPROCP *pstConn);
	
	void DealOK(STCONNPROCP *pstConn);
	void DealFAILD(STCONNPROCP *pstConn);
	void DealNEWP2P(STCONNPROCP *pstConn);
	void DealWaitS(STCONNPROCP *pstConn);
	void DealNEWP2PL(STCONNPROCP *pstConn);
	void DealWaitLWConnectOldF(STCONNPROCP *pstConn);
	void DealWaitLPWCheck(STCONNPROCP *pstConn);
	void DealNEWP2PN(STCONNPROCP *pstConn);
	void DealWaitNWConnectOldF(STCONNPROCP *pstConn);
	void DealWaitNPWCheck(STCONNPROCP *pstConn);

	void DealNEWTURN(STCONNPROCP *pstConn);
	void DealWaitTURN(STCONNPROCP *pstConn);
	void DealRecvTURN(STCONNPROCP *pstConn);
	void DealWaitTSWConnectOldF(STCONNPROCP *pstConn);
	void DealWaitTSPWCheck(STCONNPROCP *pstConn);

	void DealWaitIndexSerREQ(STCONNPROCP *pstConn);
	void DealWaitIndexSerRSP(STCONNPROCP *pstConn);

	void DealWaitNatREQ(STCONNPROCP *pstConn);//����������ѯ����
	void DealWaitNatRSP(STCONNPROCP *pstConn);//����������ѯ����

	void StartConnThread();
	void GetSerAndBegin(STCONNPROCP *pstConn);

	void DealMakeHole(STCONNPROCP *pstConn);
#ifndef WIN32
	static void* ConnProc(void* pParam);//�����̣߳����ڴ���������
#else
	static UINT WINAPI ConnProc(LPVOID pParam);//�����̣߳����ڴ���������
#endif
	
public:
	void AddRemoteConnect(SOCKET s,char* pIP,int nPort);//ʵ�����ӳɹ��� ֪ͨ����ȥ����
//	static void WaitThreadExit(HANDLE &hThread);//ǿ���˳��߳�
private:	
	BOOL m_bPass;//�����֤�ɹ�
	CCHelpCtrl *m_pHelp;
	CCWorker *m_pWorker;

	BOOL m_bExit;
	
	BOOL m_bDisConnectShow;//�Ƿ��Ѿ���ʾ�����ӶϿ�

	::std::vector<NAT> m_NATListTEMP;//���ص�IP ��NAT �б�

    NATList m_NatList;
	
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
};

#endif // !defined(AFX_CVIRTUALCHANNEL_H__7DA71EED_A3E4_4CBD_BE53_4F1CFA4944C5__INCLUDED_)
