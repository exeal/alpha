// splitter.hpp
// (c) 2005-2007 exeal

#ifndef MANAH_SPLITTER_HPP
#define MANAH_SPLITTER_HPP
#include "window.hpp"

namespace manah {
namespace windows {
namespace ui {

class AbstractPane {
public:
	virtual ~AbstractPane() throw() {}
	virtual HWND getWindow() const throw() = 0;	// provides pane's window handle
};

struct SplitterBase {
	enum ChildrenDestructionPolicy {
		STANDARD_DELETE,					// standard delete
		DONT_DELETE,						// does nothing about the panes
		DONT_DELETE_AND_SET_PARENT_TO_NULL	// calls SetParent(pane.GetWindow(), 0) when the root window is destroyed
	};
	enum Direction {
		NO_SPLIT, NS, WE
	};
	enum PanePosition {
		LEFT = 0, TOP = LEFT, RIGHT = 1, BOTTOM = RIGHT
	};
};

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp> class Splitter;

namespace internal {
	template<class Pane /* implements AbstractPane */, SplitterBase::ChildrenDestructionPolicy cdp>
	class SplitterItem : public Noncopyable, public SplitterBase {
	public:
		enum PaneType {PANETYPE_EMPTY, PANETYPE_SINGLE, PANETYPE_SPLITTER};
		SplitterItem() throw() : parent_(0), direction_(NO_SPLIT), firstPaneSizeRatio_(0.5F) {}
		~SplitterItem() throw();
		void						adjustPanes(const ::RECT& newRect, uint frameWidth, uint frameHeight);
		void						draw(gdi::PaintDC& dc, uint frameWidth, uint frameHeight);
		void						getFirstPane(bool leftTop, Pane*& pane, SplitterItem<Pane, cdp>** parent = 0) const;
		bool						getNextPane(const Pane& pane, bool next, Pane*& nextPane, SplitterItem<Pane, cdp>** parent = 0) const;
		SplitterItem<Pane, cdp>*	getParent() const throw() {return parent_;}
		SplitterItem<Pane, cdp>*	hitTest(const ::POINT& pt) const;
		void						sendMessageToChildren(UINT message);
	private:
		struct Child {
			PaneType type;
			union {
				Pane* pane;
				SplitterItem<Pane, cdp>* splitter;
			} body;
			Child() throw() : type(PANETYPE_EMPTY) {}
			Child(Pane& pane) throw() : type(PANETYPE_SINGLE) {body.pane = &pane;}
		} children_[2];						// children ([RIGHT] is EMPTY if this pane is not split)
		SplitterItem<Pane, cdp>* parent_;	// parent splitter
		Direction direction_;				// NO_SPLIT for only root
		::RECT rect_;
		double firstPaneSizeRatio_;			// (left or top pane's size) / (whole splitter size)
		friend Splitter<Pane, cdp>;
	};
} // namespace internal

template<class Pane /* implements AbstractPane*/, SplitterBase::ChildrenDestructionPolicy cdp = SplitterBase::STANDARD_DELETE>
/* final */ class Splitter : public CustomControl<Splitter>, public Noncopyable, public SplitterBase {
public:
	class Iterator : public Unassignable {	// once the structure is changed, iterator is inavailable
	public:
		Iterator(const Iterator& rhs) throw() : parent_(rhs.parent_), pane_(rhs.pane_) {}
		Pane&	get() const {if(pane_ == 0) throw std::logic_error(""); return *pane_;}
		bool	isEnd() const throw() {return pane_ == 0;}
		void	next() throw();
		void	reset() throw();
	private:
		typedef internal::SplitterItem<Pane, cdp> SplitterItem;
		explicit Iterator(const SplitterItem& root) throw() : parent_(&root) {reset();}
		Iterator(const SplitterItem& parent, Pane& pane) throw() : parent_(&parent), pane_(&pane) {}
		const SplitterItem* parent_;
		Pane* pane_;
		friend Splitter<Pane, cdp>;
	};

	DEFINE_WINDOW_CLASS() {
		name = _T("manah:splitter-root");
		bgColor = COLOR_BTNFACE;
		style = CS_DBLCLKS;
	}

public:
	Splitter() throw() : defaultActivePane_(0),
			frameWidth_(::GetSystemMetrics(SM_CXSIZEFRAME)), frameHeight_(::GetSystemMetrics(SM_CYSIZEFRAME)),
			minimumPaneWidth_(::GetSystemMetrics(SM_CXMIN)), minimumPaneHeight_(::GetSystemMetrics(SM_CYMIN)),
			draggingSplitter_(0) {}
public:
	void		activateNextPane() {doActivateNextPane(true);}
	void		activatePreviousPane() {doActivateNextPane(false);}
	void		adjustPanes() throw();
	bool		create(HWND parent, const ::RECT& rect, DWORD style, DWORD exStyle, Pane& initialPane) throw();
	Iterator	enumeratePanes() const throw() {return Iterator(root_);}
	Pane&		getActivePane() const;
	uint		getSplitterSize(uint& width, uint& height) const throw() {width = splitterWidth_; height = splitterHeight_;}
	bool		isSplit(const Pane& pane) const;
	void		removeActivePane() {unsplit(getActivePane());}
	void		removeInactivePanes();
	void		setDefaultActivePane(Pane& pane);
	void		setPaneMinimumSize(uint width, uint height) throw() {minimumPaneWidth_ = width; minimumPaneHeight_ = height;}
	void		setSplitterSize(uint width, uint height);
	void		splitNS(Pane& pane, Pane& clone) {split(pane, clone, true);}
	void		splitWE(Pane& pane, Pane& clone) {split(pane, clone, false);}
	void		unsplit(Pane& pane);
protected:
	LRESULT	dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam);
	void	onCaptureChanged(HWND newWindow);
	void	onDestroy();
	void	onLButtonDown(UINT, const ::POINT& pt);
	void	onLButtonDblClk(UINT, const ::POINT& pt);
	void	onLButtonUp(UINT, const ::POINT&) {releaseCapture();}
	void	onMouseMove(UINT, const ::POINT& pt);
	void	onPaint(gdi::PaintDC& dc) {root_.draw(dc, frameWidth_, frameHeight_);}
	bool	onSetCursor(HWND window, UINT hitTest, UINT);
	void	onSetFocus(HWND);
	void	onSize(UINT, int cx, int cy);
private:
	typedef internal::SplitterItem<Pane, cdp>	SplitterItem;
	void					doActivateNextPane(bool next);
	void					drawSizingSplitterXorBar();
	static SplitterItem*	findPane(const SplitterItem& splitter, const Pane& pane);	// recursive method which returns pane's parent
	void					split(Pane& pane, Pane& clone, bool ns);
private:
	SplitterItem root_;
	Pane* defaultActivePane_;
	uint frameWidth_, frameHeight_;
	uint minimumPaneWidth_, minimumPaneHeight_;
	SplitterItem* draggingSplitter_;	// sizing splitter
	uint sizingFirstPaneSize_;	// first pane size of m_pDraggingSplitter (-1 when full dragging is enabled)
};


// implementation ///////////////////////////////////////////////////////////

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::adjustPanes() throw() {
	::RECT rect;
	getWindowRect(rect);
	::OffsetRect(&rect, -rect.left, -rect.top);
	root_.adjustPanes(rect, frameWidth_, frameHeight_);
	invalidateRect(0);
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline bool Splitter<Pane, cdp>::create(HWND parent, const RECT& rect, DWORD style, DWORD exStyle, Pane& initialPane) throw() {
	assert(parent == 0 || toBoolean(::IsWindow(parent)));
	if(!CustomControl<Splitter>::create(parent, rect, 0, style, exStyle))
		return false;
	::SetParent(static_cast<AbstractPane&>(initialPane).getWindow(), get());
	root_.children_[LEFT].type = SplitterItem::PANETYPE_SINGLE;
	root_.children_[LEFT].body.pane = defaultActivePane_ = &initialPane;
	return true;
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline LRESULT Splitter<Pane, cdp>::dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	if(message == WM_CAPTURECHANGED)
		onCaptureChanged(reinterpret_cast<HWND>(lParam));
	return CustomControl<Splitter<Pane, cdp> >::dispatchEvent(message, wParam, lParam);
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::doActivateNextPane(bool next) {
	Pane& activePane = getActivePane();
	if(SplitterItem* parent = findPane(root_, activePane)) {
		Pane* nextPane = 0;
		if(parent->getNextPane(activePane, next, nextPane))
			defaultActivePane_ = nextPane;
		else	// current active pane is end/first
			root_.getFirstPane(next, defaultActivePane_);
		if(HWND window = static_cast<AbstractPane*>(defaultActivePane_)->getWindow())
			::SetFocus(window);
	}
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::drawSizingSplitterXorBar() {
	::RECT rect;
	const ::RECT& splitterRect = draggingSplitter_->rect_;
	manah::windows::gdi::WindowDC dc = getWindowDC();

	// create half tone brush (from MFC)
	ushort grayPattern[8];
	for(int i = 0; i < countof(grayPattern); ++i)
		grayPattern[i] = static_cast<ushort>(0x5555 << (i & 1));
	HBITMAP bitmap = ::CreateBitmap(countof(grayPattern), countof(grayPattern), 1, 1, grayPattern);
	HBRUSH brush = ::CreatePatternBrush(bitmap), oldBrush;

	assert(draggingSplitter_ != 0 && sizingFirstPaneSize_ != -1);
	if(draggingSplitter_->direction_ == NS)
		::SetRect(&rect, splitterRect.left, splitterRect.top + sizingFirstPaneSize_,
			splitterRect.right, splitterRect.top + sizingFirstPaneSize_ + frameHeight_);
	else {
		assert(draggingSplitter_->direction_ == WE);
		::SetRect(&rect, splitterRect.left + sizingFirstPaneSize_, splitterRect.top,
			splitterRect.left + sizingFirstPaneSize_ + frameWidth_, splitterRect.bottom);
	}
	oldBrush = dc.selectObject(brush);
	dc.patBlt(rect, PATINVERT);
	dc.selectObject(oldBrush);
	::DeleteObject(brush);
	::DeleteObject(grayPattern);
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline internal::SplitterItem<Pane, cdp>* Splitter<Pane, cdp>::findPane(const SplitterItem& splitter, const Pane& pane) {
	const SplitterItem::Child& leftTop = splitter.children_[LEFT];
	const SplitterItem::Child& rightBottom = splitter.children_[RIGHT];

	if(leftTop.type == SplitterItem::PANETYPE_EMPTY)
		return 0;
	else if(leftTop.type == SplitterItem::PANETYPE_SINGLE && leftTop.body.pane == &pane)
		return const_cast<SplitterItem*>(&splitter);
	else if(leftTop.type == SplitterItem::PANETYPE_SPLITTER) {
		if(SplitterItem* p = findPane(*leftTop.body.splitter, pane))
			return p;
	}

	if(rightBottom.type == SplitterItem::PANETYPE_EMPTY)
		return 0;
	else if(rightBottom.type == SplitterItem::PANETYPE_SINGLE && rightBottom.body.pane == &pane)
		return const_cast<SplitterItem*>(&splitter);
	else if(rightBottom.type == SplitterItem::PANETYPE_SPLITTER) {
		if(SplitterItem* p = findPane(*rightBottom.body.splitter, pane))
			return p;
	}

	return 0;
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline Pane& Splitter<Pane, cdp>::getActivePane() const {
	HWND focused = ::GetFocus();
	for(Iterator it = enumeratePanes(); !it.isEnd(); it.next()) {
		if(static_cast<AbstractPane&>(it.get()).getWindow() == focused)
			return it.get();
	}
	if(defaultActivePane_ != 0)
		return *defaultActivePane_;
	else
		throw std::logic_error("There are no panes.");
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline bool Splitter<Pane, cdp>::isSplit(const Pane& pane) const {
	if(SplitterItem* parent = findPane(root_, pane))
		return parent->direction_ != NO_SPLIT;
	else
		throw std::invalid_argument("The specified pane does not belong to this splitter.");
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::onCaptureChanged(HWND newWindow) {
	if(draggingSplitter_ != 0) {
		root_.sendMessageToChildren(WM_EXITSIZEMOVE);
		if(sizingFirstPaneSize_ != -1) {
			const RECT&	rect = draggingSplitter_->rect_;
			drawSizingSplitterXorBar();	// erase ghost bar
			draggingSplitter_->firstPaneSizeRatio_ =
				(draggingSplitter_->direction_ == NS) ?
				(sizingFirstPaneSize_ + frameHeight_ / 2) / static_cast<double>(rect.bottom - rect.top)
				: (sizingFirstPaneSize_ + frameWidth_ / 2) / static_cast<double>(rect.right - rect.left);
			draggingSplitter_->adjustPanes(rect, frameWidth_, frameHeight_);
		}
		draggingSplitter_ = 0;
	}
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::onDestroy() {
	if(cdp == DONT_DELETE_AND_SET_PARENT_TO_NULL) {
		for(Iterator it = enumeratePanes(); !it.isEnd(); it.next())
			::SetParent(static_cast<AbstractPane&>(it.get()).getWindow(), 0);
	}
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::onLButtonDown(UINT, const ::POINT& pt) {
	// begin sizing
	if(root_.direction_ == NO_SPLIT)
		return;
	else if(draggingSplitter_ = root_.hitTest(pt)) {
		const ::RECT& rect = draggingSplitter_->rect_;
		BOOL fullDraggingEnabled;

		::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &fullDraggingEnabled, 0);
		if(toBoolean(fullDraggingEnabled))
			sizingFirstPaneSize_ = -1;
		else if(draggingSplitter_->direction_ == NS) {
			sizingFirstPaneSize_ =
				static_cast<uint>((rect.bottom - rect.top) * draggingSplitter_->firstPaneSizeRatio_)
				- frameHeight_ / 2;
			drawSizingSplitterXorBar();
		} else {
			assert(draggingSplitter_->direction_ == WE);
			sizingFirstPaneSize_ =
				static_cast<uint>((rect.right - rect.left) * draggingSplitter_->firstPaneSizeRatio_)
				- frameWidth_ / 2;
			drawSizingSplitterXorBar();
		}
		setCapture();
		root_.sendMessageToChildren(WM_ENTERSIZEMOVE);
	}
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::onLButtonDblClk(UINT, const ::POINT& pt) {
	// double click splitter bar to unsplit
	if(root_.direction_ == NO_SPLIT)
		return;
	else if(SplitterItem* splitter = root_.hitTest(pt)) {
		if(splitter->children_[LEFT].type == SplitterItem::PANETYPE_SINGLE)
			unsplit(*splitter->children_[LEFT].body.pane);
		else if(splitter->children_[RIGHT].type == SplitterItem::PANETYPE_SINGLE)
			unsplit(*splitter->children_[RIGHT].body.pane);
	}
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::onMouseMove(UINT, const ::POINT& pt) {
	if(draggingSplitter_ == 0)
		return;
	const ::RECT& rect = draggingSplitter_->rect_;
	double ratio;

	if(draggingSplitter_->direction_ == NS) {
		if(static_cast<uint>(rect.bottom - rect.top) <= minimumPaneHeight_ * 2 + frameHeight_)	// too short
			ratio = 0.5F;
		else {
			long y = std::max(pt.y, rect.top + static_cast<long>(minimumPaneHeight_));
			y = std::min(y, rect.bottom - static_cast<long>(minimumPaneHeight_ + frameHeight_));
			ratio = (rect.top != rect.bottom) ?
				static_cast<double>(y - rect.top) / static_cast<double>(rect.bottom - rect.top) : 0.5F;
		}
	} else {
		assert(draggingSplitter_->direction_ == WE);
		if(static_cast<uint>(rect.right - rect.left) <= minimumPaneWidth_ * 2 + frameWidth_)	// too narrow
			ratio = 0.5F;
		else {
			long x = std::max(pt.x, rect.left + static_cast<long>(minimumPaneWidth_));
			x = std::min(x, rect.right - static_cast<long>(minimumPaneWidth_ + frameWidth_));
			ratio = (rect.left != rect.right) ?
				static_cast<double>(x - rect.left) / static_cast<double>(rect.right - rect.left) : 0.5F;
		}
	}

	if(sizingFirstPaneSize_ == -1) {	// dynamic sizing
		draggingSplitter_->firstPaneSizeRatio_ = ratio;
		draggingSplitter_->adjustPanes(rect, frameWidth_, frameHeight_);
		invalidateRect(0);
	} else {	// static sizing
		drawSizingSplitterXorBar();	// erase previous ghost
		sizingFirstPaneSize_ = (draggingSplitter_->direction_ == NS) ?
			static_cast<uint>((rect.bottom - rect.top) * ratio) : static_cast<uint>((rect.right - rect.left) * ratio);
		drawSizingSplitterXorBar();	// draw new ghost
	}
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline bool Splitter<Pane, cdp>::onSetCursor(HWND window, UINT hitTest, UINT) {
	if(window == get() && hitTest == HTCLIENT) {
		const ::POINT pt = getCursorPos();
		if(SplitterItem* splitter = root_.hitTest(pt)) {
			if(splitter->direction_ != NO_SPLIT) {
				::SetCursor(::LoadCursor(0, (splitter->direction_ == NS) ? IDC_SIZENS : IDC_SIZEWE));
				return true;
			}
		}
	}
	return false;
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::onSetFocus(HWND) {
	if(defaultActivePane_ != 0)
		::SetFocus(static_cast<AbstractPane*>(defaultActivePane_)->getWindow());
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::onSize(UINT, int cx, int cy) {
	HWND window = get(), parent = ::GetParent(get());
	do {
		if(toBoolean(::IsIconic(window)))	// ignore if the window is iconic
			return;
		window = parent;
		parent = ::GetParent(window);
	} while(parent != 0);

	::RECT rect = {0, 0, cx, cy};
	root_.adjustPanes(rect, frameWidth_, frameHeight_);
	invalidateRect(0);
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::removeInactivePanes() {
	Pane& activePane = getActivePane();
	SplitterItem* parent = findPane(root_, activePane);
	const bool hasFocus = isChild(::GetFocus());

	// prevent active pane from deletion
	assert(parent != 0);
	if(parent->children_[LEFT].type == SplitterItem::PANETYPE_SINGLE
			&& parent->children_[LEFT].body.pane == &activePane)
		parent->children_[LEFT].type = SplitterItem::PANETYPE_EMPTY;
	else {
		assert(parent->children_[RIGHT].type == SplitterItem::PANETYPE_SINGLE
			&& parent->children_[RIGHT].body.pane == &activePane);
		parent->children_[RIGHT].type = SplitterItem::PANETYPE_EMPTY;
	}

	if(root_.children_[LEFT].type == SplitterItem::PANETYPE_SINGLE && cdp == DELETE)
		delete root_.children_[LEFT].body.pane;
	else if(root_.children_[LEFT].type == SplitterItem::PANETYPE_SPLITTER)
		delete root_.children_[LEFT].body.splitter;
	if(root_.direction_ != NO_SPLIT) {
		if(root_.children_[RIGHT].type == SplitterItem::PANETYPE_SINGLE && cdp == DELETE)
			delete root_.children_[RIGHT].body.pane;
		else if(root_.children_[RIGHT].type == SplitterItem::PANETYPE_SPLITTER)
			delete root_.children_[RIGHT].body.splitter;
	}

	// same as construction...
	root_.children_[RIGHT].type = SplitterItem::PANETYPE_EMPTY;
	root_.direction_ = NO_SPLIT;
	root_.firstPaneSizeRatio_ = 0.5F;
	root_.children_[LEFT] = SplitterItem::Child(activePane);
	defaultActivePane_ = &activePane;
	if(hasFocus && static_cast<AbstractPane&>(activePane).getWindow() != ::GetFocus())
		::SetFocus(static_cast<AbstractPane&>(activePane).getWindow());
	adjustPanes();
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::setDefaultActivePane(Pane& pane) {
	if(findPane(root_, pane) == 0)
		throw std::invalid_argument("Specified pane does not belong to this splitter.");
	defaultActivePane_ = &pane;
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::setSplitterSize(uint width, uint height) {
	splitterWidth_ = width;
	splitterHeight_ = height;
	adjustPanes();
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::split(Pane& pane, Pane& clone, bool ns) {
	SplitterItem* const parent = findPane(root_, pane);
	const pos1 = ns ? TOP : LEFT;	// no matter...
	const pos2 = ns ? BOTTOM : RIGHT;

	if(parent == 0)
		throw std::invalid_argument("The specified pane does not belong to this splitter.");
	if(parent->children_[pos2].type == SplitterItem::PANETYPE_EMPTY) {
		// there is another empty pane
		assert(parent->direction_ == NO_SPLIT);
		assert(parent->children_[pos1].type == SplitterItem::PANETYPE_SINGLE);
		assert(parent->children_[pos1].body.pane == &pane);
		parent->children_[pos2] = SplitterItem::Child(clone);
		parent->direction_ = ns ? NS : WE;
	} else {
		// this splitter is already split
		SplitterItem::Child* child = 0;

		assert(parent->direction_ != NO_SPLIT);
		if(parent->children_[pos1].body.pane == &pane)
			child = &parent->children_[pos1];
		else if(parent->children_[pos2].body.pane == &pane)
			child = &parent->children_[pos2];

		assert(child != 0);
		assert(child->type == SplitterItem::PANETYPE_SINGLE);
		child->type = SplitterItem::PANETYPE_SPLITTER;
		child->body.splitter = new SplitterItem;
		child->body.splitter->parent_ = parent;
		child->body.splitter->direction_ = ns ? NS : WE;
		child->body.splitter->children_[LEFT] = SplitterItem::Child(pane);
		child->body.splitter->children_[RIGHT] = SplitterItem::Child(clone);
	}
	adjustPanes();
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::unsplit(Pane& pane) {
	SplitterItem* parent = findPane(root_, pane);

	if(parent == 0)
		throw std::invalid_argument("Specified pane does not belong to this splitter.");
	else if(parent->direction_ == NO_SPLIT)
		throw std::invalid_argument("Specified pane is not split.");

	SplitterItem::Child& leftTop = parent->children_[LEFT];
	SplitterItem::Child& rightBottom = parent->children_[RIGHT];
	const bool removedPaneWasDefaultActive = &pane == defaultActivePane_;
	const bool removedPaneHadFocus = static_cast<AbstractPane&>(pane).getWindow() == ::GetFocus();

	// swap the panes if the deleting pane is left/top
	if(leftTop.type == SplitterItem::PANETYPE_SINGLE && leftTop.body.pane == &pane)
		std::swap(leftTop, rightBottom);
	assert(rightBottom.type == SplitterItem::PANETYPE_SINGLE && rightBottom.body.pane == &pane);

	Pane* nextFirstPane = 0;
	if(removedPaneWasDefaultActive || removedPaneHadFocus) {
		if(leftTop.type == SplitterItem::PANETYPE_SINGLE)
			nextFirstPane = leftTop.body.pane;
		else
			leftTop.body.splitter->getFirstPane(true, nextFirstPane);
	}

	if(cdp == STANDARD_DELETE)
		delete rightBottom.body.pane;

	// there are 4 scenarios...
	if(parent == &root_) {	// the parent is the root
		if(leftTop.type == SplitterItem::PANETYPE_SINGLE) {	// the live pane is a single pane
			rightBottom = SplitterItem::Child();
			root_.direction_ = NO_SPLIT;
			root_.firstPaneSizeRatio_ = 0.5F;
		} else {	// the live pane is a splitter
			SplitterItem& liveSplitter = *leftTop.body.splitter;
			root_.direction_ = liveSplitter.direction_;
			root_.firstPaneSizeRatio_ = liveSplitter.firstPaneSizeRatio_;
			leftTop.type = liveSplitter.children_[LEFT].type;
			leftTop.body = liveSplitter.children_[LEFT].body;
			rightBottom.type = liveSplitter.children_[RIGHT].type;
			rightBottom.body = liveSplitter.children_[RIGHT].body;

			if(liveSplitter.children_[LEFT].type == SplitterItem::PANETYPE_SPLITTER)
				liveSplitter.children_[LEFT].body.splitter->parent_ = &root_;
			if(liveSplitter.children_[RIGHT].type == SplitterItem::PANETYPE_SPLITTER)
				liveSplitter.children_[RIGHT].body.splitter->parent_ = &root_;
			liveSplitter.children_[LEFT].type
				= liveSplitter.children_[RIGHT].type = SplitterItem::PANETYPE_EMPTY;
			delete &liveSplitter;	// this splitter should delete no children
		}
	} else {	// parent is not root
		SplitterItem& grandParent = *parent->parent_;
		const bool parentIsLeftChildOfGrandParent = parent == grandParent.children_[LEFT].body.splitter;
		SplitterItem::Child& replacedChild = grandParent.children_[parentIsLeftChildOfGrandParent ? LEFT : RIGHT];
		if(leftTop.type == SplitterItem::PANETYPE_SINGLE) {	// live pane is a single pane
			replacedChild.type = SplitterItem::PANETYPE_SINGLE;
			replacedChild.body.pane = leftTop.body.pane;
		} else {	// live pane is a splitter
			replacedChild.type = SplitterItem::PANETYPE_SPLITTER;
			replacedChild.body.splitter = leftTop.body.splitter;
			leftTop.body.splitter->parent_ = &grandParent;
		}
		parent->children_[LEFT].type = parent->children_[RIGHT].type = SplitterItem::PANETYPE_EMPTY;
		delete parent;	// this splitter should delete no children
	}

	adjustPanes();

	if(removedPaneWasDefaultActive)	defaultActivePane_ = nextFirstPane;
	if(removedPaneHadFocus)			::SetFocus(static_cast<AbstractPane*>(nextFirstPane)->getWindow());
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::Iterator::next() throw() {
	Pane* next = 0;
	SplitterItem* parent = 0;
	if(parent_->getNextPane(*pane_, true, next, &parent)) {
		parent_ = parent;
		pane_ = next;
	} else	// at end
		pane_ = 0;
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void Splitter<Pane, cdp>::Iterator::reset() throw() {
	while(parent_->getParent() != 0)
		parent_ = parent_->getParent();
	parent_->getFirstPane(true, pane_, const_cast<SplitterItem**>(&parent_));
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline internal::SplitterItem<Pane, cdp>::~SplitterItem() throw() {
	Child& leftTop = children_[LEFT];
	Child& rightBottom = children_[RIGHT];

	if(leftTop.type == PANETYPE_SINGLE) {
		if(cdp == STANDARD_DELETE)
			delete leftTop.body.pane;
		else if(cdp == DONT_DELETE_AND_SET_PARENT_TO_NULL)
			::SetParent(static_cast<AbstractPane*>(leftTop.body.pane)->getWindow(), 0);
	} else if(leftTop.type == PANETYPE_SPLITTER)
		delete leftTop.body.splitter;
	if(rightBottom.type == PANETYPE_SINGLE) {
		if(cdp == STANDARD_DELETE)
			delete rightBottom.body.pane;
		else if(cdp == DONT_DELETE_AND_SET_PARENT_TO_NULL)
			::SetParent(static_cast<AbstractPane*>(rightBottom.body.pane)->getWindow(), 0);
	} else if(rightBottom.type == PANETYPE_SPLITTER)
		delete rightBottom.body.splitter;
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void internal::SplitterItem<Pane, cdp>::adjustPanes(const RECT& newRect, uint frameWidth, uint frameHeight) {
	rect_ = newRect;
	if(direction_ == NO_SPLIT) {	// is not split
		if(children_[LEFT].type != PANETYPE_EMPTY)
			::MoveWindow(
				static_cast<AbstractPane&>(*children_[LEFT].body.pane).getWindow(),
				newRect.left, newRect.top,
				newRect.right - newRect.left, newRect.bottom - newRect.top, true);
	} else {
		::RECT rect = newRect;

		if(direction_ == NS)
			rect.bottom = newRect.top +
				static_cast<long>((newRect.bottom - newRect.top) * firstPaneSizeRatio_) - frameHeight / 2;
		else
			rect.right = newRect.left +
				static_cast<long>((newRect.right - newRect.left) * firstPaneSizeRatio_) - frameWidth / 2;
		if(children_[LEFT].type == PANETYPE_SINGLE)
			::MoveWindow(static_cast<AbstractPane*>(children_[LEFT].body.pane)->getWindow(),
				rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
		else
			children_[LEFT].body.splitter->adjustPanes(rect, frameWidth, frameHeight);

		if(direction_ == NS) {
			rect.top = rect.bottom + frameHeight;
			rect.bottom = newRect.bottom;
		} else {
			rect.left = rect.right + frameWidth;
			rect.right = newRect.right;
		}
		if(children_[RIGHT].type == PANETYPE_SINGLE)
			::MoveWindow(static_cast<AbstractPane*>(children_[RIGHT].body.pane)->getWindow(),
				rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
		else
			children_[RIGHT].body.splitter->adjustPanes(rect, frameWidth, frameHeight);
	}
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void internal::SplitterItem<Pane, cdp>::draw(gdi::PaintDC& dc, uint frameWidth, uint frameHeight) {
	if(direction_ == NO_SPLIT)
		return;

	::RECT rect;
	rect.left = (direction_ == NS) ? rect_.left :
		rect_.left + static_cast<long>((rect_.right - rect_.left) * firstPaneSizeRatio_) - frameWidth / 2;
	rect.top = (direction_ == WE) ? rect_.top :
		rect_.top + static_cast<long>((rect_.bottom - rect_.top) * firstPaneSizeRatio_) - frameHeight / 2;
	rect.right = (direction_ == NS) ? rect_.right : rect.left + frameWidth;
	rect.bottom = (direction_ == WE) ? rect_.bottom : rect.top + frameHeight;
	dc.fillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

	if(children_[LEFT].type == PANETYPE_SPLITTER)
		children_[LEFT].body.splitter->draw(dc, frameWidth, frameHeight);
	if(children_[RIGHT].type == PANETYPE_SPLITTER)
		children_[RIGHT].body.splitter->draw(dc, frameWidth, frameHeight);
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void internal::SplitterItem<Pane, cdp>::getFirstPane(bool leftTop, Pane*& pane, SplitterItem<Pane, cdp>** parent = 0) const {
	SplitterItem<Pane, cdp>* p = const_cast<SplitterItem<Pane, cdp>*>(this);
	const PanePosition seekDirection = leftTop ? LEFT : RIGHT;
	pane = 0;
	do {
		if(p->children_[seekDirection].type == PANETYPE_SINGLE) {
			pane = p->children_[seekDirection].body.pane;
			assert(pane != 0);
		} else {
			assert(p->children_[seekDirection].type == PANETYPE_SPLITTER);
			p = p->children_[seekDirection].body.splitter;
		}
	} while(pane == 0);
	if(parent != 0)
		*parent = p;
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline bool internal::SplitterItem<Pane, cdp>::getNextPane(
		const Pane& pane, bool next, Pane*& nextPane, SplitterItem<Pane, cdp>** parent = 0) const {
	// returns false if pane is rightmost
	const PanePosition forwardPosition = next ? RIGHT : LEFT;
	const PanePosition backPosition = next ? LEFT : RIGHT;

	if(children_[backPosition].type == PANETYPE_SINGLE && children_[backPosition].body.pane == &pane) {
		if(children_[forwardPosition].type == PANETYPE_SINGLE) {
			nextPane = children_[forwardPosition].body.pane;
			if(parent != 0)	*parent = const_cast<SplitterItem<Pane, cdp>*>(this);
			return true;
		} else if(children_[forwardPosition].type == PANETYPE_SPLITTER) {
			children_[forwardPosition].body.splitter->getFirstPane(next, nextPane, parent);
			return true;
		} else {
			assert(children_[forwardPosition].type == PANETYPE_EMPTY);
			return false;
		}
	}

	// up one level
	assert(children_[forwardPosition].type == PANETYPE_SINGLE && children_[forwardPosition].body.pane == &pane);
	SplitterItem* childSplitter = const_cast<SplitterItem<Pane, cdp>*>(this);
	SplitterItem* p = parent_;

	if(p == 0)	// `this' is root
		return false;
	while(p != 0) {
		if(p->children_[backPosition].type == PANETYPE_SPLITTER
				&& p->children_[backPosition].body.splitter == childSplitter) {
			if(p->children_[forwardPosition].type == PANETYPE_SINGLE) {
				nextPane = p->children_[forwardPosition].body.pane;
				if(parent != 0)	*parent = p;
				return true;
			} else if(p->children_[forwardPosition].type == PANETYPE_SPLITTER) {
				p->children_[forwardPosition].body.splitter->getFirstPane(next, nextPane, parent);
				return true;
			} else {
				assert(p->children_[forwardPosition].type == PANETYPE_EMPTY);
				break;
			}
		} else {
			// up one level
			assert(p->children_[forwardPosition].type == PANETYPE_SPLITTER
				&& p->children_[forwardPosition].body.splitter == childSplitter);
			childSplitter = p;
			p = p->parent_;
		}
	}
	return false;
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline internal::SplitterItem<Pane, cdp>* internal::SplitterItem<Pane, cdp>::hitTest(const ::POINT& pt) const {
	if(!toBoolean(::PtInRect(&rect_, pt)))
		return 0;
	if(children_[LEFT].type == PANETYPE_SPLITTER) {
		if(SplitterItem<Pane, cdp>* p = children_[LEFT].body.splitter->hitTest(pt))
			return p;
	}
	if(children_[RIGHT].type == PANETYPE_SPLITTER) {
		if(SplitterItem<Pane, cdp>* p = children_[RIGHT].body.splitter->hitTest(pt))
			return p;
	}
	return const_cast<SplitterItem<Pane, cdp>*>(this);
}

template<class Pane, SplitterBase::ChildrenDestructionPolicy cdp>
inline void internal::SplitterItem<Pane, cdp>::sendMessageToChildren(UINT message) {
	Child& leftTop = children_[LEFT];
	Child& rightBottom = children_[RIGHT];

	if(leftTop.type == PANETYPE_SINGLE)
		::SendMessage(static_cast<AbstractPane*>(leftTop.body.pane)->getWindow(), message, 0, 0L);
	else if(leftTop.type == PANETYPE_SPLITTER)
		leftTop.body.splitter->sendMessageToChildren(message);
	if(rightBottom.type == PANETYPE_SINGLE)
		::SendMessage(static_cast<AbstractPane*>(rightBottom.body.pane)->getWindow(), message, 0, 0L);
	else if(rightBottom.type == PANETYPE_SPLITTER)
		rightBottom.body.splitter->sendMessageToChildren(message);
}

}}} // namespace manah::windows::ui

#endif /* MANAH_SPLITTER_HPP */
