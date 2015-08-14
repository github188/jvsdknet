// RunLog.cpp: implementation of the CRunLog class.
//
//////////////////////////////////////////////////////////////////////

#include "RunLog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRunLog::CRunLog()
{
	m_nLanguage = JVN_LANGUAGE_ENGLISH;
	m_bWriteLog = FALSE;
	memset(m_chPath, 0, MAX_PATH);
	
#ifndef WIN32
	pthread_mutex_init(&m_ct, NULL);
#else
	InitializeCriticalSection(&m_ct); //��ʼ���ٽ���
#endif
}

CRunLog::~CRunLog()
{
#ifndef WIN32
	pthread_mutex_destroy(&m_ct);
#else
	DeleteCriticalSection(&m_ct); //�ͷ��ٽ���
#endif
}


/*�ڲ���������ȡ��ǰĿ¼*/
bool CRunLog::GetCurrentPath(char chLocalPath[MAX_PATH])
{
	if(strlen(chLocalPath) >= 3)
	{
		strcpy(m_chPath, chLocalPath);
	}
	else
	{
		int i = 0;
		int iLastSperate = 0;
	#ifndef WIN32
		std::string _exeName = "/proc/self/exe";
		size_t linksize = MAX_PATH;
		char achCurPath[MAX_PATH]={0};
		int ncount = readlink(_exeName.c_str() , achCurPath, linksize);
		if(ncount < 0 || ncount >= MAX_PATH)
		{
			memset(achCurPath,0,MAX_PATH);
		}
		else
		{
			achCurPath[ncount] = '\0';
		}
	#else
		TCHAR achCurPath[MAX_PATH];
		GetModuleFileName(GetModuleHandle(NULL), achCurPath, MAX_PATH);
	#endif  
		for (i=0; i<256; i++)
		{
		#ifndef WIN32
			if (achCurPath[i] == '/')
		#else
			if (achCurPath[i] == '\\')
		#endif  
			{
				iLastSperate = i;
			}
			else if(achCurPath[i] == '\0')
			{
				break;
			}
		}
		
		if (iLastSperate > 0 && i < 256)
		{
			achCurPath[iLastSperate] = '\0';	
		}
		else
		{
			//return;//·��������ȡ·��ʧ��
		}
		strcpy(m_chPath, achCurPath);
	}
	
#ifndef WIN32
	strcat(m_chPath,"/JVNSDKLOG/");
	
	if(mkdir(m_chPath,S_IRWXU) < 0)
	{
		//...
	}
#else
	strcat(m_chPath,"\\JVNSDKLOG\\");
	
	//�ж�Ŀ¼�Ƿ���ڣ��������򴴽�
	SECURITY_ATTRIBUTES s; 
	s.nLength=sizeof(SECURITY_ATTRIBUTES); 
	s.lpSecurityDescriptor=NULL; 
	s.bInheritHandle=TRUE; 
	if(!CreateDirectory(m_chPath, &s)) 
	{ 
		if(ERROR_ALREADY_EXISTS!=GetLastError()) 
		{ 
			memset(m_chPath, 0, MAX_PATH);
			return FALSE;//�ļ��в����ڵ�����ʧ��
		} 
	}
#endif
	return TRUE;
}

bool CRunLog::EnableLog(bool bEnable, char chLocalPath[MAX_PATH])
{
	m_bWriteLog = bEnable;
	if(!bEnable)
	{
		return TRUE;
	}
	memset(m_chPath, 0, MAX_PATH);
	return GetCurrentPath(chLocalPath);
}

/****************************************************************************************
*���ƣ�  SetRunInfo
*���ܣ�  д��־�ļ�(����)
*������  
*����ֵ��0  �ɹ�
���� ʧ��
*****************************************************************************************/
void CRunLog::SetRunInfo(int nLocalChannel, char *pchEvent, char *pchFile, int nLine,char *pchJUDT)
{
	if(!m_bWriteLog || pchFile == NULL || strlen(pchFile) > MAX_PATH || pchEvent == NULL)
	{
		return;
	}
	char chFilePathName[MAX_PATH]={0};
	char chFileName[MAX_PATH]={0};
	char chTime[MAX_PATH]={0};
	char chLine[MAX_PATH] = {0};
	char chChannel[10]={0};
	
	sprintf(chLine,"%d",nLine);
	
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
	time_t now = time(0); 
	tm *tnow = localtime(&now);
	sprintf(chFileName,"%4d_%02d.jvc",1900+tnow->tm_year,tnow->tm_mon+1);
	sprintf(chTime,"%4d-%02d-%02d %02d:%02d:%02d",1900+tnow->tm_year,tnow->tm_mon+1,tnow->tm_mday,tnow->tm_hour,tnow->tm_min,tnow->tm_sec); 
#else
	EnterCriticalSection(&m_ct);
	
	SYSTEMTIME sys; 
	GetLocalTime(&sys); 
	
	sprintf(chFileName,"%4d_%02d.jvc",sys.wYear,sys.wMonth);
	sprintf(chTime,"%4d-%02d-%02d %02d:%02d:%02d ",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond); 
#endif
	
	if(strlen(m_chPath)<=0)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return;
	}
	strcat(chFilePathName, m_chPath);
	strcat(chFilePathName, chFileName);
	
	char chInfo[JVN_RUNEVENTLEN + 2*MAX_PATH]={0};
	
	FILE *pfile=NULL;
	pfile = fopen(chFilePathName, "a+");
	if(pfile != NULL)
	{
		fseek(pfile,0,SEEK_END);
		long len=ftell(pfile);
		fseek(pfile,0,SEEK_SET);
		if(len > JVN_RUNFILELEN)
		{//�ļ��������
			fclose(pfile);
			pfile = NULL;
			pfile = fopen(chFilePathName, "w");
			if(pfile != NULL)
			{
				fclose(pfile);
				pfile = NULL;
			}

			//���´��ļ�
			pfile = fopen(chFilePathName, "a+");
			if(pfile == NULL)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return;
			}
		}

		strcat(chInfo, chTime);
		if(nLocalChannel > 0)
		{
			sprintf(chChannel,"%d",nLocalChannel);
			if(JVN_LANGUAGE_CHINESE == m_nLanguage)
			{
				strcat(chInfo, " [ͨ��:");
			}
			else
			{
				strcat(chInfo, " [CHANNEL:");
			}
			
			strcat(chInfo, chChannel);
			strcat(chInfo, "] ");
		}
		strcat(chInfo, "<");
		strcat(chInfo, pchEvent);
		if(pchJUDT != NULL)
		{
			strcat(chInfo, pchJUDT);
		}
	
		strcat(chInfo, "> [FILE:");
		strcat(chInfo, "...");//pchFile);
		strcat(chInfo, " LINE:");
		strcat(chInfo, chLine);
		strcat(chInfo, "]\r\n");

		fwrite(chInfo,sizeof(char),jvs_min(strlen(chInfo), JVN_RUNEVENTLEN),pfile);
		fclose(pfile);
	}
	
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}
