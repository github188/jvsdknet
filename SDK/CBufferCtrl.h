// CBufferCtrl.h: interface for the CCBufferCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CBUFFERCTRL_H__1D463487_9BA1_444B_88F8_999E89DBFC50__INCLUDED_)
#define AFX_CBUFFERCTRL_H__1D463487_9BA1_444B_88F8_999E89DBFC50__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JVNSDKDef.h"
#include "RunLog.h"
#include "CPartner.h"
#include <map>
#include <algorithm>

#define JVN_CHUNK_PRIVATE  20//���������ݿ���Ŀ����Щ�鲻���⹫���������ڲ�����Щ̫��������
#define JVN_CHUNK_LEN      16384//ÿ�����ݿ�Ĵ�С <<<ע�����ֿر���ͳһ>>>
//#define JVN_BUF_MINM       10240000//�ಥ��ʽʱ �ڴ淽ʽ��Ҫ����С�ڴ��С
#define JVN_BUF_MINS       4096000//800000//819200//��ͨ��ʽʱ ��Ҫ����С�ڴ��С
#ifndef WIN32
	#define JVN_BUF_MINM       1024000//10240000//�ಥ��ʽʱ �ڴ淽ʽ��Ҫ����С�ڴ��С
#else
	#define JVN_BUF_MINM       4096000//8192000//10240000//�ಥ��ʽʱ �ڴ淽ʽ��Ҫ����С�ڴ��С
#endif

#define JVN_REQBEGINMAX  100//Ĭ����������ֵ����ɵ���Щ���ݿ鲻�������󣬷�ֹ�������ڵ�������˷�
#define JVN_PLAYIMD   40//Ĭ��Σ�շ�ֵ�������������б���������˷ѵ�Σ�գ���Ҫ��������
#define JVN_PLAYNOW   20//50//���巧ֵ�������������ٸ����ݿ鿪ʼ���ţ�������ָ�����Σ�����󣬸�ֵ��Ч

#define JVN_FIRSTREQ  50//��һ���������ݵ������ʼ��

#define JVN_TIME_FIRSTWAIT 15000//��ʼ�������ʱ��
#define JVN_TIME_MAXWAIT   60000//�����е�����󻺳�ʱ��

#define BM_CHUNK_TIMEOUT5   5000//��������Ƭ��ʱʱ��
#define BM_CHUNK_TIMEOUT10  10000//��������Ƭ��ʱʱ��
#define BM_CHUNK_TIMEOUT20  20000//��������Ƭ��ʱʱ��
#define BM_CHUNK_TIMEOUT40  40000//��������Ƭ��ʱʱ��
#define BM_CHUNK_TIMEOUT60  60000//��������Ƭ��ʱʱ��
#define BM_CHUNK_TIMEOUT100 100000//��������Ƭ��ʱʱ��
#define BM_CHUNK_TIMEOUT120 120000//��������Ƭ��ʱʱ��
#define BM_CHUNK_TIMEOUT240 240000//��������Ƭ��ʱʱ��
#define BM_ARRIVETIMEOUT   60000//BM��ʱʱ��

//////////////////////////////////////////////////////////////////////////�ɰ����
#define BUFHEADSIZE 13
typedef struct 
{
	BYTE uchType;//int nType;//���ͣ�I P B
	long lStartPos;
	int nLen;
	int nNeedDelay;
}STBUFNODE;
//////////////////////////////////////////////////////////////////////////

typedef struct STFRAMEHEAD
{
	BYTE  uchType;//�ؼ�֡����ͨ��Ƶ֡����Ƶ֡
	int   nSize;  //֡���س���
	unsigned int unFTime;//��ǰ֡��������е����ʱ��
	int   nFrameID;//��Ƶ֡��ţ������֡���У�I֡���Ϊ0�������ڲ���ʱ������Ծʱ��
}STFRAMEHEAD;//֡ͷ

