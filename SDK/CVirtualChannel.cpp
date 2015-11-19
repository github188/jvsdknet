// CVirtualChannel.cpp: implementation of the CCVirtualChannel class.
//
//////////////////////////////////////////////////////////////////////

#include "CVirtualChannel.h"

#include "CWorker.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern int JVC_MSS;
CCVirtualChannel::CCVirtualChannel()
{
    m_bDirectConnect = FALSE;
    m_dwSenTime = CCWorker::JVGetTime();
    m_dwRecvTime = 0;
    m_dwConnectTime = CCWorker::JVGetTime();
    
    m_NatList.clear();
}
CCVirtualChannel::CCVirtualChannel(STCONNECTINFO stConnectInfo, CCHelpCtrl *pHelp, CCWorker *pWorker,BOOL bMobile)
{
    m_dwAddTime =CCWorker::JVGetTime();
    m_bMobile = bMobile;
    m_sSocket = pWorker->m_WorkerUDPSocket;
    m_dwStartConnectTime = 0;
    m_bDirectConnect = FALSE;
    m_nNatTypeA = NAT_TYPE_0_UNKNOWN;
    m_dwSenTime = CCWorker::JVGetTime();
    m_dwRecvTime = 0;
    m_dwConnectTime = CCWorker::JVGetTime();
    
    m_bDisConnectShow = FALSE;
    
    m_bExit = FALSE;
    m_stConnInfo.nShow = 0;
    
    m_nLocalStartPort = stConnectInfo.nLocalPort;
    m_pHelp = pHelp;
    m_pWorker = pWorker;
    m_bPass = FALSE;
    
#ifndef WIN32
    m_bEndC = FALSE;
    
    pthread_mutex_init(&m_ct, NULL); //��ʼ���ٽ���
#else
    m_hStartEventC = 0;
    m_hEndEventC = 0;
    
    InitializeCriticalSection(&m_ct); //��ʼ���ٽ���
#endif
    
    m_hConnThread = 0;
    m_ServerSocket = 0;//��Ӧ�����socket
    m_SocketSTmp = 0;
    
    memset(&m_addrLastLTryAL, 0, sizeof(SOCKADDR_IN));
    memcpy(&m_stConnInfo, &stConnectInfo, sizeof(STCONNECTINFO));
    
    m_bTURN = FALSE;
    m_bLocal = FALSE;
    
    m_bJVP2P = TRUE;
    
    m_bPassWordErr = FALSE;
    m_nStatus = NEW_IP;
    
    m_NatList.clear();
    //��������
    StartConnThread();
}

void CCVirtualChannel::ReConnect(STCONNECTINFO stConnectInfo, CCHelpCtrl *pHelp, CCWorker *pWorker)
{
    
    m_dwSenTime = CCWorker::JVGetTime();
    m_dwRecvTime = 0;
    m_dwConnectTime = CCWorker::JVGetTime();
    
    m_bDisConnectShow = FALSE;
    
    m_bExit = FALSE;
    m_stConnInfo.nShow = 0;
    
    m_nLocalStartPort = stConnectInfo.nLocalPort;
    m_pHelp = pHelp;
    m_pWorker = pWorker;
    m_bPass = FALSE;
    
#ifndef WIN32
    m_bEndC = FALSE;
    
    //	pthread_mutex_init(&m_ct, NULL); //��ʼ���ٽ���
#else
    m_hStartEventC = 0;
    m_hEndEventC = 0;
    
    //	InitializeCriticalSection(&m_ct); //��ʼ���ٽ���
#endif
    
    m_hConnThread = 0;
    m_ServerSocket = 0;//��Ӧ�����socket
    m_SocketSTmp = 0;
    
    memset(&m_addrLastLTryAL, 0, sizeof(SOCKADDR_IN));
    memcpy(&m_stConnInfo, &stConnectInfo, sizeof(STCONNECTINFO));
    
    m_bTURN = FALSE;
    m_bLocal = FALSE;
    
    m_bJVP2P = TRUE;
    
    m_bPassWordErr = FALSE;
    m_nStatus = NEW_IP;
    
    m_NatList.clear();
    //��������
    StartConnThread();
}

CCVirtualChannel::~CCVirtualChannel()
{
    m_bExit = TRUE;
    
#ifndef WIN32
    if (0 != m_hConnThread)
    {
        m_bEndC = TRUE;
        pthread_join(m_hConnThread, NULL);
        m_hConnThread = 0;
    }
#else
    if(m_hEndEventC>0)
    {
        SetEvent(m_hEndEventC);
    }
    
    CCWorker::jvc_sleep(10);
    
    CCChannel::WaitThreadExit(m_hConnThread);
    
    if(m_hEndEventC > 0)
    {
        CloseHandle(m_hEndEventC);
        m_hEndEventC = 0;
    }
    if(m_hStartEventC > 0)
    {
        CloseHandle(m_hStartEventC);
        m_hStartEventC = 0;
    }
    if(m_hConnThread > 0)
    {
        CloseHandle(m_hConnThread);
        m_hConnThread = 0;
    }
#endif
    
    if(m_ServerSocket > 0)
    {
        m_pWorker->pushtmpsock(m_ServerSocket);
    }
    m_ServerSocket = 0;
    
    m_nStatus = FAILD;
    
#ifndef WIN32
    pthread_mutex_destroy(&m_ct);
#else
    DeleteCriticalSection(&m_ct); //�ͷ��ٽ���
#endif
}

