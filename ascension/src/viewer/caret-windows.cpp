/**
 * @file caret-windows.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2011-10-03 separated from caret.cpp
 * @date 2011-2015
 */

#include <ascension/viewer/caret.hpp>

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/text-editor/commands/inputs.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/win32/com/unknown-impl.hpp>

namespace ascension {
	namespace viewer {
		namespace {
#pragma comment(lib, "urlmon.lib")

			// copied from point.cpp
			inline const text::IdentifierSyntax& identifierSyntax(const kernel::Point& p) {
				return p.document().contentTypeInformation().getIdentifierSyntax(kernel::contentType(p));
			}

			template<typename Procedure>
			inline HRESULT tryOleClipboard(Procedure procedure) {
				HRESULT hr;
				for(int i = 0; i < 100; ++i) {
					if(CLIPBRD_E_CANT_OPEN != (hr = procedure()))
						break;
					::Sleep(0);
				}
				return hr;
			}

			template<typename Procedure, typename Parameter>
			inline HRESULT tryOleClipboard(Procedure procedure, Parameter parameter) {
				HRESULT hr;
				for(int i = 0; i < 100; ++i) {
					if(CLIPBRD_E_CANT_OPEN != (hr = procedure(parameter)))
						break;
					::Sleep(0);
				}
				return hr;
			}
		} // namespace @0


		// ClipboardException /////////////////////////////////////////////////////////////////////////////////////////

		ClipboardException::ClipboardException(HRESULT hr) : system_error(makePlatformError(hr)) {
		}


		// Caret //////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
		void Caret::abortInput() {
			if(context_.inputMethodCompositionActivated)	// stop IME input
				::ImmNotifyIME(inputMethod(textArea().textViewer()).get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
		}
#endif
		/// Moves the IME form to valid position.
		void Caret::adjustInputMethodCompositionWindow() {
			assert(win32::boole(::IsWindow(textArea().textViewer().handle().get())));
			if(!context_.inputMethodCompositionActivated)
				return;
			if(auto imc = win32::inputMethod(textArea().textViewer())) {
				// composition window placement
				COMPOSITIONFORM cf;
				cf.rcArea = toNative<RECT>(textArea().contentRectangle());
				cf.dwStyle = CFS_POINT;
				cf.ptCurrentPos = toNative<POINT>(modelToView(textArea().textViewer(), TextHit::leading(*boost::const_begin(selectedRegion())) /*, false */));
				if(cf.ptCurrentPos.y == std::numeric_limits<graphics::Scalar>::max() || cf.ptCurrentPos.y == std::numeric_limits<graphics::Scalar>::min())
					cf.ptCurrentPos.y = (cf.ptCurrentPos.y == std::numeric_limits<graphics::Scalar>::min()) ? cf.rcArea.top : cf.rcArea.bottom;
				else
					cf.ptCurrentPos.y = std::max(cf.ptCurrentPos.y, cf.rcArea.top);
				::ImmSetCompositionWindow(imc.get(), &cf);
				cf.dwStyle = CFS_RECT;
				::ImmSetCompositionWindow(imc.get(), &cf);

				// composition font
				LOGFONTW font;
				::GetObjectW(textArea().textRenderer()->defaultFont()->native().get(), sizeof(LOGFONTW), &font);
				::ImmSetCompositionFontW(imc.get(), &font);	// this may be ineffective for IME settings
			}
		}

		bool Caret::canPastePlatformData() const {
			if(win32::boole(::IsClipboardFormatAvailable(CF_UNICODETEXT)) || win32::boole(::IsClipboardFormatAvailable(CF_TEXT)))
				return true;
			try {
				if(win32::boole(::IsClipboardFormatAvailable(utils::rectangleTextMimeDataFormat())))
					return true;
			} catch(const std::system_error&) {
			}
			return false;
		}

		/**
		 * Returns the locale identifier used to convert non-Unicode text.
		 * @see #setClipboardLocale
		 */
		LCID Caret::clipboardLocale() const BOOST_NOEXCEPT {
			return clipboardLocale_;
		}

#if 0
			switch(message) {
				case WM_IME_NOTIFY:
					if(wp == IMN_SETOPENSTATUS)
						inputModeChangedSignal_(*this, INPUT_METHOD_OPEN_STATUS);
					break;
				case WM_IME_REQUEST:
					return onImeRequest(wp, lp, consumed);
				case WM_INPUTLANGCHANGE:
					inputModeChangedSignal_(*this, INPUT_LOCALE);
					break;
			}
#endif

		/// Handles Win32 @c WM_IME_REQUEST window message.
		LRESULT Caret::onImeRequest(WPARAM command, LPARAM lp, bool& consumed) {
			const auto& doc = document();

			// this command will be sent two times when reconversion is invoked
			if(command == IMR_RECONVERTSTRING) {
				if(doc.isReadOnly() || isSelectionRectangle()) {
					textArea().textViewer().beep();
					return 0L;
				}
				consumed = true;
				if(isSelectionEmpty(*this)) {	// IME selects the composition target automatically if no selection
					if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
						const auto& lineString = doc.lineString(kernel::line(*this));
						rcs->dwStrLen = static_cast<DWORD>(lineString.length());
						rcs->dwStrOffset = sizeof(RECONVERTSTRING);
						rcs->dwTargetStrOffset = rcs->dwCompStrOffset = static_cast<DWORD>(sizeof(Char) * kernel::offsetInLine(*this));
						rcs->dwTargetStrLen = rcs->dwCompStrLen = 0;
						lineString.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
					}
					return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(kernel::line(*this));
				} else {
					const auto selection(selectedString(*this, text::Newline::USE_INTRINSIC_VALUE));
					if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
						rcs->dwStrLen = rcs->dwTargetStrLen = rcs->dwCompStrLen = static_cast<DWORD>(selection.length());
						rcs->dwStrOffset = sizeof(RECONVERTSTRING);
						rcs->dwTargetStrOffset = rcs->dwCompStrOffset = 0;
						selection.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
					}
					return sizeof(RECONVERTSTRING) + sizeof(Char) * selection.length();
				}
			}

