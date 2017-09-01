/*
    
  This file is a part of EMIPLIB, the EDM Media over IP Library.
  
  Copyright (C) 2006-2010  Hasselt University - Expertise Centre for
                      Digital Media (EDM) (http://www.edm.uhasselt.be)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  
  USA

*/

/**
 * \file mipdirectshowcapture.h
 */

#ifndef MIPDIRECTSHOWCAPTURE_H
#define MIPDIRECTSHOWCAPTURE_H

#include "mipconfig.h"
#ifdef MIPCONFIG_SUPPORT_DIRECTSHOW


#include "mipcomponent.h"
#include "mipsignalwaiter.h"
#include "miptime.h"
#include <dshow.h>

#include <Qedit.h>
// #import "qedit.dll" raw_interfaces_only named_guids
// using namespace DexterLib;

#include <list>

class MIPVideoMessage;

/** DirectShow 输入组件，比如摄像头
 *  它可以接受MIPSYSTEMMESSAGE_TYPE_WAITTIME消息和MIPSYSTEMMESSAGE_TYPE_ISTIME消息，
 *  并创建YUV420P编码的原始视频消息。
 */
class MIPDirectShowCapture : public MIPComponent
{
public:
	MIPDirectShowCapture();
	~MIPDirectShowCapture();

	/** 组件初始化函数
	 *  \param width 帧的宽度
	 *  \param height 帧的高度
	 *  \param frameRate 帧率.
	 *  \param devideNumber 摄像头序号，0为第一个摄像头，1为第二个...
	 */
	bool open(int width, int height, real_t frameRate, int deviceNumber = 0);

	/** 关闭 DirectShow 输入设备 */
	bool close();

	/** 返回捕获的帧宽度 */
	int getWidth() const;

	/** 返回捕获的帧高度 */
	int getHeight()	const;

	/** 设置生成消息的source ID */
	void setSourceID(uint64_t srcID);

	/** 返回生成消息的source ID */
	uint64_t getSourceID() const;

	/** 返回消息中的视频帧subtype */
	uint32_t getFrameSubtype() const;

	/** 前级组件调用此函数，传递消息到本组件。*/
	bool push(const MIPComponentChain &chain, int64_t iteration, MIPMessage *pMsg);

	/** 后级组件调用此函数，将从本组件获取消息。*/
	bool pull(const MIPComponentChain &chain, int64_t iteration, MIPMessage **pMsg);
private:
	void zeroAll();
	void clearNonZero();
	bool initCaptureGraphBuilder();
	bool getCaptureDevice(int deviceNumber);
	bool setFormat(int w, int h, real_t rate);
	bool listCameraSubtypeId(std::list<GUID> &guids);
	void copyVideoFrame();

	class GrabCallback : public ISampleGrabberCB
	{
	public:
		GrabCallback(MIPDirectShowCapture *pDSCapt);
		virtual ULONG __stdcall AddRef();
		virtual ULONG __stdcall Release();
		HRESULT __stdcall QueryInterface(const IID & riid, void ** ppv);
		HRESULT __stdcall SampleCB( double SampleTime, IMediaSample * pSample );
		HRESULT __stdcall BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize );
	private:
		MIPDirectShowCapture *m_pDSCapt;
	};

	IGraphBuilder *m_pGraphBuilder;
	ICaptureGraphBuilder2 *m_pGraphBuilder2;
	IMediaControl *m_pControl;
	IBaseFilter *m_pCaptDevice;
	IBaseFilter *m_pNullRenderer;
	IBaseFilter *m_pSampGrabFilter;
	ISampleGrabber *m_pGrabber;
	GrabCallback *m_pGrabCallback;

	int m_width;
	int m_height;
	uint8_t *m_pFullFrame;
	uint8_t *m_pMsgFrame;
	size_t m_largeFrameSize, m_targetFrameSize;
	MIPVideoMessage *m_pVideoMsg;
	JMutex m_frameMutex;
	MIPSignalWaiter m_sigWait;
	bool m_gotMsg;
	bool m_gotFrame;
	MIPTime m_captureTime;
	uint64_t m_sourceID;
	GUID m_selectedGuid;
	uint32_t m_subType;

	//for debug
	DWORD m_debug_dwRegister;
	HRESULT v_debug_AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
	void v_debug_RemoveFromRot(DWORD pdwRegister);
	HRESULT v_debug_SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath);

};

#endif // MIPCONFIG_SUPPORT_DIRECTSHOW
#endif // MIPDIRECTSHOWCAPTURE_H
