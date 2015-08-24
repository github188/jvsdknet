#ifndef __PAK_H__
#define __PAK_H__

#define PKT_HEAD_LEN 12
#define PKT_TAIL_LEN 4
#define MAX_PKT_LEN  1024*1024


#define HEADER_FLAG_ 0x38254b64			//ͷ��־
#define ENDMARK 0x872b7881				//������־


#define  CS_REQ_FILE 0x01  // �����������ַ�б�
#define  CS_ACK_FILE 0x02  // ��Ӧ��������ַ�б�����
#define  CS_ERROR_NOFILE_FOUND 0x03 // δ���������ļ�
#define  CS_ERROR_CMD 0x04  // �޷�ʶ���ָ��


typedef struct pktheader{				//��ͷ���ݽṹ
	int headermos;						//ͷģ��
	int ver;							//�汾
	int datalen;							//����
} PKTHEADER;


class CPakOp  
{
public:
	CPakOp();
	virtual ~CPakOp();
public:
	static int Decappkt(char *targetbuf, int targetbuflen, char *srcbuf, int srclen);
	static int Encappkt(char *targetbuf, int targetbuflen, char *srcbuf, int srclen);
};

extern CPakOp g_pakOp;
#endif