/**
 * @file conversions.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-04 Separated from command.cpp.
 */

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/kernel/locations.hpp>
#include <ascension/text-editor/commands/conversions.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			CharacterToCodePointConversionCommand::CharacterToCodePointConversionCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The end of the selection is the beginning of the line and couldn't find the string to
			 *               convert
			 */
			bool CharacterToCodePointConversionCommand::perform() {
				throwIfTargetIsReadOnly();
				viewer::TextViewer& viewer = target();
				abortModes();
				const auto document(viewer::document(viewer));
				const auto& peos = viewer.textArea()->caret()->end();
				const auto eos(*boost::const_end(viewer.textArea()->caret()->selectedRegion()));
				if(kernel::locations::isBeginningOfLine(peos) || (document->isNarrowed() && eos == *boost::const_begin(document->accessibleRegion())))
					return false;

				const auto caret(viewer.textArea()->caret());
				const String& lineString = document->lineString(kernel::line(eos));
				const CodePoint c = text::utf::decodeLast(std::begin(lineString), std::begin(lineString) + kernel::offsetInLine(eos));
				std::array<Char, 7> buffer;
#if(_MSC_VER < 1400)
				if(std::swprintf(buffer.data(), L"%lX", c) < 0)
#else
				if(std::swprintf(buffer.data(), buffer.size(), L"%lX", c) < 0)
#endif // _MSC_VER < 1400
					return false;
				viewer::AutoFreeze af(&viewer);
				caret->select(viewer::_anchor = kernel::Position(kernel::line(eos), kernel::offsetInLine(eos) - ((c > 0xffff) ? 2 : 1)), viewer::_caret = peos.hit());
				try {
					caret->replaceSelection(buffer.data(), false);
				} catch(const kernel::DocumentInput::ChangeRejectedException&) {
					return false;
				}
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			CodePointToCharacterConversionCommand::CodePointToCharacterConversionCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The end of the selection is the beginning of the line and couldn't find the string
			 *               to convert
			 */
			bool CodePointToCharacterConversionCommand::perform() {
				throwIfTargetIsReadOnly();
				abortModes();
				const auto document(viewer::document(target()));
				const auto textArea(target().textArea());
				const auto caret(textArea->caret());
				const auto& peos = caret->end();
				const auto eos(*boost::const_end(caret->selectedRegion()));
				if(kernel::locations::isBeginningOfLine(peos) || (document->isNarrowed() && eos == *boost::const_begin(document->accessibleRegion())))
					return false;

				const String& lineString = document->lineString(kernel::line(eos));
				const Index offsetInLine = kernel::offsetInLine(eos);

				// accept /(?:[Uu]\+)?[0-9A-Fa-f]{1,6}/
				if(iswxdigit(lineString[offsetInLine - 1]) != 0) {
					Index i = offsetInLine - 1;
					while(i != 0) {
						if(offsetInLine - i == 7)
							return false;	// too long string
						else if(iswxdigit(lineString[i - 1]) == 0)
							break;
						--i;
					}

					const CodePoint c = wcstoul(lineString.substr(i, offsetInLine - i).c_str(), nullptr, 16);
					if(text::isValidCodePoint(c)) {
						String s;
						text::utf::encode(c, std::back_inserter(s));
						if(i >= 2 && lineString[i - 1] == L'+' && (lineString[i - 2] == L'U' || lineString[i - 2] == L'u'))
							i -= 2;
						viewer::AutoFreeze af(&target());
						caret->select(viewer::_anchor = kernel::Position(kernel::line(eos), i), viewer::_caret = peos.hit());
						try {
							caret->replaceSelection(s, false);
						} catch(const kernel::DocumentInput::ChangeRejectedException&) {
							return false;
						}
						return true;
					}
				}
				return false;	// invalid code point string and can't convert
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			ReconversionCommand::ReconversionCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @return false The selection was empty or rectangle. Or the system didn't support IME reconversion
			 * @throw std#bad_alloc Out of memory
			 * @see viewer#TextViewer#onIMERequest
			 */
			bool ReconversionCommand::perform() {
				endIncrementalSearch(*viewer::document(target()));
				throwIfTargetIsReadOnly();
//	ASCENSION_CHECK_GUI_EDITABILITY();

				bool succeeded = false;
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						if(!caret->isSelectionRectangle()) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
							if(win32::Handle<HIMC>::Type imc = win32::inputMethod(target())) {
								if(!win32::boole(::ImmGetOpenStatus(imc.get())))	// without this, IME may ignore us?
									::ImmSetOpenStatus(imc.get(), true);

								// from NotePadView.pas of TNotePad (http://wantech.ikuto.com/)
								const bool multilineSelection = kernel::line(*caret) != kernel::line(caret->anchor());
								const String s(multilineSelection ? selectedString(*caret) : viewer::document(target()).line(kernel::line(*caret)));
								const DWORD bytes = static_cast<DWORD>(sizeof(RECONVERTSTRING) + sizeof(Char) * s.length());
								RECONVERTSTRING* const rcs = static_cast<RECONVERTSTRING*>(::operator new(bytes));
								rcs->dwSize = bytes;
								rcs->dwVersion = 0;
								rcs->dwStrLen = static_cast<DWORD>(s.length());
								rcs->dwStrOffset = sizeof(RECONVERTSTRING);
								rcs->dwCompStrLen = rcs->dwTargetStrLen =
									static_cast<DWORD>(multilineSelection ? s.length() : (kernel::offsetInLine(caret->end()) - kernel::offsetInLine(caret->beginning())));
								rcs->dwCompStrOffset = rcs->dwTargetStrOffset =
									multilineSelection ? 0 : static_cast<DWORD>(sizeof(Char) * kernel::offsetInLine(caret->beginning()));
								s.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), s.length());

								if(viewer::isSelectionEmpty(*caret)) {
									// IME selects the composition target automatically if no selection
									if(win32::boole(::ImmSetCompositionStringW(imc.get(), SCS_QUERYRECONVERTSTRING, rcs, rcs->dwSize, nullptr, 0))) {
										caret->select(
											Position(kernel::line(*caret), rcs->dwCompStrOffset / sizeof(Char)),
											Position(kernel::line(*caret), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
										if(win32::boole(::ImmSetCompositionStringW(imc.get(), SCS_SETRECONVERTSTRING, rcs, rcs->dwSize, nullptr, 0)))
											succeeded = true;
									}
								}
								::operator delete(rcs);
							}
#else
							// TODO: Not implemented.
#endif
						}
					}
				}

				viewer::utils::closeCompletionProposalsPopup(target());
				return succeeded;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param untabify Set @c true to untabify rather than tabify
			 */
			TabifyCommand::TabifyCommand(viewer::TextViewer& viewer, bool untabify) BOOST_NOEXCEPT : Command(viewer), untabify_(untabify) {
			}

			/**
			 * Implements @c Command#perform.
			 * @note Not implemented.
			 */
			bool TabifyCommand::perform() {
				throwIfTargetIsReadOnly();
//				ASCENSION_CHECK_GUI_EDITABILITY(1);
				abortModes();
				// TODO: not implemented.
				return false;
			}
		}
	}
}
