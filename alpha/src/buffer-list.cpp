/**
 * @file buffer-list.cpp
 * @author exeal
 * @date 2003-2006 (was AlphaDoc.cpp and BufferList.cpp)
 * @date 2006-2015
 */

#include "application.hpp"
#include "buffer-list.hpp"
#include "function-pointer.hpp"
#include <ascension/kernel/searcher.hpp>
#include <boost/foreach.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/thread/lock_guard.hpp>
#include <glibmm/i18n.h>

namespace alpha {

	// BufferList /////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * @typedef alpha::BufferList::BufferAboutToBeRemovedSignal
	 * The signal which gets emitted when a buffer was about to be removed from the buffer list.
	 * @param bufferList The buffer list
	 * @param buffer The buffer to be removed
	 * @see #bufferAboutToBeRemovedSignal, #close
	 */

	/**
	 * @typedef alpha::BufferList::BufferAddedSignal
	 * The signal which gets emitted when a new buffer was added into the buffer list.
	 * @param bufferList The buffer list
	 * @param buffer The added buffer
	 * @see #addNew, #bufferAddedSignal
	 */

	/**
	 * @typedef alpha::BufferList::BufferRemovedSignal
	 * The signal which gets emitted when a buffer was removed from the buffer list.
	 * @param bufferList The buffer list
	 * @param buffer The removed buffer
	 * @see #bufferRemovedSignal, #close
	 */

	/**
	 * @typedef alpha::BufferList::BufferSelectionChangedSignal
	 * The signal which gets emitted when the buffer selection was changed.
	 * @param bufferList The buffer list
	 * @see #selected, EditorPanes#BufferSelectionChangedSignal
	 */

	/**
	 * @typedef alpha::BufferList::DisplayNameChangedSignal
	 * The signal which gets emitted when the display name of the buffer was changed.
	 * @param buffer The buffer whose display name was changed
	 * @see #displayName, #displayNameChangedSignal, #forName
	 */

	/// Default constructor.
	BufferList::BufferList() {
		selection_ = std::end(buffers_);
		locker_.swap(boost::recursive_mutex::scoped_lock(mutex_, boost::defer_lock));
	}

	/// Destructor.
	BufferList::~BufferList() BOOST_NOEXCEPT {
//		for(EditorWindows::Iterator i(EditorWindows::instance().enumeratePanes()); !i.done(); i.next())
//			i.get().removeAll();
		boost::for_each(buffers_, [this](BufferEntry& buffer) {
			this->editorSession_.removeDocument(*buffer.buffer);
		});
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		if(icons_.get() != 0) {
			const int c = icons_.getNumberOfImages();
			for(int i = 0; i < c; ++i)
				::DestroyIcon(icons_.getIcon(i, ILD_NORMAL));
			icons_.destroy();
		}
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	}

	/**
	 * Opens the new empty buffer. This method does not select the new buffer.
	 * @param name the name of the buffer
	 * @param encoding the encoding
	 * @param newline the newline
	 * @return the buffer added
	 * @throw ascension#encoding#UnsupportedEncodingException @a encoding is not supported
	 * @throw ascension#UnknownValueException @a newline is invalid
	 */
	std::shared_ptr<Buffer> BufferList::addNew(const Glib::ustring& name /* = Glib::ustring() */,
			const std::string& encoding /* = "UTF-8" */, ascension::text::Newline newline /* = ascension::text::Newline::USE_INTRINSIC_VALUE */) {
		BufferEntry newEntry;
		newEntry.buffer.reset(new Buffer(name));
		std::shared_ptr<Buffer> newBuffer(newEntry.buffer);
		newEntry.nameChangedConnection =
			newBuffer->nameChangedSignal().connect(std::bind(&BufferList::fireDisplayNameChanged, this, std::placeholders::_1));
		newEntry.modificationSignChangedConnection =
			newBuffer->modificationSignChangedSignal().connect(std::bind(&BufferList::documentModificationSignChanged, this, std::placeholders::_1));
		newEntry.readOnlySignChangedConnection =
			newBuffer->readOnlySignChangedSignal().connect(std::bind(&BufferList::documentReadOnlySignChanged, this, std::placeholders::_1));

		newBuffer->textFile().setEncoding(encoding);
		if(newline.isLiteral())
			newBuffer->textFile().setNewline(newline);

		editorSession_.addDocument(*newBuffer);
//		newBuffer.setProperty(ascension::kernel::Document::TITLE_PROPERTY, !name.empty() ? name : L"*untitled*");
		{
			boost::lock_guard<decltype(locker_)> lg(locker_);
			const auto selectedOffset = std::distance(selection_, std::end(buffers_));
			buffers_.push_back(std::move(newEntry));
			selection_ = std::prev(std::end(buffers_), selectedOffset);
		}

//		view.addEventListener(app_);
		newBuffer->textFile().addListener(*this);
//		view.getLayoutSetter().setFont(lf);

		resetResources();
//		app_.applyDocumentType(*buffer, docType);
		bufferAddedSignal_(*this, newBuffer);

		return newBuffer;
	}
/*
	/// Shows "New with Format" dialog box and opens the new empty buffer.
	/// @param name the name of the buffer
	/// @return the buffer added or @c null if the user canceled
	Buffer* BufferList::addNewDialog(const ascension::String& name) {
		Alpha& app = Alpha::instance();
		TextFileFormat format;
		format.encoding = e::Encoder::forMIB(e::fundamental::US_ASCII)->fromUnicode(app.readStringProfile(L"File", L"defaultEncoding"));
		if(!e::Encoder::supports(format.encoding))
			format.encoding = e::Encoder::defaultInstance().properties().name();
		format.newline = static_cast<k::Newline>(app.readIntegerProfile(L"file", L"defaultNewline", k::NLF_CR_LF));

		ui::NewFileFormatDialog dlg(format.encoding, format.newline);
		if(dlg.doModal(app.getMainWindow()) != IDOK)
			return 0;
		return &addNew(name, dlg.encoding(), dlg.newline());
	}
*/
	/// Returns the @c BufferAboutToBeRemovedSignal signal connector.
	ascension::SignalConnector<BufferList::BufferAboutToBeRemovedSignal> BufferList::bufferAboutToBeRemovedSignal() BOOST_NOEXCEPT {
		return ascension::makeSignalConnector(bufferAboutToBeRemovedSignal_);
	}

