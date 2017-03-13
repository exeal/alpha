/**
 * @file caret-motions.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-05 Separated from command.cpp.
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/text-editor/commands/caret-motions.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/viewer/visual-locations.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure A function gives a motion
			 * @param direction The direction of motion
			 * @param extendSelection Set @c true to extend the selection
			 * @throw NullPointerException @a procedure is @c null
			 */
			template<typename ProcedureSignature>
			CaretMovementCommand<ProcedureSignature>::CaretMovementCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure,
					Direction direction, bool extendSelection /* = false */)
					: Command(viewer), procedure_(procedure), direction_(direction), extends_(extendSelection) {
				if(procedure == nullptr)
					throw NullPointerException("procedure");
			}

			namespace {
				template<typename CaretMovementProcedure>
				inline bool selectCompletionProposal(viewer::TextViewer&, CaretMovementProcedure*, Direction, long) {
					return false;
				}
				inline bool selectCompletionProposal(viewer::TextViewer& target, kernel::Position(*procedure)(const kernel::locations::PointProxy&, Direction, Index), Direction direction, long n) {
					if(procedure == &kernel::locations::nextLine) {
						if(const auto ca = target.contentAssistant()) {
							if(contentassist::ContentAssistant::CompletionProposalsUI* const cpui = ca->completionProposalsUI())
								return cpui->nextProposal((direction == Direction::forward()) ? +n : -n), true;
						}
					}
					return false;
				}
				inline bool selectCompletionProposal(viewer::TextViewer& target, viewer::VisualDestinationProxy(*procedure)(const viewer::VisualPoint&, Direction, Index), Direction direction, long n) {
					if(procedure == &viewer::locations::nextPage || procedure == &viewer::locations::nextVisualLine) {
						if(const auto ca = target.contentAssistant()) {
							if(contentassist::ContentAssistant::CompletionProposalsUI* const cpui = ca->completionProposalsUI()) {
								if(procedure == &viewer::locations::nextPage)
									return cpui->nextPage((direction == Direction::forward()) ? +n : -n), true;
								else
									return cpui->nextProposal((direction == Direction::forward()) ? +n : -n), true;
							}
						}
					}
					return false;
				}

				template<typename ProcedureSignature>
				inline bool moveToBoundOfSelection(viewer::Caret&, ProcedureSignature*, Direction) {
					return false;
				}
				inline bool moveToBoundOfSelection(viewer::Caret& caret, kernel::Position(*procedure)(const kernel::locations::PointProxy&, Direction direction, kernel::locations::CharacterUnit, Index), Direction direction) {
					if(procedure == &kernel::locations::nextCharacter) {
						const auto selection(caret.selectedRegion());
						const kernel::Position destination((direction == Direction::forward()) ? *boost::const_end(selection) : *boost::const_begin(selection));
						return caret.moveTo(graphics::font::TextHit<kernel::Position>::leading(destination)), true;
					}
					return false;
				}

				template<typename ProcedureSignature>
				inline void scrollTextViewer(viewer::TextViewer&, ProcedureSignature, Direction, long) {
				}
				inline void scrollTextViewer(viewer::TextViewer& target, viewer::VisualDestinationProxy(*procedure)(const viewer::VisualPoint&, Direction, Index), Direction direction, long n) {
					// TODO: consider the numeric prefix.
					if(procedure == &viewer::locations::nextPage) {
						graphics::font::TextViewport::SignedScrollOffset offset = (direction == Direction::forward()) ? n : -n;
						if(offset != 0) {
							presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::SignedScrollOffset> delta;
							delta.bpd() = offset;
							delta.ipd() = 0;
							target.textArea()->viewport()->scroll(delta);
						}
					}
				}

				template<typename PointType>
				inline void moveCaret(viewer::Caret& caret, kernel::Position(*procedure)(const PointType&), Index, bool extend) {
					const auto h(viewer::TextHit::leading((*procedure)(caret)));
					if(!extend)
						caret.moveTo(h);
					else
						caret.extendSelectionTo(h);
				}
				template<typename PointType>
				inline void moveCaret(viewer::Caret& caret, kernel::Position(*procedure)(const PointType&, Direction, Index), Direction direction, Index n, bool extend) {
					const auto h(viewer::TextHit::leading((*procedure)(caret, direction, n)));
					if(!extend)
						caret.moveTo(h);
					else
						caret.extendSelectionTo(h);
				}
				inline void moveCaret(viewer::Caret& caret, boost::optional<kernel::Position>(*procedure)(const kernel::locations::PointProxy&, Direction, Index), Direction direction, Index n, bool extend) {
					if(const boost::optional<kernel::Position> destination = (*procedure)(caret, direction, n)) {
						const auto h(viewer::TextHit::leading(boost::get(destination)));
						if(!extend)
							caret.moveTo(h);
						else
							caret.extendSelectionTo(h);
					}
				}
				template<typename PointType>
				inline void moveCaret(viewer::Caret& caret, kernel::Position(*procedure)(const PointType&, Direction, kernel::locations::CharacterUnit, Index), Direction direction, Index n, bool extend) {
					const auto h(viewer::TextHit::leading((*procedure)(caret, direction, kernel::locations::GRAPHEME_CLUSTER, n)));
					if(!extend)
						caret.moveTo(h);
					else
						caret.extendSelectionTo(h);
				}
				inline void moveCaret(viewer::Caret& caret, viewer::VisualDestinationProxy(*procedure)(const viewer::locations::PointProxy&, Direction, Index), Direction direction, Index n, bool extend) {
					if(!extend)
						caret.moveTo((*procedure)(caret, direction, n));
					else
						caret.extendSelectionTo((*procedure)(caret, direction, n));
				}
				inline void moveCaret(viewer::Caret& caret, viewer::VisualDestinationProxy(*procedure)(const viewer::VisualPoint&, Direction, Index), Direction direction, Index n, bool extend) {
					if(!extend)
						caret.moveTo((*procedure)(caret, direction, n));
					else
						caret.extendSelectionTo((*procedure)(caret, direction, n));
				}
			}

			// explicit instantiations
			template class CaretMovementCommand<kernel::Position(const kernel::locations::PointProxy&, Direction, Index)>;	// next(Line|Word|WordEnd)
			template class CaretMovementCommand<boost::optional<kernel::Position>(const kernel::locations::PointProxy&, Direction, Index)>;	// nextBookmark
			template class CaretMovementCommand<kernel::Position(const kernel::locations::PointProxy&, Direction, kernel::locations::CharacterUnit, Index)>;	// nextCharacter
			template class CaretMovementCommand<viewer::VisualDestinationProxy(const viewer::locations::PointProxy&, Direction, Index)>;	// nextPage
			template class CaretMovementCommand<viewer::VisualDestinationProxy(const viewer::VisualPoint&, Direction, Index)>;	// nextVisualLine

			/**
			 * Moves the caret or extends the selection.
			 * @return true if succeeded
			 */
			template<typename ProcedureSignature>
			bool CaretMovementCommand<ProcedureSignature>::perform() {
				const NumericPrefix n = numericPrefix();
				endIncrementalSearch(*viewer::document(target()));
				if(n == 0)
					return true;

				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						if(!extends_) {
							if(selectCompletionProposal(target(), procedure_, direction_, n))
								return true;
							caret->endRectangleSelection();
							if(!viewer::isSelectionEmpty(*caret)) {	// just clear the selection
								if(moveToBoundOfSelection(*caret, procedure_, direction_))
									return true;
							}
						}

						scrollTextViewer(target(), procedure_, direction_, n);
						moveCaret(*caret, procedure_, direction_, n, extends_);
						return true;
					}
				}
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure A function gives a motion
			 * @param extendSelection Set @c true to extend the selection
			 * @throw NullPointerException @a procedure is @c null
			 */
			template<typename ProcedureSignature>
			CaretMovementToDefinedPositionCommand<ProcedureSignature>::CaretMovementToDefinedPositionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure, bool extendSelection /* = false */)
					: Command(viewer), procedure_(procedure), extends_(extendSelection) {
				if(procedure == nullptr)
					throw NullPointerException("procedure");
			}

			namespace {
				inline viewer::TextHit makeNormalHit(const kernel::Position& p) BOOST_NOEXCEPT {
					return viewer::TextHit::leading(p);
				}
				inline viewer::TextHit makeNormalHit(const viewer::TextHit& h) BOOST_NOEXCEPT {
					return h;
				}
			}

			// explicit instantiations
			template class CaretMovementToDefinedPositionCommand<kernel::Position(const kernel::locations::PointProxy&)>;	// (beginning|end)Of(Document|Line)
			template class CaretMovementToDefinedPositionCommand<kernel::Position(const viewer::locations::PointProxy&)>;	// contextual(Beginning|End)OfLine, (beginning|end|contextualBeginning|contextualEnd)OfVisualLine, (first|last)PrintableCharacterOf(Visual)?Line

			/**
			 * Moves the caret or extends the selection.
			 * @return true if succeeded
			 */
			template<typename ProcedureSignature>
			bool CaretMovementToDefinedPositionCommand<ProcedureSignature>::perform() {
				endIncrementalSearch(*viewer::document(target()));
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						const auto h(makeNormalHit((*procedure_)(*caret)));
						if(!extends_)
							caret->moveTo(h);
						else
							caret->extendSelectionTo(h);
						return true;
					}
				}
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			EntireDocumentSelectionCreationCommand::EntireDocumentSelectionCreationCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * @see Command#perform
			 * @return @c true if succeeded
			 */
			bool EntireDocumentSelectionCreationCommand::perform() {
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						endIncrementalSearch(*viewer::document(target()));
						caret->endRectangleSelection();
						caret->select(viewer::SelectedRegion(viewer::document(target())->accessibleRegion()));
						return true;
					}
				}
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure A function gives a motion
			 * @param direction The direction of motion
			 * @throw NullPointerException @a procedure is @c null
			 */
			template<typename ProcedureSignature>
			RowSelectionExtensionCommand<ProcedureSignature>::RowSelectionExtensionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure, Direction direction)
					: Command(viewer), procedure_(procedure), direction_(direction) {
				if(procedure == nullptr)
					throw NullPointerException("procedure");
			}

			/**
			 * @see Command#perform
			 * @return true
			 */
			template<typename ProcedureSignature>
			bool RowSelectionExtensionCommand<ProcedureSignature>::perform() {
				viewer::utils::closeCompletionProposalsPopup(target());
				endIncrementalSearch(*viewer::document(target()));
			
				const auto caret(target().textArea()->caret());
				if(viewer::isSelectionEmpty(*caret) && !caret->isSelectionRectangle())
					caret->beginRectangleSelection();
				return CaretMovementCommand<ProcedureSignature>(target(), procedure_, direction_, true).setNumericPrefix(numericPrefix())();
			}

			// explicit instantiations
			template class RowSelectionExtensionCommand<kernel::Position(const kernel::locations::PointProxy&, Direction, Index)>;	// next(Line|Word|WordEnd)
			template class RowSelectionExtensionCommand<boost::optional<kernel::Position>(const kernel::locations::PointProxy&, Direction, Index)>;	// nextBookmark
			template class RowSelectionExtensionCommand<kernel::Position(const kernel::locations::PointProxy&, Direction, kernel::locations::CharacterUnit, Index)>;	// nextCharacter
			template class RowSelectionExtensionCommand<viewer::VisualDestinationProxy(const viewer::locations::PointProxy&, Direction, Index)>;	// nextPage
			template class RowSelectionExtensionCommand<viewer::VisualDestinationProxy(const viewer::VisualPoint&, Direction, Index)>;	// nextVisualLine

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure A function gives a motion
			 * @throw NullPointerException @a procedure is @c null
			 * 
			 */
			template<typename ProcedureSignature>
			RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature>::RowSelectionExtensionToDefinedPositionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure) : Command(viewer), procedure_(procedure) {
				if(procedure == nullptr)
					throw NullPointerException("procedure");
			}

			// explicit instantiations
			template class RowSelectionExtensionToDefinedPositionCommand<kernel::Position(const kernel::locations::PointProxy&)>;	// (beginning|end)Of(Document|Line)
			template class RowSelectionExtensionToDefinedPositionCommand<viewer::TextHit(const viewer::locations::PointProxy&)>;	// contextual(Beginning|End)OfLine, (beginning|end|contextualBeginning|contextualEnd)OfVisualLine, (first|last)PrintableCharacterOf(Visual)?Line

			/**
			 * Moves the caret or extends the selection.
			 * @return true
			 */
			template<typename ProcedureSignature>
			bool RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature>::perform() {
				viewer::utils::closeCompletionProposalsPopup(target());
				endIncrementalSearch(*viewer::document(target()));

				const auto caret(target().textArea()->caret());
				if(viewer::isSelectionEmpty(*caret) && !caret->isSelectionRectangle())
					caret->beginRectangleSelection();
				return CaretMovementToDefinedPositionCommand<ProcedureSignature>(target(), procedure_, true).setNumericPrefix(numericPrefix())();
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			WordSelectionCreationCommand::WordSelectionCreationCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @return true
			 */
			bool WordSelectionCreationCommand::perform() {
				endIncrementalSearch(*viewer::document(target()));
				target().textArea()->caret()->endRectangleSelection();
				selectWord(*target().textArea()->caret());
				return 0;
			}
		}
	}
}
