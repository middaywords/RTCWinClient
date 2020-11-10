#pragma once

#include "DuiLib/UIlib.h"

namespace DuiLib {
	
	class CVideoView :public CControlUI
	{
		CVideoView() : m_hWnd(NULL) {
		}

		virtual void SetInternVisible(bool bVisible = true) {
			__super::SetInternVisible(bVisible);
			::ShowWindow(m_hWnd, bVisible);
		}

		virtual void SetPos(RECT rc) {
			__super::SetPos(rc);
			::SetWindowPos(m_hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
				SWP_NOZORDER | SWP_NOACTIVATE);
		}

		BOOL Attach(HWND hWndNew) {
			if (!::IsWindow(hWndNew)) {
				return FALSE;
			}
			m_hWnd = hWndNew;
			return TRUE;
		}

		HWND Detach() {
			HWND hWnd = m_hWnd;
			m_hWnd = NULL;
			return hWnd;
		}

		HWND GetHwnd() {
			HWND hWnd = m_hWnd;
			return hWnd;
		}
	
	protected:
		HWND m_hWnd;
	};
}
