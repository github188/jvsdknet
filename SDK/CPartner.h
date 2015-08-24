// CPartner.h: interface for the CCPartner class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CPARTNER_H__F524111D_BF6F_4ED0_B958_A8BE57CD15BB__INCLUDED_)
#define AFX_CPARTNER_H__F524111D_BF6F_4ED0_B958_A8BE57CD15BB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"

#include <map>

#define JVN_TIMEOUTCOUNT 40//15//����JVN_TIMEOUTCOUNT������ʱ���ýڵ������⣬ֱ�ӹر�

#define JVN_TIME_WAITLRECHECK  10000//�ȴ�������֤��Ϣ
#define JVN_TIME_WAITNRECHECK  15000//�ȴ�������֤��Ϣ

#define JVN_TIME_WAIT          10000//�ȴ�

#define JVN_PTBUFSIZE   102400//�������ݿ��С
#define JVN_PACKTIMEOUT 10000

#define PNODE_STATUS_NEW           1//�½ڵ�
#define PNODE_STATUS_CONNECTING    2//������
#define PNODE_STATUS_CONNECTED     4//���ӳɹ�
#define PNODE_STATUS_WAITLCHECK    5//����̽�����֤(�跢����֤)
#define PNODE_STATUS_WAITNCHECK    7//��������֤(�跢����֤)
#define PNODE_STATUS_FAILD         8//����ʧ��
#define PNODE_STATUS_ACCEPT        9//�յ�����(��ȴ���֤)

#define PNODE_STATUS_TCCONNECTING  10//����������
#define PNODE_STATUS_TCWAITTS      11//�����������ڵȴ���������ַ
#define PNODE_STATUS_TCWAITRECHECK 12//�����������ȴ�Ԥ��֤
#define PNODE_STATUS_TCWAITPWCHECK 13//�����������ȴ������֤

#define PTYPE_N      0//�������
#define PTYPE_L      1//�������
#define PTYPE_A      2//���ر�������
#define PTYPE_NULL   3//��ʼ״̬

typedef struct STPTLI
{
	int nLinkID;
	BOOL bIsSuper;
	BOOL bIsLan2A;
	BOOL bIsLan2B;
	BOOL bIsCache;
	BOOL bIsTC;
	SOCKADDR_IN sAddr;
}STPTLI;

typedef struct STREQ
{
	unsigned int unChunkID;//����Ƭ��ţ��ñ�Ŵ�0��ʼ
	DWORD dwStartTime;//��ʼ����ʱ��
	BOOL bAnswer;//�Ƿ���й��ظ�
	BOOL bNEED;//���ڻ��崰�ڣ����裬��������
//	int nNeed;//������ ��ֵԽ�� ˵���������յ���Խ�� ��ǰԽ����
}STREQ;//������Ҫ������Щ���� �Է���������Щ����

typedef struct STMAPINFO
{
	unsigned int unBeginChunkID;//���ݿ���ʼID
	int nChunkCount;
	BYTE uchMAP[5000];//����ӳ��
	::std::map<unsigned int, DWORD> ChunkBTMap;//ĳ���ݿ鿪ʼ����ʱ���¼
	
	STMAPINFO()
	{
		unBeginChunkID = 0;
		nChunkCount = 0;
		memset(uchMAP, 0, 5000);
		ChunkBTMap.clear();
	}
}STMAPINFO;

typedef ::std::vector<STREQ> STREQS;

class CCPartnerCtrl;
class CCChannel;
class CCWorker;

class CCPartner  
{
public:
	CCPartner();
	CCPartner(STPTLI ptli, CCWorker *pWorker, CCChannel *pChannel);
	CCPartner(STPTLI ptli, CCWorker *pWorker, CCChannel *pChannel,SOCKADDR_IN addr, UDTSOCKET socket, BOOL bTCP);

	virtual ~CCPartner();

	int CheckLink(CCPartnerCtrl *pPCtrl, BOOL bNewConnect, DWORD &dwlasttime);//������������ ���л������

	int PartnerLink(CCPartnerCtrl *pPCtrl);//���л������

	BOOL BaseRecv(CCPartnerCtrl *pPCtrl);
	BOOL BaseRecvTCP(CCPartnerCtrl *pPCtrl);

	void DisConnectPartner();

	int Send2Partner(BYTE *puchBuf, int nSize, CCPartnerCtrl *pPCtrl, int ntrylimit, BOOL bComplete=TRUE);//������ͨ����
	int Send2PartnerTCP(BYTE *puchBuf, int nSize, CCPartnerCtrl *pPCtrl, int ntrylimit, BOOL bComplete=TRUE);//������ͨ����
	
	BOOL SendBM(BYTE *puchBuf, int nSize);//����BM
	BOOL SendBMD(CCPartnerCtrl *pPCtrl);//��������Ƭ
	