	/// Returns the @c BufferAddedSignal signal connector.
	ascension::SignalConnector<BufferList::BufferAddedSignal> BufferList::bufferAddedSignal() BOOST_NOEXCEPT {
		return ascension::makeSignalConnector(bufferAddedSignal_);
	}

	/// Returns the @c BufferRemovedSignal signal connector.
	ascension::SignalConnector<BufferList::BufferRemovedSignal> BufferList::bufferRemovedSignal() BOOST_NOEXCEPT {
		return ascension::makeSignalConnector(bufferRemovedSignal_);
	}

	/// Returns the @c BufferSelectionChangedSignal signal connector.
	ascension::SignalConnector<BufferList::BufferSelectionChangedSignal> BufferList::bufferSelectionChangedSignal() BOOST_NOEXCEPT {
		return ascension::makeSignalConnector(bufferSelectionChangedSignal_);
	}

	/**
	 * Closes the specified buffer.
	 * @param buffer The buffer to close
	 * @return true if the buffer was closed successfully
	 * @throw ascension#NoSuchElementException @a buffer is not in this list
	 */
	void BufferList::close(Buffer& buffer) {
		const boost::optional<std::size_t> position = find(buffer);
		if(position == boost::none)
			throw ascension::NoSuchElementException("buffer");
		if(buffers_.size() > 1) {
			bufferAboutToBeRemovedSignal_(*this, buffer);
			{
				boost::lock_guard<decltype(locker_)> lg(locker_);
				// track the selection
				std::size_t selection = std::distance(std::begin(buffers_), selection_);
				if(selection > boost::get(position) || (selection == boost::get(position) && selection == buffers_.size() - 1))
					--selection;

				BufferEntry entryToRemove(std::move(buffers_[boost::get(position)]));
				buffers_.erase(buffers_.begin() + boost::get(position));
				selection_ = std::next(std::begin(buffers_), selection);
				editorSession_.removeDocument(buffer);
				buffer.textFile().removeListener(*this);
				buffer.textFile().unbind();

				resetResources();
			}
			bufferRemovedSignal_(*this, buffer);
		} else {	// the buffer is last one
			buffer.textFile().unbind();
			buffer.resetContent();
		}
	}

	/**
	 * Returns the presentative name of the specified buffer used to display in GUIs.
	 * @param buffer The buffer
	 * @return The display name
	 */
	Glib::ustring BufferList::displayName(const Buffer& buffer) const BOOST_NOEXCEPT {
		Glib::ustring name(buffer.name());
		if(buffer.isModified())
			name.append(" *");	// TODO: Be customizable.
		if(buffer.isReadOnly())
			name.append(" #");	// TODO: Be customizable.
		return name;
	}

	/// Returns the @c DisplayNameChangedSignal signal connector.
	ascension::SignalConnector<BufferList::DisplayNameChangedSignal> BufferList::displayNameChangedSignal() BOOST_NOEXCEPT {
		return ascension::makeSignalConnector(displayNameChangedSignal_);
	}

	/// @see ascension#kernel#Document#documentModificationSignChangedSignal
	void BufferList::documentModificationSignChanged(const ascension::kernel::Document& document) {
		fireDisplayNameChanged(document);
	}

	/// @see ascension#kernel#Document#documentReadOnlySignChangedSignal
	void BufferList::documentReadOnlySignChanged(const ascension::kernel::Document& document) {
		fireDisplayNameChanged(document);
	}

