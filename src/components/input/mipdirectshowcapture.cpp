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
  License along with this library; if not, write to7 the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  
  USA

*/

#include "mipconfig.h"

#ifdef MIPCONFIG_SUPPORT_DIRECTSHOW

#pragma warning(disable:4995)
#pragma warning(disable:4996)

#include "mipdirectshowcapture.h"
#include "mipsystemmessage.h"
#include "miprawvideomessage.h"

#include <iostream>
using namespace std;

#include "mipdebug.h"

#define MIPDIRECTSHOWCAPTURE_ERRSTR_ALREADYOPEN								"Already a DirectShow capture device open"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATECAPTUREBUILDER				"Can't create a capture graph builder"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATEMANAGER						"Can't create filter graph manager"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATEDEVICEENUMERATOR				"Can't create device enumerator"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCAPTUREENUMERATOR					"Can't create video capture device enumerator"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTFINDCAPTUREDEVICE					"Can't find the requested capture device"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTBINDCAPTUREDEVICE					"Can't bind the requested capture device"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATENULLRENDERER					"Can't create null renderer"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATEGRABFILTER					"Can't create sample grabber filter"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATEGRABIFACE						"Unable to obtain sample grabber intereface"

//debug
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSELECTYUV420PORYUY2					"Unable to select YUV420P/YUY2/RGB24 encoding" 

#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSETCALLBACK							"Can't set grab callback"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTADDCAPTUREFILTER					"Can't add capture filter to filter graph"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTADDNULLRENDERER						"Can't add null renderer to filter graph"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTADDSAMPLEGRABBER					"Can't add sample grabber filer to filter graph"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTBUILDGRAPH							"Can't build render graph"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETFRAMESIZE						"Can't get frame size"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETCONTROLINTERFACE					"Can't get media control interface"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSTARTGRAPH							"Can't start filter graph"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTINITSIGWAIT							"Can't initialize signal waiter"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_DEVICENOTOPENED							"No DirectShow capture device was opened"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_BADMESSAGE								"Bad message"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETDEVICECONFIG						"Can't get device configuration"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETDEVICECAPS						"Can't get device capabilities"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_INVALIDCAPS								"Can't handle returned device capabilities"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSETCAPS								"Unable to install desired format"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_CANTLISTGUIDS							"Unable to list the supported video formats"
#define MIPDIRECTSHOWCAPTURE_ERRSTR_NOAVCODEC								"YUY2 format is available, but need libavcodec to convert to YUV420P"

extern "C" const __declspec(selectany) GUID EMIP_MEDIASUBTYPE_I420 = {0x30323449,0x0000,0x0010, {0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}};

#define HR_SUCCEEDED(x) ((x==S_OK))
#define HR_FAILED(x)	((x!=S_OK))

HRESULT MIPDirectShowCapture::v_debug_AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
	IMoniker * pMoniker=NULL;
	IRunningObjectTable *pROT=NULL;
	if (FAILED(GetRunningObjectTable(0, &pROT))) 
	{
		return E_FAIL;
	}

	wchar_t wcs[200];
	swprintf(wcs,L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());

	HRESULT hr = CreateItemMoniker(L"!", wcs, &pMoniker);
	if (SUCCEEDED(hr)) 
	{
		hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph,	pMoniker, pdwRegister);
		pMoniker->Release();
	}
	pROT->Release();
	return hr;
}

void MIPDirectShowCapture::v_debug_RemoveFromRot(DWORD pdwRegister)
{
	IRunningObjectTable *pROT;
	if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
	{
		pROT->Revoke(pdwRegister);
		pROT->Release();
	}
}


HRESULT MIPDirectShowCapture::v_debug_SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath)
{
	const WCHAR wszStreamName[] = L"ActiveMovieGraph";
	HRESULT hr;

	IStorage *pStorage = NULL;
	hr = StgCreateDocfile(wszPath,STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,0, &pStorage);
	if(FAILED(hr))
	{
		return hr;
	}

	IStream *pStream;
	hr = pStorage->CreateStream(wszStreamName,STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,0, 0, &pStream);
	if (FAILED(hr))
	{
		pStorage->Release();
		return hr;
	}

	IPersistStream *pPersist = NULL;
	pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersist);
	hr = pPersist->Save(pStream, TRUE);
	pStream->Release();
	pPersist->Release();

	if (SUCCEEDED(hr))
	{
		hr = pStorage->Commit(STGC_DEFAULT);
	}
	pStorage->Release();
	return hr;
}