void CCVirtualChannel::StartConnThread()
{
    m_dwStartConnectTime = CCWorker::JVGetTime();
#ifndef WIN32
    pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
    if (0 != pthread_create(&m_hConnThread, pAttr, ConnProc, this))
    {
        m_hConnThread = 0;
#else
        //���������߳�
        UINT unTheadID;
        m_hStartEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hEndEventC = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hConnThread = (HANDLE)_beginthreadex(NULL, 0, ConnProc, (void *)this, 0, &unTheadID);
        SetEvent(m_hStartEventC);
        if (m_hConnThread == 0)//�����߳�ʧ��
        {
            //����Conn�߳�
            if(m_hStartEventC > 0)
            {
                CloseHandle(m_hStartEventC);
                m_hStartEventC = 0;
            }
            
            if(m_hEndEventC > 0)
            {
                CloseHandle(m_hEndEventC);
                m_hEndEventC = 0;
            }
#endif
            
            if(m_pWorker != NULL)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "����������ʧ��. ���������߳�ʧ��", __FILE__,__LINE__);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "vconnect failed. create connect thread failed.", __FILE__,__LINE__);
                }
            }
            
            m_nStatus = FAILD;
        }
    }
    
    //�µ�����ͨ���ӣ��жϴ�С�������뻹�Ǵӱ���ֱ�ӽ���
    void CCVirtualChannel::DealNewYST(STCONNPROCP *pstConn)
    {
        if(m_stConnInfo.nWhoAmI == JVN_WHO_H || m_stConnInfo.nWhoAmI == JVN_WHO_M)
        {//С����������
            m_dwConnectTime = CCWorker::JVGetTime();
            GetSerAndBegin(pstConn);//��ȡ��������ַ������ֱ�ӻ�ȡ
        }
        else
        {//����
            m_nStatus = FAILD;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                char chMsg[] = "��������ʧ��!";
                m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "��������ʧ��. ��������ʧ��", __FILE__,__LINE__);
            }
            else
            {
                char chMsg[] = "dataerr failed!";
                m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "connect failed. dataerr", __FILE__,__LINE__);
            }
        }
    }
    
    //����������Ͳ�ѯ��������
    void CCVirtualChannel::DealWaitSerREQ(STCONNPROCP *pstConn)
    {
        int ncount = m_SList.size();
        
        memset(pstConn->chdata, 0, 50);
        //���ͣ�����(4)+����(4)+������(1)+�����(4)+����ͨ����(4)
        int nt = JVN_CMD_YSTCHECK;
        int nl = 9;
        memcpy(&pstConn->chdata[0], &nt, 4);
        memcpy(&pstConn->chdata[4], &nl, 4);
        pstConn->chdata[8] = 0;//��ѯ����
        memcpy(&pstConn->chdata[9], m_stConnInfo.chGroup, 4);
        memcpy(&pstConn->chdata[13], &m_stConnInfo.nYSTNO, 4);
        nl += 8;
        for(int i=0; i<ncount; i++)
        {
            CCChannel::sendtoclient(pstConn->udpsocktmp, (char*)pstConn->chdata, nl, 0, (sockaddr *)&m_SList[i].addr, sizeof(sockaddr_in), 1);
        }
        m_nStatus = WAIT_SER_RSP;//��ʼ�ȴ���������Ӧ
        m_dwStartTime = CCWorker::JVGetTime();
        pstConn->slisttmp.clear();
    }
    
    //�ȴ���������� �ù��������2����
    void CCVirtualChannel::DealWaitSerRSP(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 2500 || (pstConn->slisttmp.size() == m_SList.size()) )//2000
        {//�ȴ���ʱ���ѽ���ȫ��Ӧ��
//            printf("pstConn->udpsocktmp %d\n",pstConn->udpsocktmp);
            shutdown(pstConn->udpsocktmp,SD_BOTH);
            closesocket(pstConn->udpsocktmp);
            pstConn->udpsocktmp = 0;
            
            //�������������
            char chIP1[30]={0};
            char chIP2[30]={0};
            int nc = m_SList.size();
            for(int i=0; i<nc; i++)
            {//��δ���صķ������������
                sprintf(chIP1, "%s", inet_ntoa(m_SList[i].addr.sin_addr));
                BOOL bfind = FALSE;
                int ncnew = pstConn->slisttmp.size();
                for(int j=0; j<ncnew; j++)
                {
                    sprintf(chIP2, "%s", inet_ntoa(pstConn->slisttmp[j].addr.sin_addr));
                    if(strcmp(chIP1, chIP2) == 0)
                    {//�÷�������Ӧ��
                        bfind = TRUE;
                        break;
                    }
                }
                if(!bfind)
                {//�÷�����û��Ӧ��Ҫô�����������⣬Ҫô���������ܲ�
                    m_SList[i].buseful = TRUE;//FALSE;//ע������Ŀǰ��������Ŀ���٣�û���������ߵ�ʱ������Ҳ������ʱÿ��������������
                    pstConn->slisttmp.push_back(m_SList[i]);
                }
            }
            m_SList.clear();
            m_SList = pstConn->slisttmp;

            m_SListTurn.clear();
            m_SListTurn = m_SList;
            //		m_nStatus = NEW_P2P;//�ѶԷ��������й�ɸѡ����ʼ��ʽ����
            m_nStatus = WAIT_CHECK_A_NAT;
            
            //ȡNAT
            //		m_pWorker->GetWANIPList(m_SList,&m_NatList,1000);//���±���������ַ
            //////////////////////////////////////////////////////////////////////////
            //char ch[100]={0};
            //for(i=0; i<m_SList.size(); i++)
            //{
            //	sprintf(ch,"%d**********IP:%s OnOff:%d \n",i,inet_ntoa(m_SList[i].addr.sin_addr), m_SList[i].buseful);
            //	OutputDebugString(ch);
            //}
            //////////////////////////////////////////////////////////////////////////
        }
        else
        {//��������
            memset(pstConn->chdata, 0, JVN_ASPACKDEFLEN);
            if(CCChannel::receivefromm(pstConn->udpsocktmp, (char *)pstConn->chdata, JVN_ASPACKDEFLEN, 0, &pstConn->sockAddr, &pstConn->nSockAddrLen, 100) > 0)
            {//���գ�����(4)+����(4)+������(1)+�����(4)+����ͨ����(4)+����Э��汾��(4)+�Ƿ�����(1)
                int ntp = 0;
                memcpy(&ntp, &pstConn->chdata[0], 4);
                if(ntp == JVN_CMD_YSTCHECK)
                {//�յ��������ɹ�����
                    int nl = 0, nystno = 0, nv = 0; char chg[4]={0};
                    memcpy(&nl, &pstConn->chdata[4], 4);
                    memcpy(chg, &pstConn->chdata[9], 4);
                    memcpy(&nystno, &pstConn->chdata[13], 4);
                    memcpy(&nv, &pstConn->chdata[17], 4);
                    if(pstConn->chdata[8] == 1 && (nystno==m_stConnInfo.nYSTNO))// && (strcmp(chg, pWorker->m_stConnInfo.chGroup) == 0)
                    {//��ȷ�Ļظ�
                        int nc = m_SList.size();
                        char chIP1[30]={0}, chIP2[30]={0};
                        sprintf(chIP1, "%s", inet_ntoa(((SOCKADDR_IN*)&pstConn->sockAddr)->sin_addr));
                        for(int i=0; i<nc; i++)
                        {
                            sprintf(chIP2, "%s", inet_ntoa(m_SList[i].addr.sin_addr));
                            if(strcmp(chIP1, chIP2)==0)
                            {//��ַ��Ч
                                if(!CCChannel::ServerIsExist(pstConn,chIP1))
                                {
                                    STSERVER stserver;
                                    memcpy(&stserver.addr, &pstConn->sockAddr, sizeof(SOCKADDR_IN));
                                    stserver.buseful = (pstConn->chdata[21] == 0)?FALSE:TRUE;//�����ߵķ���������ʹ��
                                    stserver.nver = nv;
                                    pstConn->slisttmp.push_back(stserver);
                                    //////////////////////////////////////////////////////////////////////////
                                    //char ch[100]={0};
                                    //sprintf(ch,"SerRSP IP:%s ONOFF:%d\n",chIP1,stserver.buseful);
                                    //OutputDebugString(ch);
                                    //////////////////////////////////////////////////////////////////////////
                                    
                                    break;
                                }
                            }
                        }
                    }//��ƥ�䲻��ȷ�Ļظ�
                }//�յ���������������
            }//δ���յ�����
        }
    }
    
    //ȥ������������ѯ�������߷����������������
    void CCVirtualChannel::DealWaitIndexSerREQ(STCONNPROCP *pstConn)
    {
        int ncount = m_ISList.size();
        if(ncount == 0)
        {
            if(!m_pWorker->GetSerList(m_stConnInfo.chGroup, m_SList, 1, m_stConnInfo.nLocalPort))
            {
                if(!m_pWorker->GetSerList(m_stConnInfo.chGroup, m_SList, 2, m_stConnInfo.nLocalPort))
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        //	char chMsg[] = "��ȡYST��������Ϣʧ��!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST��ʽ��������ʧ��.ԭ��:��ȡ������ʧ��", __FILE__,__LINE__);
                    }
                    else
                    {
                        //	char chMsg[] = "get server address failed!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST connect failed.Info:get server address failed.", __FILE__,__LINE__);
                    }
                }
            }
            
            if(m_SList.size() > 0)
            {
                m_SListTurn = m_SList;
                m_SListBak = m_SList;
                m_nStatus = WAIT_SER_REQ;
            }
            else
            {
                m_nStatus = NEW_P2P;
            }
            shutdown(pstConn->udpsocktmp,SD_BOTH);
            closesocket(pstConn->udpsocktmp);
            pstConn->udpsocktmp = 0;
            return;
        }
        
        //���ͣ�����(1)+����������(4)+�����(4)+����ͨ����(4)
        memset(pstConn->chdata, 0, 50);
        pstConn->chdata[0] = JVN_REQ_QUERYYSTNUM;
        *(DWORD*)&(pstConn->chdata[1]) = htonl(8);
        memcpy(&(pstConn->chdata[5]), m_stConnInfo.chGroup, 4);
        *(DWORD*)&(pstConn->chdata[9]) = htonl(m_stConnInfo.nYSTNO);
        ncount = m_ISList.size();
        for(int i=0; i<ncount; i++)
        {
            CCChannel::sendtoclient(pstConn->udpsocktmp, (char*)pstConn->chdata, 13, 0, (sockaddr *)&m_ISList[i].addr, sizeof(sockaddr_in), 1);
        }
        m_nStatus = WAIT_INDEXSER_RSP;//��ʼ�ȴ���������Ӧ
        m_dwStartTime = CCWorker::JVGetTime();
        pstConn->slisttmp.clear();
    }
    
    //ȥ������������ѯ�������߷����������շ��ذ�
    void CCVirtualChannel::DealWaitIndexSerRSP(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        
        if(pstConn->dwendtime > m_dwStartTime + 2000)
        {
            //�ȴ���ʱ,���з�����̽��
            if(!m_pWorker->GetSerList(m_stConnInfo.chGroup, m_SList, 1, m_stConnInfo.nLocalPort))
            {
                if(!m_pWorker->GetSerList(m_stConnInfo.chGroup, m_SList, 2, m_stConnInfo.nLocalPort))
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        //	char chMsg[] = "��ȡYST��������Ϣʧ��!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST��ʽ��������ʧ��.ԭ��:��ȡ������ʧ��", __FILE__,__LINE__);
                    }
                    else
                    {
                        //	char chMsg[] = "get server address failed!";
                        m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST connect failed.Info:get server address failed.", __FILE__,__LINE__);
                    }
                }
            }
            
            if(m_SList.size() > 0)
            {
                m_SListTurn = m_SList;
                m_SListBak = m_SList;
                m_nStatus = WAIT_SER_REQ;
            }
            else
            {
                shutdown(pstConn->udpsocktmp,SD_BOTH);
                closesocket(pstConn->udpsocktmp);
                pstConn->udpsocktmp = 0;
                m_nStatus = NEW_P2P;
            }
        }
        else
        {//��������
            //���Ͳ�ѯ ���շ�����
            //��������
            memset(pstConn->chdata, 0, JVN_ASPACKDEFLEN);
            int nRecvSize = CCChannel::receivefromm(pstConn->udpsocktmp, (char *)pstConn->chdata, JVN_ASPACKDEFLEN, 0, &pstConn->sockAddr, &pstConn->nSockAddrLen, 100);
		OutputDebug("query index nRecvSize...%d",nRecvSize);
            if(nRecvSize > 0)
            {
                //��֤���ݰ�����ʽ������(1)+����������(4)+n*��������ַ(sizeof(sockaddr_in))
                int nServerNum = 0;
                bool bPkgTrue = (pstConn->nSockAddrLen >= 5 && pstConn->chdata[0] == JVN_REQ_QUERYYSTNUM);
                if(bPkgTrue)
                {
                    DWORD dwDataSize = ntohl(*(DWORD*)&(pstConn->chdata[1]));
                    if(dwDataSize % sizeof(sockaddr_in) != 0)
                    {
                        bPkgTrue = false;
                    }
                    
                    nServerNum = dwDataSize / sizeof(sockaddr_in);
                }
                
                if(nServerNum > 0 && bPkgTrue)
                {
                    shutdown(pstConn->udpsocktmp,SD_BOTH);
                    closesocket(pstConn->udpsocktmp);
                    pstConn->udpsocktmp = 0;
				OutputDebug("query index result...%d",nServerNum);
                    
                    sockaddr_in *pAddrs = (sockaddr_in*)&(pstConn->chdata[5]);
                    for(int i = 0; i < nServerNum; ++i)
                    {
                        STSERVER server = {0};
                        memcpy(&server.addr, pAddrs, sizeof(sockaddr_in));
                        server.buseful = TRUE;
                        server.nver = 0;
                        pstConn->slisttmp.push_back(server);
                        
					OutputDebug(" index server  %s : %d \n",inet_ntoa(server.addr.sin_addr),ntohs(server.addr.sin_port));
                        pAddrs++;
                    }
                    
                    //��ȡ�ɹ���������һ��
                    m_SList.clear();
                    m_SList = pstConn->slisttmp;


                    m_SListTurn.clear();
                    m_SListTurn = m_SList;
                    m_nStatus = WAIT_CHECK_A_NAT;//��ѯ����NAT����
                    
                    if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
                    {
                        m_pWorker->WriteYSTNOInfo(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, m_SList, m_addressA, 0);
                    }
                }
                
            }//δ���յ�����
        }
    }
    
    void CCVirtualChannel::DealWaitNatREQ(STCONNPROCP *pstConn)//����������ѯ����
    {
        if(pstConn->udpsocktmp > 0)
        {
            shutdown(pstConn->udpsocktmp,SD_BOTH);
            closesocket(pstConn->udpsocktmp);
            pstConn->udpsocktmp = 0;
        }
        
        pstConn->udpsocktmp = socket(AF_INET, SOCK_DGRAM, 0);
        
        sockaddr_in sin = {0};
#ifndef WIN32
        sin.sin_addr.s_addr = INADDR_ANY;
#else
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
        sin.sin_family = AF_INET;
        sin.sin_port = htons(0);
        
        BOOL bReuse = TRUE;
        setsockopt(pstConn->udpsocktmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        
        bind(pstConn->udpsocktmp, (struct sockaddr*)&sin, sizeof(sin));
        
        char data[20] = {0};
        int nType = JVN_REQ_NATA_A;
        memcpy(&data[0],&nType,4);
        memcpy(&data[4],&m_stConnInfo.nYSTNO,4);
        STSERVER server = {0};
        int nServerNum = m_SList.size();
        for(int i = 0;i < nServerNum;i ++)
        {
            memcpy(&server,&m_SList[i],sizeof(STSERVER));
            CCChannel::sendtoclientm(pstConn->udpsocktmp,data,8,0,(sockaddr *)&server.addr,sizeof(server.addr),1);
            
            SOCKADDR_IN srv = {0};
            memcpy(&srv,&server.addr,sizeof(SOCKADDR_IN));
            
            //		OutputDebug("============== ��ѯ������������  %s : %d \n",inet_ntoa(server.addr.sin_addr),ntohs(server.addr.sin_port));
            
        }
        //OutputDebug("============== ��ѯ������������  %d \n",nServerNum);
        
        m_nStatus = WAIT_NAT_A;//�ȴ��������͵ķ���
        m_dwStartTime = CCWorker::JVGetTime();
    }
    
    void CCVirtualChannel::DealWaitNatRSP(STCONNPROCP *pstConn)//����������ѯ����
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        
        if(pstConn->dwendtime > m_dwStartTime + 1500)//δ�ȵ�������ȵ�һ������Ϳ���
        {
            if(m_bMobile)
            {
                m_nStatus = NEW_P2P;
                return;
            }
            m_nStatus = WAIT_MAKE_HOLE;//�ѶԷ��������й�ɸѡ����ʼ��ʽ����
            //��ʱ���Դ�׼��
            //OutputDebug("��ѯ������������ ��ʱ");
            
            //���е��˴�ʱһ�����µĳ��򣬼����������������Ĺ���
            DealMakeHole(pstConn);
            return;
        }
        unsigned char chdata[JVN_ASPACKDEFLEN] = {0};
        SOCKADDR sockAddr = {0};
        int nSockAddrLen = sizeof(SOCKADDR);
        int nRecvSize = CCChannel::receivefromm(pstConn->udpsocktmp,(char*)chdata, JVN_ASPACKDEFLEN, 0, &sockAddr, &nSockAddrLen, 100);
        if(nRecvSize > 0)//5 or 11 = 4:type 1:NAT(4:ip 2:port)
        {
            int nType = 0;
            memcpy(&nType,chdata,4);
            if(JVN_REQ_NATA_A == nType)
            {
                m_nNatTypeA = chdata[4];
                
						OutputDebug("Get1 a type %d",m_nNatTypeA);
                if(m_nNatTypeA == NAT_TYPE_0_PUBLIC_IP || m_nNatTypeA == NAT_TYPE_1_FULL_CONE)//������ȫ͸����NATֱ����TCP
                {
                    //ȡ�õ�ַ��ֱ��TCP
                    sprintf(m_stConnInfo.chServerIP, "%d.%d.%d.%d",chdata[5],chdata[6],chdata[7],chdata[8]);
                    memcpy(&m_stConnInfo.nServerPort,&chdata[9],2);
                    m_nStatus = NEW_HAVEIP;
                    
                    m_sSocket = m_pWorker->m_WorkerUDPSocket;
                    strcpy(m_strIP,m_stConnInfo.chServerIP);
                    m_nPort = m_stConnInfo.nServerPort;
                    
                    m_pWorker->m_Helper.AddOkYST(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,
                                                 m_stConnInfo.chServerIP,m_stConnInfo.nServerPort);
                    return;
                }
                else
                {
                    if(m_bMobile)
                    {
                        m_nStatus = NEW_P2P;
					OutputDebug("Get2 a type %d",m_nNatTypeA);
                        return;
                    }
                    
                    m_nStatus = WAIT_MAKE_HOLE;//�ѶԷ��������й�ɸѡ����ʼ��ʽ����
                    //��ʱ���Դ�׼��
                    //				OutputDebug("��ѯ���������� ���...%d",nServerNum);
                    
                    //���е��˴�ʱһ�����µĳ��򣬼����������������Ĺ���
                    DealMakeHole(pstConn);
                    
                }
                
                return;
            }
        }
    }
    
    void CCVirtualChannel::DealMakeHole(STCONNPROCP *pstConn)
    {
        if(!m_pWorker->m_MakeHoleGroup.AddConnect(this,m_stConnInfo.chGroup,m_stConnInfo.nYSTNO,
                                                  m_stConnInfo.nChannel,m_stConnInfo.bLocalTry,m_SList,m_nNatTypeA,m_pWorker->m_nLocalPortWan,TRUE))
        {
            m_nStatus = NEW_P2P;
        }
        else
        {
            m_nStatus = WAIT_MAKE_HOLE;
        }
        //	m_nStatus = NEW_TURN;
    }
    
    //���ӳɹ�
    void CCVirtualChannel::DealOK(STCONNPROCP *pstConn)
    {
        m_dwRecvTime = CCWorker::JVGetTime();
        m_dwConnectTime = CCWorker::JVGetTime();
        m_nFYSTVER = UDT::getystverF(m_ServerSocket);//��ȡԶ��Э��汾
        
        if(m_stConnInfo.nShow == JVN_CONNTYPE_TURN)
        {
            m_bTURN = TRUE;
            char chMsg[] = "(TURN)";
            m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg);
            //OutputDebugString("channel connectok(turn)\n");
        }
        else
        {
            m_bLocal = FALSE;
            if(m_stConnInfo.nShow == JVN_CONNTYPE_LOCAL)
            {
                m_bLocal = TRUE;
            }
            
            char chMsg[] = "(P2P)";
            m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNOK,chMsg);
            //OutputDebugString("channel connectok(p2p)\n");
            if(m_stConnInfo.bYST && m_stConnInfo.nYSTNO > 0)
            {
                m_pWorker->WriteYSTNOInfo(m_stConnInfo.chGroup,m_stConnInfo.nYSTNO, m_SList, m_addressA, 1);
                m_pWorker->YSTNOCushion(m_stConnInfo.chGroup, m_stConnInfo.nYSTNO, -1);
                
            }
        }
        
        m_bPass = TRUE;//
        
