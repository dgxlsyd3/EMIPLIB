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

/** 要显示视频的窗口继承此类，OnOutput在每一帧到来时被调用 */
class MIPWin32OutputEvent
{
public:
	virtual void OnOutput(MIPRawRGBVideoMessage *pRawRGBMsg) = 0;
};

/** Win32 视频输出组件 */
class MIPWin32Output : public MIPComponent, private JThread
{
public:
	MIPWin32Output();
	~MIPWin32Output();

	/** 初始化 Win32 视频输出组件 */
	bool init(MIPWin32OutputEvent *pMIPWin32OutputEvent);

	/** 反初始化 */
	bool shutdown();

	/** 必须实现的MIPComponent成员函数*/
	bool push(const MIPComponentChain &chain, int64_t iteration, MIPMessage *pMsg);
	bool pull(const MIPComponentChain &chain, int64_t iteration, MIPMessage **pMsg);
private:

	/** 必须实现的JThread成员函数，该函数保证chain不会立即退出*/
	void *Thread();

	/** 用于m_stopOutput的互斥访问 */
	JMutex m_lock;

	/** 是否停止输出到窗口 */
	bool m_stopOutput;
 private:
	 MIPWin32OutputEvent *m_pMIPWin32OutputEvent;
};

#endif // MIPCONFIG_SUPPORT_WIN32OUT
#endif // MIPWIN32OUTPUT_H