MIPDirectShowCapture::MIPDirectShowCapture() : MIPComponent("MIPDirectShowCapture")
{
	zeroAll();
	
	int status;

	if ((status = m_frameMutex.Init()) < 0)
	{
		std::cerr << "Error: can't initialize DirectShow capture frame mutex (JMutex error code " 
				  << status << ")" << std::endl;
		exit(-1);
	}
}

MIPDirectShowCapture::~MIPDirectShowCapture()
{
#ifdef _DEBUG
	v_debug_RemoveFromRot(m_debug_dwRegister);
#endif // _DEBUG
	close();
}

bool MIPDirectShowCapture::initCaptureGraphBuilder()
{
	IGraphBuilder *pGraphBuilder = NULL;
	ICaptureGraphBuilder2 *pGraphBuilder2 = NULL;

	HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&pGraphBuilder2);
	if (HR_FAILED(hr))
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATECAPTUREBUILDER);
		return false;
	}

	hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraphBuilder);
	if (HR_FAILED(hr))
	{
		pGraphBuilder2->Release();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATEMANAGER);
		return false;
	}

	pGraphBuilder2->SetFiltergraph(pGraphBuilder);
	m_pGraphBuilder2 = pGraphBuilder2;
	m_pGraphBuilder = pGraphBuilder;

	return true;
}

bool MIPDirectShowCapture::getCaptureDevice(int deviceNumber)
{
	//ICreateDevEnum接口建立指定类型的列表。
	ICreateDevEnum *pDevEnum = 0;
	IEnumMoniker *pEnum = 0;

	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,IID_ICreateDevEnum, (void **)(&pDevEnum));
	if (HR_FAILED(hr))
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATEDEVICEENUMERATOR);
		return false;
	}

	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
	if (HR_FAILED(hr))
	{
		pDevEnum->Release();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCAPTUREENUMERATOR);
		return false;
	}

	if (pEnum == 0)
	{
		pDevEnum->Release();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTFINDCAPTUREDEVICE);
		return false;
	}

	IMoniker *pMoniker = NULL;
	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		if (deviceNumber == 0)
		{
			hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pCaptDevice);
			pMoniker->Release();
			if (HR_SUCCEEDED(hr))
			{
				pEnum->Release();
				pDevEnum->Release();
				return true;
			}
			else
			{
				m_pCaptDevice = 0;
				pEnum->Release();
				pDevEnum->Release();
				setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTBINDCAPTUREDEVICE);
				return false;
			}
		}
		else
		{
			deviceNumber--;
		}
	}
	pEnum->Release();
	pDevEnum->Release();
	setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTFINDCAPTUREDEVICE);
	return false;
}

