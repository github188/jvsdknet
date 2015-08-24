// CPartnerCtrl.cpp: implementation of the CCPartnerCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "CPartnerCtrl.h"
#include "CChannel.h"
#include "CWorker.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCPartnerCtrl::CCPartnerCtrl()
{
	m_PList.reserve(30);
	m_PList.clear();

	m_PLINKList.reserve(30);
	m_PLINKList.clear();

	m_bClearing = FALSE;

	m_dwLastPTData = CCWorker::JVGetTime();

	m_pChannel = NULL;

	m_unChunkIDNew = 0;
	m_unChunkIDOld = 0;
	m_ChunkBTMap.clear();
	
	m_dwLastREQListTime = 0;
	m_dwLastTryNewLTime = 0;
	m_dwLastTryNewNTime = 0;
	m_dwLastDisOldLTime = 0;
	m_dwLastDisOldNTime = 0;
#ifndef WIN32
	pthread_mutex_init(&m_ct, NULL); //��ʼ���ٽ���
	pthread_mutex_init(&m_ctCONN, NULL); //��ʼ���ٽ���
	pthread_mutex_init(&m_ctPTINFO, NULL); //��ʼ���ٽ���
#else
	InitializeCriticalSection(&m_ct); //��ʼ���ٽ���
	InitializeCriticalSection(&m_ctCONN); //��ʼ���ٽ���
	InitializeCriticalSection(&m_ctPTINFO); //��ʼ���ٽ���
#endif
}

CCPartnerCtrl::~CCPartnerCtrl()
{
	ClearPartner();

#ifndef WIN32
	pthread_mutex_destroy(&m_ctPTINFO); //�ͷ��ٽ���
	pthread_mutex_destroy(&m_ctCONN); //�ͷ��ٽ���
	pthread_mutex_destroy(&m_ct);//�ͷ��ٽ���
#else
	DeleteCriticalSection(&m_ctPTINFO); //�ͷ��ٽ���
	DeleteCriticalSection(&m_ctCONN); //�ͷ��ٽ���
	DeleteCriticalSection(&m_ct); //�ͷ��ٽ���
#endif
}

BOOL CCPartnerCtrl::SetPartnerList(PartnerIDList partneridlist)
{
//OutputDebugString("ptcrl setplist.....0\n");
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
//OutputDebugString("ptcrl setplist.....1\n");
	//������б�;��б�Ĳ��죬����ĶϿ����ӣ��Ѵ��ڵı�����û�е����
	int ncountnew = partneridlist.size();
	int ncountl = m_PList.size();
	BOOL bfind=FALSE;
	
	//�������ڵ�
	int i=0;
	int j=0;
	for(j=0; j<ncountl; j++)
	{
		bfind = FALSE;
		if(m_PList[j] == NULL || (m_PList[j]->m_ndesLinkID <= 0 && !m_PList[j]->m_bTURNC) || m_PList[j]->m_bTURNC)
		{
			continue;
		}

		//��鱾��ĳ���ڵ��Ƿ��������б���
		for(i=0; i<ncountnew; i++)
		{
			if(m_PList[j]->m_ndesLinkID == partneridlist[i].nLinkID)
			{
				bfind = TRUE;
				break;
			}
		}
		
		if(!bfind)
		{//���������
			m_PList[j]->m_nstatus = PNODE_STATUS_FAILD;
			m_PList[j]->DisConnectPartner();
			m_PList[j]->m_ndesLinkID = 0;
			continue;
		}

		//����ظ��Ľڵ�
		for(i=j+1; i<ncountl; i++)
		{
			if(m_PList[j]->m_ndesLinkID == m_PList[i]->m_ndesLinkID)
			{//�ظ��ڵ�
				if(m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD)
				{//ɾ�����нڵ�
					m_PList[i]->m_ndesLinkID = 0;
					continue;
				}
				else if(m_PList[j]->m_nstatus == PNODE_STATUS_NEW || m_PList[j]->m_nstatus == PNODE_STATUS_FAILD)
				{
					m_PList[j]->m_ndesLinkID = 0;
					break;
				}
				else
				{
					m_PList[i]->m_nstatus = PNODE_STATUS_FAILD;
					m_PList[i]->DisConnectPartner();
					m_PList[i]->m_ndesLinkID = 0;
					continue;
				}
			}
		}
	}
	
	//��Ч�ڵ����
	ncountl = m_PList.size();
	for(i=0; i<ncountnew; i++)
	{
		bfind = FALSE;
		for(j=0; j<ncountl; j++)
		{
			if(partneridlist[i].nLinkID == m_PList[j]->m_ndesLinkID)
			{//�ɻ�鱣��
				m_PList[j]->m_bSuperProxy = partneridlist[i].bIsSuper;
				m_PList[j]->m_bLan2A = partneridlist[i].bIsLan2A;
				m_PList[j]->m_bLan2B = partneridlist[i].bIsLan2B;
				bfind = TRUE;
				break;
			}
		}

		if(!bfind)
		{//�»�����
			CCPartner *p = new CCPartner(partneridlist[i], m_pWorker, m_pChannel);
			m_PList.push_back(p);
			ncountl = m_PList.size();
		}
	}
	std::random_shuffle(m_PList.begin(), m_PList.end());//����˳�򣬷�ֹ��Ϊ�Ⱥ��ϵ����ѡ��ĳһ��
/*
char ch[100];
sprintf(ch,"client.............lan2A:%d\n",m_pChannel->m_bLan2A);
OutputDebugString(ch);
for(i=0; i<partneridlist.size(); i++)
{
	sprintf(ch,"cache:%d lan2A:%d  lan2B:%d  super:%d IP:%s\n",
		partneridlist[i].bIsCache,
		partneridlist[i].bIsLan2A,
		partneridlist[i].bIsLan2B,
		partneridlist[i].bIsSuper,
		inet_ntoa(partneridlist[i].sAddr.sin_addr));
	OutputDebugString(ch);
}
*/
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return FALSE;
}

//���л������
BOOL CCPartnerCtrl::PartnerLink(BOOL bExit)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ctCONN);
#else
	EnterCriticalSection(&m_ctCONN);
#endif

	int ncount = m_PLINKList.size();
	for(int i=0; i<ncount; i++)
	{
		if(bExit || m_bClearing)
		{
			break;
		}

		if(m_PLINKList[i] != NULL && m_PLINKList[i]->m_bTryNewLink)
		{//�ɽ������ӳ���
			m_PLINKList[i]->PartnerLink(this);
			m_PLINKList[i]->m_bTryNewLink = FALSE;
		}

		//���б���������ѳ��Թ��Ľڵ�
	#ifndef WIN32
		pthread_mutex_lock(&m_ct);
	#else
		EnterCriticalSection(&m_ct);
	#endif
		
		m_PLINKList.erase(m_PLINKList.begin()+i);

	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif

		i--;
		ncount--;
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ctCONN);
#else
	LeaveCriticalSection(&m_ctCONN);
#endif

	return TRUE;
}