			// before reconversion. a RECONVERTSTRING contains the ranges of the composition
			else if(command == IMR_CONFIRMRECONVERTSTRING) {
				if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
					const auto region(doc.accessibleRegion());
					if(!isSelectionEmpty(*this)) {
						// reconvert the selected region. the selection may be multi-line
						if(rcs->dwCompStrLen < rcs->dwStrLen)	// the composition region was truncated.
							rcs->dwCompStrLen = rcs->dwStrLen;	// IME will alert and reconversion will not be happen if do this
																// (however, NotePad narrows the selection...)
					} else {
						// reconvert the region IME passed if no selection (and create the new selection).
						// in this case, reconversion across multi-line (prcs->dwStrXxx represents the entire line)
						if(doc.isNarrowed() && kernel::line(*this) == kernel::line(boost::const_begin(region))) {	// the document is narrowed
							if(rcs->dwCompStrOffset / sizeof(Char) < kernel::offsetInLine(*boost::const_begin(region))) {
								rcs->dwCompStrLen += static_cast<DWORD>(sizeof(Char) * kernel::offsetInLine(*boost::const_begin(region)) - rcs->dwCompStrOffset);
								rcs->dwTargetStrLen = rcs->dwCompStrOffset;
								rcs->dwCompStrOffset = rcs->dwTargetStrOffset = static_cast<DWORD>(sizeof(Char) * kernel::offsetInLine(*boost::const_begin(region)));
							} else if(rcs->dwCompStrOffset / sizeof(Char) > kernel::offsetInLine(*boost::const_end(region))) {
								rcs->dwCompStrOffset -= rcs->dwCompStrOffset - sizeof(Char) * kernel::offsetInLine(*boost::const_begin(region));
								rcs->dwTargetStrOffset = rcs->dwCompStrOffset;
								rcs->dwCompStrLen = rcs->dwTargetStrLen
									= static_cast<DWORD>(sizeof(Char) * kernel::offsetInLine(*boost::const_begin(region)) - rcs->dwCompStrOffset);
							}
						}
						select(
							_anchor = kernel::Position(line(*this), rcs->dwCompStrOffset / sizeof(Char)),
							_caret = TextHit::leading(kernel::Position(line(*this), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen)));
					}
					consumed = true;
					return true;
				}
			}

			// queried position of the composition window
			else if(command == IMR_QUERYCHARPOSITION)
				return false;	// handled by updateIMECompositionWindowPosition...