typedef struct STCHUNKHEAD
{
	unsigned int unChunkID;//����Ƭ��ţ���1��ʼ ˳���ۼ�
	long lWriteOffset;//Ŀǰ������ʵ�ʴ��볤��,���´�д��λ��,�����lBeginPos.Ҳ�ǵ�ǰһЩ������ݳ���
	BOOL bHaveI;//������Ƭ���Ƿ��йؼ�֡
	int nIOffset;//���йؼ�֡ʱ��һ���ؼ�֡��λ�ã������lBeginPos
	int nNOffset;//��һ��֡(�ؼ�֡������֡)��λ�ã������lBeginPos
	unsigned int unCTime;//��ǰ����Ƭ��������е����ʱ��
}STCHUNKHEAD;//���ݿ�ͷ��Ϣ

typedef struct STPLAYHEAD
{
	BOOL bPlayed;//��ǰ���ݿ��Ƿ��Ѿ����Ź�
	BOOL bRateOK;//��ǰ���ݿ��Ƿ��Ѿ�������
	long lNextOffset;//��ǰ���ݿ����´���Ҫ���ŵ���ʼλ��(����ĳ��֡����ʼλ��)
}STPLAYHEAD;

typedef struct STCHUNK
{
	STCHUNKHEAD stHead;//���ݿ�ͷ
	STPLAYHEAD stPHead;//���ſ���

	long lBeginPos;//ÿ��chunk���ܻ������ʼλ��,������ܻ���
	BOOL bHaveData;//�����ݿ��Ƿ�����
	BOOL bNeedPush;//�Ƿ���Ҫ����
	STCHUNK()
	{
		stHead.unChunkID = 0;
		stHead.bHaveI = FALSE;
		stHead.nIOffset = -1;
		stHead.nNOffset = -1;
		stHead.lWriteOffset = 0;
		stHead.unCTime = 0;
	
		stPHead.bPlayed = FALSE;
		stPHead.bRateOK = FALSE;
		stPHead.lNextOffset = -1;

		lBeginPos = 0;
		bHaveData = FALSE;
		bNeedPush = FALSE;
	}
	
	void ResetChunk()
	{
		stHead.unChunkID = 0;
		stHead.bHaveI = FALSE;
		stHead.nIOffset = -1;
		stHead.nNOffset = -1;
		stHead.lWriteOffset = 0;
		stHead.unCTime = 0;

		stPHead.bPlayed = FALSE;
		stPHead.bRateOK = FALSE;
		stPHead.lNextOffset = -1;
		bHaveData = FALSE;
		bNeedPush = FALSE;
	}
}STCHUNK;//һ������Ƭ��λ(һ��֡����)

typedef struct STBM
{
	BYTE *pBuffer;//����ʵ�ʴ洢����
	::std::map<int, STCHUNK> ChunksMap;//���ݿ鼯��
	
	BOOL bBMChanged;//����Ƭ�Ƿ��б仯
	
	int *pnChunkIndex;//������ѭ������ʱֻ�л�����ֵ��ά�������˳��źͻ�ַ�Ķ�Ӧ��ϵ
	                  //���磺pnChunkIndex[0] ���������Զ���������ݿ飬�������ݿ�Ļ�ַ��ѭ�����ǵģ��Ƕ�̬�仯�ġ�
	
	int nOldestChunk;//��ǰ�������������ݿ��˳���ַ����������ID�������Ϻ�Ϊm_nChunkNUM-1������Ӧ���ǳ�ʼ���ݲ���������

	int nNeedPlayIndex;//�´���Ҫ���ŵ����ݿ��ַλ��
	unsigned int unNeedPlayID;//�´���Ҫ���ŵ����ݿ�ID
	BOOL bNeedI;//�Ƿ���ҪѰ���¸��ؼ�֡
	BOOL bNeedA;//Ѱ�ҹؼ�֡�������Ƿ�����Ƶ֡
	unsigned int unPlayedID;//�Ѳ���������ID

	STBM()
	{
		pBuffer = NULL;
		bBMChanged = FALSE;
		ChunksMap.clear();
		pnChunkIndex = NULL;
		nOldestChunk = -1;
	
		nNeedPlayIndex = -1;
		unNeedPlayID = 0;
		bNeedI = TRUE;
		bNeedA = FALSE;
		unPlayedID = 0;
	}
}STBM;//���浥λ(�ڴ潻���ĵ�λ)

