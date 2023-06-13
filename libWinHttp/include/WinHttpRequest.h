#pragma once

#include "WinHttpConnection.h"

template <typename T>
class WinHttpRequest : public WinHttpHandle
{
public:
    virtual HRESULT Initialize(PCWSTR path, __in_opt PCWSTR verb, const WinHttpConnection& connection)
    {
        HINTERNET request = ::WinHttpOpenRequest(connection.m_handle,
            verb,
            path,
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!Attach(request))
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        if (WINHTTP_INVALID_STATUS_CALLBACK == ::WinHttpSetStatusCallback(m_handle, Callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0)) 
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        m_buffer.assign(32 * 1024, 0);

        return S_OK;
    }

    virtual HRESULT SendRequest(__in_opt PCWSTR headers, DWORD headersLength, __in_opt const void* optional, DWORD optionalLength, DWORD totalLength)
    {
        T* pT = static_cast<T*>(this);

        if (!::WinHttpSendRequest(m_handle, headers, headersLength, const_cast<LPVOID>(optional), optionalLength, totalLength, reinterpret_cast<DWORD_PTR>(pT)))
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        return S_OK;
    }

protected:
    static void CALLBACK Callback(HINTERNET handle, DWORD_PTR context, DWORD code, void* info, DWORD length)
    {
        if (0 != context)
        {
            T* pT = reinterpret_cast<T*>(context);

            HRESULT result = pT->OnCallback(code, info, length);
            if (FAILED(result))
            {
                pT->OnResponseComplete(result);
            }
        }
    }

    using Buffer = std::vector<char>;
    Buffer m_buffer;
};