bool MIPDirectShowCapture::open(int width, int height, real_t frameRate, int deviceNumber)
{
	if (m_pGraphBuilder != 0)
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_ALREADYOPEN);
		return false;
	}

	if (!initCaptureGraphBuilder())
		return false;

	if (!getCaptureDevice(deviceNumber))
	{
		clearNonZero();
		return false;
	}

	std::list<GUID> guids;
	std::list<GUID>::const_iterator guidIt;

	if (!listCameraSubtypeId(guids))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTLISTGUIDS);
		return false;
	}

	bool haveI420 = false;
	bool haveYUY2 = false;
	bool haveRGB24 = false;//debug
	for (guidIt = guids.begin() ; guidIt != guids.end() ; guidIt++)
	{
		if (*guidIt == EMIP_MEDIASUBTYPE_I420)
			haveI420 = true;
		if (*guidIt == MEDIASUBTYPE_YUY2)
			haveYUY2 = true;
		if(*guidIt == MEDIASUBTYPE_RGB24)
			haveRGB24 = true;
	}

	if (haveI420)
	{
		//std::cout << "Trying I420" << std::endl;		
		m_selectedGuid = EMIP_MEDIASUBTYPE_I420;
		m_subType = MIPRAWVIDEOMESSAGE_TYPE_YUV420P;
	}
	else if (haveYUY2)
	{
		//std::cout << "Trying YUY2" << std::endl;		
		m_selectedGuid = MEDIASUBTYPE_YUY2;
		m_subType = MIPRAWVIDEOMESSAGE_TYPE_YUYV;
	}
	else if (haveRGB24)
	{
		//std::cout << "Trying RGB24" << std::endl;		
		m_selectedGuid = MEDIASUBTYPE_RGB24;
		m_subType = MIPRAWVIDEOMESSAGE_TYPE_RGB24;
	}
	else
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSELECTYUV420PORYUY2);
		return false;
	}

	HRESULT hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter,(void **)(&m_pNullRenderer));
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATENULLRENDERER);
		return false;
	}

	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter,(void **)(&m_pSampGrabFilter));
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATEGRABFILTER);
		return false;
	}

	hr = m_pSampGrabFilter->QueryInterface(IID_ISampleGrabber, (void**)&m_pGrabber);
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTCREATEGRABIFACE);
		return false;
	}

	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = m_selectedGuid;
	hr = m_pGrabber->SetMediaType(&mt);
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSELECTYUV420PORYUY2);
		return false;
	}

	m_pGrabCallback = new GrabCallback(this);

	hr = m_pGrabber->SetCallback(m_pGrabCallback, 1);
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSETCALLBACK);
		return false;
	}

	hr = m_pGraphBuilder->AddFilter(m_pCaptDevice, L"Capture Filter");
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTADDCAPTUREFILTER);
		return false;
	}

	if (!setFormat(width, height, frameRate))
	{
		clearNonZero();
		return false;
	}

	hr = m_pGraphBuilder->AddFilter(m_pNullRenderer, L"Null Renderer");
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTADDNULLRENDERER);
		return false;
	}

	hr = m_pGraphBuilder->AddFilter(m_pSampGrabFilter, L"Sample Grabber");
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTADDSAMPLEGRABBER);
		return false;
	}

	hr = m_pGraphBuilder2->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCaptDevice, m_pSampGrabFilter, m_pNullRenderer);
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTBUILDGRAPH);
		return false;
	}

	hr = m_pGrabber->GetConnectedMediaType(&mt);
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETFRAMESIZE);
		return false;
	}

	VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt.pbFormat;
	m_width  = vih->bmiHeader.biWidth;
	m_height = vih->bmiHeader.biHeight;
	CoTaskMemFree(mt.pbFormat);

	hr = m_pGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&m_pControl);
	if (HR_FAILED(hr))
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETCONTROLINTERFACE);
		return false;
	}

	if (m_selectedGuid == EMIP_MEDIASUBTYPE_I420)
	{
		m_largeFrameSize = (size_t)((m_width*m_height*3)/2);
	}
	else if(m_selectedGuid == MEDIASUBTYPE_YUY2)
	{
		m_largeFrameSize = (size_t)(m_width*m_height*2);
	}
	else if(m_selectedGuid == MEDIASUBTYPE_RGB24)
	{
		m_largeFrameSize = (size_t)(m_width*m_height*3);
	}

	m_pFullFrame = new uint8_t [m_largeFrameSize];
	m_pMsgFrame = new uint8_t [m_largeFrameSize];
	memset(m_pMsgFrame, 0, m_largeFrameSize);
	memset(m_pFullFrame, 0, m_largeFrameSize);

	if (m_subType == MIPRAWVIDEOMESSAGE_TYPE_YUV420P)
	{
		m_pVideoMsg = new MIPRawYUV420PVideoMessage(m_width, m_height, m_pMsgFrame, false);
	}
	else if(m_subType == MIPRAWVIDEOMESSAGE_TYPE_YUYV)
	{
		m_pVideoMsg = new MIPRawYUYVVideoMessage(m_width, m_height, m_pMsgFrame, false);
	}
	else if (m_subType == MIPRAWVIDEOMESSAGE_TYPE_RGB24)
	{
		m_pVideoMsg = new MIPRawRGBVideoMessage(m_width, m_height, m_pMsgFrame, false,false);//debug
	}

	if (!m_sigWait.init())
	{
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTINITSIGWAIT);
		return false;
	}

	m_captureTime = MIPTime::getCurrentTime();
	m_gotMsg = false;
	m_gotFrame = false;

	hr = m_pControl->Run();
	if (hr != S_OK && hr != S_FALSE) // Apparently S_FALSE is returned if the graph is preparing to run
	{
		m_sigWait.destroy();
		clearNonZero();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSTARTGRAPH);
		return false;
	}

	m_sourceID = 0;