	/// @see ascension#kernel#fileio#FilePropertyListener#fileNameChanged
	void BufferList::fileNameChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile) {
		// TODO: call mode-application.
		resetResources();
		fireDisplayNameChanged(textFile.document());
	}

	/// @see ascension#kernel#fileio#FilePropertyListener#fileEncodingChanged
	void BufferList::fileEncodingChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile) {
		// do nothing
	}

	/**
	 * Finds the buffer in the list.
	 * @param buffer the buffer to find
	 * @return the index of the buffer or @c boost#none if not found
	 */
	boost::optional<std::size_t> BufferList::find(const Buffer& buffer) const BOOST_NOEXCEPT {
		for(std::size_t i = 0, c = buffers_.size(); i < c; ++i) {
			if(buffers_[i].buffer.get() == &buffer)
				return i;
		}
		return boost::none;
	}

	/// @internal
	inline void BufferList::fireDisplayNameChanged(const ascension::kernel::Document& buffer) {
		Application::instance()->window().updateTitle();
		displayNameChangedSignal_(getConcreteDocument(buffer));
	}

	/**
	 * Returns the buffer named @a name.
	 * @param The name of buffer
	 * @return The buffer, or @c boost#python#object() if there is no buffer with @a name
	 */
	Buffer* BufferList::forName(const Glib::ustring& name) const {
		const auto found = boost::find_if(buffers_, [&name](const BufferEntry& e) {
			return e.buffer->name() == name;
		});
		return (found != boost::end(buffers_)) ? found->buffer.get() : nullptr;
	}

	/**
	 * Translates the abstract document into a @c Buffer.
	 * @document the document
	 * @throw std#invalid_argument @a document is not found
	 * @see BufferList#find
	 */
	Buffer& BufferList::getConcreteDocument(ascension::kernel::Document& document) const {
		const auto found = boost::find_if(buffers_, [&document](const BufferEntry& e) {
			return e.buffer.get() == &document;
		});
		if(found != boost::end(buffers_))
			return *found->buffer;
		throw std::invalid_argument("The specified document is not in the list.");
	}

	/// Const-version of @c #getConcreteDocument(Document&).
	const Buffer& BufferList::getConcreteDocument(const ascension::kernel::Document& document) const {
		return getConcreteDocument(const_cast<ascension::kernel::Document&>(document));
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	/// Handles @c WM_NOTIFY message from the buffer bar.
	LRESULT BufferList::handleBufferBarNotification(NMTOOLBARW& nmhdr) {
		if(nmhdr.hdr.code == NM_RCLICK) {	// right click -> context menu
			const NMMOUSE& mouse = *reinterpret_cast<NMMOUSE*>(&nmhdr.hdr);
			if(mouse.dwItemSpec != -1) {
				POINT pt = mouse.pt;
				bufferBar_.clientToScreen(pt);
				EditorWindows::instance().activePane().showBuffer(at(mouse.dwItemSpec));
				contextMenu_.trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, Alpha::instance().getMainWindow().use());
				return true;
			}
		}

		else if(nmhdr.hdr.code == TTN_GETDISPINFOW) {	// show a tooltip
			assert(static_cast<int>(nmhdr.hdr.idFrom) < bufferBar_.getButtonCount());
			static wchar_t tipText[500];
			NMTTDISPINFOW& nmttdi = *reinterpret_cast<NMTTDISPINFOW*>(&nmhdr.hdr);

//			nmttdi->hinst = getHandle();
			const Buffer& buffer = at(nmttdi.hdr.idFrom);
			wcscpy(tipText, (buffer.textFile().isBoundToFile() ? buffer.textFile().location() : buffer.name()).c_str());
			nmttdi.lpszText = tipText;
			return true;
		}

		else if(nmhdr.hdr.code == TBN_ENDDRAG && bufferBar_.getButtonCount() > 1) {
			TBINSERTMARK mark;
			bufferBar_.getInsertMark(mark);
			if(mark.iButton != -1) {
				// move the button
				move(bufferBar_.commandToIndex(nmhdr.iItem),
					toBoolean(mark.dwFlags & TBIMHT_AFTER) ? mark.iButton + 1 : mark.iButton);
				// delete the insert mark
				mark.dwFlags = 0;
				mark.iButton = -1;
				bufferBar_.setInsertMark(mark);
			}
		}

		// drag -> switch the selected buffer
		else if(nmhdr.hdr.code == TBN_GETOBJECT) {
			::NMOBJECTNOTIFY& n = *reinterpret_cast<::NMOBJECTNOTIFY*>(&nmhdr.hdr);
			if(n.iItem != -1) {
				EditorWindows::instance().activePane().showBuffer(at(bufferBar_.commandToIndex(n.iItem)));	// n.iItem は ID
				n.pObject = 0;
				n.hResult = E_NOINTERFACE;
			}
			return 0;
		}

		else if(nmhdr.hdr.code == TBN_HOTITEMCHANGE && bufferBar_.getButtonCount() > 1 && bufferBar_.get() == ::GetCapture()) {
			::NMTBHOTITEM& hotItem = *reinterpret_cast<::NMTBHOTITEM*>(&nmhdr.hdr);
			if(toBoolean(hotItem.dwFlags & HICF_MOUSE)) {	// dragging a button...
				::TBINSERTMARK mark;
				if(!toBoolean(hotItem.dwFlags & HICF_LEAVING)) {
					// move the insert mark
					mark.dwFlags = 0;
					mark.iButton = bufferBar_.commandToIndex(hotItem.idNew);
				} else {
					mark.dwFlags = TBIMHT_AFTER;
					mark.iButton = bufferBar_.getButtonCount() - 1;
				}
				bufferBar_.setInsertMark(mark);
			}
		}

		return false;
	}

	/// Handles @c WM_NOTIFY message from the pager of the buffer bar.
	LRESULT BufferList::handleBufferBarPagerNotification(NMHDR& nmhdr) {
		if(nmhdr.code == PGN_CALCSIZE) {	// ページャサイズの計算
			LPNMPGCALCSIZE p = reinterpret_cast<LPNMPGCALCSIZE>(&nmhdr);
			if(p->dwFlag == PGF_CALCWIDTH) {
				SIZE size;
				bufferBar_.getMaxSize(size);
				p->iWidth = size.cx;
			} else if(p->dwFlag == PGF_CALCHEIGHT) {
				SIZE size;
				bufferBar_.getMaxSize(size);
				p->iHeight = size.cy;
			}
			return true;
		}

		else if(nmhdr.code == PGN_SCROLL) {	// ページャのスクロール量の設定
			NMPGSCROLL* p = reinterpret_cast<NMPGSCROLL*>(&nmhdr);
			p->iScroll = 20;
			if(toBoolean(p->fwKeys & PGK_SHIFT))	// 逆方向
				p->iScroll *= -1;
			if(toBoolean(p->fwKeys & PGK_CONTROL))	// 倍速
				p->iScroll *= 2;
			return true;
		}

		return false;
	}
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

	/// Returns the singleton instance.
	BufferList& BufferList::instance() {
		static BufferList singleton;
		return singleton;
	}

	/**
	 * Returns a string that is the name of no existing buffer based on the specified string.
	 * @param name The base name string
	 * @return The generated name string
	 * @throw std#overflow_error
	 */
	Glib::ustring BufferList::makeUniqueName(const Glib::ustring& name) const {
		if(forName(name) == nullptr)
			return name;
		const Glib::ustring format(name + "<%1>");
		for(std::size_t n = 2; ; ++n) {
			const Glib::ustring newName(Glib::ustring::compose(format, n));
			if(forName(newName) == boost::python::object())
				return newName;
			if(n == std::numeric_limits<decltype(n)>::max())
				break;
		}
		throw std::overflow_error("name");
	}

	/**
	 * Moves the specified buffer in the buffer list.
	 * @param from The index of the buffer to move
	 * @param to The index of the destination
	 * @throw std::out_of_range @a from is invalid
	 */
	void BufferList::move(boost::python::ssize_t from, boost::python::ssize_t to) {
		boost::lock_guard<decltype(locker_)> lg(locker_);
#if 0
		if(from < 0 || to < 0 || static_cast<std::size_t>(from) >= buffers_.size() || static_cast<std::size_t>(to) > buffers_.size()) {
			::PyErr_SetString(PyExc_IndexError, "The specified index is out of range.");
			py::throw_error_already_set();
		} else if(from == to)
			return;

		// リスト内で移動
		Buffer* buffer = buffers_[from];
		buffers_.erase(buffers_.begin() + from);
		buffers_.insert((from < to) ? buffers_.begin() + to - 1 : buffers_.begin() + to, buffer);

		// バッファバーのボタンを並び替え
		const int end = std::min(static_cast<int>(std::max(from, to)), bufferBar_.getButtonCount() - 1);
		for(int i = static_cast<int>(std::min(from, to)); i <= end; ++i) {
			bufferBar_.setCommandID(i, i);
			bufferBar_.setButtonText(i, getDisplayName(*buffers_[i]).c_str());
		}
		EditorWindows::instance().activePane().showBuffer(*buffer);
		resetResources();
#endif
	}

	namespace {
#if defined(BOOST_OS_WINDOWS) && 0
		std::basic_string<WCHAR> resolveShortcut(const std::basic_string<WCHAR>& s) {
			const WCHAR* extension = ::PathFindExtensionW(s.c_str());
			if(std::wcslen(extension) != 0 && (
					(::StrCmpIW(extension + 1, L"lnk") == 0)
					/*|| (::StrCmpIW(extension + 1, L"url") == 0)*/)) {
				HRESULT hr;
				ascension::win32::com::ComPtr<IShellLinkW> shellLink(CLSID_ShellLink, IID_IShellLinkW, CLSCTX_ALL, 0, &hr);
				if(SUCCEEDED(hr)) {
					ascension::win32::com::ComPtr<IPersistFile> file;
					if(SUCCEEDED(hr = shellLink->QueryInterface(IID_IPersistFile, file.initializePPV()))) {
						if(SUCCEEDED(hr = file->Load(s.c_str(), STGM_READ))) {
							if(SUCCEEDED(hr = shellLink->Resolve(0, SLR_ANY_MATCH | SLR_NO_UI))) {
								WCHAR resolved[MAX_PATH];
								if(SUCCEEDED(hr = shellLink->GetPath(resolved, MAX_PATH, 0, 0)))
									return resolved;
							}
						}
					}
				}
				throw ascension::win32::com::ComException(hr, IID_NULL, OLESTR("@0.resolveShortcut"));
			} else
				return ascension::kernel::fileio::canonicalizePathName(s.c_str());
		}
#endif // BOOST_OS_WINDOWS
	}
