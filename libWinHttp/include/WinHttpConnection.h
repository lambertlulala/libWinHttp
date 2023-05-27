#pragma once

#include "WinHttpHandle.h"
#include "WinHttpSession.h"

class WinHttpConnection : public WinHttpHandle
{
public:
    virtual HRESULT Initialize(PCWSTR serverName, INTERNET_PORT portNumber, const WinHttpSession& session);
};

