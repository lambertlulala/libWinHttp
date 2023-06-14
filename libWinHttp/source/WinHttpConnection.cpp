#include "WinHttpConnection.h"

HRESULT WinHttpConnection::Initialize(PCWSTR serverName, INTERNET_PORT serverPort, const WinHttpSession& session)
{
    if (!Attach(::WinHttpConnect(session.m_handle, serverName, serverPort, 0)))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}