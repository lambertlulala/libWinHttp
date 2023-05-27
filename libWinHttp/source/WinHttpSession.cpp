#include "WinHttpSession.h"

HRESULT WinHttpSession::Initialize(const std::wstring &agent)
{
    if (!Attach(::WinHttpOpen(agent.c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC)))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}