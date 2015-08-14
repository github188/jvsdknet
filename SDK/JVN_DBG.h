#ifndef __JVN_DBG_H__
#define __JVN_DBG_H__

#ifndef WIN32
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#else
#include <process.h>
#endif
#include <vector>


#define ENABLE_DBG_MODE //ʹ��

#define RUN_MODE_PUB 0 //����������ģʽ
#define RUN_MODE_DBG 1 // ���԰�����ģʽ

#define OUT_TO_CONSOLE 2 //�������׼���
#define OUT_TO_FILE 3   // ������ļ�
#define OUT_TO_FILE_ON 4 // ������������ļ�
#define OUT_TO_NET 5    // ���������

#define OUT_NORM_MSG 0 //��ͨ��Ϣ���
#define OUT_JVC_MSG 6 // �ֿ���Ϣ���
#define OUT_JVS_MSG 7 // ������Ϣ���
#define OUT_LOG_MSG 8 // �����־��Ϣ
#define OUT_JVS_ERR 9 // ���ش�����Ϣ���
#define OUT_JVC_ERR 14 // �ֿش�����Ϣ���
#define OUT_JVS_WARN 16 // ���ؾ�����Ϣ���
#define OUT_JVC_WARN 17 //�ֿؾ�����Ϣ���
#define OUT_ON_EVENT 18 // �¼��������
#define OUT_YST_ONLINE 19 // ������Ϣ���
#define OUT_ALL_MSG 20 //������Ϣ���
#define OUT_JVS_VIDEO_IN 21 //������Ƶ���������Ϣ
#define OUT_JVS_VIDEO_SND 22// ������Ƶ���ͳ�����Ϣ
#define OUT_JVS_VIDEO_RCV 24 //���ؽ�����Ϣ
#define OUT_SOCKET_CHECK 29 // �׽��ִ�����
#define OUT_JVS_CONNECT  32// // ������Ϣ���
#define OUT_JVS_CHAT_SDN 33  // chat ��Ϣ���
#define OUT_JVS_LANSERCH 34 //�豸����������
#define OUT_JVS_BCSERCH 35 // �㲥




#define DBG_CTRL_OUTTO  10 // ��������ն�ָ��
#define DBG_CTRL_RUN     11 // ��������ģʽ
#define DBG_CTRL_MSG     12 // ���������Ϣ�����ֿ���Ϣ�������־��Ϣ�����������Ϣ�����
#define DBG_CTRL_COND     15 // ���õ��Ա���ֵ
#define DBG_CTRL_VIDEO    23 // ������Ƶ�������Ϣ cmd(1):type(1):chnum(4):clientIndex(4):enable(1):other(4)
#define DBG_CTRL_THREAD_CPUTIME 25 // ��ʾ�߳�ָ��ʱ����cpuʹ��ʱ�� cmd(1):threadid(4):time(4��


#define SHOW_JVS_CLIENT_MSG 13 // ��ʾ���ӵ����صķֿ���Ϣ
#define SHOW_JVS_THREAD_MSG 27 // ��ʾ�����߳�ID
#define SHOW_SYS_MEM 28 // ��ʾϵͳ�ڴ�ʣ��
#define SHOW_JVS_CH_CONFIG 31 // ��ʾ����������Ϣ
#define SHOW_JVS_CH_FRAMEBUFFER 30 // ��ʾ����ͨ��������Ϣ
#define CHECK_JVS_LOCK_STAT 36 //�������������� 

#define HEART_BEAT 37   //������
#ifdef WIN32
#define JVN_DBG_LOG_DIR "JVNDBG"
#else
#define JVN_DBG_LOG_DIR "/etc/conf.d/JVNDBG"
#endif
#ifndef WIN32
	#define closesocket(s)   g_dbg.closesocketdbg(s, __FILE__, __LINE__)
	#define outdbg(...)      g_dbg.out((__FILE__),(__LINE__),(__FUNCTION__),##__VA_ARGS__)  // ��ͨ��Ϣ���
	//#define outdbgto(A,...)  g_dbg.outto((__FILE__),(__LINE__),(__FUNCTION__),A,##__VA_ARGS__) //�Զ�������ն�
	#define outdbgc(A,...)   g_dbg.jvcout(A,(__FILE__),(__LINE__),(__FUNCTION__),##__VA_ARGS__) //�ֿ���Ϣ���
	#define outdbgs(A,...)   g_dbg.jvsout(A,(__FILE__),(__LINE__),(__FUNCTION__),##__VA_ARGS__) // ������Ϣ���
#else
	#define __FUNCTION__ ""
	#define outdbg      //
	#define outdbgc    //
	#define outdbgs   //
	#define  pid_t unsigned int
#endif


