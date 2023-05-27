#include "WinHttpHandle.h"

WinHttpHandle::WinHttpHandle() :m_handle(nullptr)
{
}

WinHttpHandle::WinHttpHandle(WinHttpHandle&& rhs) noexcept
{
    m_handle = rhs.m_handle;
    rhs.m_handle = nullptr;
}
WinHttpHandle::~WinHttpHandle()
{
    Close();
}

bool WinHttpHandle::Attach(HINTERNET handle)
{
    m_handle = handle;
    return nullptr != m_handle;
}

HINTERNET WinHttpHandle::Detach()
{
    HANDLE handle = m_handle;
    m_handle = nullptr;
    return handle;
}

void WinHttpHandle::Close()
{
    if (nullptr != m_handle)
    {
        WinHttpCloseHandle(m_handle);
        m_handle = nullptr;
    }
}

HRESULT WinHttpHandle::SetOption(DWORD option, const void* value, DWORD length)
{
    if (!::WinHttpSetOption(m_handle, option, const_cast<void*>(value), length))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}

HRESULT WinHttpHandle::QueryOption(DWORD option, void* value, DWORD & length) const
{
    if (!::WinHttpQueryOption(m_handle, option, value, &length))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}
