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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#include "MMapStore.h"
#include "SessionID.h"
#include "Parser.h"
#include "Utility.h"
#include <fstream>

namespace FIX
{
    MemoryMappedFile::MemoryMappedFile(const char* filename, size_t size)
        : m_filename(filename), m_size(size), m_ptr(nullptr)
    {}

    MemoryMappedFile::~MemoryMappedFile()
    {
        close();
    }

    void MemoryMappedFile::open()
    {
#ifdef _WIN32
        m_hFile = CreateFileA(m_filename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (m_hFile == INVALID_HANDLE_VALUE)
            throw ConfigError("CreateFile failed");

        m_hMap = CreateFileMappingA(m_hFile, NULL, PAGE_READWRITE, 0, (DWORD)m_size, NULL);
        if (!m_hMap)
        {
            CloseHandle(m_hFile);
            throw ConfigError("CreateFileMapping failed");
        }

        m_ptr = MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, m_size);
        if (!m_ptr)
        {
            CloseHandle(m_hMap);
            CloseHandle(m_hFile);
            throw ConfigError("MapViewOfFile failed");
        }
#else
        m_fd = open(m_filename.c_str(), O_RDWR | O_CREAT, 0666);
        if (m_fd == -1)
            throw ConfigError("open failed");

        if (ftruncate(m_fd, m_size) == -1)
        {
            close(m_fd);
            throw ConfigError("ftruncate failed");
        }

        m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
        if (m_ptr == MAP_FAILED)
        {
            close(m_fd);
            throw ConfigError("mmap failed");
        }
#endif
    }

    void MemoryMappedFile::close()
    {
#ifdef _WIN32
        if (m_ptr)
            UnmapViewOfFile(m_ptr);
        if (m_hMap)
            CloseHandle(m_hMap);
        if (m_hFile != INVALID_HANDLE_VALUE)
            CloseHandle(m_hFile);
#else
        if (m_ptr)
            munmap(m_ptr, m_size);
        if (m_fd != -1)
            close(m_fd);
#endif
    }

    void* MemoryMappedFile::data()
    {
        return m_ptr;
    }

    MMapStore::MMapStore(std::string path, const SessionID& s)
        : m_msgFile(nullptr),
        m_headerFile(nullptr)
    {
        file_mkdir(path.c_str());

        if (path.empty())
            path = ".";
        const std::string& begin = s.getBeginString().getString();
        const std::string& sender = s.getSenderCompID().getString();
        const std::string& target = s.getTargetCompID().getString();
        const std::string& qualifier = s.getSessionQualifier();

        std::string sessionid = begin + "-" + sender + "-" + target;
        if (qualifier.size())
            sessionid += "-" + qualifier;

        std::string prefix = file_appendpath(path, sessionid + ".");

        m_msgFileName = prefix + "body";
        m_headerFileName = prefix + "header";
        m_mmapFileName = prefix + "mmap";

        m_mmapFile.reset(new MemoryMappedFile(m_mmapFileName.c_str(), sizeof(MMapFileData)));

        try
        {
            open(false);
        }
        catch (IOException& e)
        {
            throw ConfigError(e.what());
        }
    }

    MMapStore::~MMapStore()
    {
        if (m_msgFile)
            fclose(m_msgFile);
        if (m_headerFile)
            fclose(m_headerFile);
        m_mmapFile->close();
    }

    void MMapStore::open(bool deleteFile)
    {
        if (m_msgFile)
            fclose(m_msgFile);
        if (m_headerFile)
            fclose(m_headerFile);
        m_mmapFile->close();

        m_msgFile = nullptr;
        m_headerFile = nullptr;

        if (deleteFile)
        {
            file_unlink(m_msgFileName.c_str());
            file_unlink(m_headerFileName.c_str());
            file_unlink(m_mmapFileName.c_str());
        }

        populateCache();
        m_msgFile = file_fopen(m_msgFileName.c_str(), "r+");
        if (!m_msgFile)
            m_msgFile = file_fopen(m_msgFileName.c_str(), "w+");
        if (!m_msgFile)
            throw ConfigError("Could not open body file: " + m_msgFileName);

        m_headerFile = file_fopen(m_headerFileName.c_str(), "r+");
        if (!m_headerFile)
            m_headerFile = file_fopen(m_headerFileName.c_str(), "w+");
        if (!m_headerFile)
            throw ConfigError("Could not open header file: " + m_headerFileName);

        setNextSenderMsgSeqNum(getNextSenderMsgSeqNum());
        setNextTargetMsgSeqNum(getNextTargetMsgSeqNum());
    }

    void MMapStore::populateCache()
    {
        FILE* headerFile = file_fopen(m_headerFileName.c_str(), "r+");
        if (headerFile)
        {
            int num;
            long offset;
            std::size_t size;

            while (FILE_FSCANF(headerFile, "%d,%ld,%zu ", &num, &offset, &size) == 3)
            {
                std::pair<NumToOffset::iterator, bool> it = m_offsets.insert(
                    NumToOffset::value_type(num, std::make_pair(offset, size)));
                if (it.second == false)
                {
                    it.first->second = std::make_pair(offset, size);
                }
            }
            fclose(headerFile);
        }

        m_mmapFile->open();
        m_mmapData = reinterpret_cast<MMapFileData*>(m_mmapFile->data());
        
        if (m_mmapData->senderSeqNum > 0)
            m_cache.setNextSenderMsgSeqNum(m_mmapData->senderSeqNum);
        if (m_mmapData->targetSeqNum)
            m_cache.setNextTargetMsgSeqNum(m_mmapData->targetSeqNum);
        if (strlen(m_mmapData->time) > 0)
            m_cache.setCreationTime(UtcTimeStampConvertor::convert(m_mmapData->time));
    }