#ifdef WIN32
        //�����߳̽���
        if(m_hEndEventC > 0)
        {
            CloseHandle(m_hEndEventC);
        }
        m_hEndEventC = 0;
#endif
    }
    
    //����ʧ��
    void CCVirtualChannel::DealFAILD(STCONNPROCP *pstConn)
    {
        m_dwConnectTime = CCWorker::JVGetTime();
        //�����߳�
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        
#ifdef WIN32
        if(m_hEndEventC > 0)
        {
            CloseHandle(m_hEndEventC);
            m_hEndEventC = 0;
        }
#endif
    }
    
    //��ʼ��������ͨ���뷽ʽ����
    void CCVirtualChannel::DealNEWP2P(STCONNPROCP *pstConn)
    {
        m_stConnInfo.nShow = JVN_CONNTYPE_P2P;
        
        BOOL bfind = FALSE;
        if(JVN_ONLYTURN != m_stConnInfo.nTURNType)
        {//ֻ����ת������ʱ������Ҫ����ֱ����ش���
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {//��ʼ�����������ص�ַ
                    if(SendSP2P(m_SList[i].addr, i, pstConn->chError))
                    {//���������������ɹ�
                        m_nStatus = WAIT_S;
                        m_dwStartTime = CCWorker::JVGetTime();
                        bfind = TRUE;
                        break;
                    }
                    else
                    {//���������������ʧ�� ������һ��������
                        m_SList[i].buseful = FALSE;
                    }
                }
            }
        }
        if(!bfind)
        {//��û�п��Գ���ֱ���ķ�����
            //�ж��Ƿ���ҪTURN����
            if(JVN_NOTURN == m_stConnInfo.nTURNType)
            {//����ת��;tcp���ӷ�ʽ����ֱ���ɹ��ʵͣ��ݲ�����ת�������������ת������������
                m_nStatus = FAILD;
                
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    if(!m_bShowInfo)
                    {
                        if(m_bPassWordErr)
                        {
                            char chMsg[] = "���δͨ����֤!";
                            m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                        }
                        else
                        {
                            char chMsg[] = "���ӳ�ʱ!";
                            m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                        }
                    }
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST(P2P)��ʽ��������ʧ��.ԭ��ֱ�Ϊ:", __FILE__,__LINE__,pstConn->chError);
                }
                else
                {
                    if(!m_bShowInfo)
                    {
                        if(m_bPassWordErr)
                        {
                            char chMsg[] = "password is wrong!";
                            m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                        }
                        else
                        {
                            char chMsg[] = "connect timeout!";
                            m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                        }
                    }
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST(P2P) connect failed.Infos:", __FILE__,__LINE__,pstConn->chError);
                }
            }
            else
            {//��ʼ����TURN����
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST(P2P)��ʽ��������ʧ��,����ת��.ԭ��ֱ�Ϊ:", __FILE__,__LINE__,pstConn->chError);
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST(P2P) connect failed,try TURN type.Infos:", __FILE__,__LINE__,pstConn->chError);
                }
                
                memset(pstConn->chError, 0, 20480);
                
                m_SList.clear();
                m_SList = m_SListTurn;

                
                //�������Ӳ��࣬���¿�ʼ
                //�����߳�
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                m_ServerSocket = 0;
                
                if(m_SocketSTmp > 0)
                {
                    shutdown(m_SocketSTmp, SD_BOTH);
                    closesocket(m_SocketSTmp);
                }
                m_SocketSTmp = 0;
                m_nStatus = NEW_TURN;
            }
        }
    }
    
    //�ȴ���������ӦP2P��������
    void CCVirtualChannel::DealWaitS(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 2500)//5000
        {//�ȴ���ʱ������ʧ��
            m_nStatus = NEW_P2P;
            
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {
                    m_SList[i].buseful = FALSE;
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[MAX_PATH] = {0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]�ȴ����������ݳ�ʱ>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]wait info from S timeout>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                    break;
                }
            }
        }
        else
        {//��������
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {
                    int nret = RecvS(m_SList[i].addr, i, pstConn->chError);
                    switch(nret)
                    {//�յ��������ظ�
                        case JVN_RSP_CONNA://ȡ�õ�ַ ��ʼ����
                        {
                            //�Ƚ��м���Ԥ�� ��Ҫ2s��ʱ��
                            PrePunch(m_pWorker->m_WorkerUDPSocket, m_addrAN);//����Է��򶴣����Է�����������ǰ��
                            //	Punch(m_stConnInfo.nYSTNO, m_pWorker->m_WorkerUDPSocket,&m_addrAN);
                            
                            m_dwStartTime = CCWorker::JVGetTime();
                            if(m_stConnInfo.bLocalTry)
                            {
                                char chIP1[30]={0};
                                char chIP2[30]={0};
                                sprintf(chIP1, "%s:%d", inet_ntoa(m_addrLastLTryAL.sin_addr),ntohs(m_addrLastLTryAL.sin_port));
                                sprintf(chIP2, "%s:%d", inet_ntoa(m_addrAL.sin_addr),ntohs(m_addrAL.sin_port));
                                
                                if(strcmp(chIP1, chIP2))
                                {//��һ�£��õ�ַû���Թ�
                                    m_nStatus = NEW_P2P_L;//��������̽��
                                }
                                else
                                {
                                    //��
                                    //////////////////////////////////////////////////////////////////////////
                                    UDTSOCKET udts = UDT::socket(AF_INET, SOCK_STREAM, 0);
                                    BOOL bReuse = TRUE;
                                    UDT::setsockopt(udts, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                                    //////////////////////////////////////////////////////////////////////////
									int len1 = JVC_MSS;
                                    UDT::setsockopt(udts, 0, UDT_MSS, &len1, sizeof(int));
                                    //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                                    len1=1500*1024;
                                    UDT::setsockopt(udts, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                                    
                                    len1=1000*1024;
                                    UDT::setsockopt(udts, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                                    if(0!=UDT::bind(udts, m_pWorker->m_WorkerUDPSocket)){
                                    OutputDebug("************************************virtualChannel  line: %d",__LINE__);
                                    }
                                    
                                    //���׽�����Ϊ������ģʽ
                                    BOOL block = FALSE;
                                    UDT::setsockopt(udts, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                                    UDT::setsockopt(udts, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                                    LINGER linger;
                                    linger.l_onoff = 0;
                                    linger.l_linger = 0;
                                    UDT::setsockopt(udts, 0, UDT_LINGER, &linger, sizeof(LINGER));
                                    
                                    //								UDT::ystpunch(udts, &m_addrAN, m_stConnInfo.nYSTNO, 500);
                                    m_pWorker->pushtmpsock(udts);
                                    //////////////////////////////////////////////////////////////////////////
                                    
                                    m_nStatus = NEW_P2P_N;//����Ҫ�ظ���ͬһ��ַ��������̽��
                                }
                            }
                            else
                            {
                                //��
                                //////////////////////////////////////////////////////////////////////////
                                UDTSOCKET udts = UDT::socket(AF_INET, SOCK_STREAM, 0);
                                BOOL bReuse = TRUE;
                                UDT::setsockopt(udts, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                                //////////////////////////////////////////////////////////////////////////
							int len1 = JVC_MSS;
                                UDT::setsockopt(udts, 0, UDT_MSS, &len1, sizeof(int));
                                //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                                len1=1500*1024;
                                UDT::setsockopt(udts, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                                
                                len1=1000*1024;
                                UDT::setsockopt(udts, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                                if(0!=UDT::bind(udts, m_pWorker->m_WorkerUDPSocket)){
                                OutputDebug("************************************virtualChannel  line: %d",__LINE__);
                                }
                                
                                //���׽�����Ϊ������ģʽ
                                BOOL block = FALSE;
                                UDT::setsockopt(udts, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                                UDT::setsockopt(udts, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                                LINGER linger;
                                linger.l_onoff = 0;
                                linger.l_linger = 0;
                                UDT::setsockopt(udts, 0, UDT_LINGER, &linger, sizeof(LINGER));
                                
                                //							UDT::ystpunch(udts, &m_addrAN, m_stConnInfo.nYSTNO, 500);
                                m_pWorker->pushtmpsock(udts);
                                //////////////////////////////////////////////////////////////////////////
                                
                                m_nStatus = NEW_P2P_N;//������������
                            }
                        }
                            break;
                        case JVN_RSP_CONNAF://ȡ��ַʧ�� ��������������
                            m_SList[i].buseful = FALSE;
                            m_SListTurn[i].buseful = FALSE;//..δ���ߵķ�����Ҳ�����ٳ���ת��
                            m_nStatus = NEW_P2P;
                            m_dwStartTime = CCWorker::JVGetTime();
                            
                            break;
                        case JVN_CMD_TRYTOUCH://�յ����ش򶴰� Ϊ��ֹ��������ʧ��ֱ������(���ܻ�©�������������ӵĻ���)
                            m_nStatus = NEW_P2P_N;
                            m_dwStartTime = CCWorker::JVGetTime();
                            break;
                        default://δ�յ���Ϣ���յ�����������Ϣ ��������
                            break;
                    }
                    
                    break;//�ҵ���Ӧ�������������˴ν���
                }
            }
        }
    }
    
    //��Ҫ����̽��
    void CCVirtualChannel::DealNEWP2PL(STCONNPROCP *pstConn)
    {
        m_stConnInfo.nShow = JVN_CONNTYPE_LOCAL;
        int ncount = m_SList.size();
        int i=-1;
        for(i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                break;//�ҵ���Ӧ����������������
            }
        }
        
        if(ConnectLocalTry(i, pstConn->chError))
        {//��Զ˷�����Ч����֤��Ϣ
            if(SendPWCheck())
            {//���������֤��Ϣ�ɹ� �ȴ����
                m_nStatus = WAIT_LPWCHECK;
                m_dwStartTime = CCWorker::JVGetTime();
			OutputDebug("virtualConnect localtry sendPwcheck success\n");
            }
            else
            {//���������֤��Ϣʧ�� ��ʼ��������
                m_nStatus = NEW_P2P_N;
			OutputDebug("virtualConnect localtry sendPwcheck failed\n");
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[3*MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]����̽��ʧ��. ���������֤����ʧ�� ��ϸ:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                        strcat(pstConn->chError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]LocalTry failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                        strcat(pstConn->chError, chMsg);
                    }
                }
            }
        }
        else
        {//����ʧ�ܣ�ֱ�ӽ������ӹ���
		OutputDebug("virtualConnect localtry connect failed failed\n");
            m_nStatus = NEW_P2P_N;
        }
    }
    
    //�ȴ������֤
    void CCVirtualChannel::DealWaitLPWCheck(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 4000)//1000
        {//�ȴ���ʱ����������
            m_nStatus = NEW_P2P;
            int ncount = m_SList.size();
            int nIndex=-1;
            for(int i=ncount-1; i>=0; i--)
            {
                if(m_SList[i].buseful)
                {
                    nIndex = i;
                }
                m_SList[i].buseful = FALSE;
            }
            m_stConnInfo.nTURNType = JVN_NOTURN;//���ٽ���ת������
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            if(m_pWorker != NULL)
            {
                m_bShowInfo = TRUE;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "�����֤��ʱ!";
                    m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                }
                else
                {
                    char chMsg[] = "check password timeout!";
                    m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                }
            }
            
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]����̽��ʧ��. �ȴ������֤���ݳ�ʱ",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]LocalTry failed. wait pass info failed.",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
            }
        }
        else
        {//��������
            int nPWData = 0;
            int nret = RecvPWCheck(nPWData);
            if(nret == 1)
            {//��֤ͨ�� ���ӳɹ�����
                m_dwRecvTime = CCWorker::JVGetTime();
                m_nStatus = OK;
                memcpy(&m_addressA, &m_addrAL, sizeof(SOCKADDR_IN));
            }
            else if(nret == 0)
            {//�����֤δͨ�� ֱ�ӽ�������
                m_bPassWordErr = TRUE;
                m_nStatus = NEW_P2P;
                int ncount = m_SList.size();
                int nIndex=-1;
                for(int i=ncount-1; i>=0; i--)
                {
                    if(m_SList[i].buseful)
                    {
                        nIndex = i;
                    }
                    m_SList[i].buseful = FALSE;
                }
                m_stConnInfo.nTURNType = JVN_NOTURN;//���ٽ���ת������
                
                SendData(JVN_CMD_DISCONN, NULL, 0);
                
                m_bPass = FALSE;
                if(m_pWorker != NULL)
                {
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "���δͨ����֤!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                    else
                    {
                        char chMsg[] = "password is wrong!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                }
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]����̽��ʧ��. �����֤ʧ��",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]LocalTry failed.pass failed.",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                }
            }
        }
    }
    
    //��Ҫ����ֱ��
    void CCVirtualChannel::DealNEWP2PN(STCONNPROCP *pstConn)
    {
        m_stConnInfo.nShow = JVN_CONNTYPE_P2P;
        BOOL bfind = FALSE;
        int ncount = m_SList.size();
        for(int i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                bfind = TRUE;
                if(ConnectNet(i, pstConn->chError))
                {//���ӳɹ�������Ч����֤��Ϣ����ΪWAIT_NRECHECK
                    if(SendPWCheck())
                    {//���������֤��Ϣ�ɹ� �ȴ����
                        m_nStatus = WAIT_NPWCHECK;
                        m_dwStartTime = CCWorker::JVGetTime();
                    }
                    else
                    {//���������֤��Ϣʧ�� ��������������
                        m_nStatus = NEW_P2P;
                        
                        if(i>=0)
                        {
                            m_SList[i].buseful = FALSE;
                        }
                        
                        if(m_pWorker->m_bNeedLog)
                        {
                            char chMsg[3*MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]����ֱ��ʧ��. ���������֤����ʧ�� ��ϸ:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(pstConn->chError, chMsg);
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]Net Connect failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(pstConn->chError, chMsg);
                            }
                        }
                    }
                }
                else
                {//����ʧ����Ԥ��������
                    if(ConnectNetTry(m_SList[i].addr, i, pstConn->chError))
                    {
                        if(SendPWCheck())
                        {//���������֤��Ϣ�ɹ� �ȴ����
                            m_nStatus = WAIT_NPWCHECK;
                            m_dwStartTime = CCWorker::JVGetTime();
                        }
                        else
                        {//���������֤��Ϣʧ�� ��������������
                            m_nStatus = NEW_P2P;
                            
                            if(i>=0)
                            {
                                m_SList[i].buseful = FALSE;
                            }
                            
                            if(m_pWorker->m_bNeedLog)
                            {
                                char chMsg[3*MAX_PATH]={0};
                                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                                {
                                    sprintf(chMsg,"<[S%d]����ֱ��ʧ��. ���������֤����ʧ�� ��ϸ:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                    strcat(pstConn->chError, chMsg);
                                }
                                else
                                {
                                    sprintf(chMsg,"<[S%d]Net Connect failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                    strcat(pstConn->chError, chMsg);
                                }
                            }
                        }
                        break;
                    }
                    else
                    {//ʧ�� ������һ��������
                        m_nStatus = NEW_P2P;
                        m_SList[i].buseful = FALSE;
                    }
                }
                
                break;
            }
        }
        
        if(!bfind)
        {
            m_nStatus = NEW_P2P;
        }
    }
    
    //�ȴ������֤
    void CCVirtualChannel::DealWaitNPWCheck(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 10000)
        {//�ȴ���ʱ����������
            m_nStatus = NEW_P2P;
            int ncount = m_SList.size();
            int nIndex=-1;
            for(int i=ncount-1; i>=0; i--)
            {
                if(m_SList[i].buseful)
                {
                    nIndex = i;
                }
                m_SList[i].buseful = FALSE;
            }
            m_stConnInfo.nTURNType = JVN_NOTURN;//���ٽ���ת������
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            if(m_pWorker != NULL)
            {
                m_bShowInfo = TRUE;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "�����֤��ʱ!";
                    m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                }
                else
                {
                    char chMsg[] = "check password timeout!";
                    m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                }
            }
            
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]����ֱ��ʧ��. �ȴ������֤���ݳ�ʱ",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]Net Connect failed. wait pass info failed.",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
            }
        }
        else
        {//��������
            int nPWData=0;
            int nret = RecvPWCheck(nPWData);
            if(nret == 1)
            {//��֤ͨ�� ���ӳɹ�����
                m_dwRecvTime = CCWorker::JVGetTime();
                m_nStatus = OK;
                memcpy(&m_addressA, &m_addrAN, sizeof(SOCKADDR_IN));
            }
            else if(nret == 0)
            {//�����֤δͨ�� ֱ�ӽ�������
                m_bPassWordErr = TRUE;
                m_nStatus = NEW_P2P;
                int ncount = m_SList.size();
                int nIndex=-1;
                for(int i=ncount-1; i>=0; i--)
                {
                    if(m_SList[i].buseful)
                    {
                        nIndex = i;
                    }
                    m_SList[i].buseful = FALSE;
                }
                m_stConnInfo.nTURNType = JVN_NOTURN;//���ٽ���ת������
                
                SendData(JVN_CMD_DISCONN, NULL, 0);
                
                m_bPass = FALSE;
                if(m_pWorker != NULL)
                {
                    m_bShowInfo = TRUE;
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "���δͨ����֤!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                    else
                    {
                        char chMsg[] = "password is wrong!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                }
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]����ֱ��ʧ��. �����֤ʧ��",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]Net Connect failed.pass failed.",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                }
            }
        }
    }
    
    //�ȴ�����ת������
    void CCVirtualChannel::DealNEWTURN(STCONNPROCP *pstConn)
    {
        m_stConnInfo.nShow = JVN_CONNTYPE_TURN;
        
        BOOL bfind = FALSE;
        if(JVN_NOTURN != m_stConnInfo.nTURNType)
        {
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {//��ʼ�����������ص�ַ
                    if(SendSTURN(m_SList[i].addr, i, pstConn->chError))
                    {//���������������ɹ�
                        m_nStatus = WAIT_TURN;
                        m_dwStartTime = CCWorker::JVGetTime();
                        bfind = TRUE;
                        break;
                    }
                    else
                    {//���������������ʧ�� ������һ��������
                        m_SList[i].buseful = FALSE;
                    }
                }
            }
        }
        if(!bfind)
        {
            m_nStatus = FAILD;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                if(!m_bShowInfo)
                {
                    if(m_bPassWordErr)
                    {
                        char chMsg[] = "���δͨ����֤!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                    else
                    {
                        char chMsg[] = "���ӳ�ʱ!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                    
                }
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST(TURN)��ʽ��������ʧ��.ԭ��ֱ�Ϊ:", __FILE__,__LINE__,pstConn->chError);
            }
            else
            {
                if(!m_bShowInfo)
                {
                    if(m_bPassWordErr)
                    {
                        char chMsg[] = "password is wrong!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                    else
                    {
                        char chMsg[] = "connect timeout!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                }
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST(TURN) connect failed.Infos:", __FILE__,__LINE__,pstConn->chError);
            }
        }
    }
    
    //�ȴ�ת����ַ
    void CCVirtualChannel::DealWaitTURN(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 3000)//3000)
        {//�ȴ���ʱ������ʧ�� ��������������
            m_nStatus = NEW_TURN;
            
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
                m_SocketSTmp = 0;
            }
            
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {
                    m_SList[i].buseful = FALSE;
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[MAX_PATH] = {0};
                        if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                        {
                            sprintf(chMsg,"<[S%d]�ȴ�����������TS��ʱ>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                        else
                        {
                            sprintf(chMsg,"<[S%d]wait info TS from S timeout>**",i);
                            strcat(pstConn->chError, chMsg);
                        }
                    }
                    break;
                }
            }
        }
        else
        {//��������
            int ncount = m_SList.size();
            for(int i=0; i<ncount; i++)
            {
                if(m_SList[i].buseful)
                {
                    int nret = RecvSTURN(i, pstConn->chError);
                    switch(nret)
                    {//�յ��������ظ�
                        case JVN_CMD_CONNS2://ȡ�õ�ַ ��ʼ����
                        {
                            m_nStatus = RECV_TURN;
                            
                            m_dwStartTime = CCWorker::JVGetTime();
                        }
                            
                            break;
                        case JVN_RSP_CONNAF://ȡ��ַʧ�� ��������������
                            m_SList[i].buseful = FALSE;
                            m_nStatus = NEW_TURN;
                            m_dwStartTime = CCWorker::JVGetTime();
                            break;
                        case JVN_CMD_TRYTOUCH://�յ��򶴰� ֱ������
                        default://δ�յ���Ϣ���յ�����������Ϣ ��������
                            break;
                    }
                    break;
                }
            }
        }
    }
    
    //ȡ��ת����ַ
    void CCVirtualChannel::DealRecvTURN(STCONNPROCP *pstConn)
    {
        BOOL bfind=FALSE;
        int ncount = m_SList.size();
        for(int i=0; i<ncount; i++)
        {
            if(m_SList[i].buseful)
            {
                bfind = TRUE;
                if(ConnectTURN(i, pstConn->chError))
                {//��Զ˷�����Ч����֤��Ϣ
                    if(SendPWCheck())
                    {//���������֤��Ϣ�ɹ� �ȴ����
                        m_nStatus = WAIT_TSPWCHECK;
                        m_dwStartTime = CCWorker::JVGetTime();
                    }
                    else
                    {//���������֤��Ϣʧ�� ��������������
                        m_nStatus = NEW_TURN;
                        
                        if(i>=0)
                        {
                            m_SList[i].buseful = FALSE;
                        }
                        
                        if(m_pWorker->m_bNeedLog)
                        {
                            char chMsg[3*MAX_PATH]={0};
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                sprintf(chMsg,"<[S%d]TURN����ʧ��. ���������֤����ʧ�� ��ϸ:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(pstConn->chError, chMsg);
                            }
                            else
                            {
                                sprintf(chMsg,"<[S%d]TURN Connect failed. send pass info failed. Info:%s>**",i,(char *)UDT::getlasterror().getErrorMessage());
                                strcat(pstConn->chError, chMsg);
                            }
                        }
                    }
                }
                else
                {//����ʧ�ܣ���������������
                    m_nStatus = NEW_TURN;
                    m_SList[i].buseful = FALSE;
                }
                
                break;
            }
        }
        if(!bfind)
        {
            m_nStatus = NEW_TURN;
        }
    }
    
    //�ȴ������֤
    void CCVirtualChannel::DealWaitTSPWCheck(STCONNPROCP *pstConn)
    {
        pstConn->dwendtime = CCWorker::JVGetTime();
        if(pstConn->dwendtime > m_dwStartTime + 5000)
        {//�ȴ���ʱ����������
            m_nStatus = NEW_TURN;
            int ncount = m_SList.size();
            int nIndex=-1;
            for(int i=ncount-1; i>=0; i--)
            {
                if(m_SList[i].buseful)
                {
                    nIndex = i;
                }
                m_SList[i].buseful = FALSE;
            }
            
            SendData(JVN_CMD_DISCONN, NULL, 0);
            
            m_bPass = FALSE;
            if(m_pWorker != NULL)
            {
                m_bShowInfo = TRUE;
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    char chMsg[] = "�����֤��ʱ!";
                    m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                }
                else
                {
                    char chMsg[] = "check password timeout!";
                    m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                }
            }
            
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]TURN����ʧ��. �ȴ������֤���ݳ�ʱ",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]TURN Connect failed. wait pass info failed.",nIndex);
                    strcat(pstConn->chError, chMsg);
                }
            }
        }
        else
        {//��������
            int nPWData=0;
            int nret = RecvPWCheck(nPWData);
            if(nret == 1)
            {//��֤ͨ�� ���ӳɹ�����
                m_dwRecvTime = CCWorker::JVGetTime();
                m_nStatus = OK;
                memcpy(&m_addressA, &m_addrTS, sizeof(SOCKADDR_IN));
            }
            else if(nret == 0)
            {//�����֤δͨ�� ֱ�ӽ�������
                m_bPassWordErr = TRUE;
                m_nStatus = NEW_TURN;
                int ncount = m_SList.size();
                int nIndex=-1;
                for(int i=ncount-1; i>=0; i--)
                {
                    if(m_SList[i].buseful)
                    {
                        nIndex = i;
                    }
                    m_SList[i].buseful = FALSE;
                }
                
                SendData(JVN_CMD_DISCONN, NULL, 0);
                
                m_bPass = FALSE;
                if(m_pWorker != NULL)
                {
                    m_bShowInfo = TRUE;
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        char chMsg[] = "���δͨ����֤!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                    else
                    {
                        char chMsg[] = "password is wrong!";
                        m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg);
                    }
                }
                
                if(m_pWorker->m_bNeedLog)
                {
                    char chMsg[MAX_PATH]={0};
                    if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                    {
                        sprintf(chMsg,"<[S%d]TURN����ʧ��. �����֤ʧ��",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                    else
                    {
                        sprintf(chMsg,"<[S%d]TURN Connect failed.pass failed.",nIndex);
                        strcat(pstConn->chError, chMsg);
                    }
                }
            }
        }
    }
    
    //��ȡ��������ַ������ֱ�ӻ�ȡ
    void CCVirtualChannel::GetSerAndBegin(STCONNPROCP *pstConn)
    {
        ServerList slist;
        char chIPA[16]={0};
        int nport = 0;
        
        if(!m_pWorker->GetIndexServerList(m_stConnInfo.chGroup, m_ISList, 1, m_stConnInfo.nLocalPort))
        {
            if(!m_pWorker->GetIndexServerList(m_stConnInfo.chGroup, m_ISList, 2, m_stConnInfo.nLocalPort))
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    //	char chMsg[] = "��ȡYST��������Ϣʧ��!";
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST��ʽ��������ʧ��.ԭ��:��ȡ����������ʧ��", __FILE__,__LINE__);
                }
                else
                {
                    //	char chMsg[] = "get server address failed!";
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "YST connect failed.Info:get index server address failed.", __FILE__,__LINE__);
                }
            }
        }
        
        m_SList.clear();
        m_SListTurn.clear();
        m_SListBak.clear();
        
        m_nStatus = WAIT_INDEXSER_REQ;//������ͨ��ʽ����ʱ�����Ƚ��з����ڲ�ѯ����߷���������Ч��
        
        //��ʼ����ʱ�׽������ں����������ѯ
        pstConn->udpsocktmp = socket(AF_INET, SOCK_DGRAM, 0);
        if (pstConn->udpsocktmp > 0)
        {
#ifndef WIN32
            pstConn->sin.sin_addr.s_addr = INADDR_ANY;
#else
            pstConn->sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            pstConn->sin.sin_family = AF_INET;
            pstConn->sin.sin_port = htons(0);
            
            BOOL bReuse = TRUE;
            setsockopt(pstConn->udpsocktmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
            
            if (bind(pstConn->udpsocktmp, (struct sockaddr*)&pstConn->sin, sizeof(pstConn->sin)) < 0)
            {
                shutdown(pstConn->udpsocktmp,SD_BOTH);
                closesocket(pstConn->udpsocktmp);
                pstConn->udpsocktmp = 0;
                m_nStatus = NEW_P2P;//����ֱ�ӽ�������
            }
        }
        else
        {
            m_nStatus = NEW_P2P;//����ֱ�ӽ�������
        }
    }
    
