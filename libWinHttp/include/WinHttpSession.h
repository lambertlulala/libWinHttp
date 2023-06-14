#pragma once

#include "WinHttpHandle.h"

#include <string>

class WinHttpSession : public WinHttpHandle
{
public:
    virtual HRESULT Initialize(const std::wstring &agent = L"", DWORD accessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        LPCWSTR proxy = WINHTTP_NO_PROXY_NAME, LPCWSTR proxyBypass = WINHTTP_NO_PROXY_BYPASS, DWORD flags = WINHTTP_FLAG_ASYNC);
};
