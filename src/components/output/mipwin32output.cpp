#include "mipwin32output.h"

#include <iostream>
using namespace std;     

MIPWin32Output::MIPWin32Output(): MIPComponent("MIPWin32Output")
{
	m_pMIPWin32OutputEvent=NULL;

	int status=0;
	if ((status = m_lock.Init()) < 0)
	{
		std::cerr << "无法初始化JMutex:" << status << std::endl;
		exit(-1);
	}
}

MIPWin32Output::~MIPWin32Output()
{
	shutdown();
}

bool MIPWin32Output::shutdown()
{
	if (!JThread::IsRunning())
	{
		setErrorString("输出线程没有运行");
		return false;
	}

	m_lock.Lock();
	m_stopOutput = true;
	m_lock.Unlock();

	MIPTime startTime = MIPTime::getCurrentTime();
	while (
		JThread::IsRunning() && 
		(MIPTime::getCurrentTime().getValue() - startTime.getValue()) < 5.0)
	{
		MIPTime::wait(MIPTime(0.050));
	}

	if (JThread::IsRunning())
	{
		cerr << "结束输出线程" << endl;
		JThread::Kill();
	}

	return true;
}

bool MIPWin32Output::init(MIPWin32OutputEvent *pMIPWin32OutputEvent)
{
	m_pMIPWin32OutputEvent=pMIPWin32OutputEvent;

	if (JThread::IsRunning())
	{
		setErrorString("输出线程已经运行");
		return false;
	}

	m_stopOutput = false;
	if (JThread::Start() < 0)
	{
		setErrorString("无法启动后台线程");
		return false;
	}

	return true;
}

bool MIPWin32Output::pull( const MIPComponentChain &chain, int64_t iteration, MIPMessage **pMsg )
{
	setErrorString("不支持Pull操作");
	return true;
}

bool MIPWin32Output::push( const MIPComponentChain &chain, int64_t iteration, MIPMessage *pMsg )
{
	if (!JThread::IsRunning())
	{
		setErrorString("输出线程没有运行");
		return false;
	}

	MIPVideoMessage *pVideoMsg = (MIPVideoMessage *)pMsg;
	if (pVideoMsg->getMessageSubtype() != MIPRAWVIDEOMESSAGE_TYPE_RGB24)
	{
		setErrorString("消息子类型不是RGB24");
		return false;
	}

	MIPRawRGBVideoMessage *pRawRGBMsg = (MIPRawRGBVideoMessage *)pVideoMsg;
	if(m_pMIPWin32OutputEvent!=NULL)
	{
		m_pMIPWin32OutputEvent->OnOutput(pRawRGBMsg);
	}
	
	return true;
}

void * MIPWin32Output::Thread()
{
	JThread::ThreadStarted();

	bool done = false;
	do
	{
		MIPTime::wait(MIPTime(0.100));
		m_lock.Lock();
		done = m_stopOutput;
		m_lock.Unlock();
	} while (!done);

	return 0;
}