	BOOL SendBMDREQ(unsigned int unChunkID, CCPartnerCtrl *pPCtrl);//��������Ƭ����

	BOOL CheckREQ(unsigned int unChunkID, BOOL bTimeOut);//����Ƿ��и�����Ƭ���Ƿ���û���������ǰ����Ƭ���ǵĻ����ø����󣬲����»�����ܣ�����������ֵ

	void GetPInfo(char *pMsg, int &nSize, DWORD dwend);//��ȡ�����Ϣ
	float GetPower();//��ȡ��ǰ��������(�����ٶ�)

private:
	void SetBM(unsigned int unChunkID, int nCount, BYTE *pBuffer, int nLen, CCPartnerCtrl *pPCtrl);//���µ�ǰ���BM
	void RefreshPartner(unsigned int unChunkID, int nLen, BOOL bNULL=FALSE);//ˢ�µ�ǰ������ܲ���

	BOOL ParseMsg(CCPartnerCtrl *pPCtrl);//�������յ������ݰ�����Ҫ�����Ǽ���Ƿ������ϣ��Լ�����ճ��

	void ResetPack(BOOL bAccept=FALSE);//���������շ�����

	BOOL SendSTURN(SOCKADDR_IN addrs);
	int RecvSTURN();
	BOOL ConnectTURN();
	BOOL SendReCheck(BOOL bYST);
	BOOL SendPWCheck();
public:
	BOOL m_bTCP;
	int m_ndesLinkID;//�����ض˵�����ID��
	UDTSOCKET m_socket;//�����׽���
	SOCKET    m_socketTCP;
	SOCKET    m_socketTCtmp;
	SOCKADDR_IN m_addr;//���ض˼�¼�ĸýڵ��ַ
	int m_nstatus;//�ڵ�״̬(�½ڵ㣬�����ַ�У������ַ�ɹ������ӳɹ�����������֤������̽����֤ʧ�ܣ���������֤������ʧ��)

	BOOL m_bTryNewLink;//�Ƿ����µĳ��Խڵ�

	DWORD m_dwstart;//��ʱ��ʱ��ʼʱ��

	BOOL m_bTURNC;//�Ƿ�������ת���ڵ�
	BOOL m_bCache;//�Ƿ��Ǹ��ٻ���ڵ�
	BOOL m_bAccept;//�Ƿ��Ǳ�������
	BOOL m_bSuperProxy;//�Ƿ��ǳ����ڵ� �����ڵ������������Ȩ
	BOOL m_bProxy2;//�Ƿ��Ƕ�������ڵ� �����Ǹ��ٻ���ʱ���ֽڵ�������ȴ���Ȩ
	BOOL m_bLan2A;//�Ƿ������ص��������
	BOOL m_bLan2B;
	unsigned int m_unADDR;
	
	int m_nConnFCount;//�����ۼ�ʧ�ܴ���

	//------�����������-----------
	DWORD m_dwRUseTTime;
	int m_nLPSAvage;//�������д���/�û������д���(KB/S)(����������С,���ػظ���������,��������Ϊ��)
	int m_nRealLPSAvage;
	int m_nDownLoadTotalB;
	int m_nDownLoadTotalKB;
	int m_nDownLoadTotalMB;
	int m_nUpTotalB;
	int m_nUpTotalKB;
	int m_nUpTotalMB;
	int m_nLastDownLoadTotalB;
	int m_nLastDownLoadTotalKB;
	int m_nLastDownLoadTotalMB;
	DWORD m_dwLastInfoTime;

	DWORD m_dwLastDataTime;//���һ���յ����ݵ�ʱ��

	int m_nTimeOutCount;//��������ʱ������¼��������ֵ�رո�����

	int m_nConnTotalSpeed;
	DWORD m_dwLastConnTime;
	float m_fLastConnTotal;
	//------�������BM-------------
	STMAPINFO m_stMAP;

	STREQS m_PushQueue;
	
private:
	CCChannel *m_pChannel;
	CCWorker *m_pWorker;

	BYTE m_chdata[1000];
	BYTE *m_puchSendBuf;//������ʱ���棬���ڿ��ƶ�η���
	int m_nSendPos;//��ʱ�������ݰ��ѷ��ͳ���
	int m_nSendPackLen;//��ʱ�������ݰ��ܳ���
	DWORD m_dwSendPackLast;

	BYTE *m_puchRecvBuf;//������ʱ���棬���ڿ��ƶ�η���
	int m_nRecvPos;//��ʱ�������ݰ��ѷ��ͳ���
	int m_nRecvPackLen;//��ʱ�������ݰ��ܳ���
	DWORD m_dwRecvPackLast;
};

#endif // !defined(AFX_CPARTNER_H__F524111D_BF6F_4ED0_B958_A8BE57CD15BB__INCLUDED_)