//������������ ���л������
BOOL CCPartnerCtrl::CheckPartnerLinks(BOOL bExit)
{
	if(bExit || m_bClearing)
	{
		return TRUE;
	}
//OutputDebugString("ptcrl checklink.....0\n");
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
//OutputDebugString("ptcrl checklink.....1\n");

	BOOL bhaveTURNpt = FALSE;//�����������ӽڵ�
	int nactiveL = 0;//��������������
	int nactiveN = 0;//��������������
	DWORD dwendtime = 0;
	DWORD dwlastdatatime=0;
	DWORD dwtmp=0;
	::std::map<unsigned int, unsigned int> PTaddrMap;
	PTaddrMap.clear();
	//���memlist��������ڵ�״̬��
	int ncount = m_PList.size();
	if(ncount > 0)
	{//�л��
		//������ӹ��̣����������ǰ�������
		for(int i=0; i<ncount; i++)
		{
			if(bExit || m_bClearing)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return TRUE;
			}
			if(m_PList[i] == NULL || (m_PList[i]->m_ndesLinkID <= 0 && !m_PList[i]->m_bTURNC))
			{
				continue;
			}

			int nret = m_PList[i]->CheckLink(this, FALSE, dwlastdatatime);//<0δ���� 0������ 1���ӳɹ�
			if(m_PList[i]->m_bTURNC)
			{
				bhaveTURNpt = TRUE;
				continue;
			}
 
			if(nret >= 0)
			{//���ӳɹ�������������
				if(!m_PList[i]->m_bAccept && m_PList[i]->m_bLan2B)
				{//�������
					nactiveL++;
				}
				else if(!m_PList[i]->m_bAccept && !m_PList[i]->m_bLan2B)
				{//�������
					nactiveN++;
				}

				//��¼�õ�ַ
				::std::map<unsigned int, unsigned int>::iterator iter;
				iter = PTaddrMap.find(m_PList[i]->m_unADDR);
				if(iter == PTaddrMap.end())
				{//δ�ҵ�
					PTaddrMap.insert(::std::map<unsigned int, unsigned int>::value_type(m_PList[i]->m_unADDR, m_PList[i]->m_unADDR));
				}
			}

			//�ҳ��������ʱ��
			if(dwlastdatatime > 0 && dwlastdatatime < dwtmp)
			{
				dwtmp = dwlastdatatime;
			}
		}
//char ch[100];
//sprintf(ch,"ptcrl ncount:%d active:%d\n",ncount,nactive);
//OutputDebugString(ch);
		dwendtime = CCWorker::JVGetTime();
		if(dwendtime > dwtmp + PT_TIME_NODATA)
		{//����������ʱ�䶼������1min��˵���ڵ㼫��������� ���ٰ���������
			m_dwLastDisOldLTime = 0;
			m_dwLastDisOldNTime = 0;
		}

		//�����������Ӽ��
		if(nactiveL <= PT_LCONNECTMAX || m_pChannel->m_bCache)
		{//�����������δ�ﵽ���� ��������������
			if(dwendtime > PT_TIME_CNNEWPT + m_dwLastTryNewLTime)
			{
				m_dwLastTryNewLTime = dwendtime;
	
				//ֻѰ�������ڵ��������
				int nnewindex = -1;
				ncount = m_PList.size();
				int nFCcount = 0;
				for(int i=0; i<ncount; i++)
				{
					if(m_PList[i]->m_ndesLinkID > 0 
					   && (m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD) 
					   && m_PList[i]->m_bLan2B
					   && !m_PList[i]->m_bTURNC)
					{//�ҵ�δ���ӵ�ͬ���ڵ�
						if(nnewindex >= 0)
						{//�������ҵ���һ����ѡ�ڵ㣬�вο��Ƚ϶���
							if(m_PList[i]->m_nConnFCount <= nFCcount)
							{//����ʧ�ܴ����� ��ζ�����ӳɹ�����Ҳ�͸���
								nnewindex = i;
								nFCcount = m_PList[i]->m_nConnFCount;
							}
						}
						else
						{//��û�п�ѡ�ڵ�Ҳ��û�вο�ֵ��ֱ�ӽ���ǰ�����Ϊ�ο�ֵ
							nnewindex = i;
							nFCcount = m_PList[i]->m_nConnFCount;
						}
					}
				}
				if(nnewindex >= 0)
				{//�ҵ�����ʧ�ܴ������ٵ������ڵ��������
					//m_PList[nnewindex]->CheckLink(this, TRUE, dwlastdatatime);
					m_PList[nnewindex]->m_bTryNewLink = TRUE;
					m_PLINKList.push_back(m_PList[nnewindex]);//���û��������Ӷ���
				}
			}
		}
		else
		{//������������ﵽ���� �Ͽ������
			if(dwendtime > PT_TIME_DISBADPT + m_dwLastDisOldLTime)
			{
				m_dwLastDisOldLTime = dwendtime;

				ncount = m_PList.size();
				float flittle = -1;
				float ftmp = -1;
				int nlittleindex = -1;
				for(int i=0; i<ncount; i++)
				{
					if(m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED && dwendtime > m_PList[i]->m_dwLastConnTime + PT_TIME_DISBADPT && m_PList[i]->m_bLan2B)
					{//�ҵ�����ڵ� ֻ������ӳ���2���ӽڵ�ɱ���������
						ftmp = m_PList[i]->GetPower();
						if(flittle < 0 || ftmp <= flittle)
						{//�ҵ�������
							flittle = ftmp;
							nlittleindex = i;
						}
					}
				}
				if(nlittleindex >= 0)
				{//���ڵ�Ͽ�
					m_PList[nlittleindex]->m_nstatus = PNODE_STATUS_FAILD;
					m_PList[nlittleindex]->DisConnectPartner();
//OutputDebugString("checklink nlittleindex->faild\n");
				}
			}
		}

		if(!m_pChannel->m_bLan2A)
		{//�������������ͬ���� �򲻻���������� ����Ҫ���
			//�����������Ӽ��
			if(nactiveN <= PT_NCONNECTMAX || m_pChannel->m_bCache || bhaveTURNpt)
			{//�������δ�ﵽ���� ������ȫ���ϴ��ڵ� ���� ������Ҫ�������� ��������������
				if(dwendtime > PT_TIME_CNNEWPT + m_dwLastTryNewNTime)
				{
					m_dwLastTryNewNTime = dwendtime;
					
					//�ҳ�һ�������ڵ����Ƚ�������
					BOOL bHaveNew = FALSE;
					int nFCcount = 0;
					int nnewindex = -1;
					ncount = m_PList.size();
					for(int i=0; i<ncount; i++)
					{
						if(m_PList[i]->m_ndesLinkID > 0 
							&& (m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD) 
							&& m_PList[i]->m_bSuperProxy 
							&& !m_PList[i]->m_bLan2A
							&& !m_PList[i]->m_bTURNC)
						{//�ҵ�δ���ӵ����������ڵ�
							
							::std::map<unsigned int, unsigned int>::iterator iter;
							iter = PTaddrMap.find(m_PList[i]->m_unADDR);
							if(iter != PTaddrMap.end())
							{//�ҵ���IP�����ӻ���������,���� ��ֹ��ͬһ�������ж������
								continue;
							}

							if(nnewindex >= 0)
							{//�������ҵ���һ����ѡ�ڵ㣬�вο��Ƚ϶���
								if(m_PList[i]->m_nConnFCount <= nFCcount)
								{//����ʧ�ܴ����� ��ζ�����ӳɹ�����Ҳ�͸���
									nnewindex = i;
									nFCcount = m_PList[i]->m_nConnFCount;
								}
							}
							else
							{//��û�п�ѡ�ڵ�Ҳ��û�вο�ֵ��ֱ�ӽ���ǰ�����Ϊ�ο�ֵ
								nnewindex = i;
								nFCcount = m_PList[i]->m_nConnFCount;
							}

							if(m_PList[i]->m_nConnFCount <= 0)
							{//�ýڵ㻹û��ʧ�ܹ� ȷ������Ҫ������ͨ�ڵ�
								bHaveNew = TRUE;
							}
						}
					}
					
					if(nnewindex >= 0)
					{//�ҵ�����ڵ�
						//m_PList[nnewindex]->CheckLink(this, TRUE, dwlastdatatime);
						m_PList[nnewindex]->m_bTryNewLink = TRUE;
						m_PLINKList.push_back(m_PList[nnewindex]);//���û��������Ӷ���
					}

					if(nnewindex < 0 || (nnewindex >= 0 && !bHaveNew))
					{//û�д���ڵ� ���볢����ͨ�ڵ�
					 //�д���ڵ㣬���������ڵ㶼����ʧ�ܹ������п��ܴ���ڵ㶼�����ϣ����볢����ͨ�ڵ�
						nFCcount = 0;
						nnewindex = -1;
						for(int i=0; i<ncount; i++)
						{
							if(m_PList[i]->m_ndesLinkID > 0 
							   && (m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD) 
							   && !m_PList[i]->m_bLan2A
							   && !m_PList[i]->m_bTURNC)
							{//�ҵ�δ���ӵ���ͨ�ڵ�
								::std::map<unsigned int, unsigned int>::iterator iter;
								iter = PTaddrMap.find(m_PList[i]->m_unADDR);
								if(iter != PTaddrMap.end())
								{//�ҵ���IP�����ӻ���������,���� ��ֹ��ͬһ�������ж������
									continue;
								}

								if(nnewindex >= 0)
								{//�������ҵ���һ����ѡ�ڵ㣬�вο��Ƚ϶���
									if(m_PList[i]->m_nConnFCount <= nFCcount)
									{//����ʧ�ܴ����� ��ζ�����ӳɹ�����Ҳ�͸���
										nnewindex = i;
										nFCcount = m_PList[i]->m_nConnFCount;
									}
								}
								else
								{//��û�п�ѡ�ڵ�Ҳ��û�вο�ֵ��ֱ�ӽ���ǰ�����Ϊ�ο�ֵ
									nnewindex = i;
									nFCcount = m_PList[i]->m_nConnFCount;
								}
							}
						}

						if(nnewindex >= 0)
						{//�ҵ���ͨ�ڵ�
							//m_PList[nnewindex]->CheckLink(this, TRUE, dwlastdatatime);
							m_PList[nnewindex]->m_bTryNewLink = TRUE;
							m_PLINKList.push_back(m_PList[nnewindex]);//���û��������Ӷ���
						}
					}

					//ǿ���������ٽڵ�
					if(bhaveTURNpt)
					{
						for(int i=0; i<ncount; i++)
						{
							if(m_PList[i]->m_bTURNC && (m_PList[i]->m_nstatus == PNODE_STATUS_NEW || m_PList[i]->m_nstatus == PNODE_STATUS_FAILD))
							{//�ҵ�δ���ӵ����ٽڵ�
								if(m_PList[i]->m_nConnFCount <= 2)
								{//����ʧ�ܴ����϶࣬û��Ҫ����������
									//m_PList[i]->CheckLink(this, TRUE, dwlastdatatime);
									m_PList[i]->m_bTryNewLink = TRUE;
									m_PLINKList.push_back(m_PList[i]);//���û��������Ӷ���
									break;
								}
							}
						}
					}
				}
			}
			else
			{//��������ﵽ���� �Ͽ������
				if(dwendtime > PT_TIME_DISBADPT + m_dwLastDisOldNTime)
				{
					m_dwLastDisOldNTime = dwendtime;
					
					ncount = m_PList.size();
					float flittle = -1;
					float ftmp = -1;
					int nlittleindex = -1;
					for(int i=0; i<ncount; i++)
					{
						if(m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED && dwendtime > m_PList[i]->m_dwLastConnTime + PT_TIME_DISBADPT && !m_PList[i]->m_bLan2B)
						{//�ҵ����������ڵ� ֻ������ӳ���2���ӽڵ�ɱ���������
							ftmp = m_PList[i]->GetPower();
							if(flittle < 0 || ftmp <= flittle)
							{//�ҵ�������
								flittle = ftmp;
								nlittleindex = i;
							}
						}
					}
					if(nlittleindex >= 0)
					{//���ڵ�Ͽ�
						m_PList[nlittleindex]->m_nstatus = PNODE_STATUS_FAILD;
						m_PList[nlittleindex]->DisConnectPartner();
//OutputDebugString("checklink nlittleindex->faild\n");
					}
				}
			}
		}
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
//OutputDebugString("ptcrl ncount<=0 \n");
	dwendtime = CCWorker::JVGetTime();
	if(dwendtime > PT_TIME_REQPTLIST + m_dwLastREQListTime)
	{//�ж��ϴ��������б��ѹ�ȥ��ã�ÿ��2���ӳ���һ��
		if(m_dwLastREQListTime > 0)
		{
			m_pChannel->SendData(JVN_CMD_PLIST, NULL, 0);
		}
		m_dwLastREQListTime = dwendtime;
	}

	return TRUE;
}