typedef struct STSTAT
{
	char chBegainTime[MAX_PATH];//��ʼʱ��
	char chEndTime[MAX_PATH];//����ʱ��
	DWORD dwBeginTime;
	DWORD dwEndTime;
	DWORD dwTimeTotal;
	unsigned int unWaitCount;//�����ܴ���
	unsigned int unWaitTimeTotal;//������ʱ��
	unsigned int unWaitTimeAvg;//����ƽ��ʱ��
	unsigned int unBufErrCount;//�����������ݵ����ݿ��ܸ���
	unsigned int unBufJumpCount;//��Ϊ���ݲ����������������ݿ�����
	unsigned int unWaitFrequence;//����Ƶ��

	unsigned int unDelayCount;

	unsigned int unBeginReqID;
	unsigned int unFirstRcvID;
	unsigned int unBeginPlayID0;
	unsigned int unBeginPlayID1;

	unsigned int unNobufCount;
	unsigned int unRepeatCount;
	unsigned int unNotfindCount;

	unsigned int unNoDataCount;
	unsigned int unNoICount;

	unsigned int unbeginid;
	DWORD dwbegin;
	DWORD dwend;
	STSTAT()
	{
		memset(chBegainTime, 0, MAX_PATH);
		memset(chEndTime, 0, MAX_PATH);
		unWaitCount = 0;
		unWaitTimeTotal = 0;
		unWaitTimeAvg = 0;
		unBufErrCount = 0;
		unBufJumpCount = 0;
		unDelayCount = 0;

		dwBeginTime = 0;
		dwEndTime = 0;
		dwTimeTotal = 0;
		unWaitFrequence = 0;

		unBeginReqID = 0;
		unFirstRcvID = 0;
		unBeginPlayID0 = 0;
		unBeginPlayID1 = 0;

		unNobufCount = 0;
		unRepeatCount = 0;
		unNotfindCount = 0;

		unNoDataCount = 0;
		unNoICount = 0;
	}

	void begin(unsigned int uncurid)
	{
		if(uncurid != unbeginid)
		{//���ظ����壬��ʼ�㲻����˵����ͬһ�λ���
//			finish();//�����ϴλ��壬���ﲻ��ִ�У���Ϊ��������Ĺ��������ɵ����̣�����ֻ���𻺳壬û�������������������̵Ĳ�Ӧ�ü�ʱ

			unbeginid = uncurid;
		#ifndef WIN32
			struct timeval start;
			gettimeofday(&start,NULL);	
			dwbegin = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
		#else
			dwbegin = GetTickCount();
		#endif
		}
	}
	void finish()
	{
		if(unbeginid > 0 && dwbegin > 0)
		{
			unWaitCount++;
			
		#ifndef WIN32
			struct timeval start;
			gettimeofday(&start,NULL);	
			dwend = (DWORD)(start.tv_sec*1000 + start.tv_usec/1000);
		#else
			dwend = GetTickCount();
		#endif
			if(dwend >= dwbegin)
			{
				unWaitTimeTotal += ((dwend-dwbegin)/1000);
			}
		}

		unbeginid = 0;
		dwbegin = 0;
		dwend = 0;
	}
	void jump(unsigned int unplayedid, unsigned int unneedid)
	{
		if(unneedid > (unplayedid+1))
		{
			int ntmp = unneedid - unplayedid - 1;
			unBufJumpCount += ntmp;
		}
	}

}STSTAT;//����Ч������


class CCBaseBufferCtrl  
{
public:
	CCBaseBufferCtrl();
	CCBaseBufferCtrl(BOOL bTURN);
	virtual ~CCBaseBufferCtrl();
	
