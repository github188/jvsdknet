// CRtmpChannel.h: interface for the CCRtmpChannel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CRTMPCHANNEL_H__34FD0757_3378_41D8_AD51_4B203DB23D66__INCLUDED_)
#define AFX_CRTMPCHANNEL_H__34FD0757_3378_41D8_AD51_4B203DB23D66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef void (*FUNC_CRTMP_CONNECT_CALLBACK)(int nLocalChannel, unsigned char uchType, char *pMsg, int nPWData);//1 �ɹ� 2 ʧ�� 3 �Ͽ� 4 �쳣�Ͽ�
typedef void (*FUNC_CRTMP_NORMALDATA_CALLBACK)(int nLocalChannel, unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp);

class CCRtmpChannel  
{
public:
	CCRtmpChannel();
	virtual ~CCRtmpChannel();

	bool ConnectServer(int nLocalChannel,char* strURL,FUNC_CRTMP_CONNECT_CALLBACK rtmpConnectChange,FUNC_CRTMP_NORMALDATA_CALLBACK rtmpNormalData,int nTimeout);
	void ShutDown();

	void Rtmp_ConnectChange(unsigned char uchType,const char *pMsg, int nPWData);

	void RTMP_NormaData(unsigned char uchType, unsigned char *pBuffer, int nSize,int nTimestamp);

	int m_nLocalChannel;//����ͨ����
    //std::string m_strURL;
    //char m_strURL[64];//��������ַ

	FUNC_CRTMP_CONNECT_CALLBACK m_RtmpConnectCallBack;//���ӻص� �ɹ� ʧ�� �Ͽ�
	FUNC_CRTMP_NORMALDATA_CALLBACK m_RtmpNormalDataCallBack;//��Ƶ����

	void *m_hRtmpClient;
};

#endif // !defined(AFX_CRTMPCHANNEL_H__34FD0757_3378_41D8_AD51_4B203DB23D66__INCLUDED_)
