/* WinInetGet.cpp
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinInet.h>
#pragma comment(lib, "WinInet.lib")
#include <stdio.h>
#include <malloc.h>

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Usage:\n\tWinInetGet server object\ne.g.\n\tWinInetGet slashdot.org /\n");
		return 1;
	}

	LPCTSTR lpszServerName = argv[1];
	LPCTSTR lpszObjectName = argv[2];

	LPCTSTR lpszAgent = "WinInetGet/0.1";
	HINTERNET hInternet = InternetOpen(lpszAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hInternet)
	{
		fprintf(stderr, "InternetOpen failed, error = %d (0x%x)\n", GetLastError(), GetLastError());
		return 1;
	}

	INTERNET_PORT nServerPort = INTERNET_DEFAULT_HTTP_PORT;
	LPCTSTR lpszUserName = NULL;
	LPCTSTR lpszPassword = NULL;
	DWORD dwConnectFlags = 0;
	DWORD dwConnectContext = 0;
	HINTERNET hConnect = InternetConnect(hInternet, lpszServerName, nServerPort,
											lpszUserName, lpszPassword,
											INTERNET_SERVICE_HTTP,
											dwConnectFlags, dwConnectContext);
	if (!hConnect)
	{
		InternetCloseHandle(hInternet);
		fprintf(stderr, "InternetConnect failed, error = %d (0x%x)\n", GetLastError(), GetLastError());
		return 1;
	}

	LPCTSTR lpszVerb = "GET";
	LPCTSTR lpszVersion = NULL;			// Use default.
	LPCTSTR lpszReferrer = NULL;		// No referrer.
	LPCTSTR *lplpszAcceptTypes = NULL;	// Whatever the server wants to give us.
	DWORD dwOpenRequestFlags = INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
								INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS | 
								INTERNET_FLAG_KEEP_CONNECTION |
								INTERNET_FLAG_NO_AUTH |
								INTERNET_FLAG_NO_AUTO_REDIRECT |
								INTERNET_FLAG_NO_COOKIES |
								INTERNET_FLAG_NO_UI |
								INTERNET_FLAG_RELOAD;
	DWORD dwOpenRequestContext = 0;
	HINTERNET hRequest = HttpOpenRequest(hConnect, lpszVerb, lpszObjectName, lpszVersion,
											lpszReferrer, lplpszAcceptTypes,
											dwOpenRequestFlags, dwOpenRequestContext);
	if (!hRequest)
	{
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		fprintf(stderr, "HttpOpenRequest failed, error = %d (0x%x)\n", GetLastError(), GetLastError());
		return 1;
	}

	BOOL bResult = HttpSendRequest(hRequest, NULL, 0, NULL, 0);
	if (!bResult)
	{
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		fprintf(stderr, "HttpSendRequest failed, error = %d (0x%x)\n", GetLastError(), GetLastError());
		return 1;
	}

	DWORD dwInfoLevel = HTTP_QUERY_RAW_HEADERS_CRLF;
	DWORD dwInfoBufferLength = 10;
	BYTE *pInfoBuffer = (BYTE *)malloc(dwInfoBufferLength+1);
	while (!HttpQueryInfo(hRequest, dwInfoLevel, pInfoBuffer, &dwInfoBufferLength, NULL))
	{
		DWORD dwError = GetLastError();
		if (dwError == ERROR_INSUFFICIENT_BUFFER)
		{
			free(pInfoBuffer);
			pInfoBuffer = (BYTE *)malloc(dwInfoBufferLength+1);
		}
		else
		{
			fprintf(stderr, "HttpQueryInfo failed, error = %d (0x%x)\n", GetLastError(), GetLastError());
			break;
		}
	}

	pInfoBuffer[dwInfoBufferLength] = '\0';
	printf("%s", pInfoBuffer);
	free(pInfoBuffer);

	DWORD dwBytesAvailable;
	while (InternetQueryDataAvailable(hRequest, &dwBytesAvailable, 0, 0))
	{
		BYTE *pMessageBody = (BYTE *)malloc(dwBytesAvailable+1);

		DWORD dwBytesRead;
		BOOL bResult = InternetReadFile(hRequest, pMessageBody, dwBytesAvailable, &dwBytesRead);
		if (!bResult)
		{
			fprintf(stderr, "InternetReadFile failed, error = %d (0x%x)\n", GetLastError(), GetLastError());
			break;
		}

		if (dwBytesRead == 0)
			break;	// End of File.

		pMessageBody[dwBytesRead] = '\0';
		printf("%s", pMessageBody);
		free(pMessageBody);
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return 0;
}
