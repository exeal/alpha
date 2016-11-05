/**
 * @file inputs.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-05 Separated from command.cpp.
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/text-editor/commands/inputs.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/visual-locations.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param c The code point of the character to input
			 * @throw text#InvalidScalarValueException @a c is not valid Unicode scalar value
			 */
			CharacterInputCommand::CharacterInputCommand(viewer::TextViewer& viewer, CodePoint c) : Command(viewer), c_(c) {
				if(!text::isScalarValue(c))
					throw text::InvalidScalarValueException(c);
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false Failed and the incremental search is not active
			 * @retval 0 Otherwise
			 * @see Caret#inputCharacter, TextViewer#onChar, TextViewer#onUniChar
			 */
			bool CharacterInputCommand::perform() {
				if(numericPrefix() == 1) {
#if 0
					if(Session* const session = target().document().session()) {
						if(session->incrementalSearcher().isRunning()) {
							viewer::utils::closeCompletionProposalsPopup(target());
							if(c_ == 0x0009u || std::iswcntrl(static_cast<std::wint_t>(c_)) == 0)
								session->incrementalSearcher().addCharacter(c_);
							return true;
						}
					}
#endif
					try {
						return target().textArea()->caret()->inputCharacter(c_);
					} catch(const kernel::DocumentCantChangeException&) {
						return false;
					}
				} else {
					throwIfTargetIsReadOnly();
					if(numericPrefix() > 0) {	// ...
						String s;
						text::utf::encode(c_, back_inserter(s));
						return TextInputCommand(target(), s).setNumericPrefix(numericPrefix())();
					}
					return true;
				}
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param fromPreviousLine Set @c true to use a character on the previous visual line. Otherwise
			 *                         one on the next visual line is used
			 */
			CharacterInputFromNextLineCommand::CharacterInputFromNextLineCommand(
					viewer::TextViewer& viewer, bool fromPreviousLine) BOOST_NOEXCEPT : Command(viewer), fromPreviousLine_(fromPreviousLine) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The caret was the first/last line in the document and couldn't copy a character from the
			 *               previous/next line. Or the next/previous line was too short to locate the character to
			 *               copy. Or internal performance of @c CharacterInputCommand failed
			 */
			bool CharacterInputFromNextLineCommand::perform() {
				abortIncrementalSearch(*viewer::document(target()));
				throwIfTargetIsReadOnly();

				// TODO: recognizes narrowing.

				const std::shared_ptr<const viewer::VisualPoint> caret(target().textArea()->caret());

				if((fromPreviousLine_ && kernel::line(*caret) == 0)
						|| (!fromPreviousLine_ && kernel::line(*caret) >= viewer::document(target())->numberOfLines() - 1))
					return false;
	
				const auto p(viewer::insertionPosition(*viewer::document(target()),
					viewer::locations::nextVisualLine(*caret, fromPreviousLine_ ? Direction::backward() : Direction::forward())));
				const String& lineString = viewer::document(target())->lineString(kernel::line(*caret) + (fromPreviousLine_ ? -1 : 1));
				if(kernel::offsetInLine(p) >= lineString.length())
					return false;
				setNumericPrefix(1);
				return CharacterInputCommand(target(), text::utf::decodeFirst(std::begin(lineString) + kernel::offsetInLine(p), std::end(lineString)))();
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param direction Set @c boost#none to break current line at the caret position. Otherwise, this command
			 *                  inserts newline(s) at the beginning of the next (@c Direction#forward()) or the
			 *                  previous (@c Direction#backward()) line. In this case, the command ends the active mode
			 *                  and inserts newline character(s)
			 */
			NewlineCommand::NewlineCommand(viewer::TextViewer& viewer, boost::optional<Direction> direction) BOOST_NOEXCEPT : Command(viewer), direction_(direction) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The document was read only or the change was rejected
			 */
			bool NewlineCommand::perform() {
				if(numericPrefix() <= 0)
					return true;

				if(contentassist::ContentAssistant* const ca = target().contentAssistant()) {
					if(contentassist::ContentAssistant::CompletionProposalsUI* cpui = ca->completionProposalsUI()) {
						if(cpui->complete())
							return true;
					}
				}

				if(endIncrementalSearch(*viewer::document(target())) && direction_ == boost::none)
					return true;

				throwIfTargetIsReadOnly();
//				ASCENSION_CHECK_GUI_EDITABILITY(1);

				const auto caret(target().textArea()->caret());
				const auto oldSelection(caret->selectedRegion());
				const auto document(viewer::document(target()));
				viewer::AutoFreeze af(&target());

				if(direction_ != boost::none) {
					viewer::TextHit h(caret->hit());	// initial value is no matter...
					if(*direction_ == Direction::forward())
						h = viewer::locations::endOfVisualLine(*caret);
					else if(kernel::line(*caret) != kernel::line(*boost::const_begin(document->region()))) {
						const Index line = kernel::line(*caret) - 1;
						h = viewer::TextHit::leading(kernel::Position(line, document->lineLength(line)));
					} else
						h = viewer::TextHit::leading(*boost::const_begin(document->region()));
					if(!encompasses(document->accessibleRegion(), h.characterIndex()))
						return false;
					const bool autoShow = caret->isAutoShowEnabled();
					caret->enableAutoShow(false);
					caret->moveTo(h);
					caret->enableAutoShow(autoShow);
				}

				try {
					document->insertUndoBoundary();
					breakLine(*caret, false, numericPrefix());
				} catch(const kernel::DocumentInput::ChangeRejectedException&) {
					document->insertUndoBoundary();
					caret->select(oldSelection);
					return false;
				}
				document->insertUndoBoundary();
				caret->moveTo(caret->anchor().hit());
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param text The text to input. This can be empty or ill-formed UTF-16 sequence
			 */
			TextInputCommand::TextInputCommand(viewer::TextViewer& viewer, const String& text) BOOST_NOEXCEPT : Command(viewer), text_(text) {
			}

			namespace {
				inline String multiplyString(const String& s, std::size_t n) {
					const std::size_t len = s.length();
					String temp;
					temp.reserve(n * len);
					for(std::size_t i = 0; i < n; ++i)
						temp.append(s.data(), len);
					return temp;
				}
			}

			/**
			 * Inserts a text. If the incremental search is active, appends a string to the end of the pattern.
			 * @retval false The change was rejected
			 * @throw ... Any exceptions @c searcher#IncrementalSearcher#addString and @c viewer#replaceSelection
			 *            throw other than @c kernel#IDocumentInput#ChangeRejectedException
			 */
			bool TextInputCommand::perform() {
				const NumericPrefix n(numericPrefix());
				if(n == 0)
					return true;
#if 0
				if(Session* const session = target().document().session()) {
					if(session->incrementalSearcher().isRunning()) {
						session->incrementalSearcher().addString((n > 1) ? multiplyString(text_, static_cast<std::size_t>(n)) : text_);
						return true;
					}
				}
#endif
				throwIfTargetIsReadOnly();
//				ASCENSION_CHECK_GUI_EDITABILITY();
				if(n > 1) {
					try {
						target().textArea()->caret()->replaceSelection(multiplyString(text_, static_cast<std::size_t>(n)));
					} catch(const kernel::DocumentInput::ChangeRejectedException&) {
						return false;
					}
				} else {
					try {
						target().textArea()->caret()->replaceSelection(text_);
					} catch(const kernel::DocumentInput::ChangeRejectedException&) {
						return false;
					}
				}
				return true;
			}
		}
	}
}
