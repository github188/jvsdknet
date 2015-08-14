//?//----------------------------------------------------------------------
// Thread.h
// �߳����
//
// ���ߣ�����ͨ Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#pragma once

#include "JRCDef.h"

namespace JMS
{
	/**
	* @class Mutex
	* @brief ������
	*/
	class Mutex
	{
	public:
		Mutex();
		~Mutex();

	public:
		/**
		* @brief ����ͬ����
		*/
		void Lock();

		/**
		* @brief ��������ͬ����
		* @return ���ز������
		*/
		bool TryLock();

		/**
		* @brief ����ͬ����
		*/
		void Unlock();

	private:
#ifdef WIN32
		CRITICAL_SECTION m_cs;		//�ٽ������
#else 
		pthread_mutex_t m_cs;		//�ٽ������
#endif
	};

	/**
	* @class Event
	* @brief ͬ���¼�
	*/
	class Event
	{
	public:
		Event(bool bManualReset, bool bInitState);
		~Event();

	public:
		/**
		* @brief �����¼�
		* @return ���ز������
		*/
		bool Set();

		/**
		* @brief ��λ�¼�
		* @return ���ز������
		*/
		bool Reset();

		/**
		* @brief �ȴ��¼�
		* @return ���ز������
		*/
		bool Wait();

		/**
		* @brief ��ʱ�ȴ�
		* @param nTimeout ��ʱʱ��(����)
		* @return �ɹ�����������ʧ�ܷ��ظ�������ʱ����0
		*/
		int TimedWait(int nTimeout);

	private:
#ifdef WIN32
		HANDLE m_hHandle;			//ͬ���¼�������
#else
		bool m_bState;			//��ǰ״̬
		bool m_bManualReset;		//�Զ���λ
		pthread_mutex_t m_hMutex;	//�ٽ������
		pthread_cond_t m_hCond;		//�ź������
#endif
	};

	/**
	* @class RWLock
	* @brief ��д��
	*/
	class RWLock
	{
	public:
		RWLock();
		~RWLock();

	public:
		/**
		* @brief ������
		*/
		void ReadLock();

		/**
		* @brief ���Զ�����
		* @return ���ز������
		*/
		bool TryReadLock();

		/**
		* @brief ������
		*/
		void ReadUnlock();

		/**
		* @brief д����
		*/
		void WriteLock();

		/**
		* @brief ����д����
		* @return ���ز������
		*/
		bool TryWriteLock();

		/**
		* @brief д����
		*/
		void WriteUnlock();

	private:
#ifdef LINUX
		pthread_rwlock_t m_cs;			//�����
#else
		volatile LONG m_lCount;			//��������
		volatile LONG m_lDirect;		//����״̬,1��ʾд,0��ʾ��
		HANDLE m_hFinishEvent;			//�¼����
		CRITICAL_SECTION m_hStartLock;	//ͬ����
#endif
	};

	/**
	* @class Semaphore
	* @brief �ź���
	*/
	class Semaphore
	{
	public:
		Semaphore(int nInitCount);
		~Semaphore();

	public:
		/**
		* @brief �ȴ��ź���
		* @return ���ز������
		*/
		bool Wait();

		/**
		* @brief �ͷ��ź���
		* @return ���ز������
		*/
		bool Post();

	private:
#ifdef WIN32
		HANDLE m_hHandle;		//�ź������
#else
		sem_t m_sem;			//�ź������
#endif
	};

	/**
	* @class Thread
	* @brief �߳���
	*/
	class Thread
	{
	public:
		typedef void (*ThreadProc_t)(void *pParam);

#ifdef WIN32
		typedef void ThreadProcRetType;
#else
		typedef void* ThreadProcRetType;
#endif

		Thread();
		~Thread();

	public:
		/**
		* @brief �����������߳�
		* @param proc �̴߳�����
		* @param pParam �û�����ָ��
		* @return ����ִ�н��
		*/
		bool Run(ThreadProc_t proc, void *pParam = NULL);

		/**
		* @brief �ȴ�����
		* @param dwTimeOut ��ʱʱ��(ms)
		* @param bKillWhenTimeout �ȴ���ʱ���Ƿ�ɱ���߳�
		* @return �߳������˳�����true,�ȴ���ʱ����false
		*/
		bool WaitForEnd(uint32_t dwTimeOut = -1);

		/**
		* @brief ɱ���߳�
		* @return ����ִ�н��
		*/
		bool KillThread();

		/**
		* @brief �߳��Ƿ���������
		* @return �����߳��Ƿ���������
		*/
		bool IsRunning();

		/**
		* @brief ���߱��߳�
		* @param dwMiliSec ����ʱ��(ms)
		*/
		static void Sleep(uint32_t dwMiliSec);

		/**
		* @brief ��ȡ�߳�ID
		* @return ���ص�ǰ�߳�ID
		*/
		static int GetThreadID();

	private:
		static ThreadProcRetType AriesThreadProc(void *pParam);

	protected:
		ThreadProc_t m_pThreadProc;		//�̺߳���ָ��
		void *m_pParam;					//�û�����ָ��

#ifdef WIN32
		HANDLE m_hHandle;				//�߳̾��
#else
		pthread_t m_hHandle;			//�߳̾��
#endif
	};
}
