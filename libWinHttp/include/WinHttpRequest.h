#pragma once

#include "WinHttpConnection.h"

#include <vector>
#include <string>

template <typename T>
class WinHttpRequest : public WinHttpHandle
{
public:
    virtual HRESULT Initialize(LPCWSTR path, LPCWSTR verb, const WinHttpConnection& connection,
        LPCWSTR version = nullptr, LPCWSTR referrer = WINHTTP_NO_REFERER,
        LPCWSTR *acceptTypes = WINHTTP_DEFAULT_ACCEPT_TYPES, DWORD flags = WINHTTP_FLAG_SECURE,
        size_t bufferSize = 32 * 1024)
    {
        HINTERNET request = ::WinHttpOpenRequest(connection.m_handle, verb, path, nullptr,
            referrer, acceptTypes, flags);
        if (!Attach(request))
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        if (WINHTTP_INVALID_STATUS_CALLBACK == ::WinHttpSetStatusCallback(m_handle, Callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0)) 
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        m_buffer.assign(bufferSize, 0);

        return S_OK;
    }

    virtual HRESULT SendRequest(PCWSTR headers, DWORD headersLength, const void* optional, DWORD optionalLength, DWORD totalLength)
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