#if 0
	/**
	 * Opens the specified file.
	 * This method may show a dialog to indicate the result.
	 * @param fileName the name of the file to open
	 * @param encoding the encoding. auto-detect if omitted
	 * @param lockMode the lock mode
	 * @param asReadOnly set true to open as read only
	 * @return the opened buffer or @c None if failed
	 */
	py::object BufferList::open(const basic_string<WCHAR>& fileName,
			const string& encoding /* = "UniversalAutoDetect" */,
			e::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
			f::TextFileDocumentInput::LockType lockMode /* = DONT_LOCK */, bool asReadOnly /* = false */) {
		// TODO: this method is too complex.
		Alpha& app = Alpha::instance();

		// resolve shortcut
		basic_string<WCHAR> resolvedName;
		try {
			resolvedName = resolveShortcut(fileName);
		} catch(com::ComException&) {
			::PyErr_SetObject(PyExc_IOError, convertWideStringToUnicodeObject(
				app.loadMessage(MSG_IO__FAILED_TO_RESOLVE_SHORTCUT, MARGS % fileName)).ptr());
			py::throw_error_already_set();
		}

		// check if the file is already open with other text editor
		const size_t oldBuffer = find(resolvedName);
		if(oldBuffer != -1) {
			EditorWindows::instance().activePane().showBuffer(at(oldBuffer));
			return py::object();
		}

		Buffer* buffer = &EditorWindows::instance().selectedBuffer();
		if(buffer->isModified() || buffer->textFile().isBoundToFile()) {	// open in the new container
			if(e::Encoder::supports(encoding))
				buffer = &addNew(L"", encoding);
			else
				buffer = &addNew();
		}
/*
		if(Encoder::supports(encoding)) {
//			try {
				buffer->setEncoding(encoding);
//			} catch(invalid_argument&) {
//				if(IDNO == app_.messageBox(MSG_ILLEGAL_CODEPAGE, MB_YESNO | MB_ICONEXCLAMATION))
//					return OPENRESULT_USERCANCELED;
//				encoding = ::GetACP();
//				buffer->setCodePage(encoding);
//			}
		}
*/
		{
			win32::ui::WaitCursor wc;
			app.statusBar().setText(app.loadMessage(MSG_STATUS__LOADING_FILE, MARGS % resolvedName).c_str());
			app.getMainWindow().lockUpdate();

			try {
				// TODO: check the returned value.
				buffer->textFile().open(resolvedName, lockMode, encoding, encodingSubstitutionPolicy);
			} catch(const f::FileNotFoundException& e) {
				::PyErr_SetString(PyExc_IOError, e.what());
			} catch(const f::AccessDeniedException& e) {
				::PyErr_SetString(PyExc_IOError, e.what());
			} catch(const f::UnmappableCharacterException& e) {
				::PyErr_SetString(PyExc_UnicodeDecodeError, e.what());
			} catch(const f::MalformedInputException& e) {
				::PyErr_SetString(PyExc_UnicodeDecodeError, e.what());
			} catch(const f::PlatformDependentIOError&) {
				::PyErr_SetFromWindowsErr(0);
			} catch(f::IOException& e) {
				::PyErr_SetObject(PyExc_IOError, py::object(e.type()).ptr());
			}
			app.statusBar().setText(0);
			app.getMainWindow().unlockUpdate();
		}
		app.getMainWindow().show(app.getMainWindow().isVisible() ? SW_SHOW : SW_RESTORE);

		if(::PyErr_Occurred() != 0)
			py::throw_error_already_set();

		if(asReadOnly)
			buffer->setReadOnly();
		return buffers_.back()->self();
	}
