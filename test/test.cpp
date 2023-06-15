#include "pch.h"

#include "WinHttpRequest.h"

#include <fstream>
#include <vector>
#include <format>

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

    virtual HRESULT Initialize(PCWSTR source, const std::wstring& destination, const WinHttpConnection& connection, DWORD flags = WINHTTP_FLAG_SECURE)
    {
        m_stream.open(destination, std::ios_base::out);

        return WinHttpRequest<TestGetRequest>::Initialize(source, 0, connection, nullptr, nullptr, nullptr, flags);
    }

    void Cleanup()
    {
        WinHttpSetStatusCallback(m_handle, nullptr, 0, 0);
        Close();
    }

    HRESULT OnReadComplete(const void* buffer, DWORD bytesRead)
    {
        m_stream.write(static_cast<const char*>(buffer), bytesRead);
        return S_OK;
    }

    void OnResponseComplete(HRESULT result)
    {
        m_stream.close();
        Cleanup();
        SetEvent(m_waitEvent);
        m_result = S_OK == result;
    }

    virtual HRESULT OnCallback(DWORD code, const void* info, DWORD length)
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
                    auto result = HRESULT_FROM_WIN32(::GetLastError());
                    SetEvent(m_waitEvent);
                    return result;
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
            auto pAR = static_cast<WINHTTP_ASYNC_RESULT*>(const_cast<void*>(info));
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

            return E_FAIL;
        }

        case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        {
            break;
        }

        default:
            break;
        }

        return S_OK;
    }

    bool WaitResult() const
    {
        if (WAIT_OBJECT_0 != WaitForSingleObject(m_waitEvent, INFINITE))
            return false;

        return m_result;
    }

private:
    std::fstream m_stream;
};

typedef struct Request
{
    WinHttpConnection connection;
    TestGetRequest testGetRequest;
    Request() {}
    Request(const Request& rhs) = delete;
    Request(Request&& rhs) noexcept
    {
        connection.m_handle = rhs.connection.m_handle;
        rhs.connection.m_handle = nullptr;
        testGetRequest.m_handle = rhs.testGetRequest.m_handle;
        rhs.testGetRequest.m_handle = nullptr;
    }
    void operator= (Request& rhs) = delete;
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

TEST(TestRequest, TestMoreGet)
{
    WinHttpSession session;
    WinHttpConnection connection1;
    WinHttpConnection connection2;
    TestGetRequest testGetRequest1;
    TestGetRequest testGetRequest2;

    HRESULT res = E_FAIL;
    res = session.Initialize();
    ASSERT_EQ(S_OK, res);
    res = connection1.Initialize(L"github.com", INTERNET_DEFAULT_HTTPS_PORT, session);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest1.Initialize(L"FarGroup/FarManager", L"test1.html", connection1);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest1.SendRequest(nullptr, 0, nullptr, 0, 0);
    EXPECT_TRUE(S_OK == res);

    res = connection2.Initialize(L"learn.microsoft.com", INTERNET_DEFAULT_HTTPS_PORT, session);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest2.Initialize(nullptr, L"test2.html", connection2);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest2.SendRequest(nullptr, 0, nullptr, 0, 0);
    EXPECT_TRUE(S_OK == res);

    auto success = testGetRequest1.WaitResult();
    EXPECT_TRUE(success);
    success = testGetRequest2.WaitResult();
    EXPECT_TRUE(success);
}

TEST(TestRequest, TestBatchGet)
{
    const std::vector<std::wstring> urls = { L"microsoft/vscode", L"microsoft/TypeScript",
        L"microsoft/PowerToys", L"microsoft/terminal", L"microsoft/semantic-kernel"
    };

    WinHttpSession session;
    HRESULT res = E_FAIL;
    res = session.Initialize();
    ASSERT_EQ(S_OK, res);
    std::vector<Request> requests(urls.size());
    for (size_t i = 0; i < urls.size(); i++)
    {
        auto& request = requests[i];
        request.connection.Initialize(L"github.com", INTERNET_DEFAULT_HTTPS_PORT, session);
        request.testGetRequest.Initialize(urls[i].c_str(), std::format(L"test_{}.html", i).c_str(), request.connection);
        res = request.testGetRequest.SendRequest(nullptr, 0, nullptr, 0, 0);
        EXPECT_TRUE(S_OK == res);
    }

    for (const auto& request : requests)
    {
        auto success = request.testGetRequest.WaitResult();
        EXPECT_TRUE(success);
    }
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

TEST(TestRequest, TestGetRequestHttpWithoutSecurity)
{
    WinHttpSession session;
    WinHttpConnection connection;
    TestGetRequest testGetRequest;

    HRESULT res = E_FAIL;
    res = session.Initialize();
    ASSERT_EQ(S_OK, res);
    res = connection.Initialize(L"www.bing.com", INTERNET_DEFAULT_HTTP_PORT, session);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest.Initialize(nullptr, L"test_bing.html", connection, 0);
    EXPECT_TRUE(S_OK == res);
    res = testGetRequest.SendRequest(nullptr, 0, nullptr, 0, 0);
    EXPECT_TRUE(S_OK == res);
    auto success = testGetRequest.WaitResult();
    EXPECT_TRUE(success);
}