//���յ�һ��������� �����жϴ���
void CCPartnerCtrl::AcceptPartner(UDTSOCKET socket, SOCKADDR_IN clientaddr, int nDesID, BOOL bTCP)
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	BOOL bFind=FALSE;
	int ncount = m_PList.size();
	int nacceptL = 0;
	int nacceptN = 0;
	unsigned int tmp = ntohl(clientaddr.sin_addr.s_addr);
	int i=0;
	for(i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_bTURNC)
		{
			continue;
		}

		if((m_PList[i]->m_nstatus != PNODE_STATUS_NEW && m_PList[i]->m_nstatus != PNODE_STATUS_FAILD) && m_PList[i]->m_bAccept)
		{//ͳ�Ʊ�����������
			if(m_PList[i]->m_bLan2B)
			{
				nacceptL++;
			}
			else
			{
				nacceptN++;
			}
		}
	}

	if(CheckInternalIP(ntohl(clientaddr.sin_addr.s_addr)))
	{//��������
		if(nacceptL >= PT_LACCEPTMAX)
		{//�����������Ӵﵽ���� ������������
			if(bTCP)
			{
				closesocket(socket);
			}
			else
			{
				UDT::close(socket);
			}
			
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif

//OutputDebugString("acceptpartner nacceptL > LMAX\n");
			return;
		}
	}
	else
	{//��������
		if(nacceptN >= PT_NACCEPTMAX && !m_pChannel->m_bCache)
		{//�������Ӵﵽ���� ������������
			if(bTCP)
			{
				closesocket(socket);
			}
			else
			{
				UDT::close(socket);
			}
			
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif

//OutputDebugString("acceptpartner nacceptN > NMAX\n");
			return;
		}

		//��鵱ǰ��ַ�Ƿ����������л��������ӳɹ� ���ǵĻ������ܸ����� ��ֹͬ�����������
		for(i=0; i<ncount; i++)
		{
			if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_bTURNC)
			{
				continue;
			}
			
			if((m_PList[i]->m_nstatus != PNODE_STATUS_NEW && m_PList[i]->m_nstatus != PNODE_STATUS_FAILD) && tmp == m_PList[i]->m_unADDR)
			{//��ͬ��ip�����ӹ������������� �����ܴ˴�����
				if(bTCP)
				{
					closesocket(socket);
				}
				else
				{
					UDT::close(socket);
				}
				
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
//OutputDebugString("acceptpartner IP exist\n");
				return;
			}
		}
	}

	for(i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_bTURNC)
		{
			continue;
		}

		if(nDesID == m_PList[i]->m_ndesLinkID)
		{//�ҵ��û���¼
			bFind = TRUE;
			
			if(m_PList[i]->m_nstatus == PNODE_STATUS_NEW 
				|| m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTING
				|| m_PList[i]->m_nstatus == PNODE_STATUS_FAILD)
			{
				//�������
				if(m_PList[i]->m_socketTCP > 0)
				{
					closesocket(m_PList[i]->m_socketTCP);
				}
				m_PList[i]->m_socketTCP = 0;

				if(m_PList[i]->m_socket > 0)
				{
					UDT::close(m_PList[i]->m_socket);
				}
				m_PList[i]->m_socket = 0;

				m_PList[i]->m_bTCP = bTCP;
				if(bTCP)
				{
					m_PList[i]->m_socketTCP = socket;
				}
				else
				{
					m_PList[i]->m_socket = socket;
				}
				
				m_PList[i]->m_nstatus = PNODE_STATUS_ACCEPT;
				m_PList[i]->m_dwstart = CCWorker::JVGetTime();

				m_PList[i]->m_bAccept = TRUE;
				
				memcpy(&m_PList[i]->m_addr, &clientaddr, sizeof(SOCKADDR_IN));

				break;
			}
			else
			{//���������ӳɹ����������ӣ�������ǰ����
				if(bTCP)
				{
					closesocket(socket);
				}
				else
				{
					UDT::close(socket);
				}
				
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif

//OutputDebugString("acceptpartner connecting or connected\n");
				return;
			}
		}
	}
	
	if(!bFind)
	{//���л���б���û�иü�¼��ֱ�����
		STPTLI pttmp;
		pttmp.nLinkID = nDesID;
		pttmp.bIsSuper = FALSE;
		pttmp.bIsLan2A = FALSE;
		pttmp.bIsCache = FALSE;
		pttmp.bIsTC = FALSE;
		memcpy(&pttmp.sAddr, &clientaddr, sizeof(SOCKADDR_IN));
		//������������־
		if(CheckInternalIP(ntohl(clientaddr.sin_addr.s_addr)))
		{
			pttmp.bIsLan2B = TRUE;
		}
		else
		{
			pttmp.bIsLan2B = FALSE;
		}

		CCPartner *p = new CCPartner(pttmp, m_pWorker, m_pChannel, clientaddr, socket, bTCP);
			
		m_PList.push_back(p);
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
}

