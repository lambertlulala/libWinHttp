#include "WinHttpConnection.h"

HRESULT WinHttpConnection::Initialize(PCWSTR serverName, INTERNET_PORT portNumber, const WinHttpSession& session)
{
    if (!Attach(::WinHttpConnect(session.m_handle, serverName, portNumber, 0)))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    return S_OK;
}