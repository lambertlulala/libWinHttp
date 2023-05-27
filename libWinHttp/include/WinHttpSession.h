#pragma once

#include "WinHttpHandle.h"

#include <string>

class WinHttpSession : public WinHttpHandle
{
public:
    virtual HRESULT Initialize(const std::wstring &agent = L"");
};
