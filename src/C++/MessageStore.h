/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef FIX_MESSAGESTORE_H
#define FIX_MESSAGESTORE_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#include "Message.h"
#include <map>
#include <vector>
#include <string>

namespace FIX
{
	class MessageStore;

	/**
	 * This interface must be implemented to create a MessageStore.
	 */
	class MessageStoreFactory
	{
	public:
		virtual ~MessageStoreFactory() {}
		virtual MessageStore* create(const SessionID&) = 0;
		virtual void destroy(MessageStore*) = 0;
	};

	/**
	 * Creates a memory based implementation of MessageStore.
	 *
	 * This will lose all data on process termination. This class should only
	 * be used for test applications, never in production.
	 */
	class MemoryStoreFactory : public MessageStoreFactory
	{
	public:
		MessageStore* create(const SessionID&);
		void destroy(MessageStore*);
	};

	/**
	 * This interface must be implemented to store and retrieve messages and
	 * sequence numbers.
	 */
	class MessageStore
	{
	public:
		virtual ~MessageStore() {}

		virtual bool set(int64_t, const std::string&) = 0;
		virtual void get(int64_t, int64_t, std::vector < std::string >&) const = 0;

		virtual int64_t getNextSenderMsgSeqNum() const = 0;
		virtual int64_t getNextTargetMsgSeqNum() const = 0;
		virtual void setNextSenderMsgSeqNum(int64_t) = 0;
		virtual void setNextTargetMsgSeqNum(int64_t) = 0;
		virtual void incrNextSenderMsgSeqNum() = 0;
		virtual void incrNextTargetMsgSeqNum() = 0;

		virtual UtcTimeStamp getCreationTime() const = 0;

		virtual void reset() = 0;
		virtual void refresh() = 0;
	};
	/*! @} */

	/**
	 * Memory based implementation of MessageStore.
	 *
	 * This will lose all data on process terminition. This class should only
	 * be used for test applications, never in production.
	 */
	class MemoryStore : public MessageStore
	{
	public:
		MemoryStore() : m_nextSenderMsgSeqNum(1), m_nextTargetMsgSeqNum(1) {}

		bool set(int64_t, const std::string&);
		void get(int64_t, int64_t, std::vector < std::string >&) const;

		int64_t getNextSenderMsgSeqNum() const
		{
			return m_nextSenderMsgSeqNum;
		}
		int64_t getNextTargetMsgSeqNum() const
		{
			return m_nextTargetMsgSeqNum;
		}
		void setNextSenderMsgSeqNum(int64_t value)
		{
			m_nextSenderMsgSeqNum = value;
		}
		void setNextTargetMsgSeqNum(int64_t value)
		{
			m_nextTargetMsgSeqNum = value;
		}
		void incrNextSenderMsgSeqNum()
		{
			++m_nextSenderMsgSeqNum;
		}
		void incrNextTargetMsgSeqNum()
		{
			++m_nextTargetMsgSeqNum;
		}

		void setCreationTime(const UtcTimeStamp& creationTime)
		{
			m_creationTime = creationTime;
		}
		UtcTimeStamp getCreationTime() const
		{
			return m_creationTime;
		}

		void reset()
		{
			m_nextSenderMsgSeqNum = 1;
			m_nextTargetMsgSeqNum = 1;
			m_messages.clear();
			m_creationTime.setCurrent();
		}
		void refresh() {}

	private:
		typedef std::map < int64_t, std::string > Messages;

		Messages m_messages;
		int64_t m_nextSenderMsgSeqNum;
		int64_t m_nextTargetMsgSeqNum;
		UtcTimeStamp m_creationTime;
	};

	class MessageStoreFactoryExceptionWrapper
	{
	private:
		MessageStoreFactory* m_pFactory;
	public:
		MessageStoreFactoryExceptionWrapper(MessageStoreFactory* pFactory)
			: m_pFactory(pFactory) {}

		MessageStore* create(const SessionID&, bool&, ConfigError&);
		void destroy(MessageStore*);
	};

	class MessageStoreExceptionWrapper
	{
	private:
		MessageStore* m_pStore;
	public:
		MessageStoreExceptionWrapper(MessageStore* pStore) : m_pStore(pStore) {}
		~MessageStoreExceptionWrapper() { delete m_pStore; }

		bool set(int, const std::string&, bool&, IOException&);
		void get(int, int, std::vector < std::string >&, bool&, IOException&) const;
		int64_t getNextSenderMsgSeqNum(bool&, IOException&) const;
		int64_t getNextTargetMsgSeqNum(bool&, IOException&) const;
		void setNextSenderMsgSeqNum(int64_t, bool&, IOException&);
		void setNextTargetMsgSeqNum(int64_t, bool&, IOException&);
		void incrNextSenderMsgSeqNum(bool&, IOException&);
		void incrNextTargetMsgSeqNum(bool&, IOException&);

		UtcTimeStamp getCreationTime(bool&, IOException&);

		void reset(bool&, IOException&);
		void refresh(bool&, IOException&);
	};
}

#endif //FIX_MESSAGESTORE_H
