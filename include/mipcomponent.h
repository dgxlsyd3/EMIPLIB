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
 * \file mipcomponent.h
 */

#ifndef MIPCOMPONENT_H

#define MIPCOMPONENT_H

#include "mipconfig.h"
#include "miperrorbase.h"
#include "miptypes.h"
#include <jmutex.h>
#include <string>

class MIPComponentChain;
class MIPMessage;
class MIPFeedback;

/** ���Է���������ϵ�����Ļ��ࡣ
 *  This class serves as a base class from which actual components can be derived. A working component
 *  needs to implement the MIPComponent::pull and MIPComponent::push methods.
 *  ʵ�ʹ������������ʵ��MIPComponent::pull �� MIPComponent::push������
 */
class MIPComponent : public MIPErrorBase
{
protected:

	/** Create a component with the specified name.
	 *  Constructor which can only be called from derived classes and which stores the name of
	 *  the component.
	 */
	MIPComponent(const std::string &componentName);
public:
	virtual ~MIPComponent();

	/** Locks the current component.
	 *  This function locks the component. It is used in the MIPComponentChain background thread
	 *  to prevent a component being accessed at the same time in different threads.
	 */
	virtual void lock()										{ m_componentMutex.Lock(); }

	/** Unlocks the current component.
	 *  This function removes the lock on the current component. It too is used in the MIPComponentChain
	 *  background thread.
	 */
	virtual void unlock()										{ m_componentMutex.Unlock(); }

	/** ǰ��������ô˺�����������Ϣ���������
	 *  This function needs to be implemented by a derived class. It is part of the message passing system
	 *  and is intended to feed messages to the current component. The function is called from the background
	 *  thread of a MIPComponentChain object. In principle, it is possible that a component can be accessed
	 *  by different chains. For this reason, the calling chain is passed as the first argument. 
	 *  The second argument describes the current iteration in the chain's background thread. Finally,
	 *  the third argument is a pointer to the message itself.
	 *  �������ʵ�ִ˷�����
	 *  ������Ϣ����ϵͳ��һ���֣����ڸ���ǰ���������Ϣ��MIPComponentChain����ĺ�̨�̵߳��øú�����
	 *  ԭ���ϣ�������Ա���ͬ�������ʡ�
	 *  ��һ�������ǵ��øú����������ڶ��������Ǻ�̨�̱߳�ʶ����������������Ϣָ�롣
	 */
	virtual bool push(const MIPComponentChain &chain, int64_t iteration, MIPMessage *pMsg) = 0;

	/** ��������ô˺��������ӱ������ȡ��Ϣ��
	 *  This function needs to be implemented by a derived class. It is part of the message passing system
	 *  and is intended to retrieve messages from the current component. The function is called from the background
	 *  thread of a MIPComponentChain object. In principle, it is possible that a component can be accessed
	 *  by different chains. For this reason, the calling chain is passed as the first argument. 
	 *  The second argument describes the current iteration in the chain's background thread. Finally,
	 *  in the third parameter a pointer to the retrieved message is stored.
	 *
	 *  In one specific iteration of the background thread, it is possible that multiple messages need
	 *  to be retrieved (received RTP packets for example). To make this possible, the component should
	 *  have a list of messages ready and each time the pull function is called, another message should
	 *  be stored in the pMsg parameter. After the last message has been passed, a NULL pointer should
	 *  be stored in pMsg to indicate that it was indeed the last message. At this point, the retrieval
	 *  system should be reinitialized in such a way that a successive call to the pull function will
	 *  again store the first message in pMsg. This way it is possible to pass messages from one component
	 *  to multiple components.
	 */
	virtual bool pull(const MIPComponentChain &chain, int64_t iteration, MIPMessage **pMsg) = 0;

	/** ����йش�����ķ�����Ϣ��
	 *  If the component implements this function, it can add feedback information to the MIPFeedback object
	 *  passed as the third argument. As with the push and pull functions, the current chain is also passed
	 *  as an argument. Since it is possible that two feedback chains end in the same component, an
	 *  identifier describing the specific feedback chain is passed as the second argument.
	 */
	virtual bool processFeedback(const MIPComponentChain &chain, int64_t feedbackChainID, MIPFeedback *feedback)			
													{ return true; }

	/** ������������ƣ��������ǹ��캯�����õġ�
	 *  This function returns the name of the component, as it was specified in the constructor.
	 */
	std::string getComponentName() const								{ return m_componentName; }

	// TODO: is it necessary to explain this in the documentation? Most likely only useful in the
	//       MIPComponentAlias component
	virtual const MIPComponent *getComponentPointer() const						{ return this; }
private:
	JMutex m_componentMutex;
	std::string m_componentName;
};

#endif // MIPCOMPONENT_H

