// CPartnerCtrl.h: interface for the CCPartnerCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPARTNERCTRL_H__E831B592_259C_497D_9F0A_04DB99E3BC3E__INCLUDED_)
#define AFX_CPARTNERCTRL_H__E831B592_259C_497D_9F0A_04DB99E3BC3E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CPartner.h"  

#define PT_LCONNECTMAX 5//10//5//�����������������(������������
#define PT_NCONNECTMAX 5//10//5//�����������������(������������)
#define PT_LACCEPTMAX  10//10//5//���ؽ�����������������
#define PT_NACCEPTMAX  10//10//5//���ؽ�����������������

#define PT_TIME_CNNEWPT   5000//���������µĻ������ 
#define PT_TIME_DISBADPT  120000//�Ͽ����������
#define PT_TIME_REQPTLIST 60000//���ڻ�ȡ���µĻ���б�
#define PT_TIME_NODATA    20000//60000//������ʱ��δ�յ����ݣ���Ϊ�Ǽ���ڵ�

#define JVC_CLIENTS_BM3 3

class CCChannel;
class CCWorker;


typedef ::std::vector<STPTLI> PartnerIDList;//���ID�б�
typedef ::std::vector<CCPartner*> PartnerList;//����б�
class CCPartnerCtrl  
{
public:
	CCPartnerCtrl();
	virtual ~CCPartnerCtrl();

	BOOL SetPartnerList(PartnerIDList partneridlist);//�յ�����б����±����б�

	void GetPartnerInfo(char *pMsg, int &nSize, DWORD dwend);//��ȡ���״̬��Ϣ
	void ClearPartner();
	void DisConnectPartners();

	BOOL CheckAndSendChunk(unsigned int unChunkID, BOOL bTimeOut);//�ж��Ƿ�����Ч���ṩ��

	void SetReqStartTime(BOOL bA, unsigned int unChunkID, DWORD dwtime);
	//------------------------------------
	void AddTurnCachPartner();//��Ҫ����ת������ �����жϴ���
	void AcceptPartner(UDTSOCKET socket, SOCKADDR_IN clientaddr, int nDesID, BOOL bTCP=FALSE);//���յ�һ��������� �����жϴ���
	BOOL CheckPartnerLinks(BOOL bExit);//������������ ���л������

	BOOL PartnerLink(BOOL bExit);//���л������

	BOOL RecvFromPartners(BOOL bExit, HANDLE hEnd);
	
	BOOL SendBM2Partners(BYTE *puchBuf, int nSize,BOOL bExit, HANDLE hEnd);//��ȡBM����ȫ���򲿷ֻ�鷢��BM
	BOOL SendBMDREQ2Partner(STREQS streqs, int ncount, BOOL bExit);

	void CheckIfNeedSetBuf(unsigned int unChunkID, int ncount, DWORD dwCTime[10], BOOL bA=FALSE);

	void ResetProxy2();
	//------------------------------------

	void CheckGarbage();//��Ч�ڵ�����

	BOOL SendBMD();

	static BOOL CheckInternalIP(const unsigned int ip_addr);
	static int tcpreceive(SOCKET m_hSocket,char *pchbuf,int nsize,int ntimeoverSec);
	static int tcpsend(SOCKET m_hSocket,char *pchbuf,int nsize,int ntimeoverSec, int &ntimeout);
	static int connectnb(SOCKET s,struct sockaddr * to,int namelen,int ntimeoverSec);

private:
	DWORD m_dwLastREQListTime;
	DWORD m_dwLastTryNewLTime;
	DWORD m_dwLastTryNewNTime;
	DWORD m_dwLastDisOldLTime;
	DWORD m_dwLastDisOldNTime;

//	::std::map<unsigned int, unsigned int> m_PTaddrMap;
public:
	PartnerList m_PList;//�������л���б�
	PartnerList m_PListTMP;//���л���б���
	PartnerList m_PLINKList;//���д������

	CCChannel *m_pChannel;
	CCWorker *m_pWorker;

	BOOL m_bClearing;//�ӿ������ٶ�

	DWORD m_dwLastPTData;//�ϴεõ�������ݵ�ʱ�䣬�����ʱ��ò���������ݣ���Ҫ���Ǵ�������ȡ

#ifndef WIN32
	pthread_mutex_t m_ct;
	pthread_mutex_t m_ctCONN;
	pthread_mutex_t m_ctPTINFO;
#else
	CRITICAL_SECTION m_ct;
	CRITICAL_SECTION m_ctCONN;
	CRITICAL_SECTION m_ctPTINFO;
#endif

	unsigned int m_unChunkIDNew;//���µ����ݿ�ID
	unsigned int m_unChunkIDOld;//��ɵ����ݿ�ID

	::std::map<unsigned int, DWORD> m_ChunkBTMap;//ĳ���ݿ鿪ʼ����ʱ���¼
};

#endif // !defined(AFX_CPARTNERCTRL_H__E831B592_259C_497D_9F0A_04DB99E3BC3E__INCLUDED_)
