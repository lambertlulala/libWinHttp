#pragma once

#include <Windows.h>
#include <winhttp.h>

class WinHttpHandle
{
public:
    WinHttpHandle();
    WinHttpHandle(const WinHttpHandle& rhs) = delete;
    WinHttpHandle(WinHttpHandle&& rhs) noexcept;
    void operator= (WinHttpHandle& rhs) = delete;

    virtual ~WinHttpHandle();

    bool Attach(HINTERNET handle);
    HINTERNET Detach();

    void Close();
    HRESULT SetOption(DWORD option, const void* value, DWORD length);
    HRESULT QueryOption(DWORD option, void* value, DWORD& length) const;

    HINTERNET m_handle;
};