//��Ҫ����ת������
void CCPartnerCtrl::AddTurnCachPartner()
{
	int nc = m_pChannel->m_SList.size();
	int i=0;
	for(i=0; i<nc; i++)
	{
		m_pChannel->m_SList[i].buseful = TRUE;
	}

	//�ȼ���Ƿ��Ѿ������������ӣ�����Ҫ�ظ�����
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	int ncount = m_PList.size();
	for(i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL)
		{
			continue;
		}

		if(m_PList[i]->m_bTURNC)
		{//������������
			m_PList[i]->m_nConnFCount = 0;

		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return;
		}
	}

	//��ǰ��û�����ٽڵ㣬����һ��
	STPTLI pttmp;
	pttmp.nLinkID = 0;
	pttmp.bIsSuper = FALSE;
	pttmp.bIsLan2A = FALSE;
	pttmp.bIsCache = FALSE;
	pttmp.bIsTC = TRUE;
	pttmp.bIsLan2B = FALSE;
	
	CCPartner *p = new CCPartner(pttmp, m_pWorker, m_pChannel);
	
	m_PList.push_back(p);

	//�����������ӽڵ㣬�ڵ�ľ������������Ӻ����п���
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}

BOOL CCPartnerCtrl::RecvFromPartners(BOOL bExit, HANDLE hEnd)
{
//OutputDebugString("ptcrl recvfromp.....0\n");
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
//OutputDebugString("ptcrl recvfromp.....1\n");
//	int nLen = 0;
	int ncount = m_PList.size();
	int i=0;
	for(i=0; i<ncount; i++)
	{
	#ifndef WIN32
		if(bExit)
	#else
		if(bExit || WAIT_OBJECT_0 == WaitForSingleObject(hEnd, 0))
	#endif
		{//�߳���Ҫ�˳� �����л��֪ͨ�Ͽ�����
			ncount = m_PList.size();
			for(i=0; i<ncount; i++)
			{
				if(m_PList[i] != NULL && ((m_PList[i]->m_ndesLinkID > 0 && !m_PList[i]->m_bTURNC) || m_PList[i]->m_bTURNC))
				{
					m_PList[i]->DisConnectPartner();

					m_PList[i]->m_ndesLinkID = 0;

					m_PList[i]->m_bTURNC = FALSE;//�ó���ͨ�ڵ㣬������������
				}
			}
				
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return FALSE;
		}

		if(m_bClearing)
		{
			break;
		}

		if(m_PList[i] == NULL || (m_PList[i]->m_ndesLinkID <= 0 && !m_PList[i]->m_bTURNC))
		{
			continue;
		}

		m_PList[i]->BaseRecv(this);
	}

	if(bExit)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ct);
	#else
		LeaveCriticalSection(&m_ct);
	#endif
		
		return FALSE;
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return TRUE;
}

