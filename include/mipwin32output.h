#ifndef MIPWIN32OUTPUT_H
#define MIPWIN32OUTPUT_H

#include "mipconfig.h"

#ifdef MIPCONFIG_SUPPORT_WIN32OUT

#include "miptime.h"
#include "mipmessage.h"
#include "mipcomponent.h"
#include "miptypes.h"
#include "miprawvideomessage.h"
#include <jthread.h>

/** Ҫ��ʾ��Ƶ�Ĵ��ڼ̳д��࣬OnOutput��ÿһ֡����ʱ������ */
class MIPWin32OutputEvent
{
public:
	virtual void OnOutput(MIPRawRGBVideoMessage *pRawRGBMsg) = 0;
};

/** Win32 ��Ƶ������ */
class MIPWin32Output : public MIPComponent, private JThread
{
public:
	MIPWin32Output();
	~MIPWin32Output();

	/** ��ʼ�� Win32 ��Ƶ������ */
	bool init(MIPWin32OutputEvent *pMIPWin32OutputEvent);

	/** ����ʼ�� */
	bool shutdown();

	/** ����ʵ�ֵ�MIPComponent��Ա����*/
	bool push(const MIPComponentChain &chain, int64_t iteration, MIPMessage *pMsg);
	bool pull(const MIPComponentChain &chain, int64_t iteration, MIPMessage **pMsg);
private:

	/** ����ʵ�ֵ�JThread��Ա�������ú�����֤chain���������˳�*/
	void *Thread();

	/** ����m_stopOutput�Ļ������ */
	JMutex m_lock;

	/** �Ƿ�ֹͣ��������� */
	bool m_stopOutput;
 private:
	 MIPWin32OutputEvent *m_pMIPWin32OutputEvent;
};

#endif // MIPCONFIG_SUPPORT_WIN32OUT
#endif // MIPWIN32OUTPUT_H