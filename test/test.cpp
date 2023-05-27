#include "pch.h"

#include "WinHttpRequest.h"

#include <fstream>

class TestGetRequest : public WinHttpRequest<TestGetRequest>
{
private:
    HANDLE m_waitEvent;
    bool m_result;

public:
    TestGetRequest() : m_waitEvent{ nullptr }, m_result{ false }
    {
        m_waitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    }

    virtual ~TestGetRequest()
    {
        if (m_waitEvent)
        {
            CloseHandle(m_waitEvent);
        }
    }

    virtual HRESULT Initialize(PCWSTR source, const std::wstring& destination, const WinHttpConnection& connection)
    {
        m_stream.open(destination, std::ios_base::out);

        return WinHttpRequest<TestGetRequest>::Initialize(source, 0, connection);
    }

    HRESULT OnReadComplete(const void* buffer, DWORD bytesRead)
    {
        m_stream.write(static_cast<const char*>(buffer), bytesRead);
        return S_OK;
    }

    void OnResponseComplete(HRESULT result)
    {
        m_stream.close();
        Close();
        m_result = S_OK == result;
    }

    virtual HRESULT OnCallback(DWORD code,
        const void* info,
        DWORD length)
    {
        switch (code)
        {
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            if (!::WinHttpReceiveResponse(m_handle, 0))
            {
                return HRESULT_FROM_WIN32(::GetLastError());
            }

            break;
        }

        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(DWORD);
            if (!::WinHttpQueryHeaders(m_handle,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &statusCode,
                &statusCodeSize,
                WINHTTP_NO_HEADER_INDEX))
            {
                return HRESULT_FROM_WIN32(::GetLastError());
            }

            if (HTTP_STATUS_OK != statusCode)
            {
                return E_FAIL;
            }

            if (!::WinHttpReadData(m_handle,
                m_buffer.data(),
                m_buffer.size(),
                nullptr))
            {
                return HRESULT_FROM_WIN32(::GetLastError());
            }

            break;
        }

        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            if (length > 0)
            {
                OnReadComplete(m_buffer.data(),
                    length);

                if (!::WinHttpReadData(m_handle,
                    m_buffer.data(),
                    m_buffer.size(),
                    nullptr)) // async result
                {
                    return HRESULT_FROM_WIN32(::GetLastError());
                }
            }
            else
            {
                OnResponseComplete(S_OK);
            }

            break;
        }

        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            auto pAR = static_cast<WINHTTP_ASYNC_RESULT*>(const_cast<void *>(info));
            std::string result;
            switch (pAR->dwResult)
            {
                case API_RECEIVE_RESPONSE:
                    result = "API_RECEIVE_RESPONSE";
                    break;
                case API_QUERY_DATA_AVAILABLE:
                    result = "API_QUERY_DATA_AVAILABLE";
                    break;
                case API_READ_DATA:
                    result = "API_READ_DATA";
                    break;
                case API_WRITE_DATA:
                    result = "API_WRITE_DATA";
                    break;
                case API_SEND_REQUEST:
                    result = "API_SEND_REQUEST";
                    break;
                default:
                    result = "Unknown failure";
                    break;
            }

            printf("Error: %d, result: %s\n", pAR->dwError, result.c_str());

            return E_FAIL;
        }

        case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        {
            SetEvent(m_waitEvent);
            break;
        }

        default:
            break;
        }

        return S_OK;
    }

    bool WaitResult()
    {
        if (WAIT_OBJECT_0 != WaitForSingleObject(m_waitEvent, INFINITE))
            return false;

        return m_result;
    }

private:
    std::fstream m_stream;
};

TEST(TestRequest, TestGetSuccessfully)
{
    WinHttpSession session;
    WinHttpConnection connection;
    TestGetRequest testGetRequest;

    HRESULT res = E_FAIL;
    res = session.Initialize();
    ASSERT_EQ(S_OK, res);
    res = connection.Initialize(L"github.com", INTERNET_DEFAULT_HTTPS_PORT, session);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest.Initialize(nullptr, L"test.html", connection);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest.SendRequest(nullptr, 0, nullptr, 0, 0);
    EXPECT_TRUE(S_OK == res);
    auto success = testGetRequest.WaitResult();
    EXPECT_TRUE(success);
}

TEST(TestRequest, TestGetError)
{
    WinHttpSession session;
    WinHttpConnection connection;
    TestGetRequest testGetRequest;

    HRESULT res = E_FAIL;
    res = session.Initialize();
    ASSERT_EQ(S_OK, res);
    res = connection.Initialize(L"wrongandinaccessible.com", INTERNET_DEFAULT_HTTPS_PORT, session);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest.Initialize(nullptr, L"test_error.html", connection);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest.SendRequest(nullptr, 0, nullptr, 0, 0);
    EXPECT_TRUE(S_OK == res);
    auto success = testGetRequest.WaitResult();
    EXPECT_FALSE(success);
}
