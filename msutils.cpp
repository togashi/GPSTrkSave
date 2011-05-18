#include "stdafx.h"
#include "msutils.h"

HWND FindMapSource(void) {
	CString wndClassName;
	wndClassName.LoadString(IDS_WNDCLASSNAME);
	return ::FindWindow(_T("{7A96B96B-E756-4e42-8274-54CBF24F7944}"), NULL);
}

HWND WaitForPopup(HWND hWndParent, DWORD dwTimeout = 3000) {
	HWND hWnd;
	DWORD dwLimit = ::GetTickCount() + dwTimeout;
	do {
		hWnd = ::GetLastActivePopup(hWndParent);
		CString wt;
		{
			CWindow wnd;
			wnd.Attach(hWnd);
			wnd.GetWindowText(wt);
		}
		ATLTRACE2("WaitForPopup - 0x%8.8x: 0x%8.8x (%s)\r\n", hWndParent, hWnd, CT2A(wt));
		if (hWnd != hWndParent) {
			return hWnd;
		}
		::Sleep(100);
	} while (dwLimit > ::GetTickCount());
	return NULL;
}

HWND WaitForAnotherPopup(HWND hWndParent, HWND hWndPrevPopup, DWORD dwTimeout = 3000) {
	HWND hWnd;
	DWORD dwLimit = ::GetTickCount() + dwTimeout;
	do {
		hWnd = ::GetLastActivePopup(hWndParent);
		ATLTRACE2("WaitForAnotherPopup - 0x%8.8x, 0x%8.8x: 0x%8.8x\r\n", hWndParent, hWndPrevPopup, hWnd);
		if (hWnd != hWndParent && hWnd != hWndPrevPopup) {
			return hWnd;
		}
		::Sleep(100);
	} while (dwLimit > ::GetTickCount());
	return NULL;
}

BOOL WaitForPopupEnds(HWND hWndParent, DWORD dwTimeout = 3000) {
	HWND hWnd;
	DWORD dwLimit = ::GetTickCount() + dwTimeout;
	do {
		hWnd = ::GetLastActivePopup(hWndParent);
		if (hWnd == hWndParent) {
			return TRUE;
		}
		::Sleep(100);
	} while (dwLimit > ::GetTickCount());
	return FALSE;
}

BOOL MapSourceNewDocument(HWND hWndMS) {
	::PostMessage(hWndMS, WM_COMMAND, MAKEWPARAM(57600, 0), 0);
	HWND hWndDlg = WaitForPopup(hWndMS);
	if (hWndDlg != NULL) {
		::PostMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(IDNO, 0), 0);
	}
	return TRUE;
}

HWND PopTransferDialog(HWND hWndMS) {
	::PostMessage(hWndMS, WM_COMMAND, MAKEWPARAM(32897, 0), 0);
	HWND hWndDlg = WaitForPopup(hWndMS);
	if (hWndDlg == NULL) {
		return NULL;
	}
	
	CString ctlCaption;
	ctlCaption.LoadString(IDS_NEVERSHOWCHECKBOXCAPTION);
	HWND hWndCheck = ::FindWindowEx(hWndDlg, NULL, _T("Button"), ctlCaption);
	if (hWndCheck != NULL) {
		::PostMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
		return NULL;
	}
	
	DWORD dwPID;
	::GetWindowThreadProcessId(hWndMS, &dwPID);
	HANDLE hP = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	::WaitForInputIdle(hP, 3000);
	::CloseHandle(hP);
	
	return hWndDlg;
}



