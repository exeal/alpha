// WinThread.h
/////////////////////////////////////////////////////////////////////////////

#ifndef WIN_THREAD_H_
#define WIN_THREAD_H_

#include "Window.h"


// Thread class definition
/////////////////////////////////////////////////////////////////////////////

namespace Manah {
namespace Windows {

class Thread : public SelfAssertable {
	// constructors
public:
	Thread();
	virtual ~Thread();

	// methods
public:
	void	beginWaitCursor();
	void	endWaitCursor();
	void	restoreWaitCursor();

	bool			createThread(DWORD flags = 0, UINT stackSize = 0, LPSECURITY_ATTRIBUTES securityAttrs = 0);
	virtual Window*	getMainWindow() const;
	int				getThreadPriority();
	bool			setThreadPriority(int priority) const;
	bool			postThreadMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	DWORD			resumeThread();
	DWORD			suspendThread();

	virtual int		exitInstance();
	virtual bool	initInstance();
	virtual bool	onIdle(long count);
	virtual bool	preTranslateMessage(MSG* pMsg);
	virtual bool	isIdelMessage(MSG* pMsg);
	virtual LRESULT	processWndProcException(std::exception& e, const MSG* pMSG);
	virtual bool	processMessageFilter(int code, MSG* pMSG);
	virtual int		run();

	// data members
private:
	long				m_cWaitCursor;
	HCURSOR				m_hOriginalCursor;

	HANDLE				m_hThread;
	CWindow*			m_pMainWindow;
};


// CWinThread class implementation
/////////////////////////////////////////////////////////////////////////////

CWinThread::CWinThread() : m_cWaitCursor(0), m_hOriginalCursor(0), m_pMainWindow(0) {
}

CWinThread::~CWinThread() {
}

void CWinThread::BeginWaitCursor() {
	if(m_cWaitCursor == 0) {
		m_cWaitCursor++;
		m_hOriginalCursor = ::GetCursor();
		::SetCursor(::LoadCursor(0, IDC_WAIT));
	}
}

void CWinThread::EndWaitCursor() {
	--m_cWaitCursor;
	if(m_cWaitCursor <= 0) {
		m_cWaitCursor = 0;
		if(m_hOriginalCursor != 0)
			::SetCursor(m_hOriginalCursor);
		m_hOriginalCursor = 0;
	}
}

void CWinThread::RestoreWaitCursor() {
	m_cWaitCursor = 0;
	if(m_hOriginalCursor != 0)
		::SetCursor(m_hOriginalCursor);
	m_hOriginalCursor = 0;
}

CWindow* CWinThread::GetMainWindow() const {
	return m_pMainWindow;
}

int CWinThread::GetThreadPriority() const {
	return ::GetThreadPriority(m_hThread);
}

}; /* namespace Manah */

#endif /* WIN_THREAD_H_ */

/*[EOF]*/