//��ȡBM����ȫ���򲿷ֻ�鷢��BM
BOOL CCPartnerCtrl::SendBM2Partners(BYTE *puchBuf, int nSize,BOOL bExit, HANDLE hEnd)
{
	if(puchBuf != NULL && nSize > 0)
	{
		if(!m_pChannel->m_bCache)
		{//��ͨ��飬�������зǸ��ٻ������ӹ㲥
		#ifndef WIN32
			pthread_mutex_lock(&m_ct);
		#else
			EnterCriticalSection(&m_ct);
		#endif
			
			int ncount = m_PList.size();
			for(int i=0; i<ncount; i++)
			{
			#ifndef WIN32
				if(bExit)
			#else
				if(bExit || WAIT_OBJECT_0 == WaitForSingleObject(hEnd, 0))
			#endif
				{
				#ifndef WIN32
					pthread_mutex_unlock(&m_ct);
				#else
					LeaveCriticalSection(&m_ct);
				#endif
					
					return FALSE;
				}
				if(m_bClearing)
				{
					break;
				}
				if(m_PList[i] != NULL && m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED && !m_PList[i]->m_bTURNC)
				{//��ͨ���ڵ㶼�ǹ㲥Ŀ�꣬�������������������������˲���Ҫ����㲥
					m_PList[i]->SendBM(puchBuf, nSize);
				}
			}
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
		}
		else
		{//��Ϊ���ٻ��棬�����й㲥����ɱ����ϴ����޴����ֻ���ٶȽϿ�ڵ�㲥
		#ifndef WIN32
			pthread_mutex_lock(&m_ct);
		#else
			EnterCriticalSection(&m_ct);
		#endif
			
			int ncount = m_PList.size();
			for(int i=0; i<ncount; i++)
			{
			#ifndef WIN32
				if(bExit)
				{
					pthread_mutex_unlock(&m_ct);
					return FALSE;
				}
			#else
				if(bExit || WAIT_OBJECT_0 == WaitForSingleObject(hEnd, 0))
				{
					LeaveCriticalSection(&m_ct);
					return FALSE;
				}
			#endif	
				
				if(m_bClearing)
				{
					break;
				}
				if(m_PList[i] != NULL && m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED && m_PList[i]->m_bProxy2)
				{
					m_PList[i]->SendBM(puchBuf, nSize);
				}
			}
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
		}
	}

	return TRUE;
}

BOOL CCPartnerCtrl::SendBMDREQ2Partner(STREQS stpullreqs, int ncount, BOOL bExit)
{//�������л�� �ҳ����Ż�鷢����������
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	BOOL btimeout=FALSE;
	ncount = jvs_min(stpullreqs.size(), ncount);
	for(int i=0; i<ncount; i++)
	{
		if(bExit || m_bClearing)
		{
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return TRUE;
		}
	
		btimeout = FALSE;
		//�ȴ�������ʷ��¼�����ų�������������ݣ������ִ��Ч��
		::std::map<unsigned int, DWORD>::iterator iterbt;
		iterbt = m_ChunkBTMap.find(stpullreqs[i].unChunkID);
		if(iterbt != m_ChunkBTMap.end())
		{//�ҵ���¼
//			DWORD dw = iterbt->second;
			if(iterbt->second > 0)
			{
				DWORD dwend = CCWorker::JVGetTime();
				if(stpullreqs[i].bNEED)
				{
					if(dwend < iterbt->second + BM_CHUNK_TIMEOUT60)//120)//20)
					{//�������� �����ظ�
						continue;
					}
					btimeout = TRUE;
				}
				else
				{
					if(dwend < iterbt->second + BM_CHUNK_TIMEOUT240)//100)
					{//�������� �����ظ�
						continue;
					}
					btimeout = TRUE;
				}						
			}

//char ch[100]={0};
//sprintf(ch,"pull(%d)==================find[time:%d]\n",pstreqs[i].unChunkID,dw);
//OutputDebugString(ch);
			//����Ƿ����ṩ�� ������
			CheckAndSendChunk(stpullreqs[i].unChunkID, btimeout);
		}
		else
		{//û�ҵ���¼ �Ǹ�ȫ�����ݿ�����
//char ch[100]={0};
//sprintf(ch,"pull(%d)==================new\n",pstreqs[i].unChunkID);
//OutputDebugString(ch);
			m_ChunkBTMap.insert(::std::map<unsigned int, DWORD>::value_type(stpullreqs[i].unChunkID, 0));
			
			//����Ƿ����ṩ�� ������ ����һ��chunk�ɹ��������� �ɷ�ֹ��������˲��ѻ�
			CheckAndSendChunk(stpullreqs[i].unChunkID, btimeout);
		}		
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return FALSE;
}


//�ж��Ƿ�����Ч���ṩ��
BOOL CCPartnerCtrl::CheckAndSendChunk(unsigned int unChunkID, BOOL bTimeOut)
{
	int nTCAVG = 0;
	int nTCIndex = -1;
	int nBest = 0;//��������ֵ(����)
	int nPartnerIndex = -1;//�����ṩ�߱��
	
	BOOL bfindWANLink = FALSE;//�ҵ�������Ч���
	BOOL bfindLink = FALSE;//�ҵ����ӳɹ��Ļ��
	//�������л�飬����Ƿ���Ǳ���ṩ�ߣ��Ƿ�����ĳ����������������
	int ncount = m_PList.size();
	for(int i=0; i<ncount; i++)
	{
		if(m_bClearing)
		{
			return FALSE;
		}

		if(m_PList[i] != NULL && m_PList[i]->m_ndesLinkID > 0 && m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED)
		{
			bfindLink = TRUE;
			if(!m_PList[i]->m_bLan2B && !m_PList[i]->m_bLan2A)
			{//��鲻������ͬ��������Ҳ���뱾��ͬ����������Ӧ�����������
				bfindWANLink = TRUE;
			}
			//����Ƿ��и�����Ƭ���Ƿ���û���������ǰ����Ƭ���ǵĻ����ø����󣬲����»�����ܣ�����������ֵ
			if(m_PList[i]->CheckREQ(unChunkID, bTimeOut))
			{//�û���и�����Ƭ��Ч
				if((m_PList[i]->m_nLPSAvage >= nBest && !m_PList[i]->m_bTURNC) || m_PList[i]->m_bLan2B)
				{//�û�����ܸ��� ���Ǹû���ǵ�ǰ�ڵ��������� ����
					nPartnerIndex = i;
					nBest = m_PList[i]->m_nLPSAvage;
				}
				else if(m_PList[i]->m_bTURNC)
				{
					nTCIndex = i;
					nTCAVG = m_PList[i]->m_nLPSAvage;
				}
			}
		}
	}

	if(nTCIndex >= 0)
	{//�����ٽڵ㣬�ж������ٽڵ�������Ƿ��ǿ��������ͨ�ڵ�������Ƿ����
		if(nPartnerIndex < 0)
		{//û����ͨ�ڵ㣬ֻ�ܴ����ٽڵ�ȡ����
			nPartnerIndex = nTCIndex;
		}
		else
		{//����ͨ�ڵ㣬������ͨ�ڵ��Ƿ�ǳ�����������Ǻ���������Ҫ�����ٽڵ�ȡ���ݣ��������ٽڵ�ѹ��
			if(nBest < 10 && !m_PList[nPartnerIndex]->m_bLan2B)
			{
				nPartnerIndex = nTCIndex;
			}
		}
	}
	
	if(nPartnerIndex >= 0)
	{//��������ṩ�ߣ�������ṩ�ߺ��������ܱȽ� �������ṩ�߷�������
	 //(���￼������ѹ����ֻҪ����оͲ�������ȡ, �������غ͵�ǰ�ڵ�ͬ����)		
//if(m_pChannel->m_nLocalChannel == 2)
//{		
//char ch[100]={0};
//sprintf(ch,"B1A?_B(%d)==================BM:%d [B:%d A:%d]\r\n",nPartnerIndex, unChunkID, nBest, m_pChannel->m_nLPSAvage);
//OutputDebugString(ch);
//}
//		if(m_PList[nPartnerIndex]->m_bLan2B)
//		{//���Ż���Ǹ������ڵ� ����ֱ�Ӵ���ȡ
//			m_PList[nPartnerIndex]->SendBMDREQ(unChunkID, this);
//			return TRUE;
//		}
		
//		if(m_pChannel->CheckREQ(unChunkID) && m_pChannel->m_bLan2A)
//		{//����������Ч ���������뱾����ͬ����  ������ȡ����
//			m_pChannel->SendBMDREQ2A(unChunkID);
//			return TRUE;
//		}
		
		//����û���ݻ��������������ڵ� ���Ҳ�������ڵ� ��ӻ��ȡ ��������ѹ��
		m_PList[nPartnerIndex]->SendBMDREQ(unChunkID, this);
		return TRUE;
	}
	else
	{//���û���ṩ�� ��������Ƿ����ṩ��
		if(m_pChannel->CheckREQ(unChunkID))
		{//����chunk��Ч ���ṩ��
			DWORD dwcur = m_pChannel->m_pBuffer->JVGetTime();
			if((dwcur < m_pChannel->m_pBuffer->m_dwBeginBFTime + 3000) || ((!bfindLink || bfindWANLink) && dwcur > m_dwLastPTData + 10000))
			{//�����ӵ�ǰ3�롢û�л����������������������10��û�л�鴫�����ݣ���Ҫ������������ȡ���ݣ���ֹͼ���ж�
				m_pChannel->SendBMDREQ2A(unChunkID);

//if(m_pChannel->m_nLocalChannel == 2)
//{			
//	char ch[100]={0};
//	sprintf(ch,"B0A1_A==================%d [%d:%d][%d:%d]\r\n",unChunkID, nBest, m_pChannel->m_nLPSAvage,dwcur,m_dwLastPTData);
//	OutputDebugString(ch);
//}

				return TRUE;
			}
		}
//if(m_pChannel->m_nLocalChannel == 2)
//{
//	char ch[100]={0};
//	sprintf(ch,"B0A0_0==================%d [%d:%d]\r\n",unChunkID, nBest, m_pChannel->m_nLPSAvage);
//	OutputDebugString(ch);
//}		
		return FALSE;
	}
}

BOOL CCPartnerCtrl::SendBMD()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	int ncount = m_PList.size();
	for(int i=0; i<ncount; i++)
	{
		if(m_bClearing)
		{
			break;
		}
		if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_nstatus != PNODE_STATUS_CONNECTED)// || m_PList[i]->m_bTURNC)
		{//��Ч�ڵ�����ٽڵ㲻��Ҫ�������ݿ�
			continue;
		}

		m_PList[i]->SendBMD(this);
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
	return TRUE;
}