#ifndef WIN32
    void* CCVirtualChannel::ConnProc(void* pParam)
#else
    UINT WINAPI CCVirtualChannel::ConnProc(LPVOID pParam)
#endif
    {
        CCVirtualChannel *pWorker = (CCVirtualChannel *)pParam;
        
#ifndef WIN32
        if(pWorker == NULL)
        {
            return NULL;
        }
#else
        if(pWorker == NULL)
        {
            return 0;
        }
        
        ::WaitForSingleObject(pWorker->m_hStartEventC, INFINITE);
        if(pWorker->m_hStartEventC > 0)
        {
            CloseHandle(pWorker->m_hStartEventC);
        }
        pWorker->m_hStartEventC = 0;
#endif
        
        STCONNPROCP stConnP;
        
        stConnP.udpsocktmp=0;
        stConnP.nSockAddrLen = sizeof(SOCKADDR_IN);
        stConnP.slisttmp.clear();
        
        //�ж���������
        if(pWorker->m_stConnInfo.bYST)
        {//����ͨ��ʽ����
            pWorker->m_nStatus = NEW_YST;
        }
        else
        {//IPֱ��
            pWorker->m_nStatus = FAILD;//NEW_IP;
        }
        
        stConnP.dwendtime = 0;
        memset(stConnP.chAVersion, 0, MAX_PATH);
        memset(stConnP.chError, 0, 20480);
        
        //0�汾���� >0���ذ汾̫�� <0�Է��汾̫��
        
        pWorker->m_bShowInfo = FALSE;//���ڱ�������ͨ����ʱ�ظ���ʾ
        //////////////////////////////////////////////////////////////////////////
        
        memset(stConnP.szIPAddress, 0, 100);
        
        //////////////////////////////////////////////////////////////////////////
        
        /*
         һ.IPֱ��
         ����Э�鷢������->��������֤��Ϣ->�յ�����֤��Ϣ->�����֤->�ɹ���ʧ��
         ->δ�յ�����֤��Ϣ->����Э�鷢�������֤��Ϣ->�ɹ���ʧ��
         
         ��.��������
         ����̽�� ֱ�� ת��
         */
        if(pWorker->m_bDirectConnect)
        {
            pWorker->m_bDirectConnect = FALSE;
            
            pWorker->m_nStatus = DIRECT_CONNECT;
        }
        
        while(TRUE)
        {
#ifndef WIN32
            if(pWorker->m_bExit || pWorker->m_bEndC)
            {
                break;
            }
#else
            if(pWorker->m_bExit || WAIT_OBJECT_0 == WaitForSingleObject(pWorker->m_hEndEventC, 0))
            {
                if(pWorker->m_hEndEventC > 0)
                {
                    CloseHandle(pWorker->m_hEndEventC);
                    pWorker->m_hEndEventC = 0;
                }
                
                break;
            }
#endif
            
            switch(pWorker->m_nStatus)
            {
                case DIRECT_CONNECT://ʵ�����ӽ���
                {
                    pWorker->DealConnectIP(&stConnP);
                }
                    break;
                case NEW_HAVEIP://��������
                {
                    pWorker->DealConnectIP(&stConnP);
                }
                    break;
                case NEW_YST://�µ�����ͨ��������
                {
                    pWorker->DealNewYST(&stConnP);
                }
                    break;
                case WAIT_SER_REQ://����������Ͳ�ѯ��������
                {
                    pWorker->DealWaitSerREQ(&stConnP);
                }
                    break;
                case WAIT_SER_RSP://�ȴ���������� �ù��������2����
                {//���ݷ�������Է������������Ӻ���Ч���ж�
                    pWorker->DealWaitSerRSP(&stConnP);
                }
                    break;
                case WAIT_INDEXSER_REQ://���������������Ͳ�ѯ��������
                {
                    pWorker->DealWaitIndexSerREQ(&stConnP);
                }
                    break;
                case WAIT_INDEXSER_RSP://�ȴ���������� �ù��������2����
                {
                    pWorker->DealWaitIndexSerRSP(&stConnP);
                }
                    break;
                case WAIT_CHECK_A_NAT:
                {
                    pWorker->DealWaitNatREQ(&stConnP);
                }
                    break;
                case WAIT_NAT_A:
                {
                    pWorker->DealWaitNatRSP(&stConnP);
                }
                    break;
                case WAIT_MAKE_HOLE://�ȴ����ӹ��̣�һֱ�ڵȴ�
                {
                    CCWorker::jvc_sleep(2);
                }
                    break;
                    
                case OK://���ӳɹ�
                {//�������������߳�
                    pWorker->DealOK(&stConnP);
                    return 0;
                }
                    break;
                case FAILD://����ʧ��
                {
                    pWorker->DealFAILD(&stConnP);
                    return 0;
                }
                    break;
                case NEW_P2P://�ȴ�������������
                {
                    pWorker->DealNEWP2P(&stConnP);
                }
                    break;
                case WAIT_S://�ȴ���������Ӧ
                {
                    pWorker->DealWaitS(&stConnP);
                }
                    break;
                case NEW_P2P_L://��Ҫ����̽��
                {//ֱ������IP���ɹ�������Ч����֤��Ϣ����ΪWAIT_LRECHECK;ʧ����ΪNEW_P2P_N��ʼ��������
                    pWorker->DealNEWP2PL(&stConnP);
                }
                    break;
                case WAIT_LPWCHECK://�ȴ������֤
                {//��֤�ɹ���ΪOK, ������ΪFAILD
                    pWorker->DealWaitLPWCheck(&stConnP);
                }
                    break;
                case NEW_P2P_N://��Ҫ����ֱ��
                {
                    pWorker->DealNEWP2PN(&stConnP);
                }
                    break;
                case WAIT_NPWCHECK://�ȴ������֤
                {
                    pWorker->DealWaitNPWCheck(&stConnP);
                }
                    break;
                case NEW_TURN://�ȴ�����ת������
                {
#ifdef MOBILE_CLIENT
                    pWorker->m_nStatus = FAILD;
                    pWorker->DealFAILD(&stConnP);
                    OutputDebug("virtualConnect turn return\n");
                    return 0;
#endif
                    
                    pWorker->DealNEWTURN(&stConnP);
                }
                    break;
                case WAIT_TURN://�ȴ�ת����ַ
                {
                    pWorker->DealWaitTURN(&stConnP);
                }
                    break;
                case RECV_TURN://ȡ��ת����ַ
                {
                    pWorker->DealRecvTURN(&stConnP);
                }
                    break;
                case WAIT_TSPWCHECK://�ȴ������֤
                {
                    pWorker->DealWaitTSPWCheck(&stConnP);
                }
                    break;
                default:
                    break;
            }
            
            CCWorker::jvc_sleep(2);
        }
        
        return 0;
    }
    
    BOOL CCVirtualChannel::SendPWCheck()
    {//����(1)+����(4)+�û�����(4)+���볤(4)+�û���+����
        int nNLen = strlen(m_stConnInfo.chPassName);
        if(nNLen > MAX_PATH)
        {
            nNLen = 0;
        }
        int nWLen = strlen(m_stConnInfo.chPassWord);
        if(nWLen > MAX_PATH)
        {
            nWLen = 0;
        }
        
        BYTE data[3*MAX_PATH]={0};
        data[0] = JVN_REQ_CHECKPASS;
        memcpy(&data[1], &nNLen, 4);
        memcpy(&data[5], &nWLen, 4);
        if(nNLen > 0 && nNLen < MAX_PATH)
        {
            memcpy(&data[9], m_stConnInfo.chPassName, nNLen);
        }
        if(nWLen > 0 && nWLen < MAX_PATH)
        {
            memcpy(&data[9+nNLen], m_stConnInfo.chPassWord, nWLen);
        }
        
        //UDP����
        if(m_stConnInfo.nConnectType == TYPE_MO_UDP || m_stConnInfo.nConnectType == TYPE_3GMO_UDP)
        {//�ֻ�����������ʱ��Ҫ��֪�Է������ֻ�����
            char chtmp[10]={0};
            chtmp[0] = JVN_CMD_MOTYPE;
            CCChannel::udpsenddata(m_ServerSocket, chtmp, 5, TRUE);
        }
        
        if(0 >= CCChannel::udpsenddata(m_ServerSocket, (char *)data, nNLen + nWLen + 9, TRUE))
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    
    int CCVirtualChannel::RecvPWCheck(int &nPWData)
    {
        int rs = 0;
        int rsize = 0;
        int nLen = 0;
        nPWData = 0;
        
        BYTE uchRecvBuf[10240]={0};
        //UDP����
        m_nFYSTVER = UDT::getystverF(m_ServerSocket);//��ȡԶ��Э��汾
        if(m_nFYSTVER >= JVN_YSTVER4)
        {//֧��msg
            if(0 < (rs = UDT::recvmsg(m_ServerSocket, (char *)uchRecvBuf, 10240)))
            {//�յ�����
                nLen=-1;
                if(uchRecvBuf[0] == JVN_RSP_CHECKPASS)
                {//�����֤ ����(1) + ����(4) + �Ƿ�ͨ��(1) + [����ֵ(4)]
                    memcpy(&nLen, &uchRecvBuf[1], 4);
                    if(nLen == 1 || nLen == 5)
                    {
                        if(nLen == 5)
                        {//��Э�� ������ֵ
                            memcpy(&nPWData, &uchRecvBuf[6], 4);
                        }
                        
                        if(uchRecvBuf[5] == 1)
                        {
                            return 1;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                }
            }
        }
        else
        {
            if(0 < (rs = UDT::recv(m_ServerSocket, (char *)uchRecvBuf, 1, 0)))
            {//�յ�����
                nLen=-1;
                if(uchRecvBuf[0] == JVN_RSP_CHECKPASS)
                {//�����֤ ����(1) + ����(4) + �Ƿ�ͨ��(1) + [����ֵ(4)]
                    rsize = 0;
                    rs = 0;
                    while (rsize < 4)
                    {
                        if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&uchRecvBuf[rsize], 4 - rsize, 0)))
                        {
                            return -1;
                        }
                        else if(rs == 0)
                        {
                            CCWorker::jvc_sleep(1);
                            continue;
                        }
                        
                        rsize += rs;
                    }
                    
                    memcpy(&nLen, uchRecvBuf, 4);
                    if(nLen == 1 || nLen == 5)
                    {
                        rsize = 0;
                        rs = 0;
                        while (rsize < nLen)
                        {
                            if(UDT::ERROR == (rs = UDT::recv(m_ServerSocket, (char *)&uchRecvBuf[rsize], nLen - rsize, 0)))
                            {
                                return -1;
                            }
                            else if(rs == 0)
                            {
                                CCWorker::jvc_sleep(1);
                                continue;
                            }
                            
                            rsize += rs;
                        }
                        
                        if(nLen == 5)
                        {//��Э�� ������ֵ
                            memcpy(&nPWData, &uchRecvBuf[1], 4);
                        }
                        
                        if(uchRecvBuf[0] == 1)
                        {
                            return 1;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                }
            }
        }
        
        return -1;
    }
    
    
    BOOL CCVirtualChannel::SendSP2P(SOCKADDR_IN addrs, int nIndex, char *pchError)
    {
        return TRUE;
    }
    
    ///////////////////������������������������������������������������������������������������������������������������������������
    
    int CCVirtualChannel::RecvS(SOCKADDR_IN addrs, int nIndex, char *pchError)
    {
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        
        m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
        BOOL bReuse = TRUE;
        UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
        UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
#ifdef MOBILE_CLIENT
        len1=1500*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
        
        len1=1000*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif

        //////////////////////////////////////////////////////////////////////////
        if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
        {//�󶨵�ָ���˿�ʧ�ܣ���Ϊ�󶨵�����˿�
            OutputDebug("************************************virtualChannel  line: %d",__LINE__);
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            return FALSE;
        }
        
        //���׽�����Ϊ������ģʽ
        BOOL block = FALSE;
        UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
        UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
        LINGER linger;
        linger.l_onoff = 0;
        linger.l_linger = 0;
        UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
        
        //////////////////////////////////////////////////////////////////////////
        //��ȡ��ǰ�ͻ��������������������ֳ��������е�ַ
        int nSVer = 0;
        m_NATListTEMP.clear();
        m_pWorker->GetNATADDR(m_NatList,&m_NATListTEMP, addrs, nSVer);
        //////////////////////////////////////////////////////////////////////////
        char chdata[1024]={0};
        //����NAT�б�
        BYTE strIP[1024] = {0};
        int maxIP = m_NATListTEMP.size() > 20 ?20:m_NATListTEMP.size();
        int nSize = maxIP * 6;
        for(int k = 0;k < maxIP;k ++)
        {
            memcpy(&strIP[k * 6 + 0],m_NATListTEMP[k].ip,4);
            memcpy(&strIP[k * 6 + 4],&m_NATListTEMP[k].port,2);
        }
        
        if(nSVer < 20336)
        {//���ض�̫�ɣ���֧�������ݣ�����ԭ����
            nSize = 0;
        }
        
        int nS = m_SList.size();
        //	nS = 1;
        for(int m = 0;m < nS;m ++)
        {
            if(!m_SList[m].buseful)
                continue;
            
//            OutputDebug("%d",m);
            m_pWorker->SendUdpDataForMobile(m_pWorker->m_WorkerUDPSocket,m_SList[m].addr,m_stConnInfo.nYSTNO,strIP,nSize);
            //	DWORD EEE =CCWorker::JVGetTime();
            CCWorker::jvc_sleep(500);
            // DWORD DDD = CCWorker::JVGetTime();
            // printf("8888888888888888 TIEM: %d\n",(DDD-EEE));
            //	usleep(10*1000);
            UDP_LIST list;
            int nret = m_pWorker->GetUdpData(m_pWorker->m_WorkerUDPSocket,m_stConnInfo.nYSTNO,&list);
            if(nret > 0)
            {
                UDP_PACKAGE pkg = {0};
                
                for(::std::vector<UDP_PACKAGE >::iterator i = list.begin(); i != list.end();i ++)
                {
#ifdef WIN32
                    memcpy(&pkg,i,sizeof(UDP_PACKAGE));
#else
                    memcpy(&pkg.addrRemote,&i->addrRemote,sizeof(SOCKADDR_IN));
                    pkg.nLen = i->nLen;
                    pkg.nLocal = i->nLocal;
                    pkg.sSocket = i->sSocket;
                    pkg.time = i->time;
                    memcpy(pkg.cData,i->cData,sizeof(pkg.cData));
#endif
                    int nType = 0;
                    memcpy(&nType,pkg.cData,4);
                    if(nType == JVN_CMD_TRYTOUCH)
                    {
                        memcpy(&m_addrAN,&pkg.addrRemote,sizeof(sockaddr));
                        if(m_ServerSocket > 0)
                        {
                            m_pWorker->pushtmpsock(m_ServerSocket);
                            m_ServerSocket = 0;
                        }
                        
                        m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
                        
                        BOOL bReuse = TRUE;
                        UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                        //////////////////////////////////////////////////////////////////////////
				int len1 = JVC_MSS;
                        UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
                        //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                        len1=1500*1024;
                        UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                        
                        len1=1000*1024;
                        UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                        if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
				{

					OutputDebug(" virtualChannel m_addrAN1:  %s : %d \n",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
                            if(m_ServerSocket > 0) 
                            { 
                                m_pWorker->pushtmpsock(m_ServerSocket); 
                            } 
                            m_ServerSocket = 0; 
                            
                            if(m_pWorker->m_bNeedLog) 
                            { 
                                char chMsg[MAX_PATH]={0}; 
                                sprintf(chMsg,"<[S%d]bind Jsock failed,INFO:",nIndex); 
                                strcat(pchError, chMsg); 
                                strcat(pchError, UDT::getlasterror().getErrorMessage()); 
                                strcat(pchError, ">**"); 
                            } 
                            
                            return JVN_RSP_CONNAF; 
                        } 
                        //���׽�����Ϊ������ģʽ 
                        BOOL block = FALSE; 
                        UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL)); 
                        UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL)); 
                        LINGER linger; 
                        linger.l_onoff = 0; 
                        linger.l_linger = 0; 
                        UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER)); 
				OutputDebug(" virtualChannel m_addrAN 2:  %s : %d \n",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
                        return JVN_CMD_TRYTOUCH; 
                    } 
                    else if(nType == YST_A_NEW_ADDRESS) 
                    { 
                        memcpy(&m_addrAN, &pkg.cData[8], sizeof(SOCKADDR_IN)); 
                        m_addrAN.sin_family = AF_INET; 
                        
                        memcpy(&m_addrAL, &pkg.cData[8 + sizeof(SOCKADDR_IN)], sizeof(SOCKADDR_IN)); 
                        m_addrAL.sin_family = AF_INET; 
				OutputDebug(" virtualChannel m_addrAL 3:  %s : %d \n",inet_ntoa(m_addrAL.sin_addr),ntohs(m_addrAL.sin_port));
				OutputDebug(" virtualChannel m_addrAN 3:  %s : %d \n",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
                        return JVN_RSP_CONNA; 
                    } 
                    else if(nType == JVN_RSP_CONNA) 
                    { 
                        memcpy(&m_addrAN, &pkg.cData[8], sizeof(SOCKADDR_IN)); 
                        m_addrAN.sin_family = AF_INET; 
                        
				memcpy(&m_addrAL, &pkg.cData[8 + sizeof(SOCKADDR_IN)], sizeof(SOCKADDR_IN));
				m_addrAL.sin_family = AF_INET;
				OutputDebug(" virtualChannel m_addrAL 4:  %s : %d \n",inet_ntoa(m_addrAL.sin_addr),ntohs(m_addrAL.sin_port));
				OutputDebug(" virtualChannel m_addrAN 4:  %s : %d \n",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
                        DWORD dwCurr = CCWorker::JVGetTime(); 
                        if(dwCurr > m_dwStartTime + 500)//��ʱ ����ԭNAT 
                        { 
                            return JVN_RSP_CONNA; 
                        } 
                        return YST_A_NEW_ADDRESS;//û��ʱ �ⲿ�����ٴε���nat��ַ 
                    } 
                } 
            } 
            
        } 
        
        return -1;//JVN_RSP_CONNAF; 
    }
    
    BOOL CCVirtualChannel::ConnectLocalTry(int nIndex, char *pchError)
    {
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
        BOOL bReuse = TRUE;
        UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
        UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
        //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
        len1=1500*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
        
        len1=1000*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
        if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
        {//�󶨵�ָ���˿�ʧ�ܣ���Ϊ�󶨵�����˿�
            OutputDebug("************************************virtualChannel  line: %d",__LINE__);
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "��������ʧ��.��������ʧ��(�����Ƕ˿ڱ�ռ) ��ϸ:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            
            return FALSE;
        }
        
        //���׽�����Ϊ������ģʽ
        BOOL block = FALSE;
        UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
        UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
        LINGER linger;
        linger.l_onoff = 0;
        linger.l_linger = 0;
        UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
        
        STJUDTCONN stcon;
		stcon.pbQuickExit = &m_bExit;
        stcon.u = m_ServerSocket;
        stcon.name = (SOCKADDR *)&m_addrAL;
        stcon.namelen = sizeof(SOCKADDR);
        stcon.nChannelID = -2;//m_stConnInfo.nChannel;
        stcon.nCheckYSTNO = m_stConnInfo.nYSTNO;
        memcpy(stcon.chCheckGroup, m_stConnInfo.chGroup, 4);
        stcon.nLVer_new = JVN_YSTVER;
        stcon.nLVer_old = JVN_YSTVER1;//�������������򲻼���49-67������
        stcon.nMinTime = 500;
        stcon.uchVirtual = 1;
        stcon.nVChannelID = m_stConnInfo.nChannel;
	SOCKADDR_IN addr = {0};
		memcpy(&addr,stcon.name,sizeof(SOCKADDR_IN));
		OutputDebug("connect localtry a ....%s:%d",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        if(UDT::ERROR == UDT::connect(stcon))//1000
        {
		OutputDebug("connect localtry a ....failed");
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[3*MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]����̽��ʧ��. ��������ʧ�� ��ϸ:%s",nIndex,(char *)UDT::getlasterror().getErrorMessage());
                    strcat(pchError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]LocalTry failed. connect op. failed. INFO:%s",nIndex,(char *)UDT::getlasterror().getErrorMessage());
                    strcat(pchError, chMsg);
                }
            }
            
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            memcpy(&m_addrLastLTryAL, &m_addrAL, sizeof(SOCKADDR_IN));
            
            //////////////////////////////////////////////////////////////////////////
            //char ch[100]={0};
            //sprintf(ch,"AL:%s:%d",inet_ntoa(m_addrAL.sin_addr),ntohs(m_addrAL.sin_port));
            //sprintf(ch,"AN:%s:%d",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
            //////////////////////////////////////////////////////////////////////////
            
            return FALSE;
        }
        else
	{OutputDebug("connect localtry a ....success");
            memcpy(&m_addressA, stcon.name, sizeof(SOCKADDR_IN));
            return TRUE;
        }
    }
    
    BOOL CCVirtualChannel::ConnectNet(int nIndex, char *pchError)
    {
        //////////////////////////////////////////////////////////////////////////
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        
        m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
        BOOL bReuse = TRUE;
        UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
        UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
        //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
        len1=1500*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
        
        len1=1000*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
        if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
        {//�󶨵�ָ���˿�ʧ�ܣ���Ϊ�󶨵�����˿�
            OutputDebug("************************************virtualChannel  line: %d",__LINE__);
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "��������ʧ��.��������ʧ��(�����Ƕ˿ڱ�ռ) ��ϸ:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            
            return FALSE;
        }
        
        //���׽�����Ϊ������ģʽ
        BOOL block = FALSE;
        UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
        UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
        LINGER linger;
        linger.l_onoff = 0;
        linger.l_linger = 0;
        UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
        //////////////////////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        //char ch[100]={0};
        //sprintf(ch,"AL:%s:%d",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
        //////////////////////////////////////////////////////////////////////////
        
        STJUDTCONN stcon;
		stcon.pbQuickExit = &m_bExit;
        stcon.u = m_ServerSocket;
        stcon.name = (SOCKADDR *)&m_addrAN;
        stcon.namelen = sizeof(SOCKADDR);
        stcon.nChannelID = -2;//m_stConnInfo.nChannel;
        stcon.nCheckYSTNO = m_stConnInfo.nYSTNO;
        memcpy(stcon.chCheckGroup, m_stConnInfo.chGroup, 4);
        stcon.nLVer_new = JVN_YSTVER;
        stcon.nLVer_old = JVN_YSTVER1;//�������������򲻼���49-67������
	stcon.nMinTime = 2000;//1000
        stcon.uchVirtual = 1;
        stcon.nVChannelID = m_stConnInfo.nChannel;
	SOCKADDR_IN addr = {0};
	memcpy(&addr,stcon.name,sizeof(SOCKADDR_IN));
	OutputDebug("connect net a ....%s:%d",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        
        if(UDT::ERROR == UDT::connect(stcon))//3000
	{OutputDebug("connect net a ....failed");
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[3*MAX_PATH]={0};
                sprintf(chMsg,"<[S%d]Net connect failed,Info:%s>**",nIndex, UDT::getlasterror().getErrorMessage());
                strcat(pchError, chMsg);
            }
            
            return FALSE;
        }
        
        memcpy(&m_addressA, stcon.name, sizeof(SOCKADDR_IN));
        return TRUE;
    }
    
    BOOL CCVirtualChannel::ConnectNetTry(SOCKADDR_IN addrs, int nIndex, char *pchError)
    {
        //Ԥ�ⲿ�����Ĳ��� ��ʱͣ��
        //////////////////////////////////////////////////////////////////////////
        return FALSE;
        //////////////////////////////////////////////////////////////////////////
    }
    
    BOOL CCVirtualChannel::SendSTURN(SOCKADDR_IN addrs, int nIndex, char *pchError)
    {
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        
        //������ʱUDP�׽���
        SOCKET stmp = socket(AF_INET, SOCK_DGRAM,0);
        
        SOCKADDR_IN addrSrv;
#ifndef WIN32
        addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
#else
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#endif
        addrSrv.sin_family = AF_INET;
        addrSrv.sin_port = htons(0);//htons(m_nLocalStartPort);
        
        BOOL bReuse = TRUE;
        setsockopt(stmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        
        //���׽���
        bind(stmp, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
        
        //	BOOL bReuse = TRUE;
        //	setsockopt(stmp, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        
        //�����������TS��ַ
        //��S��������
        BYTE data[JVN_ASPACKDEFLEN]={0};
        int nType = JVN_REQ_S2;
        memcpy(&data[0], &nType, 4);
        memcpy(&data[4], &m_stConnInfo.nYSTNO, 4);
        data[8] = (m_stConnInfo.nTURNType == JVN_ONLYTURN)?1:0;
        
        if(CCChannel::sendtoclient(stmp, (char *)data, 9, 0, (SOCKADDR *)&(addrs), sizeof(SOCKADDR),2) > 0)
        {
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
            }
            m_SocketSTmp = stmp;
            return TRUE;
        }
        else
        {
#ifdef WIN32
            if(WSAGetLastError() == 10038)
            {
                if(stmp > 0)
                {
                    closesocket(stmp);
                }
                if(m_SocketSTmp > 0)
                {
                    closesocket(m_SocketSTmp);
                }
                stmp = 0;
                m_SocketSTmp = 0;
                
                if(SendSTURN(addrs, nIndex, pchError))
                {
                    return TRUE;
                }
            }
#endif
            
            char chMsg[MAX_PATH]={0};
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                sprintf(chMsg,"<[S%d]���������������TSʧ��>**",nIndex);
                strcat(pchError, chMsg);
            }
            else
            {
                sprintf(chMsg,"<[S%d]send data TS to server failed>**",nIndex);
                strcat(pchError, chMsg);
            }
            
            if(stmp > 0)
            {
                closesocket(stmp);
            }
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
            }
            stmp = 0;
            m_SocketSTmp = 0;
            return FALSE;
        }
    }
    
    int CCVirtualChannel::RecvSTURN(int nIndex, char *pchError)
    {
        BYTE data[JVN_ASPACKDEFLEN]={0};
        int nType = JVN_CMD_CONNS2;
        memset(data, 0, JVN_ASPACKDEFLEN);
        SOCKADDR_IN addrtemp;
        int naddrlen = sizeof(SOCKADDR);
        int nrecvlen=0;
        if((nrecvlen = CCChannel::receivefromm(m_SocketSTmp, (char *)data, JVN_ASPACKDEFLEN, 0, (SOCKADDR *)&(addrtemp), &naddrlen, 100)) > 0)
        {
            memcpy(&nType, &data[0], 4);
            if(nType == JVN_CMD_CONNS2)
            {//�յ��������ɹ�����
                memcpy(&m_addrTS, &data[8], sizeof(SOCKADDR_IN));
                
                return JVN_CMD_CONNS2;
            }
            else if(nType == JVN_RSP_CONNAF)
            {//�յ�������ʧ������
                char chMsg[MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]����������ʧ�� ��ϸ:",nIndex);
                    strcat(pchError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]server return failed info:",nIndex);
                    strcat(pchError, chMsg);
                }
                memset(chMsg, 0, MAX_PATH);
                sprintf(chMsg,"%d",m_stConnInfo.nYSTNO);
                strcat(pchError, (char *)&data[4]);
                strcat(pchError, " YSTNO:");
                strcat(pchError, chMsg);
                strcat(pchError, ">**");
                
                closesocket(m_SocketSTmp);
                m_SocketSTmp = 0;
                
                //�˴����⴦����ֹ��Դ���ͷ�
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                }
                m_ServerSocket = 0;
                
                return JVN_RSP_CONNAF;
            }
            else if(nType == JVN_CMD_TRYTOUCH)
            {//�յ����ش򶴰�,���ٵȷ�������Ϣ��ֱ������
                if(nrecvlen != 8)
                {//�򶴰�������Ч���޷��ж��ǲ�����Ч�Ĵ򶴰�������
                    return -2;
                }
                else
                {
                    int nyst=0;
                    memcpy(&nyst, &data[4], 4);
                    if(nyst != m_stConnInfo.nYSTNO)
                    {//�򶴰�����������Ҫ�����أ�����
                        return -2;
                    }
                }
                
                memcpy(&m_addrAN, &addrtemp, sizeof(SOCKADDR_IN));
                
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                    m_ServerSocket = 0;
                }
                
                m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
                
                BOOL bReuse = TRUE;
                UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
                //////////////////////////////////////////////////////////////////////////
			int len1 = JVC_MSS;
                UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
                //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
                len1=1500*1024;
                UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
                
                len1=1000*1024;
                UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
                if (UDT::ERROR == UDT::bind(m_ServerSocket, m_pWorker->m_WorkerUDPSocket))
                {OutputDebug("************************************virtualChannel  line: %d",__LINE__);
                    if(m_SocketSTmp > 0)
                    {
                        closesocket(m_SocketSTmp);
                    }
                    m_SocketSTmp = 0;
                    
                    if(m_ServerSocket > 0)
                    {
                        m_pWorker->pushtmpsock(m_ServerSocket);
                    }
                    m_ServerSocket = 0;
                    
                    if(m_pWorker->m_bNeedLog)
                    {
                        char chMsg[MAX_PATH]={0};
                        sprintf(chMsg,"<[S%d]bind Jsock failed,INFO:",nIndex);
                        strcat(pchError, chMsg);
                        strcat(pchError, UDT::getlasterror().getErrorMessage());
                        strcat(pchError, ">**");
                    }
                    
                    return JVN_RSP_CONNAF;
                }
                //���׽�����Ϊ������ģʽ
                BOOL block = FALSE;
                UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
                UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
                LINGER linger;
                linger.l_onoff = 0;
                linger.l_linger = 0;
                UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
                return JVN_CMD_TRYTOUCH;
            }
            else
            {//������Ч����
                return -2;
            }
        }
        else
        {
            return -1;
        }
    }
    
    BOOL CCVirtualChannel::ConnectTURN(int nIndex, char *pchError)
    {
        UDTSOCKET SocketTmp = UDT::socket(AF_INET, SOCK_STREAM, 0);
        
        BOOL bReuse = TRUE;
        UDT::setsockopt(SocketTmp, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
        UDT::setsockopt(SocketTmp, 0, UDT_MSS, &len1, sizeof(int));
        //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
        len1=1500*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
        
        len1=1000*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
        if (UDT::ERROR == UDT::bind(SocketTmp, m_pWorker->m_WorkerUDPSocket))
        {OutputDebug("************************************virtualChannel  line: %d",__LINE__);
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
            }
            m_SocketSTmp = 0;
            
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            if(SocketTmp > 0)
            {
                m_pWorker->pushtmpsock(SocketTmp);
            }
            SocketTmp = 0;
            
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[MAX_PATH]={0};
                sprintf(chMsg,"<[S%d]bind Jsock failed,INFO:",nIndex);
                strcat(pchError, chMsg);
                strcat(pchError, UDT::getlasterror().getErrorMessage());
                strcat(pchError, ">**");
            }
            
            return FALSE;
        }
        
        //���׽�����Ϊ������ģʽ
        BOOL block = FALSE;
        UDT::setsockopt(SocketTmp, 0, UDT_SNDSYN, &block, sizeof(BOOL));
        UDT::setsockopt(SocketTmp, 0, UDT_RCVSYN, &block, sizeof(BOOL));
        LINGER linger;
        linger.l_onoff = 0;
        linger.l_linger = 0;
        UDT::setsockopt(SocketTmp, 0, UDT_LINGER, &linger, sizeof(LINGER));
        
        STJUDTCONN stcon;
		stcon.pbQuickExit = &m_bExit;
        stcon.u = SocketTmp;
        stcon.name = (SOCKADDR *)&m_addrTS;
        stcon.namelen = sizeof(SOCKADDR);
        stcon.nChannelID = -2;//m_stConnInfo.nChannel;
        memcpy(stcon.chGroup, m_stConnInfo.chGroup, 4);
        stcon.nYSTNO = m_stConnInfo.nYSTNO;
        stcon.nLVer_new = JVN_YSTVER;
        stcon.nLVer_old = JVN_YSTVER1;//�������������򲻼���49-67������
        stcon.nMinTime = 3000;
        stcon.uchVirtual = 1;
        stcon.nVChannelID = m_stConnInfo.nChannel;
        
        if(UDT::ERROR == UDT::connect(stcon))
        {
            if(m_SocketSTmp > 0)
            {
                closesocket(m_SocketSTmp);
                m_SocketSTmp = 0;
            }
            
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            if(SocketTmp > 0)
            {
                m_pWorker->pushtmpsock(SocketTmp);
            }
            SocketTmp = 0;
            
            if(m_pWorker->m_bNeedLog)
            {
                char chMsg[3*MAX_PATH]={0};
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    sprintf(chMsg,"<[S%d]TURN����ʧ��.��ϸ:%s",nIndex,(char *)UDT::getlasterror().getErrorMessage());
                    strcat(pchError, chMsg);
                }
                else
                {
                    sprintf(chMsg,"<[S%d]TURN Connect failed. INFO:%s",nIndex,(char *)UDT::getlasterror().getErrorMessage());
                    strcat(pchError, chMsg);
                }
            }
            
            return FALSE;
        }
        else
        {
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);//���������������ղ���֡ͷ�Ӷ����ӳɹ���ͼ��
            }
            m_ServerSocket = SocketTmp;
            memcpy(&m_addressA, stcon.name, sizeof(SOCKADDR_IN));
            return TRUE;
        }
    }
    
    BOOL CCVirtualChannel::SendKeep()//���������ɹ�
    {
#ifndef WIN32
        pthread_mutex_lock(&m_ct);
#else
        EnterCriticalSection(&m_ct);
#endif
        if(m_ServerSocket > 0)
        {
            BYTE data[10]={0};
            data[0] = JVN_CMD_KEEPLIVE;
            int ss = UDT::send(m_ServerSocket, (char *)data,5, 0);
            if(ss < 0)
            {
                if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                {
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "��������ʧ�� ��ϸ:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                else
                {
                    m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "Sendkeep failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                }
                //////////////////////////////////////////////////////////////////////////
                if(m_ServerSocket > 0)
                {
                    m_pWorker->pushtmpsock(m_ServerSocket);
                    m_ServerSocket = 0;
                }
                //////////////////////////////////////////////////////////////////////////
                
#ifndef WIN32
                pthread_mutex_unlock(&m_ct);
#else
                LeaveCriticalSection(&m_ct);
#endif
                
                return FALSE;
            }
        }
        
#ifndef WIN32
        pthread_mutex_unlock(&m_ct);
#else
        LeaveCriticalSection(&m_ct);
#endif
        return TRUE;
    }
    
    BOOL CCVirtualChannel::RecvKeep()//
    {
        DWORD dwbegin = CCWorker::JVGetTime();
        DWORD dwend = 0;
        BYTE uchdata[20]={0};
        BYTE uchType = 0;
        int rs = 0;
        int rsize = 0;
        if (UDT::ERROR == (rs = UDT::recvmsg(m_ServerSocket, (char *)uchdata, 20)))
        {
            if(m_pWorker != NULL)
            {
                m_pHelp->VConnectChange(m_stConnInfo.nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL);
            }
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "��������ʧ��,�����߳��˳�. ��ϸ:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "receive thread over,receive failed. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            
            return FALSE;
        }
        else if(rs == 0)
        {
            return FALSE;
        }
        
        int nLen=-1;
        uchType = uchdata[0];
        if(uchType == JVN_CMD_KEEPLIVE)
        {
            memcpy(&nLen, &uchdata[1], 4);
            
            if(nLen == 0)
            {
                return TRUE;
            }
            
            return FALSE;
        }
        
        rs = UDT::recvmsg(m_ServerSocket, (char *)uchdata, 20);
        
        if(rs > 0)
        {
            return TRUE;
        }
        
        return FALSE;
    }
    
    BOOL CCVirtualChannel::DisConnect()
    {
        m_bDisConnectShow = TRUE;
        
        SendData(JVN_CMD_DISCONN, NULL, 0);
        
        //	Sleep(1);
        
        m_bExit = TRUE;
        
#ifndef WIN32
        if (0 != m_hConnThread)
        {
            m_bEndC = TRUE;
            pthread_join(m_hConnThread, NULL);
            m_hConnThread = 0;
        }
#else
        if(m_hEndEventC>0)
        {
            SetEvent(m_hEndEventC);
        }
#endif
        
        CCWorker::jvc_sleep(10);
        //	WaitThreadExit(m_hConnThread);
        
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        
        if(m_SocketSTmp > 0)
        {
            closesocket(m_SocketSTmp);
        }
        m_SocketSTmp = 0;
        
        m_nStatus = FAILD;
        return TRUE;
    }
    
    BOOL CCVirtualChannel::SendData(BYTE uchType, BYTE *pBuffer,int nSize)
    {
#ifndef WIN32
        pthread_mutex_lock(&m_ct);
#else
        EnterCriticalSection(&m_ct);
#endif
        if(m_ServerSocket > 0)
        {
            DWORD dwbegin = CCWorker::JVGetTime();
            DWORD dwend = 0;
            switch(uchType)
            {
                case JVN_CMD_DISCONN://�Ͽ�����
                {
                    int nLen = 5;
                    BYTE data[5]={0};
                    data[0] = uchType;
                    
                    int ss=0;
                    int ssize=0;
                    while(ssize < nLen)
                    {
                        dwend = CCWorker::JVGetTime();
                        if(dwend < dwbegin || dwend > dwbegin + 5000)
                        {
#ifndef WIN32
                            pthread_mutex_unlock(&m_ct);
#else
                            LeaveCriticalSection(&m_ct);
#endif
                            return FALSE;
                        }
                        if(0 < (ss = UDT::send(m_ServerSocket, (char *)data + ssize, nLen - ssize, 0)))
                        {
                            ssize += ss;
                        }
                        else if(ss == 0)
                        {
                            CCWorker::jvc_sleep(1);
                            continue;
                        }
                        else
                        {
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "��������ʧ�� ��ϸ:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                            }
                            else
                            {
                                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "SendData failed,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
                            }
                            
                            //////////////////////////////////////////////////////////////////////////
                            if(m_ServerSocket > 0)
                            {
                                m_pWorker->pushtmpsock(m_ServerSocket);
                                m_ServerSocket = 0;
                            }
                            //////////////////////////////////////////////////////////////////////////
                            
                            char chtmp[20]={0};
                            sprintf(chtmp, "type:%X", uchType);
                            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
                            {
                                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "��������ʧ�� �ɹ��ر��׽���", __FILE__,__LINE__,chtmp);
                            }
                            else
                            {
                                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "SendData failed close socket successed", __FILE__,__LINE__,chtmp);
                            }
                            
#ifndef WIN32
                            pthread_mutex_unlock(&m_ct);
#else
                            LeaveCriticalSection(&m_ct);
#endif
                            
                            return FALSE;
                        }
                    }
                    
#ifndef WIN32
                    pthread_mutex_unlock(&m_ct);
#else
                    LeaveCriticalSection(&m_ct);
#endif
                    return TRUE;
                }
                    break;
                default:
                    break;
            }
        }
        
#ifndef WIN32
        pthread_mutex_unlock(&m_ct);
#else
        LeaveCriticalSection(&m_ct);
#endif
        
        return FALSE;
    }
    
    //Ԥ�ȴ򶴺���
    void CCVirtualChannel::PrePunch(int sock,SOCKADDR_IN addrA)
    {
        char ch[30]={0};
        ch[0] = JVN_CMD_TRYTOUCH;
        CCChannel::sendtoclient(sock,ch,20,0,(SOCKADDR *)&addrA, sizeof(SOCKADDR),1);//ԭ����1��Ϊ��ֹ��С�����ƣ�������20
        CCWorker::jvc_sleep(2);//5
        
        /*����һ���˿ڴ�(Ԥ��)*/
        SOCKADDR_IN addrB;
        memcpy(&addrB,&addrA,sizeof(SOCKADDR_IN));
        addrB.sin_port = htons(ntohs(addrA.sin_port)+1);
        CCChannel::sendtoclient(sock,ch,20,0,(SOCKADDR *)&addrB, sizeof(SOCKADDR),1);//ԭ����1��Ϊ��ֹ��С�����ƣ�������20
    }
    //p2p�򶴺���
    BOOL CCVirtualChannel::Punch(int nYSTNO,int sock,SOCKADDR_IN *addrA)
    {
        char ch[30]={0};
        ch[0] = JVN_CMD_TRYTOUCH;
        //���д�
        SOCKADDR_IN addrAtmp,addrT;
        
        int ii = 0;
        int ll=sizeof(SOCKADDR_IN);
        int nMax = 0;
        memcpy(&addrT,addrA,sizeof(SOCKADDR_IN));
        addrT.sin_port=htons(ntohs(addrT.sin_port)+1);
        while(TRUE)
        {
            ch[0] = JVN_CMD_TRYTOUCH;
            CCChannel::sendtoclientm(sock,ch,20,0,(SOCKADDR *)addrA, sizeof(SOCKADDR),200);//ԭ����1��Ϊ��ֹ��С�����ƣ�������20
            //		Sleep(5);
            CCChannel::sendtoclientm(sock,ch,20,0,(SOCKADDR *)&addrT, sizeof(SOCKADDR),200);//ԭ����1��Ϊ��ֹ��С�����ƣ�������20
            ii = 0;
            ii = CCChannel::receivefromm(sock, ch, 20, 0, (SOCKADDR *)&addrAtmp, &ll,100);//ԭ��෢��5��Ϊ��ֹ��С�����ƣ�������20
            if(ii > 0)
            {
                if(ii == 8)
                {//�����Ĵ򶴰����ϸ��ж�
                    int ntmp = 0;
                    memcpy(&ntmp, &ch[0], 4);
                    if(ntmp == JVN_CMD_TRYTOUCH)
                    {
                        int nyst=0;
                        memcpy(&nyst, &ch[4], 4);
                        if(nyst == nYSTNO)
                        {//�򶴰��Ϸ������Խ�������
                            memcpy(addrA, &addrAtmp, sizeof(SOCKADDR_IN));
                            return TRUE;
                        }
                    }
                    //�򶴰���Ч�����½���
                }
                else
                {//�������Ĵ򶴰���ֻ�жϵ�ַ�Ƿ����
                    if(addrAtmp.sin_addr.s_addr == (*addrA).sin_addr.s_addr)
                    {//Ŀ���ʵ�������������Ϊ����ȷ�Ĵ򶴰�(ʵ���Ͼ�������������ʱ�����жϴ���©��)
                        memcpy(addrA, &addrAtmp, sizeof(SOCKADDR_IN));
                        return TRUE;
                    }
                    //�򶴰���Ч�����½���
                }
            }
            
            nMax ++;
            if(nMax > 1)
            {
                return FALSE;
            }
        }
        return FALSE;
    }
    
    void CCVirtualChannel::AddRemoteConnect(SOCKET s,char* pIP,int nPort)
    {
        m_bDirectConnect = TRUE;
        
        m_sSocket = s;
        strcpy(m_strIP,pIP);
        m_nPort = nPort;
        
        m_dwConnectTime = 0;
    }
    
    BOOL CCVirtualChannel::DealConnectIP(STCONNPROCP *pstConn)
    {
        //OutputDebug("����ֱ������.......");
        //////////////////////////////////////////////////////////////////////////
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = 0;
        
        m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
        BOOL bReuse = TRUE;
        UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
        //////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
        UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
        //////////////////////////////////////////////////////////////////////////
#ifdef MOBILE_CLIENT
        len1=1500*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_RCVBUF, (char *)&len1, sizeof(int));
        
        len1=1000*1024;
        UDT::setsockopt(m_ServerSocket, 0, UDP_SNDBUF, (char *)&len1, sizeof(int));