#ifdef _DEBUG
	v_debug_AddToRot(m_pGraphBuilder,&m_debug_dwRegister);
	v_debug_SaveGraphFile(m_pGraphBuilder,L"E:\\Desktop\\a.grf");
#endif // _DEBUG

	return true;
}

bool MIPDirectShowCapture::close()
{
	if (m_pGraphBuilder == 0)
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_DEVICENOTOPENED);
		return false;
	}

	m_pControl->Stop();
	clearNonZero();
	m_sigWait.destroy();

	return true;
}

void MIPDirectShowCapture::clearNonZero()
{
	if (m_pControl)
	{
		m_pControl->Release();
	}
	if (m_pGrabber)
	{
		m_pGrabber->Release();
	}
	if (m_pSampGrabFilter)
	{
		m_pSampGrabFilter->Release();
	}
	if (m_pNullRenderer)
	{
		m_pNullRenderer->Release();
	}
	if (m_pCaptDevice)
	{
		m_pCaptDevice->Release();
	}
	if (m_pGraphBuilder2)
	{
		m_pGraphBuilder2->Release();
	}
	if (m_pGraphBuilder)
	{
		m_pGraphBuilder->Release();
	}
	if (m_pGrabCallback)
	{
		delete m_pGrabCallback;
	}
	if (m_pVideoMsg)
	{
		delete m_pVideoMsg;
	}
	if (m_pMsgFrame)
	{
		delete [] m_pMsgFrame;
	}
	if (m_pFullFrame)
	{
		delete [] m_pFullFrame;
	}
	zeroAll();
}

void MIPDirectShowCapture::zeroAll()
{
	m_pGraphBuilder = 0;
	m_pGraphBuilder2 = 0;
	m_pControl = 0;
	m_pCaptDevice = 0;
	m_pNullRenderer = 0;
	m_pSampGrabFilter = 0;
	m_pGrabber = 0;
	m_pGrabCallback = 0;
	m_pFullFrame = 0;
	m_pMsgFrame = 0;
	m_pVideoMsg = 0;
	m_debug_dwRegister = 0;
}


bool MIPDirectShowCapture::setFormat(int w, int h, real_t rate)
{
	IAMStreamConfig *pConfig = 0;

	HRESULT hr = m_pGraphBuilder2->FindInterface(&PIN_CATEGORY_CAPTURE, 0, 
		m_pCaptDevice, IID_IAMStreamConfig, (void**)&pConfig);
	if (HR_FAILED(hr))
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETDEVICECONFIG);
		return false;
	}

	int count = 0;
	int s = 0;

	hr = pConfig->GetNumberOfCapabilities(&count, &s);
	if (HR_FAILED(hr))
	{
		pConfig->Release();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETDEVICECAPS);
		return false;
	}

	if (s != sizeof(VIDEO_STREAM_CONFIG_CAPS))
	{
		pConfig->Release();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_INVALIDCAPS);
		return false;
	}

	for (int i = 0; i < count; i++)
	{
		VIDEO_STREAM_CONFIG_CAPS caps;
		AM_MEDIA_TYPE *pMediaType;

		hr = pConfig->GetStreamCaps(i, &pMediaType, (BYTE*)&caps);
		if (HR_SUCCEEDED(hr))
		{
			if ((pMediaType->majortype == MEDIATYPE_Video) &&
				(pMediaType->subtype == m_selectedGuid) &&
				(pMediaType->formattype == FORMAT_VideoInfo) &&
				(pMediaType->cbFormat >= sizeof (VIDEOINFOHEADER)) &&
				(pMediaType->pbFormat != 0))
			{
				VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pMediaType->pbFormat;

				pVih->bmiHeader.biWidth = w;
				pVih->bmiHeader.biHeight = h;
				pVih->bmiHeader.biSizeImage = DIBSIZE(pVih->bmiHeader);
				pVih->AvgTimePerFrame = (REFERENCE_TIME)(10000000.0/rate);

				hr = pConfig->SetFormat(pMediaType);
				if (HR_SUCCEEDED(hr))
				{
					CoTaskMemFree(pMediaType->pbFormat);
					pConfig->Release();
					return true;
				}
			}

			if (pMediaType->pbFormat != 0)
				CoTaskMemFree(pMediaType->pbFormat);
		}
	}

	pConfig->Release();
	setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTSETCAPS);
	return false;
}

