#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"gdiplus")
#pragma comment(lib,"shlwapi")

#include <windows.h>
#include <shlwapi.h>
#include <gdiplus.h>

using namespace Gdiplus;

TCHAR szClassName[] = TEXT("Window");

BOOL IsImageFile(LPCTSTR lpszFilePath, LPCTSTR lpszExtList)
{
	DWORD dwSize = lstrlen(lpszExtList);
	LPTSTR lpszExtList2 = (LPTSTR)GlobalAlloc(0, (dwSize + 1) * sizeof(TCHAR));
	lstrcpy(lpszExtList2, lpszExtList);
	LPCTSTR seps = TEXT(";");
	TCHAR *next;
	LPTSTR token = wcstok_s(lpszExtList2, seps, &next);
	while (token != NULL)
	{
		if (PathMatchSpec(lpszFilePath, token))
		{
			GlobalFree(lpszExtList2);
			return TRUE;
		}
		token = wcstok_s(0, seps, &next);
	}
	GlobalFree(lpszExtList2);
	return FALSE;
}

VOID CountUp(HWND hList, LPCTSTR lpInputPath, LPCTSTR lpszExtList, BOOL bSubfolder)
{
	TCHAR szFullPattern[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile;
	PathCombine(szFullPattern, lpInputPath, TEXT("*"));
	hFindFile = FindFirstFile(szFullPattern, &FindFileData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if ( bSubfolder && 
					lstrcmp(FindFileData.cFileName, TEXT("..")) != 0 &&
					lstrcmp(FindFileData.cFileName, TEXT(".")) != 0)
				{
					PathCombine(szFullPattern, lpInputPath, FindFileData.cFileName);
					CountUp(hList, szFullPattern, lpszExtList, bSubfolder);
				}
			}
			else
			{
				PathCombine(szFullPattern, lpInputPath, FindFileData.cFileName);
				if (IsImageFile(szFullPattern, lpszExtList))
				{
					SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)szFullPattern);
				}
			}
		} while (FindNextFile(hFindFile, &FindFileData));
		FindClose(hFindFile);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hCheck;
	static HWND hList;
	switch (msg)
	{
	case WM_CREATE:
		hCheck = CreateWindow(TEXT("BUTTON"), TEXT("サブフォルダも対象"), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hList = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), 0, WS_VISIBLE | WS_CHILD | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_SIZE:
		MoveWindow(hCheck, 10, 10, LOWORD(lParam) - 20, 32, TRUE);
		MoveWindow(hList, 10, 50, LOWORD(lParam) - 20, HIWORD(lParam) - 60, TRUE);
		break;
	case WM_DROPFILES:
		{
			TCHAR szTmp[MAX_PATH];
			const UINT iFileNum = DragQueryFile((HDROP)wParam, -1, NULL, 0);
			BOOL bSubfolder = (BOOL)SendMessage(hCheck, BM_GETCHECK, 0, 0);
			LPCTSTR lpszExtList = TEXT("*.png;*.jpg;");
			for (UINT i = 0; i<iFileNum; ++i)
			{
				DragQueryFile((HDROP)wParam, i, szTmp, MAX_PATH);
				if (PathIsDirectory(szTmp))
				{
					CountUp(hList, szTmp, lpszExtList, bSubfolder);
				}
				else if (IsImageFile(szTmp, lpszExtList))
				{
					SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)szTmp);
				}
			}
			DragFinish((HDROP)wParam);
			const int nMaxCount = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
			for (int i = nMaxCount - 1; i >= 0; --i)
			{
				SendMessage(hList, LB_GETTEXT, i, (LPARAM)szTmp);
				{
					Bitmap *pBitmap = Bitmap::FromFile(szTmp);
					if (pBitmap)
					{
						SendMessage(hList, LB_DELETESTRING, i, 0);
						delete pBitmap;
					}
				}
			}
			MessageBox(hWnd, TEXT("リストアップが完了しました。"), TEXT("確認"), 0);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("ドロップされたフォルダに入っているイメージがGdi+でロードできるかチェックする（ロードできないものをリストアップする）"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}