#define DBG_COND_VALUE   g_dbg.m_nCondValue  //���Ա���
#define RUN_MODE         g_dbg.m_nRunMode  // ����ģʽ����

#define  THREAD_TYPE_CH 1  //ͨ�����߳�
typedef struct _ThreadStat
{
	unsigned long threadId;
	char threadName[64];
	int nCH;
	unsigned long lastThreadTime;
}ThreadStat;



class CDbgInfo
{
public:
	CDbgInfo();
	~CDbgInfo();
	void out(const char* file,const int line,const char* func,char* format, ...); //
	void outto(const char* file,const int line,const char* func,int terminal,char* format, ...);//��������ն�
	void jvsout(int type,const char* file,const int line,const char* func,char* format, ...); //������Ϣ���
	void jvcout(int type,const char* file,const int line,const char* func,char* format, ...);//�ֿ���Ϣ���
	void logout(const char* file,const int line,const char* func,char* format,...); // ��־��Ϣ���
	int SendToNet(char* buffer,int len);

	int closesocketdbg(int &nSocket, char *szFile, int nLine);

	int SetRunMode(int type); //��������ģʽ��pubģʽ��dbgģʽ
	int SetOutTerminal(int terminal); //��������ն�
	int SetCondValue(int cond); // ���õ��Ա���ֵ
	int SetVideoMsg(char type,int nCh,int nClientID,char run,int nOther);// ������Ƶ��ʾ�����Ϣ
	int SetCpuInterval(pid_t tid,unsigned int time); // �����߳�cpu ʹ��ʱ��������

	int EnableOut(int type,bool run);// ʹ�����
	
	//void jvs_sleep(unsigned long time);

	void AddThreadID(const char* func,int type,void* data);
	void DeleteThreadID(const char* func);
	void ListThreadMsg(void);

	void GetProcCpuTime(void); //����
	void GetThreadCpuTime(void);
	unsigned long GetSysMemSnap(void);
	void GetThreadInProcRate(void);//����

#ifndef WIN32
	void SetClientAddr(sockaddr_in addr);
	void SetServerSocket(int sock);
	int try_get_lock(pthread_mutex_t *mutex,const char* pfile,const int line,const char* pfun);
#endif

public:
	int m_nJvsCH;// ѡ��ͨ��
	int m_nJvsTypeIn; //������Ƶ��������ѡ��
	int m_nJvsTypeSnd; // ���ط�������ѡ��
	int m_nJvsTypeRcv; // ���ؽ�������ѡ��
	int m_nClientIndex; // �ֿ�ID
	unsigned int m_nTimeInterval;// �̺߳���ʱ������������λmiao
	
private:
	unsigned long GetProcValue(char* src,int start,int num);
	unsigned long GetProcSnap(void);
	unsigned long GetThreadSnap(pid_t tid);
	unsigned long GetSysSnap(void);
	float GetRate(unsigned long rator,unsigned long total);
	void SetDbgByFile(char* src);
	void GetCurModuePath(char *strDir);
private:
	int m_nOutTerminal;// ������Ϣ����ն�
	int m_nSelectOut;// ������Ϣѡ�������
	bool m_bRun; // 
	bool m_bJvs;// �Ƿ������������
	bool m_bJvc;// �Ƿ������ֿ����
	bool m_bLog; //�Ƿ�������־���
	bool m_bJvsErr;//�Ƿ��������ش������
	bool m_bJvcErr;//�Ƿ������ֿش������
	bool m_bJvsWarn;//�Ƿ��������ش������
	bool m_bJvcWarn;//�Ƿ������ֿش������
	bool m_bJvsOnline; //�Ƿ�����������Ϣ���
	bool m_bJvsVideoIn;// ������Ƶ�����Ϣ
	bool m_bJvsVideoOut;// ������Ƶ������Ϣ
	bool m_bJvsRcv; // ���ؽ�����Ϣ
	bool m_bSocketCheck; // �׽��ּ��
	bool m_bJvsCon; //�Ƿ������������������Ϣ
	bool m_bJvsChat; //
	bool m_bJvsLanSerch; //
	bool m_bJvsBCSearch;
	::std::vector<ThreadStat> m_threads;
	

#ifndef WIN32
	sockaddr_in m_clientAddr;
	int m_nSock;
#endif
	
	pid_t m_pid;
	unsigned long m_lastCpuTime;
	unsigned long m_lastProcTime;
	pid_t m_tid;

#ifndef WIN32
	pthread_t m_hDbgHander;
	pthread_mutex_t m_ctThread;
#else
	
#endif
public:
	volatile int m_nCondValue;// �ڲ�������
	volatile int m_nRunMode; // ����ģʽ�����Ի򷢲�
};

extern CDbgInfo g_dbg;

#endif                                                                                                                                                                   
  