#endif
	/// @see ascension#text#UnexpectedFileTimeStampDirector::queryAboutUnexpectedTimeStamp
	bool BufferList::queryAboutUnexpectedDocumentFileTimeStamp(
			ascension::kernel::Document& document, UnexpectedFileTimeStampDirector::Context context) BOOST_NOEXCEPT {
		if(unexpectedFileTimeStampDirector == boost::python::object())
			return false;
		try {
			boost::python::object f(unexpectedFileTimeStampDirector.attr("query_about_unexpected_time_stamp"));
			const boost::python::object result(f(getConcreteDocument(document), context));
			return ::PyObject_IsTrue(result.ptr()) == 1;
		} catch(boost::python::error_already_set&) {
#ifndef ALPHA_NO_AMBIENT
			ambient::Interpreter::instance().handleException();
#endif
			return false;
		}
	}

#if 0
	/**
	 * Reopens the specified buffer.
	 * @param index the index of the buffer to reopen
	 * @param changeEncoding set true to change the encoding
	 * @return the result. see the description of @c BufferList#OpenResult
	 * @throw std#out_of_range @a index is invalid
	 */
	BufferList::OpenResult BufferList::reopen(size_t index, bool changeEncoding) {
		using namespace ascension::kernel::fileio;
		Alpha& app = Alpha::instance();
		Buffer& buffer = at(index);

		// ファイルが存在するか?
		if(!buffer.textFile().isOpen())
			return OPENRESULT_FAILED;

		// ユーザがキャンセル
		else if(buffer.isModified() && IDNO == app.messageBox(MSG_BUFFER__CONFIRM_REOPEN_EVEN_IF_DIRTY, MB_YESNO | MB_ICONQUESTION))
			return OPENRESULT_USERCANCELED;

		// エンコードを変更する場合はダイアログを出す
		string encoding(buffer.textFile().encoding());
		if(changeEncoding) {
			ui::EncodingsDialog dlg(encoding, true);
			if(dlg.doModal(app.getMainWindow()) != IDOK)
				return OPENRESULT_USERCANCELED;
			encoding = dlg.resultEncoding();
		}

		bool succeeded = true;
		IOException::Type errorType;
		while(true) {
			using namespace ascension::encoding;
			try {
				buffer.textFile().open(buffer.textFile().pathName(), buffer.textFile().lockMode(), encoding, Encoder::DONT_SUBSTITUTE);
			} catch(IOException& e) {
				succeeded = false;
				errorType = e.type();
			}
			if(!succeeded) {
				// alert the encoding error
				int userAnswer;
				if(errorType == IOException::UNMAPPABLE_CHARACTER)
					userAnswer = app.messageBox(MSG_IO__UNCONVERTABLE_NATIVE_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
						MARGS % buffer.textFile().pathName() % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(encoding).c_str());
				else if(errorType == IOException::MALFORMED_INPUT)
					userAnswer = app.messageBox(MSG_IO__MALFORMED_INPUT_FILE, MB_OKCANCEL | MB_ICONEXCLAMATION,
						MARGS % buffer.textFile().pathName() % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(encoding).c_str());
				else
					break;
				succeeded = true;
				if(userAnswer == IDYES || userAnswer == IDOK) {
					// the user want to change the encoding
					ui::EncodingsDialog dlg(encoding, true);
					if(dlg.doModal(app.getMainWindow()) != IDOK)
						return OPENRESULT_USERCANCELED;
					encoding = dlg.resultEncoding();
					continue;
				} else if(userAnswer == IDNO) {
					succeeded = true;
					try {
						buffer.textFile().open(buffer.textFile().pathName(),
							buffer.textFile().lockMode(), encoding, Encoder::REPLACE_UNMAPPABLE_CHARACTERS);
					} catch(IOException& e) {
						succeeded = false;
						if((errorType = e.type()) == IOException::MALFORMED_INPUT) {
							app.messageBox(MSG_IO__MALFORMED_INPUT_FILE, MB_OK | MB_ICONEXCLAMATION,
								MARGS % buffer.textFile().pathName() % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(encoding).c_str());
							return OPENRESULT_FAILED;
						}
					}
				} else
					return OPENRESULT_USERCANCELED;
			}
			break;
		}

		if(succeeded || handleFileIOError(buffer.textFile().pathName().c_str(), true, errorType)) {
//			app.mruManager().add(buffer.textFile().pathName());
			return OPENRESULT_SUCCEEDED;
		} else
			return OPENRESULT_FAILED;
	}