	/*�ಥ*/
	virtual BOOL ReadPlayBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize, int &nBufferRate){return FALSE;};
	virtual BOOL WriteBuffer(STCHUNKHEAD stHead, BYTE *pBuffer){return FALSE;};
	
	virtual BOOL GetBM(BYTE *pBuffer, int &nSize){return FALSE;};//����Ƿ�BM�б仯�����Ƿ��յ�chunk���ݣ��ǵĻ��ɷ���BM
	virtual BOOL ReadChunkLocalNeed(STREQS &stpullreqs, int &ncount){return FALSE;};//������chunk��δ��ʱ����������ʱ�ĸ�λ��������
	virtual BOOL ReadREQData(unsigned int unChunkID, BYTE *pBuffer, int &nLen){return FALSE;};
	virtual void AddNewBM(unsigned int unChunkID, int ncount, DWORD dwCTime[10], unsigned int &unNewID, unsigned int &unOldID, BOOL bA=FALSE){};//�����BM
	
	virtual BOOL WaitHighFrequency(){return FALSE;};//�Ƿ񻺳����Ƶ���������ж�Ч���û��ı�׼

	virtual void ClearBuffer(){};
	DWORD JVGetTime();

	virtual void SetPlayed(int ncurindex){};
	virtual BOOL ReadDataMore(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead){return FALSE;};
	virtual void ReadDataOnce(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead){};
	virtual int GetBufferPercent(){return 0;};
	virtual int NextIndex(int ncurindex){return 0;};


	/*����*/
	virtual BOOL WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, unsigned int unIID, int nFID, unsigned int unFTime){return FALSE;};
	virtual BOOL WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, int nNeedDelay){return FALSE;};
	virtual BOOL ReadBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize){return FALSE;};
	
public:
	int m_nFrameTime;//֡ʱ����
	int m_nFrames;//�ؼ�֡����
	
	unsigned int m_unMaxWaitTime;//�������󻺳�ʱ��

	BOOL m_bFirstWait;//�Ƿ񻹴�δ���Ź�
	DWORD m_dwBeginBFTime;//���ӽ���ʱ��

	CRunLog *m_pLog;
	int m_nChannel;
 
	BOOL m_bTURN;
	BOOL m_bJVP2P;
	BOOL m_bLan2A;
	int m_nClientCount;

	BOOL m_bFirstBMDREQ;//�Ƿ��ǵ�һ���������ݣ�����ȷ��������ʼ��
	BOOL m_bNoData;//��ǰ�����Ƿ��յ����ݣ�����ȷ��������ʼ��

	long m_lTLENGTH;//�������ܳ���
	int m_nChunkNUM;//���Ի�������ݿ�����(���ݻ����ܳ��ȼ�����)
	
#ifndef WIN32
	pthread_mutex_t m_ct;
#else
	CRITICAL_SECTION m_ct;
#endif

	DWORD m_dwLastPlayTime;//�ϴβ��ž���ʱ��
	DWORD m_dwLastFTime;//��һ֡���ʱ��
	DWORD m_dwLastCTime;//��һ����Ƭ���ʱ��
	DWORD m_dwSysNow;
	unsigned int m_unChunkTimeSpace;//���µ���������Ƭʱ�����ڶ�̬����ʱ��
	int m_nLastWaitCC;//�ϴμ�����Ļ�������Ƭ����,�����ڻ���ʱȷ����Ҫ�����������Ƭ
	DWORD m_dwLastJump;//��һ��ǿ����֡��ʱ��

	STSTAT m_stSTAT;//Ч��ͳ��

	int m_nRate;//���»������
	int m_nReqBegin;//�������ݿ���Ŀ����������
	int m_nPLAYIMD;//�������ŷ�ֵ
};

class CCMultiBufferCtrl:public CCBaseBufferCtrl  
{
public:
	CCMultiBufferCtrl();
	CCMultiBufferCtrl(BOOL bTURN, BOOL bJVP2P=TRUE);
	virtual ~CCMultiBufferCtrl();
	
	BOOL ReadPlayBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize, int &nBufferRate);
	BOOL WriteBuffer(STCHUNKHEAD stHead, BYTE *pBuffer);
	
	BOOL GetBM(BYTE *pBuffer, int &nSize);//����Ƿ�BM�б仯�����Ƿ��յ�chunk���ݣ��ǵĻ��ɷ���BM
	BOOL ReadChunkLocalNeed(STREQS &stpullreqs, int &ncount);//������chunk��δ��ʱ����������ʱ�ĸ�λ��������
	BOOL ReadREQData(unsigned int unChunkID, BYTE *pBuffer, int &nLen);
	void AddNewBM(unsigned int unChunkID, int ncount, DWORD dwCTime[10], unsigned int &unNewID, unsigned int &unOldID, BOOL bA=FALSE);//�����BM
	
	BOOL WaitHighFrequency();//�Ƿ񻺳����Ƶ���������ж�Ч���û��ı�׼

	void ClearBuffer();

	void SetPlayed(int ncurindex);
	BOOL ReadDataMore(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead);
	void ReadDataOnce(BYTE *pBuffer, int nchunkoffset, int nframeoffset, STFRAMEHEAD stframehead);
	int GetBufferPercent();
	int NextIndex(int ncurindex);