BOOL PrepareTransferDialog(HWND hWndDlg, LPCTSTR lpszDeviceName) {
	
	// デバイスサブダイアログ検索
	{
		HWND hWndSub;
		hWndSub = ::FindWindowEx(hWndDlg, NULL, (LPCTSTR)32770, NULL);
		if (hWndSub == NULL) {
			return FALSE;
		}
	
		// デバイスチェック
		HWND hWndCB = ::GetDlgItem(hWndSub, 8414);
		if (hWndCB == NULL) {
			return FALSE;
		}
		
		{
			CComboBox cb;
			cb.Attach(hWndCB);
			int index = cb.FindString(-1, lpszDeviceName);
			if (index >= 0) {
				cb.SetCurSel(index);
			} else {
				// デバイスが認識されていない
				return FALSE;
			}
		}
	}

	{
		CButton chk;
		// マップをOFF
		chk = ::GetDlgItem(hWndDlg, 1076);
		if (!chk.IsWindow()) {
			return FALSE;
		}
		chk.SetCheck(BST_UNCHECKED);
		
		// ウェイポイントをOFF
		chk = ::GetDlgItem(hWndDlg, 1080);
		if (!chk.IsWindow()) {
			return FALSE;
		}
		chk.SetCheck(BST_UNCHECKED);
		
		// ルートをOFF
		chk = ::GetDlgItem(hWndDlg, 1078);
		if (!chk.IsWindow()) {
			return FALSE;
		}
		chk.SetCheck(BST_UNCHECKED);
		
		// トラックをON
		chk = ::GetDlgItem(hWndDlg, 1079);
		if (!chk.IsWindow()) {
			return FALSE;
		}
		chk.SetCheck(BST_CHECKED);
	}

	return TRUE;
}

BOOL WaitForTransferEnds(HWND hWndMS, HWND hWndDlg) {
	
	CString dlgTitle;
	dlgTitle.LoadString(IDS_COMPLETEDLGTITLE);
	
	// 制限時間120秒
	DWORD dwLimit = ::GetTickCount() + 120000;
	do {
		HWND hWndTmp = ::GetLastActivePopup(hWndMS);
		if (hWndTmp == hWndMS) {
			// ダイアログ消えた
			return FALSE;
		} else if (hWndTmp != hWndDlg) {
			// 他のダイアログ出た
			return FALSE;
		} else {
			CString wt;
			CWindow wnd(hWndDlg);
			wnd.GetWindowText(wt);
			if (wt == dlgTitle) {
				wnd.PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
				WaitForPopupEnds(hWndMS);
				return TRUE;
			}
		}
		::Sleep(500);
	} while (dwLimit > ::GetTickCount());
	return FALSE;
}

BOOL ExecTransfer(HWND hWndMS, HWND hWndDlg) {
	CButton recv = ::GetDlgItem(hWndDlg, 1);
	if (!recv.IsWindow()) {
		return FALSE;
	}
	::PostMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(1, BN_CLICKED), reinterpret_cast<LPARAM>(recv.m_hWnd));
	
	ATLTRACE2("転送ダイアログを待つ\r\n");
	HWND hWndTmp = WaitForAnotherPopup(hWndMS, hWndDlg);
	if (hWndTmp == NULL) {
		return FALSE;
	}
	CButton chk = ::GetDlgItem(hWndTmp, 5851);
	if (chk.IsWindow()) {
		chk.SetCheck(BST_UNCHECKED);
	} else {
		// 想定外のダイアログ表示
		// 異常事態発生の可能性アリ
		return FALSE;
	}
	
	// 転送終了を待つ
	ATLTRACE2("転送完了ダイアログを待つ\r\n");
	if (!WaitForTransferEnds(hWndMS, hWndTmp)) {
		return FALSE;
	}
	
	if (!WaitForPopupEnds(hWndMS)) {
		// 想定外のダイアログが残っている
		return FALSE;
	}

	return TRUE;
}

