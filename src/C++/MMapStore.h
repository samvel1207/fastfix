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

#ifndef FIX_MMAPSTORE_H
#define FIX_MMAPSTORE_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#include "MessageStore.h"
#include "SessionSettings.h"
#include <fstream>
#include <string>

namespace FIX
{
	class MemoryMappedFile
	{
	public:
		MemoryMappedFile(const char* filename, size_t size);
		~MemoryMappedFile();

		void open();
		void close();
		void* data();

	private:
		std::string m_filename;
		size_t m_size;
		void* m_ptr;
#ifdef _WIN32
		HANDLE m_hFile = INVALID_HANDLE_VALUE;
		HANDLE m_hMap = nullptr;
#else
		int m_fd = -1;
#endif
	};

	class Session;

	/// Creates a memory mapped based implementation of MessageStore.
	class MMapStoreFactory : public MessageStoreFactory
	{
	public:
		MMapStoreFactory(const SessionSettings& settings)
			: m_settings(settings) {};
		MMapStoreFactory(const std::string& path)
			: m_path(path) {};

		MessageStore* create(const SessionID&);
		void destroy(MessageStore*);
	private:
		std::string m_path;
		SessionSettings m_settings;
	};
	/*! @} */

	/**
	 * Memory map based implementation of MessageStore.
	 * Three files are created by this implementation. One for storing outgoing
	 * messages, one for indexing message locations and the last one to store 
	 * senderSeqNum, targetSeqNum and the session creation time.
	 *
	 * The format of the file is:<br>
	 * &nbsp;&nbsp;
	 *   [path]+[BeginString]-[SenderCompID]-[TargetCompID].body<br>
	 * &nbsp;&nbsp;
	 *   [path]+[BeginString]-[SenderCompID]-[TargetCompID].header<br>
	 * &nbsp;&nbsp;
	 *   [path]+[BeginString]-[SenderCompID]-[TargetCompID].mmap<br>
	 */
	class MMapStore : public MessageStore
	{
		struct MMapFileData
		{
			int senderSeqNum;
			int targetSeqNum;
			char time[24]; // must include '\0'
		};

	public:
		MMapStore(std::string, const SessionID& s);
		virtual ~MMapStore();

		bool set(int, const std::string&);
		void get(int, int, std::vector < std::string >&) const;

		int getNextSenderMsgSeqNum() const;
		int getNextTargetMsgSeqNum() const;
		void setNextSenderMsgSeqNum(int value);
		void setNextTargetMsgSeqNum(int value);
		void incrNextSenderMsgSeqNum();
		void incrNextTargetMsgSeqNum();

		UtcTimeStamp getCreationTime() const;

		void reset();
		void refresh();

	private:
#ifdef _MSC_VER
		typedef std::pair < int, int > OffsetSize;
#else
		typedef std::pair < long, std::size_t > OffsetSize;
#endif
		typedef std::map < int, OffsetSize > NumToOffset;

		void open(bool deleteFile);
		void populateCache();
		void setSeqNum();
		void setSession();
		bool get(int msgSeqNum, std::string& msg) const;

		MemoryStore m_cache;
		NumToOffset m_offsets;

		std::string m_mmapFileName;
		std::string m_msgFileName;
		std::string m_headerFileName;

		FILE* m_msgFile;
		FILE* m_headerFile;
		std::unique_ptr<MemoryMappedFile> m_mmapFile;
		MMapFileData* m_mmapData;
	};
}

#endif //FIX_MMAPSTORE_H
