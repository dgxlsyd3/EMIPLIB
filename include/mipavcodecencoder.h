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
 * \file mipavcodecencoder.h
 */

#ifndef MIPAVCODECENCODER_H

#define MIPAVCODECENCODER_H

#include "mipconfig.h"

#ifdef MIPCONFIG_SUPPORT_AVCODEC

#include "mipcomponent.h"

#ifdef MIPCONFIG_SUPPORT_AVCODEC_OLD
#include <libavcodec/avcodec.h>
#else
extern "C" {
#include <libavcodec/avcodec.h>
}
#endif // MIPCONFIG_SUPPORT_AVCODEC_OLD

class MIPEncodedVideoMessage;

/** ����libavcodec��H.263+������
 *  This component is a H.263+ encoder, based on the libavcodec library. It accepts
 *  raw video messages in YUV420P format and creates encoded video messages with
 *  subtype MIPENCODEDVIDEOMESSAGE_TYPE_H263P.
 */
class MIPAVCodecEncoder : public MIPComponent
{
public:
	MIPAVCodecEncoder();
	~MIPAVCodecEncoder();

	/** Initializes the encoder.
	 *  This function initializes the encoder.
	 *  \param width The width of incoming video frames.
	 *  \param height The height of incoming video frames.
	 *  \param framerate The framerate.
	 *  \param bitrate The bitrate generated by the encoder. If the value is zero or
	 *                 negative, a default value is used.
	 */
	bool init(int width, int height, real_t framerate, int bitrate = 0);

	/** De-initializes the encoder. */
	bool destroy();
	bool push(const MIPComponentChain &chain, int64_t iteration, MIPMessage *pMsg);
	bool pull(const MIPComponentChain &chain, int64_t iteration, MIPMessage **pMsg);

	/** Initializes the libavcodec library.
	 *  This function initializes the libavcodec library. The library should only be initialized once
	 *  in an application.
	 */
	static void initAVCodec();
private:
	bool getFrameRate(real_t framerate, int *numerator, int *denominator);

	AVCodec *m_pCodec;
	AVCodecContext *m_pContext;
	AVFrame *m_pFrame;
	int m_width, m_height;
	MIPEncodedVideoMessage *m_pMsg;
	uint8_t *m_pData;
	int m_bufSize;
	bool m_gotMessage;
};

#endif // MIPCONFIG_SUPPORT_AVCODEC

#endif // MIPAVCODECENCODER_H