    MessageStore* MMapStoreFactory::create(const SessionID& s)
    {
        if (m_path.size())
            return new MMapStore(m_path, s);

        std::string path;
        Dictionary settings = m_settings.get(s);
        path = settings.getString(MMAP_STORE_PATH);
        return new MMapStore(path, s);
    }

    void MMapStoreFactory::destroy(MessageStore* pStore)
    {
        delete pStore;
    }

    bool MMapStore::set(int msgSeqNum, const std::string& msg)
    {
        if (fseek(m_msgFile, 0, SEEK_END))
            throw IOException("Cannot seek to end of " + m_msgFileName);
        if (fseek(m_headerFile, 0, SEEK_END))
            throw IOException("Cannot seek to end of " + m_headerFileName);

        long offset = ftell(m_msgFile);
        if (offset < 0)
            throw IOException("Unable to get file pointer position from " + m_msgFileName);
        std::size_t size = msg.size();

        if (fprintf(m_headerFile, "%d,%ld,%zu ", msgSeqNum, offset, size) < 0)
            throw IOException("Unable to write to file " + m_headerFileName);

        std::pair<NumToOffset::iterator, bool> it = m_offsets.insert(
            NumToOffset::value_type(msgSeqNum, std::make_pair(offset, size)));
        if (it.second == false)
        {
            it.first->second = std::make_pair(offset, size);
        }
        fwrite(msg.c_str(), sizeof(char), msg.size(), m_msgFile);
        if (ferror(m_msgFile))
            throw IOException("Unable to write to file " + m_msgFileName);
        if (fflush(m_msgFile) == EOF)
            throw IOException("Unable to flush file " + m_msgFileName);
        if (fflush(m_headerFile) == EOF)
            throw IOException("Unable to flush file " + m_headerFileName);
        return true;
    }

    void MMapStore::get(int begin, int end, std::vector < std::string >& result) const
    {
        result.clear();
        std::string msg;
        for (int i = begin; i <= end; ++i)
        {
            if (get(i, msg))
                result.push_back(msg);
        }
    }

    int MMapStore::getNextSenderMsgSeqNum() const
    {
        return m_cache.getNextSenderMsgSeqNum();
    }

    int MMapStore::getNextTargetMsgSeqNum() const
    {
        return m_cache.getNextTargetMsgSeqNum();
    }

    void MMapStore::setNextSenderMsgSeqNum(int value)
    {
        m_cache.setNextSenderMsgSeqNum(value);
        setSeqNum();
    }

    void MMapStore::setNextTargetMsgSeqNum(int value)
    {
        m_cache.setNextTargetMsgSeqNum(value);
        setSeqNum();
    }

    void MMapStore::incrNextSenderMsgSeqNum()
    {
        m_cache.incrNextSenderMsgSeqNum();
        setSeqNum();
    }

    void MMapStore::incrNextTargetMsgSeqNum()
    {
        m_cache.incrNextTargetMsgSeqNum();
        setSeqNum();
    }

    UtcTimeStamp MMapStore::getCreationTime() const
    {
        return m_cache.getCreationTime();
    }

    void MMapStore::reset()
    {
        try
        {
            m_cache.reset();
            open(true);
            setSession();
        }
        catch (std::exception& e)
        {
            throw IOException(e.what());
        }
    }

    void MMapStore::refresh()
    {
        try
        {
            m_cache.reset();
            open(false);
        }
        catch (std::exception& e)
        {
            throw IOException(e.what());
        }
    }

    void MMapStore::setSeqNum()
    {
        m_mmapData->senderSeqNum = getNextSenderMsgSeqNum();
        m_mmapData->targetSeqNum = getNextTargetMsgSeqNum();
    }

    void MMapStore::setSession()
    {
        strcpy(m_mmapData->time, UtcTimeStampConvertor::convert(m_cache.getCreationTime()).c_str());
    }

    bool MMapStore::get(int msgSeqNum, std::string& msg) const
    {
        NumToOffset::const_iterator find = m_offsets.find(msgSeqNum);
        if (find == m_offsets.end()) return false;
        const OffsetSize& offset = find->second;
        if (fseek(m_msgFile, offset.first, SEEK_SET))
            throw IOException("Unable to seek in file " + m_msgFileName);
        char* buffer = new char[offset.second + 1];
        size_t result = fread(buffer, sizeof(char), offset.second, m_msgFile);
        if (ferror(m_msgFile) || result != (size_t)offset.second)
        {
            delete[] buffer;
            throw IOException("Unable to read from file " + m_msgFileName);
        }
        buffer[offset.second] = 0;
        msg = buffer;
        delete[] buffer;
        return true;
    }
} //namespace FIX