void CCPartnerCtrl::SetReqStartTime(BOOL bA, unsigned int unChunkID, DWORD dwtime)
{
	if(unChunkID <= 0)
	{
		return;
	}
	
	m_ChunkBTMap.erase(unChunkID);
	if(dwtime == 0)
	{//�����¼
//		m_ChunkBTMap.erase(unChunkID);
		return;
	}

	m_ChunkBTMap.insert(::std::map<unsigned int, DWORD>::value_type(unChunkID, dwtime));	
}

void CCPartnerCtrl::ClearPartner()
{
	m_bClearing = TRUE;
//OutputDebugString("ptcrl clearp.....0\n");
#ifndef WIN32
	pthread_mutex_unlock(&m_ctCONN);
	pthread_mutex_unlock(&m_ct);
	pthread_mutex_unlock(&m_ctPTINFO);
#else
	EnterCriticalSection(&m_ctCONN);
	EnterCriticalSection(&m_ct);
	EnterCriticalSection(&m_ctPTINFO);
#endif

//OutputDebugString("ptcrl clearp.....1\n");
	//��������������Ӷ����е���Ч������ͷţ�������ֲ�ͬ��
	m_PLINKList.clear();

	int ncount = m_PList.size();
	for(int i=0; i< ncount; i++)
	{
		if(m_PList[i] != NULL)
		{
			delete m_PList[i];
			m_PList[i] = NULL;
		}
	}
	m_PList.clear();

#ifndef WIN32
	pthread_mutex_unlock(&m_ctPTINFO);
	pthread_mutex_unlock(&m_ct);
	pthread_mutex_unlock(&m_ctCONN);
#else
	LeaveCriticalSection(&m_ctPTINFO);
	LeaveCriticalSection(&m_ct);
	LeaveCriticalSection(&m_ctCONN);
#endif
	
	m_bClearing = FALSE;
}

void CCPartnerCtrl::DisConnectPartners()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	int ncount = m_PList.size();
	for(int i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL || m_PList[i]->m_ndesLinkID <= 0 || m_PList[i]->m_nstatus != PNODE_STATUS_CONNECTED)
		{
			continue;
		}
		
		m_PList[i]->DisConnectPartner();
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif
	
}

//����Ƿ���Ҫ����BM���µ����ػ��� �ڲ����� ���ӱ���
void CCPartnerCtrl::CheckIfNeedSetBuf(unsigned int unChunkID, int ncount, DWORD dwCTime[10], BOOL bA)
{
//	if(unChunkID+ncount-1 > m_unChunkIDNew && m_pChannel->m_pBuffer != NULL)
	if(unChunkID > m_unChunkIDNew && m_pChannel->m_pBuffer != NULL)
	{//��ȫ��BM ��Ҫ���µ����ػ��� ��֤������Զ��������Ƶ����ͬ��
//char ch[1000]={0};
//sprintf(ch,"BM(pctrl needsetbuf)=============================================[%d,%d]>[%d,%d]\r\n",m_unChunkIDOld,m_unChunkIDNew,unChunkID,unChunkID+ncount-1);
//OutputDebugString(ch);
//m_pWorker->m_Log.SetRunInfo(m_pChannel->m_nLocalChannel,"",__FILE__,__LINE__,ch);
		if(!bA)
		{
			m_unChunkIDNew = unChunkID;
			m_unChunkIDOld = unChunkID - ncount + 1;

			return;
		}

		unsigned int unnewid = 0;
		unsigned int unoldid = 0;
		//m_pChannel->m_pBuffer->AddNewBM(unChunkID, ncount, dwCTime, unnewid, unoldid, bA);
		m_pChannel->m_pBuffer->AddNewBM(unChunkID-ncount+1, ncount, dwCTime, unnewid, unoldid, bA);
		if(unnewid > 0)
		{
			m_unChunkIDNew = unnewid;
		}

		if(unoldid > 0)
		{
			m_unChunkIDOld = unoldid;
		}
	}
}