private: 
	STBM m_stBM;//���ػ���
};

typedef struct STBNODE
{
	BYTE uchType;//int nType;//���ͣ�I P B
	long lStartPos;
	int nLen;
	unsigned int unIID;
	int nFID;
	unsigned int unFTime;//��ǰ֡��������е����ʱ��
	STBNODE()
	{
		unIID = 0;
		uchType = 0;
		lStartPos = 0;
		nLen = 0;
		nFID = 0;
		unFTime = 0;
	}
}STBNODE;//TCP TURN���ݽڵ� ��������
typedef struct STBTMP
{
	unsigned int unIID;//�ؼ�֡���
	long lWritePos;//�����Ӧ��дָ��
	long lReadPos;//���黺��ֱ��Ӧ�Ķ�ָ��
	int nTotalFrames;
	int nTotalI;//֡���и���
}STBTMP;//single ֡������������

class CCSingleBufferCtrl:public CCBaseBufferCtrl  
{
public:
	CCSingleBufferCtrl();
	CCSingleBufferCtrl(int nLChannel,BOOL bTURN);
	virtual ~CCSingleBufferCtrl();
	
	BOOL ReadBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize);//��ȡһ֡����
	BOOL WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, unsigned int unIID, int nFID, unsigned int unFTime);
	
	void ClearBuffer();
public:
	CRunLog *m_pLog;
private: 
	long m_lSLENGTH;//��������������
	
	BYTE *m_pBuffer[2];//���黺��
	STBTMP m_stTMP[2];
	
	bool m_bFirstWrite;//�״�д����
	bool m_bFirstRead;//�״ζ�����
	
	int m_nWait;//���л��������
	int m_nSend;//���ͻ��������
	int m_nWrite;//��������д��Ļ��������
	
	STBNODE m_stNode;

	int m_nSpeedup;//���ٲ���
	
	//��ʱ���
	DWORD m_dStart;
	DWORD m_dEnd;
	DWORD m_dTimeUsed;
	
	DWORD m_dwLastFTime;
	DWORD m_dwLastPlayTime;
	int m_nLocalChannel;
	
	DWORD m_dwLastWTime;//ʵ��д������ʱ��
	DWORD m_dwLastWFTime;//��֡����һ֮֡�������ʱ��
	int m_nHEADSIZE;
	
};

//////////////////////////////////////////////////////////////////////////�ɰ����
class CCOldBufferCtrl:public CCBaseBufferCtrl  
{
public:
	CCOldBufferCtrl();
	CCOldBufferCtrl(int nLChannel,BOOL bTURN);
	virtual ~CCOldBufferCtrl();
	
	BOOL ReadBuffer(BYTE &uchType, BYTE *pBuffer, int &nSize);
	//��ȡһ֡����
	
	BOOL WriteBuffer(BYTE uchType, BYTE *pBuffer, int nSize, int nNeedDelay);
	
	void ClearBuffer();
public:
	CRunLog *m_pLog;
private: 
	int m_nWaitFrame;//�����Ӻ�֡��
	long m_lLength;//��������������
	
	BYTE *m_pBuffer[2];//���黺��
	
	int m_nWriteTotal[2];//��д���֡��
	int m_nSendTotalL[2];//�߼��ѷ��͵�֡��
	
	bool m_bFirstWrite;//�״�д����
	bool m_bFirstRead;//�״ζ�����
	
	int m_nWait;//���л��������
	int m_nSend;//���ͻ��������
	int m_nWrite;//��������д��Ļ��������
	
	long m_lWritePos;//�����Ӧ��дָ��
	long m_lReadPos;//���黺��ֱ��Ӧ�Ķ�ָ��
	
	STBUFNODE Node;
	
	//��ʱ���
	DWORD m_dStart;
	DWORD m_dEnd;
	DWORD m_dTimeUsed;
	
	BOOL m_bOver;//���������

	int m_nLocalChannel;
	
};

//////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_CBUFFERCTRL_H__1D463487_9BA1_444B_88F8_999E89DBFC50__INCLUDED_)
