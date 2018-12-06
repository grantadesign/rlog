#include "StreamNodes.h"

#include <rlog/RLogPublisher.h>
#include <rlog/RLogChannel.h>

#include <strsafe.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace rlog;

namespace csi
{
    const static wchar_t LOGFILE_BYTE_ORDER_MARK_UTF16LE = 0xFEFF;
    const uint64_t GrantaStreamNode::sSessionUID_Invalid = 0xFFFFFFFFFFFFFFFFUL; // static for later use

    uint64_t GrantaStreamNode::SessionUID_Invalid()
    {
    return sSessionUID_Invalid;
    }

    void RLOG_DECL EnsureLogFileEncodingCorrect(const std::wstring &pLogFile)
    {
        std::ifstream tLogStream(pLogFile.c_str(), std::ios::binary);
        if (tLogStream)
        {
            bool tShouldDelete = true; //Assume we need to unless we prove otherwise.

            char tBuf[sizeof(LOGFILE_BYTE_ORDER_MARK_UTF16LE)];
            tLogStream.read(tBuf, sizeof(LOGFILE_BYTE_ORDER_MARK_UTF16LE));

            if (sizeof(LOGFILE_BYTE_ORDER_MARK_UTF16LE) == tLogStream.gcount())
            {
                tShouldDelete = !std::equal(tBuf, tBuf + sizeof(LOGFILE_BYTE_ORDER_MARK_UTF16LE), (char*)&LOGFILE_BYTE_ORDER_MARK_UTF16LE);
            }
            
            if (tShouldDelete) //If the BOM is not as expected, or just wasn't there, delete the file, because appending to it will result in garbage.
            {
                tLogStream.close(); //Otherwise we'll fail to delete due to open handle.
                DeleteFile(pLogFile.c_str());
            }
        }
    }

    GrantaStreamNode::GrantaStreamNode(std::wostream *pStream, uint64_t nSessionUID)
        : mLogStream(pStream),
        mSessionUID(nSessionUID),
        mShouldUseMultiSessionLogging(true)
    {}

    void GrantaStreamNode::SetIncludeMultiSessionLoggingEnabled(bool bShouldBeEnabled)
    {
        mShouldUseMultiSessionLogging = bShouldBeEnabled;
    }

    GrantaConsoleLogNode::GrantaConsoleLogNode(uint64_t nSessionUID)
        : GrantaStreamNode(new std::wostream(std::wcout.rdbuf()), nSessionUID) // std::wcout)
    {}

    GrantaFileLogNode::GrantaFileLogNode(const std::wstring &pLogFilePath, uint64_t nSessionUID)
        : GrantaStreamNode(new std::wofstream(pLogFilePath.c_str(), ios_base::app | ios_base::out | ios::binary), nSessionUID) //Important: open as binary to prevent conversion during output
    {
        mLogStream->rdbuf()->pubsetbuf(mStreamBuffer, STREAM_BUF_SIZE); //Make the internal buffer a wchar_t buffer, otherwise the stream goes bad when we do the put() on the next line.
        mLogStream->put(LOGFILE_BYTE_ORDER_MARK_UTF16LE); //Set the BOM for UTF-16

        wostringstream ss;
        if (mShouldUseMultiSessionLogging)
        {
            ss << L"[ses:" << FormatU64ToHex(mSessionUID) << L"] ";
            ss << L"[pid:" << GetCurrentProcessId() << L"] ";
        }
        ss << L"[tid:" << GetCurrentThreadId() << L"] ";
        ss << L"==================================== Opening log file at " << pLogFilePath << L" ====================================\r\n";
        *mLogStream << ss.str();
    }


    GrantaStreamNode::~GrantaStreamNode()
    {
        mLogStream->flush(); //Probably not needed.
    }

    void GrantaStreamNode::subscribeTo(RLogNode *pNode)
    {
        addPublisher(pNode);
        pNode->isInterested(this, true);
    }

    void GrantaConsoleLogNode::publish(const RLogData &data)
    {
        HANDLE windowsStdOutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        ::CONSOLE_SCREEN_BUFFER_INFO currentConsoleAttributes;
        ::GetConsoleScreenBufferInfo(windowsStdOutHandle, &currentConsoleAttributes);

        if (data.publisher->channel->name() == "error")
        {
            ::SetConsoleTextAttribute(windowsStdOutHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
        }
        else if (data.publisher->channel->name() == "info")
        {
            ::SetConsoleTextAttribute(windowsStdOutHandle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        }
        else if (data.publisher->channel->name() == "warn")
        {
            ::SetConsoleTextAttribute(windowsStdOutHandle, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY /*yellow*/);
        }
        else if (data.publisher->channel->name() == "sync")
        {
            ::SetConsoleTextAttribute(windowsStdOutHandle, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY);
        }
        //debug is left as default
        //if we could be sure of targetting Vista+ we could use SetConsoleScreenBufferInfoEx, which would give more options.

        GrantaStreamNode::publish(data);

        //restore previous attributes
        ::SetConsoleTextAttribute(windowsStdOutHandle, currentConsoleAttributes.wAttributes);
    }

    void GrantaStreamNode::publish(const RLogData &pData)
    {
        tm tCurrentTime;
        localtime_s(&tCurrentTime, &pData.time);
        wchar_t tTimeStamp[32];
        StringCbPrintf(tTimeStamp, sizeof(tTimeStamp) /*in bytes*/, L"%02i:%02i:%02i %02i/%02i/%02i", tCurrentTime.tm_hour, tCurrentTime.tm_min, tCurrentTime.tm_sec,
                       tCurrentTime.tm_mday, 1 + tCurrentTime.tm_mon, 1900 + tCurrentTime.tm_year);
        //adding (%d), errTime would reinstate the old "raw" time number, which isn't massively useful.
        
        wostringstream ss;

        if (mShouldUseMultiSessionLogging)
        {
            ss << L"[ses:" << FormatU64ToHex(mSessionUID) << L"] ";
            ss << L"[pid:" << GetCurrentProcessId() << L"] ";
        }
        ss << L"[tid:" << GetCurrentThreadId() << L"] ";
        ss << L"[" << pData.publisher->channel->name().c_str() << L"] ";
        ss << L"[time:" << tTimeStamp << L"] ";
        ss << L"[loc:" << pData.publisher->fileName << ':' << pData.publisher->lineNum << L"] ";
        ss << pData.msg << L"\r\n";

        *mLogStream << ss.str();
        mLogStream->flush();

        if (TRUE == ::IsDebuggerPresent())
        {
            wostringstream clickableMessage;
            clickableMessage << pData.publisher->fileName << '(' << pData.publisher->lineNum << "): TID[ " << GetCurrentThreadId() << " ] " << pData.msg << L"\r\n";
            ::OutputDebugStringW(clickableMessage.str().c_str());
        }
    }

    std::wstring GrantaStreamNode::FormatU64ToHex(uint64_t nNumberToFormat) const
    {
        wchar_t tBuffer[32];
        StringCbPrintf(tBuffer, sizeof(tBuffer) /*in bytes*/, L"%08x%08x", ((uint32_t)(nNumberToFormat >> 32) & 0xFFFFFFFF), ((uint32_t)nNumberToFormat & 0xFFFFFFFF));
        return std::wstring(tBuffer);
    }
}