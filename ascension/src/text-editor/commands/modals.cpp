/**
 * @file modals.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-04 Separated from command.cpp.
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/text-editor/commands/modals.hpp>
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
			CancelCommand::CancelCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @return true
			 */
			bool CancelCommand::perform() {
				throwIfTargetHasNoWindow();
				abortModes();
				target().textArea()->caret()->clearSelection();
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			CompletionProposalPopupCommand::CompletionProposalPopupCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The text viewer didn't have the content assistant
			 */
			bool CompletionProposalPopupCommand::perform() {
				throwIfTargetIsReadOnly();
//				ASCENSION_CHECK_GUI_EDITABILITY();
				abortIncrementalSearch(*viewer::document(target()));
				if(contentassist::ContentAssistant* ca = target().contentAssistant()) {
					ca->showPossibleCompletions();
					return true;
				}
				return false;	// the viewer does not have a content assistant
			}

			/**
			 * Constructor.
			 * @param viewer the target text viewer
			 */
			InputMethodOpenStatusToggleCommand::InputMethodOpenStatusToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The system didn't support the input method
			 */
			bool InputMethodOpenStatusToggleCommand::perform() {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				if(win32::Handle<HIMC>::Type imc = win32::inputMethod(target()))
					return win32::boole(::ImmSetOpenStatus(imc.get(), !win32::boole(::ImmGetOpenStatus(imc.get()))));
#else
				// TODO: Not implemented.
#endif
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			InputMethodSoftKeyboardModeToggleCommand::InputMethodSoftKeyboardModeToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * @see Command#perform
			 * @retval false The system didn't support the input method
			 */
			bool InputMethodSoftKeyboardModeToggleCommand::perform() {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				if(win32::Handle<HIMC>::Type imc = win32::inputMethod(target())) {
					DWORD conversionMode, sentenceMode;
					if(win32::boole(::ImmGetConversionStatus(imc.get(), &conversionMode, &sentenceMode))) {
						conversionMode = win32::boole(conversionMode & IME_CMODE_SOFTKBD) ?
							(conversionMode & ~IME_CMODE_SOFTKBD) : (conversionMode | IME_CMODE_SOFTKBD);
						return win32::boole(::ImmSetConversionStatus(imc.get(), conversionMode, sentenceMode));
					}
				}
#else
				// TODO: Not implemented.
#endif
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			OvertypeModeToggleCommand::OvertypeModeToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * @see Command#perform
			 * @return @c true if succeeded
			 */
			bool OvertypeModeToggleCommand::perform() {
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						caret->setOvertypeMode(!caret->isOvertypeMode());
						viewer::utils::closeCompletionProposalsPopup(target());
						return true;
					}
				}
				return false;
			}
		}
	}
}