void CCPartnerCtrl::CheckGarbage()
{
//OutputDebugString("ptcrl gt.....0\n");
#ifndef WIN32
	pthread_mutex_unlock(&m_ctCONN);
	pthread_mutex_unlock(&m_ct);
	pthread_mutex_unlock(&m_ctPTINFO);
#else
	EnterCriticalSection(&m_ctCONN);
	EnterCriticalSection(&m_ct);
	EnterCriticalSection(&m_ctPTINFO);
#endif
	
//OutputDebugString("ptcrl gt.....1\n");
	int ncount = m_PList.size();
	for(int i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL)
		{
			m_PList.erase(m_PList.begin() + i);
			i--;
			ncount--;
			continue;
		}
		if(m_PList[i]->m_ndesLinkID <= 0 && !m_PList[i]->m_bTURNC)
		{
			//��������������Ӷ����е���Ч������ͷţ�������ֲ�ͬ��
			int nlinkc = m_PLINKList.size();
			for(int j=0; j<nlinkc; j++)
			{
				if(m_PLINKList[j] == NULL)
				{
					m_PLINKList.erase(m_PLINKList.begin() + j);
					j--;
					nlinkc--;
					continue;
				}
				if(m_PLINKList[j]->m_ndesLinkID <= 0 && !m_PLINKList[j]->m_bTURNC)
				{
					m_PLINKList.erase(m_PLINKList.begin() + j);
					j--;
					nlinkc--;
					continue;
				}
			}

			delete m_PList[i];
			m_PList[i] = NULL;
			m_PList.erase(m_PList.begin() + i);
			i--;
			ncount--;
			continue;
		}
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ctPTINFO);
	pthread_mutex_unlock(&m_ct);
	pthread_mutex_unlock(&m_ctCONN);
#else
	LeaveCriticalSection(&m_ctPTINFO);
	LeaveCriticalSection(&m_ct);
	LeaveCriticalSection(&m_ctCONN);
#endif
}

//���ó����ڵ� �����Ե��� ����ֻ�򳬼��ڵ㷢������
void CCPartnerCtrl::ResetProxy2()
{
#ifndef WIN32
	pthread_mutex_lock(&m_ct);
#else
	EnterCriticalSection(&m_ct);
#endif
	
	int ncount = m_PList.size();
	
	int i=0;
	::std::map<unsigned int, unsigned int> PTaddrMap;
	PartnerList partnertmp;
	partnertmp = m_PList;
	for(i=0; i<ncount; i++)
	{//����������õļ����ڵ�
		if(m_bClearing)
		{
		#ifndef WIN32
			pthread_mutex_unlock(&m_ct);
		#else
			LeaveCriticalSection(&m_ct);
		#endif
			
			return;
		}

		if(partnertmp[i] == NULL || partnertmp[i]->m_ndesLinkID <= 0 || partnertmp[i]->m_bLan2B || partnertmp[i]->m_bLan2A || partnertmp[i]->m_bTURNC)
		{//ȥ����Ч�ڵ�,����ͬ�����ڵ㲻��������ڵ�,���ٽڵ�Ҳ����������ڵ�
			partnertmp.erase(partnertmp.begin() + i);
			i--;
			ncount--;
			continue;
		}
		
		for(int j=i+1; j<ncount; j++)
		{
			if(m_bClearing)
			{
			#ifndef WIN32
				pthread_mutex_unlock(&m_ct);
			#else
				LeaveCriticalSection(&m_ct);
			#endif
				
				return;
			}
			
			if(partnertmp[j] == NULL || partnertmp[j]->m_ndesLinkID <= 0 || partnertmp[j]->m_bLan2B || partnertmp[j]->m_bLan2A || partnertmp[i]->m_bTURNC)
			{//ȥ����Ч�ڵ�,����ͬ�����ڵ㲻��������ڵ�,���ٽڵ�Ҳ����������ڵ�
				partnertmp.erase(partnertmp.begin() + j);
				j--;
				ncount--;
				continue;
			}
			
			if(partnertmp[i]->GetPower() >= partnertmp[j]->GetPower())
			{//j ���ܸ���
				CCPartner *p = partnertmp[i];
				partnertmp[i] = partnertmp[j];
				partnertmp[j] = p;
			}
		}
		
		//��¼IP ��ͬ��IPֻ����һ�δ���
		::std::map<unsigned int, unsigned int>::iterator iter;
		iter = PTaddrMap.find(ntohl(partnertmp[i]->m_addr.sin_addr.s_addr));
		if(iter == PTaddrMap.end())
		{//δ�ҵ�
			PTaddrMap.insert(::std::map<unsigned int, unsigned int>::value_type(ntohl(partnertmp[i]->m_addr.sin_addr.s_addr), 1));
		}
		else
		{//��ͬIP����ѡ �������ظ�����
			partnertmp.erase(partnertmp.begin() + i);
			i--;
			ncount--;
			continue;
		}
		
		if(i >= JVC_CLIENTS_BM3-1)
		{//������õĽڵ㹻���� ���ü���������
			break;
		}
	}
	
	//��������õĽڵ㻯Ϊ�����ڵ� ����ͬʱ����Ϊ�ǳ����ڵ�
	ncount = m_PList.size();
	for(i=0; i<ncount; i++)
	{
		if(m_PList[i] != NULL)
		{
			if(m_PList[i]->m_bLan2B)
			{
				m_PList[i]->m_bProxy2 = TRUE;
			}
			else
			{
				m_PList[i]->m_bProxy2 = FALSE;
			}
		}
	}
	ncount = partnertmp.size();

	for(i=0; i<jvs_min(ncount, JVC_CLIENTS_BM3); i++)
	{
		if(partnertmp[i] != NULL)
		{//�����ڵ��ֹ��Ϊ�����ڵ�
			partnertmp[i]->m_bProxy2 = TRUE;
		}
	}

#ifndef WIN32
	pthread_mutex_unlock(&m_ct);
#else
	LeaveCriticalSection(&m_ct);
#endif

}

