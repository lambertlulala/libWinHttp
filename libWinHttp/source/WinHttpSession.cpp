#include "WinHttpSession.h"

HRESULT WinHttpSession::Initialize(const std::wstring& agent, DWORD accessType,
    LPCWSTR proxy, LPCWSTR proxyBypass, DWORD flags)
{
    if (!Attach(::WinHttpOpen(agent.c_str(), accessType, proxy, proxyBypass, flags)))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}