BOOL MapSourceSaveDocument(HWND hWndMS, LPCTSTR lpszFilename) {
	::PostMessage(hWndMS, WM_COMMAND, MAKEWPARAM(57604, 0), 0);
	HWND hWndDlg = WaitForPopup(hWndMS);
	if (hWndDlg == NULL) {
		return FALSE;
	}
	::Sleep(500);
	
	CWindow wnd(hWndDlg);
	CString txt;
	wnd.GetWindowText(txt);
	CString dlgTitle;
	dlgTitle.LoadString(IDS_SAVEDLGTITLE);
	if (txt != dlgTitle) {
		return FALSE;
	}
	
	// ファイル名を指定
	HWND hWndCBX = ::GetDlgItem(hWndDlg, 0x47C);
	if (hWndCBX != NULL) {
		CComboBoxEx cbx(hWndCBX);
		::SendMessage(hWndCBX, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(lpszFilename));
	} else {
		DWORD tiDlg = ::GetWindowThreadProcessId(hWndDlg, NULL);
		DWORD tiCur = ::GetCurrentThreadId();
		::AttachThreadInput(tiCur, tiDlg, TRUE);
		CEdit edit(::GetFocus());
		edit.SendMessage(WM_SETTEXT, 0, reinterpret_cast<LPARAM>(lpszFilename));
		::AttachThreadInput(tiCur, tiDlg, FALSE);
	}
	
	wnd.PostMessage(WM_COMMAND, MAKEWPARAM(1, 0), 0);
	
	WaitForPopupEnds(hWndMS);

	return TRUE;
}

BOOL AutoSaveTrack(void) {
	// マップソース検知
	HWND hWndMS = FindMapSource();
	if (hWndMS == NULL) {
		return FALSE;
	}
	// 新しいドキュメント
	MapSourceNewDocument(hWndMS);
	
	// 転送ダイアログセットアップ
	HWND hWndDlg = PopTransferDialog(hWndMS);
	CString devName;
	devName.LoadString(IDS_DEVICENAME);
	if (!PrepareTransferDialog(hWndDlg, devName)) {
		::Sleep(1000);
		hWndDlg = ::GetLastActivePopup(hWndMS);
		if (!PrepareTransferDialog(hWndDlg, devName)) {
			return FALSE;
		}
	}

	// 転送
	if (!ExecTransfer(hWndMS, hWndDlg)) {
		return FALSE;
	}

	// 保存
	CString tmp;
	tmp.LoadString(IDS_SAVEFILENAMEFORMAT);
	CString fn;
	::ExpandEnvironmentStrings(tmp, fn.GetBufferSetLength(MAX_PATH), MAX_PATH);
	fn.ReleaseBuffer();
	
	CTime t;
	t = CTime::GetCurrentTime();
	fn.AppendFormat(_T("%s.gdb"), t.Format(_T("%Y%m%d_%H%M%S")));

	return MapSourceSaveDocument(hWndMS, fn);
}

BOOL MapSourcePrepare(void)
{
	if (FindMapSource() == NULL) {
		CString exe;
		exe.LoadString(IDS_MAPSOURCEPATH);
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		::GetStartupInfo(&si);
		if (::CreateProcess(NULL, exe.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			::WaitForInputIdle(pi.hProcess, INFINITE);
			::CloseHandle(pi.hProcess);
			::CloseHandle(pi.hThread);
			// マップソースの準備完了まで30秒
			DWORD dwLimit = ::GetTickCount() + 30000;
			HWND hWnd = NULL;
			do {
				::Sleep(500);
				hWnd = FindMapSource();
			} while (hWnd == NULL && dwLimit > ::GetTickCount());
			return hWnd != NULL;
		} else {
			return FALSE;
		}
	}
	return TRUE;
}

class TEST_ {
public:
	TEST_() {
		if (MapSourcePrepare()) {
			if (!AutoSaveTrack()) {
				HWND hWnd = FindMapSource();
				if (hWnd != NULL && IsWindow(hWnd)) {
					DWORD dwPID;
					::GetWindowThreadProcessId(hWnd, &dwPID);
					HANDLE hP = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
					::TerminateProcess(hP, 0);
				}
			}
		}
	}
};
static TEST_ Test;