//��ȡ���״̬��Ϣ
void CCPartnerCtrl::GetPartnerInfo(char *pMsg, int &nSize, DWORD dwend)
{
	//�Ƿ�ಥ(1)+�����ܸ���(4)+����ܸ���(4)+[IP(16) + port(4)+����״̬(1)+�����ٶ�(4)+��������(4)+�ϴ�����(4)] +[...]...
#ifndef WIN32
	pthread_mutex_lock(&m_ctPTINFO);
#else
	EnterCriticalSection(&m_ctPTINFO);
#endif
	int ncount = m_PList.size();

	int ntotal = 1;
	if((ncount+1)*sizeof(STPTINFO)+9 > nSize)
	{
	#ifndef WIN32
		pthread_mutex_unlock(&m_ctPTINFO);
	#else
		LeaveCriticalSection(&m_ctPTINFO);
	#endif
		
		memset(pMsg, 0, 9);
		memcpy(pMsg, &ntotal, 4);
		return;
	}

	int nconn=0;
	nSize = sizeof(STPTINFO)+9;
	for(int i=0; i<ncount; i++)
	{
		if(m_PList[i] == NULL)
		{
			m_PList.erase(m_PList.begin() + i);
			i--;
			ncount--;
			continue;
		}
		
		if(m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED)
		{
			m_PList[i]->GetPInfo(pMsg, nSize, dwend);
			nconn++;
		}

		if(!m_PList[i]->m_bTURNC)
		{//��ͨ�ڵ㣬ֱ�Ӽ���
			ntotal++;
		}
//		else
//		{//����ת���ڵ㣬ֻ�����ӳɹ�������²ż���
//			if(m_PList[i]->m_nstatus == PNODE_STATUS_CONNECTED)
//			{
//				ntotal++;
//			}
//		}
	}
#ifndef WIN32
	pthread_mutex_unlock(&m_ctPTINFO);
#else
	LeaveCriticalSection(&m_ctPTINFO);
#endif

	nconn++;

	memcpy(&pMsg[1], &ntotal, 4);
	memcpy(&pMsg[5], &nconn, 4);
}

/****************************************************************************
*����  : tcpreceive
*����  : ��������������(��ʱ)
*����  : 
		 [IN] ntimeoverSec
*����ֵ: >0    ���ݳ���
*����  :
*****************************************************************************/
int CCPartnerCtrl::tcpreceive(SOCKET s,char *pchbuf,int nsize,int ntimeoverSec)
{
	int   status,nbytesreceived;   
	if(s <= 0)//if(s==-1)     
	{
		return   -1;//0;   
	}
	struct   timeval   tv={ntimeoverSec,0};   
	fd_set   fd;   
	FD_ZERO(&fd);     
	FD_SET(s,&fd);     
	if(ntimeoverSec==0)   
	{
		status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,NULL);     
	}
	else
	{
		status=select(s+1,&fd,(fd_set   *)NULL,(fd_set   *)NULL,&tv);     
	}
	switch(status)     
	{   
	case   -1:     
		//printf("read   select   error\n");       
		return   -1;   
	case   0:                                             
		//printf("receive   time   out\n");
		return   0;   
	default:                   
		if(FD_ISSET(s,&fd))     
		{   
			if((nbytesreceived=recv(s,pchbuf,nsize,0))==-1) 
			{
				return   0;   
			}
			else   
			{
				return   nbytesreceived;   
			}
		}   
	}   
	return 0;   
}

/****************************************************************************
*����  : tcpsend
*����  : ��������������(��ʱ)
*����  : 
		 [IN] ntimeoverSec
*����ֵ: >0    ���ݳ���
*����  :
*****************************************************************************/
int CCPartnerCtrl::tcpsend(SOCKET s,char *pchbuf,int nsize,int ntimeoverSec, int &ntimeout)
{
	int   status=0, nbytessended=-1;   
	if(s <= 0)//if(s==-1)     
	{
		return   -1;//0;   
	}
	struct   timeval   tv={ntimeoverSec,0};   
	fd_set   fd;   
	FD_ZERO(&fd);     
	FD_SET(s,&fd);     
	if(ntimeoverSec==0)   
	{
		status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,NULL);     
	}
	else
	{
		status=select(s+1,(fd_set   *)NULL,&fd,(fd_set   *)NULL,&tv);     
	}
	switch(status)     
	{   
	case   -1:     
		//printf("read   select   error\n");       
		return   -1;   
	case   0:                                             
		//printf("receive   time   out\n");
		ntimeout = 1;
		return   0;   
	default:                   
		if(FD_ISSET(s,&fd))     
		{   
			if((nbytessended=send(s,pchbuf,nsize,0))==-1) 
			{
				return   0;//�����˴��󣬵���Щ�����������Ϊ�����������������Ĵ�����Ҫ�ж�   
			}
			else   
			{
				return   nbytessended;   
			}
		}   
	}   
	return -1;//0;   
}
/****************************************************************************
*����  : connectnb
*����  : ����������(��ʱ)
*����  : [IN] s
		[IN] to
		[IN] tolen
		[IN] ntimeoverSec
*����ֵ: >0    ���ݳ���
		JVS_SERVER_R_OVERTIME     ��ʱ
		����                      ʧ��
*����  :
*****************************************************************************/
int CCPartnerCtrl::connectnb(SOCKET s,struct sockaddr * to,int namelen,int ntimeoverSec)
{
	if(s <= 0)//if(s ==-1)     
	{
		return -1;   
	}
	struct timeval tv={ntimeoverSec,0};   
	fd_set   fd;   
	FD_ZERO(&fd);     
	FD_SET(s,&fd);
	
	int nRet = connect( s, (SOCKADDR *)to, namelen);       
//int kkk = WSAGetLastError();
#ifndef WIN32
	if (nRet != 0)
#else
	if ( SOCKET_ERROR == nRet )
#endif
	{         
		switch (select(s + 1, NULL, &fd, NULL, &tv)) 
		{
		case -1:
			return -1;//����
			break;
		case 0:
			return -1;//��ʱ
			break;
		default: //���ӳɹ�����ʧ�ܶ���writeable
			//char error = 0;
			int error = 0;

			#ifndef WIN32
				socklen_t len = sizeof(error);//sizeof(SOCKET);
			#else
				int len = sizeof(error);//int len = sizeof(error);//sizeof(SOCKET);
			#endif
			//�ɹ��Ļ�errorӦ��Ϊ0
//int kkk = WSAGetLastError();
			if ((0 == getsockopt(s,SOL_SOCKET,SO_ERROR,(char*)&error,&len)))
			{   
				if(0 == error)
				{ 
					return 0;
				}
			}
//kkk = WSAGetLastError();
//char ch[100];
//sprintf(ch,"kkk:%d\n", kkk);
			break;
		}
	}
	else
	{
		return 0;
	}
	return -1; 
}

/****************************************************************************
*����  : CheckInternalIP
*����  : ����Ƿ����ڲ���IP��ַ 
*����  : ��
*����ֵ: ��
*����  : ��
*****************************************************************************/
BOOL CCPartnerCtrl::CheckInternalIP(const unsigned int ip_addr)
{
	
    //���3���ַ
    if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF ) ||
        (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF ) ||
        (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF ) ||
		(ip_addr == 0x7F000001))
    {
        return TRUE;//����ip	
    }
	
    return FALSE;//����
	
}






