#endif

	/// Reconstructs the image list and the menu according to the current buffer list.
	void BufferList::resetResources() {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		if(icons_.get() != 0) {
			const int c = icons_.getNumberOfImages();
			for(int i = 0; i < c; ++i)
				::DestroyIcon(icons_.getIcon(i, ILD_NORMAL));
			icons_.destroy();
		}
		if(buffers_.empty())
			return;
		icons_ = win32::ui::ImageList::create(::GetSystemMetrics(SM_CXSMICON),
			::GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(buffers_.size()));
		while(listMenu_.getNumberOfItems() != 0)
			listMenu_.erase<win32::ui::Menu::BY_POSITION>(0);

		SHFILEINFOW sfi;
		for(size_t i = 0; i < buffers_.size(); ++i) {
			::SHGetFileInfoW(
				(buffers_[i]->textFile().isBoundToFile()) ? buffers_[i]->textFile().fileName().c_str() : L"",
				0, &sfi, sizeof(SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON);
			icons_.add(sfi.hIcon);
			listMenu_ << win32::ui::Menu::OwnerDrawnItem(static_cast<UINT>(i));
		}
		bufferBar_.setImageList(icons_.use());
		if(bufferBar_.isVisible())
			bufferBar_.invalidateRect(0);
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	}
#if 0
	/**
	 * Saves (overwrites) the specified buffer.
	 * @param index the index of the buffer to save
	 * @param overwrite set false to save with another name (a file dialog will be shown)
	 * @param addToMRU set true to add the file to MRU. this is effective only if the file was not exist or renamed
	 * @retval true saved successfully or not needed to
	 * @throw std#out_of_range @a index is invalid
	 */
	bool BufferList::save(std::size_t index, bool overwrite /* = true */, bool addToMRU /* = true */) {

		Alpha& app = Alpha::instance();
		Buffer& buffer = at(index);

		// 保存の必要があるか?
		if(overwrite && buffer.textFile().isOpen() && !buffer.isModified())
			return true;

		WCHAR fileName[MAX_PATH + 1];
		TextFileFormat format = {buffer.textFile().encoding(), NLF_RAW_VALUE};
		bool newName = false;

		// 別名で保存 or ファイルが存在しない
		if(!overwrite || !buffer.textFile().isOpen() || !toBoolean(::PathFileExistsW(buffer.textFile().pathName().c_str()))) {
			win32::AutoZero<OSVERSIONINFOW> osVersion;
			const wstring filterSource(app.loadMessage(MSG_DIALOG__SAVE_FILE_FILTER));
			wchar_t* const filter = new wchar_t[filterSource.length() + 6];

			osVersion.dwOSVersionInfoSize = sizeof(::OSVERSIONINFOW);
			::GetVersionExW(&osVersion);
			filterSource.copy(filter, filterSource.length());
			wcsncpy(filter + filterSource.length(), L"\0*.*\0\0", 6);
			wcscpy(fileName, (buffer.textFile().pathName().c_str()));

			win32::AutoZeroSize<OPENFILENAMEW> newOfn;
			win32::AutoZeroSize<OPENFILENAME_NT4W> oldOfn;
			OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<OPENFILENAMEW*>(&oldOfn);
			ofn.hwndOwner = app.getMainWindow().use();
			ofn.hInstance = ::GetModuleHandle(0);
			ofn.lpstrFilter = filter;
			ofn.lpstrFile = fileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE
				| OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT /* | OFN_SHOWHELP*/;
			ofn.lCustData = reinterpret_cast<LPARAM>(&format);
			ofn.lpfnHook = openFileNameHookProc;
			ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_DLG_SAVEFILE);

			bool succeeded = toBoolean(::GetSaveFileNameW(&ofn));
//			DWORD n = ::CommDlgExtendedError();
			delete[] filter;
			if(!succeeded)
				return false;

			// 既に開かれているファイルに上書きしようとしていないか?
			const size_t existing = find(fileName);
			if(existing != -1 && existing != index) {
				app.messageBox(MSG_BUFFER__SAVING_FILE_IS_OPENED, MB_ICONEXCLAMATION | MB_OK, MARGS % fileName);
				return false;
			}
			newName = true;
		} else
			wcscpy(fileName, buffer.textFile().pathName().c_str());

		using namespace ascension::encoding;
		MIBenum encodingMIB = MIB_UNKNOWN;
		{
			const auto_ptr<Encoder> temp(Encoder::forName(format.encoding));
			if(temp.get() != 0)
				encodingMIB = temp->properties().mibEnum();
		}
		const bool writeBOM =
			(encodingMIB == fundamental::UTF_8 && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF8", 0)))
			|| (encodingMIB == fundamental::UTF_16LE && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF16LE", 1)))
			|| (encodingMIB == fundamental::UTF_16BE && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF16BE", 1)))
			|| (encodingMIB == fundamental::UTF_16 && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF16", 1)))
			|| (encodingMIB == standard::UTF_32 && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF32", 1)))
			|| (encodingMIB == standard::UTF_32LE && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF32LE", 1)))
			|| (encodingMIB == standard::UTF_32BE && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF32BE", 1)));

		using namespace kernel::fileio;
		IOException::Type errorType;
		bool succeeded = true;
		while(true) {
			TextFileDocumentInput::WriteParameters params;
			params.encoding = format.encoding;
			params.encodingSubstitutionPolicy = Encoder::DONT_SUBSTITUTE;
			params.newline = format.newline;
			if(writeBOM)
				params.options = TextFileDocumentInput::WriteParameters::WRITE_UNICODE_BYTE_ORDER_SIGNATURE;

			try {
				buffer.textFile().write(fileName, params);
			} catch(IOException& e) {
				succeeded = false;
				errorType = e.type();
			}
			if(!succeeded && errorType == IOException::UNMAPPABLE_CHARACTER) {
				// alert the encoding error
				const int userAnswer = app.messageBox(
					MSG_IO__UNCONVERTABLE_UCS_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
					MARGS % fileName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(params.encoding).c_str());
				if(userAnswer == IDYES) {
					// the user want to change the encoding
					ui::EncodingsDialog dlg(params.encoding, false);
					if(dlg.doModal(app.getMainWindow()) != IDOK)
						return false;
					params.encoding = dlg.resultEncoding();
					continue;
				} else if(userAnswer == IDNO) {
					succeeded = true;
					params.encodingSubstitutionPolicy = Encoder::REPLACE_UNMAPPABLE_CHARACTER;
					try {
						buffer.textFile().write(fileName, params);
					} catch(IOException& e) {
						succeeded = false;
						errorType = e.type();
					}
					break;
				} else
					return false;
			}
			break;
		}

		succeeded = succeeded || handleFileIOError(fileName, false, errorType);
//		if(succeeded && addToMRU && newName)
//			app_.mruManager().add(fileName);
		return succeeded;
		return false;
	}

	/**
	 */
	bool BufferList::saveSomeDialog(boost::python::tuple buffersToSave /* = boost::python::tuple() */) {
		ui::SaveSomeBuffersDialog dialog;
		for(size_t i = 0, c = buffers_.size(); i < c; ++i) {
			ui::DirtyFile df;
			df.fileName = buffers_[i]->name();
			df.save = true;
			dialog.files_.push_back(df);
		}
		if(IDOK != dialog.doModal(Alpha::instance().getMainWindow()))
			return false;

		// save the checked buffers
		for(vector<ui::DirtyFile>::reverse_iterator it(dialog.files_.rbegin()); it != dialog.files_.rend(); ++it) {
			if(it->save) {
				py::object buffer(forFileName(it->fileName));
				if(buffer != py::object()) {
					py::extract<Buffer&> temp(buffer);
					if(temp.check())
						saveBuffer(static_cast<Buffer&>(temp));
				}
			}
		}
		return true;
	}
#endif
	/**
	 * Selects the specified buffer.
	 * @param buffer The buffer to select
	 * @throw ascension#NoSuchElementException @a buffer is not in this list
	 */
	void BufferList::select(Buffer& buffer) {
		boost::lock_guard<decltype(locker_)> lg(locker_);
		if(const boost::optional<std::size_t> index = find(buffer)) {
			auto i(std::begin(buffers_));
			std::advance(i, boost::get(index));
			if(i == selection_) {
				selection_ = i;
				bufferSelectionChangedSignal_(*this);
			}
		} else
			throw ascension::NoSuchElementException("buffer");
	}

	BufferList::BufferEntry::~BufferEntry() BOOST_NOEXCEPT {
		// TODO: boost.signals2.scoped_connection can't be used here because msvc10 doesn't define move constructor.
		modificationSignChangedConnection.disconnect();
		readOnlySignChangedConnection.disconnect();
	}

	BufferList::BufferEntry& BufferList::BufferEntry::operator=(BufferList::BufferEntry&& other) {
		buffer = std::move(other.buffer);
		modificationSignChangedConnection = other.modificationSignChangedConnection;
		readOnlySignChangedConnection = other.readOnlySignChangedConnection;
		return *this;
	}

#ifndef ALPHA_NO_AMBIENT
	ALPHA_EXPOSE_PROLOGUE(2)
		ambient::Interpreter& interpreter = ambient::Interpreter::instance();
		boost::python::scope temp(interpreter.toplevelPackage());

		boost::python::class_<BufferList, boost::noncopyable>("_BufferList", boost::python::no_init)
			.def_readwrite("unexpected_file_time_stamp_director", &BufferList::unexpectedFileTimeStampDirector)
//			.def("__contains__", &)
			.def("__getitem__", boost::python::make_function(
				ambient::makeFunctionPointer([](const BufferList& buffers, boost::python::ssize_t at) -> Buffer& {
					try {
						return buffers.at(at);
					} catch(const std::out_of_range& e) {
						::PyErr_SetString(PyExc_IndexError, e.what());
						boost::python::throw_error_already_set();
						throw;	// unreachable
					}
				}),
				boost::python::return_internal_reference<>()))
//			.def("__iter__", &)
			.def("__len__", &BufferList::numberOfBuffers)
			.def("add_new", &BufferList::addNew,
				(boost::python::arg("name") = Glib::ustring(), boost::python::arg("encoding") = "UTF-8", boost::python::arg("newline") = ascension::text::Newline::USE_INTRINSIC_VALUE),
				boost::python::return_value_policy<boost::python::reference_existing_object>())
#if 0
			.def("add_new_dialog", &BufferList::addNewDialog,
				boost::python::arg("name") = wstring(), boost::python::return_value_policy<py::reference_existing_object>())
#endif
//			.def("close_all", &BufferList::closeAll)
			.def("for_name", &BufferList::forName, boost::python::return_internal_reference<>())
			.def("move", &BufferList::move)
#if 0
			.def("save_some_dialog", &BufferList::saveSomeDialog, boost::python::arg("buffers_to_save") = boost::python::tuple())
#endif
			;

		boost::python::def("buffers", boost::python::make_function(
			&BufferList::instance,
			boost::python::return_value_policy<boost::python::reference_existing_object>()));
	ALPHA_EXPOSE_EPILOGUE()
#endif
}
