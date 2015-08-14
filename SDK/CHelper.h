// CHelper.h: interface for the CCHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHELPER_H__6D774948_AAF2_4750_B56D_542DBDDE47B5__INCLUDED_)
#define AFX_CHELPER_H__6D774948_AAF2_4750_B56D_542DBDDE47B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

#include "CChannel.h"
#include <map>

//������ ��¼�����������ĺ��룬�Լ�������ͨ����Ϣ
typedef struct DATA_LOCAL
{
	char strGroup[4];//����
	int nYST;//����
	int nPort;//��ѯ�˿�6666

	char strRemoteIP[20];//Զ�̵�ַ
	int nRemotePort;//Զ�̶˿�
	int nChannelNum;//ͨ������

	DWORD dwLastRecvTime;//���ͨ��ʱ�� ��ͨ�� ά������
	BOOL bActive;//�Ƿ�����
}DATA_LOCAL;

typedef std::map<std::string, DATA_LOCAL > LOCAL_LIST;//�����б������������� ����ͨ ->����

//������ ��¼�����������ĺ��룬�Լ�������ͨ����Ϣ
typedef struct DATA_OUTER
{
	char strGroup[4];//����
	int nYST;//����
	
	SOCKET sLocal;
	char strRemoteIP[20];//Զ�̵�ַ
	int nRemotePort;//Զ�̶˿�
	
	DWORD dwLastRecvTime;//���ͨ��ʱ�� ��ͨ�� ά������
	BOOL bActive;//�Ƿ�����
}DATA_OUTER;

typedef std::map<std::string, DATA_OUTER > OUTER_LIST;//�����б������������� ����ͨ ->����

class CCWorker;

#define JVN_REQ_NATA_A			0xB6		//��ѯ���ص���������

class CCHelper  
{
public:
	CCHelper();
	virtual ~CCHelper();

	BOOL Start(CCWorker* pWorker);//��������

	CCWorker* m_pWorker;

#ifndef WIN32
	pthread_t m_hSendThread;//�߳̾��
	pthread_t m_hReceivThread;//�߳̾��
#else
	HANDLE m_hSendThread;//�߳̾��
	HANDLE m_hReceivThread;//�߳̾��
#endif
	BOOL m_bExit;//�Ƿ��˳������߳�

	AdapterList m_IpList;//��ǰip
	void GetAdapterInfo();
#ifdef WIN32	
	static UINT WINAPI LanRecvProc(LPVOID pParam);
	static UINT WINAPI LANSSndProc(LPVOID pParam);
#else
	static void*  LanRecvProc(void* pParam);
	static void*  LANSSndProc(void* pParam);
#endif	

	pthread_mutex_t m_LocalLock;//�����㲥�б�
	LOCAL_LIST m_LocalList;//������б�

	pthread_mutex_t m_OuterLock;//��������
	OUTER_LIST m_OuterList;//������б� �������ӵĺ���

	SOCKET m_sBroadcast;//���ڹ㲥���׽��֣���������ʹ��
	DWORD m_dwLastBroadcastTime;
	int m_nLocalPort;//���ع㲥�˿�

	void SearchDevice(int* pIP = NULL,BOOL bIsMobile = FALSE);
	void Broadcast();
	void ReceiveBroadcast();

	int m_nIpNum[255];//����ʹ�õ�����
	int m_nDestPort;//�㲥�Ķ˿� 6666

	BOOL m_bisMobile;//�Ƿ����ֻ�

	void KeepActive();
	BYTE m_ucSearchData[30];//��ѯ����
	int m_nSearchLen;//��ѯ�����

	DWORD m_dwLastKeepActive;//����Ͳ�ѯʱ��

	BOOL GetLocalInfo(char* pGroup,int nYST,char* pIP,int &nPort);//��ѯ�Ƿ����Ѿ��ѵ������ߵ��豸

	void AddOkYST(char* pGroup,int nYST,char* pIP,int nPort);//���һ�����룬������ַ�ĺ���

	void AddYST(char* pGroup,int nYST,int* pIP,BOOL bIsMobile);//���һ������
	

	BOOL GetOuterInfo(char* pGroup,int nYST,SOCKET &sLocal,char* pIP,int &nPort);//��ѯ�Ѿ����ӹ��Ļ���ά�������ӵĺ���
	void UpdateTime(char* pGroup,int nYST,SOCKET sLocal,SOCKADDR_IN* addrRemote);

};
#endif // !defined(AFX_CHELPER_H__6D774948_AAF2_4750_B56D_542DBDDE47B5__INCLUDED_)