bool MIPDirectShowCapture::listCameraSubtypeId(std::list<GUID> &guids)
{
	guids.clear();

	IAMStreamConfig *pConfig = 0;
	HRESULT hr = m_pGraphBuilder2->FindInterface(&PIN_CATEGORY_CAPTURE, 0, m_pCaptDevice, IID_IAMStreamConfig, (void**)&pConfig);
	if (HR_FAILED(hr))
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETDEVICECONFIG);
		return false;
	}

	//debug
	//虚拟摄像头(默认设置)
	//+		majortype	{73646976-0000-0010-8000-00AA00389B71}	_GUID MEDIATYPE_Video
	//+		subtype		{E436EB7D-524F-11CE-9F53-0020AF0BA770}	_GUID MEDIASUBTYPE_RGB24
	//+		formattype	{CLSID_WDM Streaming Capture VideoInfoHeader Data Type Handler}	_GUID

	//USB摄像头
	//+		majortype	{73646976-0000-0010-8000-00AA00389B71}	_GUID MEDIATYPE_Video
	//+		subtype		{32595559-0000-0010-8000-00AA00389B71}	_GUID MEDIASUBTYPE_YUY2
	AM_MEDIA_TYPE *type=0;
	hr=pConfig->GetFormat(&type);
	if (HR_FAILED(hr))
	{
		pConfig->Release();
		setErrorString("获取摄像头输出格式失败");
		return false;
	}

	int count = 0;
	int s = 0;

	hr = pConfig->GetNumberOfCapabilities(&count, &s);
	if (HR_FAILED(hr))
	{
		pConfig->Release();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_CANTGETDEVICECAPS);
		return false;
	}

	if (s != sizeof(VIDEO_STREAM_CONFIG_CAPS))
	{
		pConfig->Release();
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_INVALIDCAPS);
		return false;
	}

	for (int i = 0; i < count; i++)
	{
		VIDEO_STREAM_CONFIG_CAPS caps;
		AM_MEDIA_TYPE *pMediaType;

		hr = pConfig->GetStreamCaps(i, &pMediaType, (BYTE*)&caps);
		if (HR_SUCCEEDED(hr))
		{
			if (pMediaType->majortype == MEDIATYPE_Video)
			{
				GUID subType = pMediaType->subtype;
				guids.push_back(subType);

				//LPOLESTR lpOleStr=NULL;
				//StringFromCLSID(subType,&lpOleStr);
				//CoTaskMemFree(lpOleStr);
			}
		}
	}

	return true;
}

void MIPDirectShowCapture::copyVideoFrame()
{
	memcpy(m_pMsgFrame, m_pFullFrame, m_largeFrameSize);
}