#endif
        if (UDT::ERROR == UDT::bind(m_ServerSocket, m_sSocket))
        {//�󶨵�ָ���˿�ʧ�ܣ���Ϊ�󶨵�����˿�
            OutputDebug("************************************virtualChannel  line: %d",__LINE__);
            if(m_ServerSocket > 0)
            {
                m_pWorker->pushtmpsock(m_ServerSocket);
            }
            m_ServerSocket = 0;
            
            if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
            {
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "��������ʧ��.��������ʧ��(�����Ƕ˿ڱ�ռ) ��ϸ:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            else
            {
                m_pWorker->m_Log.SetRunInfo(m_stConnInfo.nLocalChannel, "connect failed. connect failed(port may be invlaid) INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
            }
            
            return FALSE;
        }
        
        //���׽�����Ϊ������ģʽ
        BOOL block = FALSE;
        UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
        UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
        LINGER linger;
        linger.l_onoff = 0;
        linger.l_linger = 0;
        UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
        //////////////////////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        //char ch[100]={0};
        //sprintf(ch,"AL:%s:%d",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
        //////////////////////////////////////////////////////////////////////////
        
        m_addrAN.sin_family = AF_INET;
        m_addrAN.sin_port = htons(m_nPort);
        m_addrAN.sin_addr.s_addr = inet_addr(m_strIP);
        
        STJUDTCONN stcon;
		stcon.pbQuickExit = &m_bExit;
        stcon.u = m_ServerSocket;
        stcon.name = (SOCKADDR *)&m_addrAN;
        stcon.namelen = sizeof(SOCKADDR);
        stcon.nChannelID = -2;//m_stConnInfo.nChannel;
        stcon.nCheckYSTNO = m_stConnInfo.nYSTNO;
        memcpy(stcon.chCheckGroup, m_stConnInfo.chGroup, 4);
        stcon.nLVer_new = JVN_YSTVER;
        stcon.nLVer_old = JVN_YSTVER1;//�������������򲻼���49-67������
        stcon.nMinTime = 3000;//1000
        stcon.uchVirtual = 1;
        stcon.nVChannelID = m_stConnInfo.nChannel;
        //	SOCKADDR_IN addr = {0};
        //	memcpy(&addr,stcon.name,sizeof(SOCKADDR_IN));
        //	OutputDebug("connect a ....%s:%d",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        
        if(UDT::ERROR == UDT::connect(stcon))//3000
        {
            m_nStatus = NEW_YST;
            return FALSE;
        }
        
        if(SendPWCheck())
        {//���������֤��Ϣ�ɹ� �ȴ����
            m_nStatus = WAIT_NPWCHECK;
            m_dwStartTime = CCWorker::JVGetTime();
            return TRUE;
        }
        else
        {//���������֤��Ϣʧ�� ��������������
            m_nStatus = NEW_YST;
        }
        return FALSE;
    }
    
    
    BOOL CCVirtualChannel::ConnectStatus(SOCKET s,SOCKADDR_IN* addr,int nTimeOut,BOOL bFinish,UDTSOCKET uSocket)//���ӳɹ� ֱ��ȥ����
    {
        if(bFinish)
        {
            //ȥת��
            if(m_nStatus == WAIT_MAKE_HOLE)
                m_nStatus = NEW_TURN;
            return TRUE;
        }
        //////////////////////////////////////////////////////////////////////////
        if(m_ServerSocket > 0)
        {
            m_pWorker->pushtmpsock(m_ServerSocket);
        }
        m_ServerSocket = uSocket;
        m_sSocket = s;
        /*
         m_ServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
         BOOL bReuse = TRUE;
         UDT::setsockopt(m_ServerSocket, SOL_SOCKET, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
         //////////////////////////////////////////////////////////////////////////
         int len1 = JVC_MSS;
         UDT::setsockopt(m_ServerSocket, 0, UDT_MSS, &len1, sizeof(int));
         //////////////////////////////////////////////////////////////////////////
         if (UDT::ERROR == UDT::bind(m_ServerSocket, s))
         {//�󶨵�ָ���˿�ʧ�ܣ���Ϊ�󶨵�����˿�
         if(m_ServerSocket > 0)
         {
         m_pWorker->pushtmpsock(m_ServerSocket);
         }
         m_ServerSocket = 0;
         
         m_nStatus = NEW_TURN;
         
         return FALSE;
         }
         
         //���׽�����Ϊ������ģʽ
         BOOL block = FALSE;
         UDT::setsockopt(m_ServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
         UDT::setsockopt(m_ServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
         LINGER linger;
         linger.l_onoff = 0;
         linger.l_linger = 0;
         UDT::setsockopt(m_ServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));
         //////////////////////////////////////////////////////////////////////////
         
         //////////////////////////////////////////////////////////////////////////
         //char ch[100]={0};
         //sprintf(ch,"AL:%s:%d",inet_ntoa(m_addrAN.sin_addr),ntohs(m_addrAN.sin_port));
         //////////////////////////////////////////////////////////////////////////
         STJUDTCONN stcon;
         stcon.u = m_ServerSocket;
         stcon.name = (SOCKADDR *)addr;
         stcon.namelen = sizeof(SOCKADDR);
         stcon.nChannelID = -2;//m_stConnInfo.nChannel;
         stcon.nCheckYSTNO = m_stConnInfo.nYSTNO;
         memcpy(stcon.chCheckGroup, m_stConnInfo.chGroup, 4);
         stcon.nLVer_new = JVN_YSTVER;
         stcon.nLVer_old = JVN_YSTVER1;//�������������򲻼���49-67������
         stcon.nMinTime = nTimeOut;//1000
         stcon.uchVirtual = 1;
         stcon.nVChannelID = m_stConnInfo.nChannel;
         
         //	SOCKADDR_IN srv = {0};
         //	char strServer[100] = {0};
         //	memcpy(&srv,stcon.name,sizeof(SOCKADDR_IN));
         
         //	sprintf(strServer,"connecting a %s:%d  m_ServerSocket = %d line %d\n",inet_ntoa(srv.sin_addr),ntohs(srv.sin_port),m_ServerSocket,__LINE__);
         //	OutputDebug(strServer);
         if(UDT::ERROR == UDT::connect(stcon))//3000
         {
         m_nStatus = NEW_TURN;
         return FALSE;
         }*/
        if(SendPWCheck())
        {//���ͳɹ� �ȴ���֤���
            m_nStatus = WAIT_NPWCHECK;
            m_dwStartTime = CCWorker::JVGetTime();
            
            memcpy(&m_addrAN,addr,sizeof(SOCKADDR_IN));
            memcpy(&m_addrAL,addr,sizeof(SOCKADDR_IN));
            memcpy(&m_addressA,addr,sizeof(SOCKADDR_IN));
            //		OutputDebug("connect ok.");
            return TRUE;
        }
        
        m_nStatus = NEW_TURN;
        
        return FALSE;
    }
    
