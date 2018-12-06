#pragma once


#include <rlog/RLogNode.h>
#include <ostream>
#include <cstdint>
#include <memory>

namespace csi
{
    void RLOG_DECL EnsureLogFileEncodingCorrect(const std::wstring &pLogFile);

    class RLOG_DECL GrantaStreamNode : public rlog::RLogNode
    {
        GrantaStreamNode(const GrantaStreamNode&); //noncopyable
        GrantaStreamNode& operator=(const GrantaStreamNode&);
    public:
        void subscribeTo(RLogNode *pNode);
        virtual void publish(const rlog::RLogData &data);

        void SetIncludeMultiSessionLoggingEnabled(bool bShouldBeEnabled);

        static uint64_t SessionUID_Invalid();
    protected:
        GrantaStreamNode(std::wostream *pStream, uint64_t nSessionUID); //caller releases ownership
        virtual ~GrantaStreamNode();

        std::wstring FormatU64ToHex(uint64_t nNumberToFormat) const;
         
        const static size_t STREAM_BUF_SIZE = 512;

        wchar_t                      mStreamBuffer[STREAM_BUF_SIZE];
        std::unique_ptr<std::wostream> mLogStream;
        uint64_t                     mSessionUID;
        bool                         mShouldUseMultiSessionLogging;

        static const uint64_t         sSessionUID_Invalid;
    };

    class RLOG_DECL GrantaFileLogNode : public GrantaStreamNode
    {
    public:
        GrantaFileLogNode(const std::wstring &pLogFilePath, uint64_t nSessionUID);
    };

    class RLOG_DECL GrantaConsoleLogNode : public GrantaStreamNode
    {
    public:
        GrantaConsoleLogNode(uint64_t nSessionUID);

        virtual void publish(const rlog::RLogData &data);
    };
}