bool MIPDirectShowCapture::push(const MIPComponentChain &chain, int64_t iteration, MIPMessage *pMsg)
{
	if (m_pGraphBuilder == 0)
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_DEVICENOTOPENED);
		return false;
	}
	
	if (pMsg->getMessageType() == MIPMESSAGE_TYPE_SYSTEM && 
		pMsg->getMessageSubtype() == MIPSYSTEMMESSAGE_TYPE_ISTIME)
	{
		m_frameMutex.Lock();
		copyVideoFrame();
		m_pVideoMsg->setTime(m_captureTime);
		m_pVideoMsg->setSourceID(m_sourceID);
		m_frameMutex.Unlock();

		m_gotMsg = false;
	}
	else if (pMsg->getMessageType() == MIPMESSAGE_TYPE_SYSTEM && 
		     pMsg->getMessageSubtype() == MIPSYSTEMMESSAGE_TYPE_WAITTIME)
	{
		bool waitForFrame = false;
		
		m_frameMutex.Lock();
		if (!m_gotFrame)
		{
			copyVideoFrame();
			m_pVideoMsg->setTime(m_captureTime);
			m_gotFrame = true;
		}
		else
		{
			waitForFrame = true;
		}
		m_frameMutex.Unlock();

		if (waitForFrame)
		{
			m_sigWait.clearSignalBuffers();
			m_sigWait.waitForSignal();

			m_frameMutex.Lock();
			copyVideoFrame();
			m_pVideoMsg->setTime(m_captureTime);
			m_pVideoMsg->setSourceID(m_sourceID);
			m_gotFrame = true;
			m_frameMutex.Unlock();
		}

		m_gotMsg = false;
	}
	else
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_BADMESSAGE);
		return false;
	}

	return true;
}

bool MIPDirectShowCapture::pull(const MIPComponentChain &chain, int64_t iteration, MIPMessage **pMsg)
{
	if (m_pGraphBuilder == 0)
	{
		setErrorString(MIPDIRECTSHOWCAPTURE_ERRSTR_DEVICENOTOPENED);
		return false;
	}

	if (m_gotMsg)
	{
		m_gotMsg = false;
		*pMsg = 0;
	}
	else
	{
		m_gotMsg = true;
		*pMsg = m_pVideoMsg;
	}

	return true;
}

int MIPDirectShowCapture::getWidth() const
{
	if (m_pGraphBuilder == 0) 
	{
		return -1; 
	}
	return m_width;
}

int MIPDirectShowCapture::getHeight() const
{
	if (m_pGraphBuilder == 0) 
	{
		return -1; 
	}
	return m_height;
}

void MIPDirectShowCapture::setSourceID( uint64_t srcID )
{
	m_sourceID = srcID;
}

uint64_t MIPDirectShowCapture::getSourceID() const
{
	return m_sourceID;
}

uint32_t MIPDirectShowCapture::getFrameSubtype() const
{
	return m_subType;
}

MIPDirectShowCapture::GrabCallback::GrabCallback( MIPDirectShowCapture *pDSCapt )
{
	m_pDSCapt = pDSCapt;
}

HRESULT MIPDirectShowCapture::GrabCallback::QueryInterface( const IID & riid, void ** ppv )
{
	if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) 
	{
		*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
		return NOERROR;
	}    

	return E_NOINTERFACE;
}

HRESULT MIPDirectShowCapture::GrabCallback::SampleCB( double SampleTime, IMediaSample * pSample )
{
	return 0;
}

HRESULT MIPDirectShowCapture::GrabCallback::BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize )
{
	size_t minsize = (size_t)lBufferSize;

	if (minsize > m_pDSCapt->m_largeFrameSize)
	{
		minsize = m_pDSCapt->m_largeFrameSize;
	}

	m_pDSCapt->m_frameMutex.Lock();
	memcpy(m_pDSCapt->m_pFullFrame, pBuffer, minsize);
	m_pDSCapt->m_gotFrame = false;
	m_pDSCapt->m_captureTime = MIPTime::getCurrentTime();
	m_pDSCapt->m_frameMutex.Unlock();

	m_pDSCapt->m_sigWait.signal();
	return 0;
}

ULONG MIPDirectShowCapture::GrabCallback::AddRef()
{
	return 2;
}

ULONG MIPDirectShowCapture::GrabCallback::Release()
{
	return 1;
}

#endif // MIPCONFIG_SUPPORT_DIRECTSHOW
