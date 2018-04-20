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

/** DirectShow �����������������ͷ
 *  �����Խ���MIPSYSTEMMESSAGE_TYPE_WAITTIME��Ϣ��MIPSYSTEMMESSAGE_TYPE_ISTIME��Ϣ��
 *  ������YUV420P�����ԭʼ��Ƶ��Ϣ��
 */
class MIPDirectShowCapture : public MIPComponent
{
public:
	MIPDirectShowCapture();
	~MIPDirectShowCapture();

	/** �����ʼ������
	 *  \param width ֡�Ŀ��
	 *  \param height ֡�ĸ߶�
	 *  \param frameRate ֡��.
	 *  \param devideNumber ����ͷ��ţ�0Ϊ��һ������ͷ��1Ϊ�ڶ���...
	 */
	bool open(int width, int height, real_t frameRate, int deviceNumber = 0);

	/** �ر� DirectShow �����豸 */
	bool close();

	/** ���ز����֡��� */
	int getWidth() const;

	/** ���ز����֡�߶� */
	int getHeight()	const;

	/** ����������Ϣ��source ID */
	void setSourceID(uint64_t srcID);

	/** ����������Ϣ��source ID */
	uint64_t getSourceID() const;

	/** ������Ϣ�е���Ƶ֡subtype */
	uint32_t getFrameSubtype() const;

	/** ǰ��������ô˺�����������Ϣ���������*/
	bool push(const MIPComponentChain &chain, int64_t iteration, MIPMessage *pMsg);

	/** ��������ô˺��������ӱ������ȡ��Ϣ��*/
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