			// queried document content for higher conversion accuracy
			else if(command == IMR_DOCUMENTFEED) {
				if(kernel::line(*this) == kernel::line(anchor())) {
					consumed = true;
					if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
						rcs->dwStrLen = static_cast<DWORD>(doc.lineLength(line(*this)));
						rcs->dwStrOffset = sizeof(RECONVERTSTRING);
						rcs->dwCompStrLen = rcs->dwTargetStrLen = 0;
						rcs->dwCompStrOffset = rcs->dwTargetStrOffset = sizeof(Char) * static_cast<DWORD>(kernel::offsetInLine(beginning()));
						doc.lineString(kernel::line(*this)).copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
					}
					return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(kernel::line(*this));
				}
			}

			return 0L;
		}

		void Caret::paste(bool useKillRing) {
			AutoFreeze af(&textArea().textViewer());
			if(!useKillRing) {
				win32::com::SmartPointer<IDataObject> content;
				HRESULT hr = tryOleClipboard(::OleGetClipboard, content.initialize());
				if(hr == E_OUTOFMEMORY)
					throw std::bad_alloc();
				else if(FAILED(hr))
					throw ClipboardException(hr);
				const InterprocessData data(content);
				document().insertUndoBoundary();
				replaceSelection(data.text(), data.hasFormat(utils::rectangleTextMimeDataFormat()));	// this may throw several exceptions
			} else {
				texteditor::Session* const session = document().session();
				if(session == nullptr || session->killRing().numberOfKills() == 0)
					throw IllegalStateException("the kill-ring is not available.");
				auto& killRing = session->killRing();
				const auto& text = context_.yanking ? killRing.setCurrent(+1) : killRing.get();

				const auto temp(*boost::const_begin(selectedRegion()));
				try {
					if(!isSelectionEmpty(*this) && context_.yanking)
						document().undo();
					replaceSelection(text.first, text.second);
				} catch(...) {
					killRing.setCurrent(-1);
					throw;
				}
				if(!text.second)
					endRectangleSelection();
				else
					beginRectangleSelection();
				select(_anchor = temp, _caret = hit());
				context_.yanking = true;
			}
			document().insertUndoBoundary();
		}

		void Caret::preeditChanged(widgetapi::event::InputMethodEvent& event) BOOST_NOEXCEPT {
			if(!document().isReadOnly()) {
				auto& nativeMessage = *static_cast<const MSG*>(event.native());
				if((nativeMessage.lParam & CS_INSERTCHAR) != 0) {
					const auto p(insertionPosition(*this));	// position before motion
					try {
						if(context_.inputMethodComposingCharacter)
							document().replace(
								kernel::Region(
									p,
									static_cast<kernel::DocumentCharacterIterator&>(++kernel::DocumentCharacterIterator(document(), p)).tell()),
								String(1, static_cast<Char>(nativeMessage.wParam)));
						else
							kernel::insert(document(), p, String(1, static_cast<Char>(nativeMessage.wParam)));
						context_.inputMethodComposingCharacter = true;
						if((nativeMessage.lParam & CS_NOMOVECARET) != 0)
							moveTo(TextHit::leading(p));
					} catch(...) {
					}
					event.consume();
					resetVisualization();
				}
			 }
		 }

		/**
		 * Sets the locale used to convert non-Unicode data in the clipboard.
		 * @param newLocale The locale identifier
		 * @return The identifier of the locale set by the caret
		 * @throw std#invalid_argument @a newLocale is not installed on the system
		 * @see #clipboardLocale
		 */
		LCID Caret::setClipboardLocale(LCID newLocale) {
			if(!win32::boole(::IsValidLocale(newLocale, LCID_INSTALLED)))
				throw std::invalid_argument("newLocale");
			std::swap(clipboardLocale_, newLocale);
			return newLocale;
		}


		// viewer free functions /////////////////////////////////////////////////////////////////////////

		void copySelection(Caret& caret, bool useKillRing) {
			if(isSelectionEmpty(caret))
				return;

			auto content(utils::createInterprocessDataForSelectedString(caret, true));	// this may throw std.bad_alloc
			HRESULT hr = tryOleClipboard(::OleSetClipboard, content.native());
			if(FAILED(hr))
				throw ClipboardException(hr);
			hr = tryOleClipboard(::OleFlushClipboard);
			if(useKillRing) {
				if(texteditor::Session* const session = caret.document().session())
					session->killRing().addNew(selectedString(caret, text::Newline::USE_INTRINSIC_VALUE), caret.isSelectionRectangle());
			}
		}

		void cutSelection(Caret& caret, bool useKillRing) {
			if(isSelectionEmpty(caret))
				return;

			win32::com::SmartPointer<IDataObject> previousContent;
			HRESULT hr = tryOleClipboard(::OleGetClipboard, previousContent.initialize());
			if(hr == E_OUTOFMEMORY)
				throw std::bad_alloc();
			else if(FAILED(hr))
				throw ClipboardException(hr);
			copySelection(caret, useKillRing);	// this may throw
			try {
				viewer::eraseSelection(caret);
			} catch(...) {
				hr = tryOleClipboard(::OleSetClipboard, previousContent.get());
				throw;
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
