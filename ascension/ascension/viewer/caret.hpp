/**
 * @file caret.hpp
 * Defines the classes in the point-hierarchy related to visual presentation.
 * @author exeal
 * @date 2003-2008 was point.hpp
 * @date 2008 separated from point.hpp
 * @date 2009-2014, 2016
 */

#ifndef ASCENSION_CARET_HPP
#define ASCENSION_CARET_HPP
#include <ascension/corelib/signals.hpp>
#include <ascension/corelib/text/newline.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <ascension/viewer/caret-painter.hpp>
#include <ascension/viewer/point-proxy.hpp>
#include <ascension/viewer/selected-region.hpp>
#include <ascension/viewer/visual-point.hpp>
#include <ascension/viewer/detail/input-method.hpp>
#include <ascension/viewer/widgetapi/drag-and-drop.hpp>	// widgetapi.MimeData, widgetapi.MimeDataFormats

namespace ascension {
	namespace graphics {
		namespace font {
			class UseCalculatedLayoutTag;
			struct VisualLine;
		}
	}

	namespace viewer {
		class Caret;

		namespace utils {
			InterprocessData createInterprocessDataForSelectedString(const Caret& caret, bool rtf);
			InterprocessDataFormats::Format rectangleTextMimeDataFormat();
		}

		class VirtualBox;

		/// Clipboard Win32 API was failed.
		class ClipboardException : public std::system_error {
		public:
#if BOOST_OS_WINDOWS
			typedef HRESULT NativeErrorCode;
#else
			typedef void NativeErrorCode;
#endif
		public:
			explicit ClipboardException(NativeErrorCode);
		};
	}

	namespace viewer {
		class CaretPainter;

		// documentation is caret.cpp
		class Caret : public VisualPoint,
			public kernel::DocumentListener, public detail::InputMethodEventHandler, public detail::InputMethodQueryEventHandler {
		public:
			/// Mode of tracking match brackets.
			enum MatchBracketsTrackingMode {
				DONT_TRACK,						///< Does not track.
				TRACK_FOR_FORWARD_CHARACTER,	///< Tracks the bracket matches forward character.
				TRACK_FOR_SURROUND_CHARACTERS	///< Tracks the bracket matches backward character.
			};

		public:
			explicit Caret(kernel::Document& document, const TextHit& position = TextHit::leading(kernel::Position::zero()));
			explicit Caret(const graphics::font::TextHit<kernel::Point>& other);
			explicit Caret(const VisualPoint& other);
			explicit Caret(TextArea& textArea, const TextHit& position = TextHit::leading(kernel::Position::zero()));
			~Caret();

			/// @name Installation
			/// @{
			void install(TextArea& textArea) override;
			void uninstall() BOOST_NOEXCEPT override;
			/// @}

			/// @name The Anchor and The Caret
			/// @{
			locations::PointProxy anchor() const BOOST_NOEXCEPT;
			locations::PointProxy beginning() const BOOST_NOEXCEPT;
			Caret& enableAutoShow(bool enable = true) BOOST_NOEXCEPT;
			locations::PointProxy end() const BOOST_NOEXCEPT;
			bool isAutoShowEnabled() const BOOST_NOEXCEPT;
			/// @}

			/// @name The Selection
			/// @{
			const VirtualBox& boxForRectangleSelection() const;
			bool isSelectionRectangle() const BOOST_NOEXCEPT;
			SelectedRegion selectedRegion() const BOOST_NOEXCEPT;
			/// @}

			/// @name Character Input
			/// @{
			bool isOvertypeMode() const BOOST_NOEXCEPT;
			Caret& setOvertypeMode(bool overtype) BOOST_NOEXCEPT;
			/// @}

			/// @name Clipboard
			/// @{
			bool canPaste(bool useKillRing) const BOOST_NOEXCEPT;
#if BOOST_OS_WINDOWS
			LCID clipboardLocale() const BOOST_NOEXCEPT;
			LCID setClipboardLocale(LCID newLocale);
#endif // BOOST_OS_WINDOWS
			/// @}

			/// @name The Matched Braces/Brackets
			/// @{
			const boost::optional<std::pair<kernel::Position, kernel::Position>>& matchBrackets() const BOOST_NOEXCEPT;
			MatchBracketsTrackingMode matchBracketsTrackingMode() const BOOST_NOEXCEPT;
			Caret& trackMatchBrackets(MatchBracketsTrackingMode mode);
			/// @}

			/// @name Selection Manipulations
			/// @{
			void beginRectangleSelection();
			void clearSelection();
			void endRectangleSelection();
			void extendSelectionTo(const TextHit& to);
			void extendSelectionTo(const VisualDestinationProxy& to);
			void paste(bool useKillRing);
			void replaceSelection(const StringPiece& text, bool rectangleInsertion = false);
			void select(const SelectedRegion& region);
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_MEMBER_FUNCTION(
				(void), select, tag,
				(required
					(anchor, (const kernel::Position&))
					(caret, (const TextHit&)))) {_select(anchor, caret);}
#else
			void select(const kernel::Position& anchor, const TextHit& caret);
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			/// @}

			/// @name Text Manipulation
			/// @{
			bool inputCharacter(CodePoint c, bool validateSequence = true, bool blockControls = true);
			/// @}

			/// @name Visibility and Rendering
			/// @{
			void hide() BOOST_NOEXCEPT;
			void paint(graphics::PaintContext& context) const;
			void setPainter(std::unique_ptr<CaretPainter> newPainter);
			void show() BOOST_NOEXCEPT;
			BOOST_CONSTEXPR bool shows() const BOOST_NOEXCEPT;
			/// @}
#if 0
			/// @name Visualization Updates
			/// @{
			void resetVisualization();
			void updateLocation();
			/// @}
#endif
			/// @name Signals
			/// @{
			typedef boost::signals2::signal<void(const Caret&, CodePoint)> CharacterInputSignal;
			/// Describes types of @c InputModeChangedSignal.
			enum InputModeChangedSignalType {
				INPUT_LOCALE,				///< The text viewer's input locale had been changed.
				INPUT_METHOD_OPEN_STATUS,	///< The text viewer's input method open status had been changed.
				OVERTYPE_MODE				///< The overtype mode of the caret is changed.
			};
			typedef boost::signals2::signal<void(const Caret&, InputModeChangedSignalType)> InputModeChangedSignal;
			typedef boost::signals2::signal<void(const Caret&,
				const boost::optional<std::pair<kernel::Position, kernel::Position>>& oldPair, bool outsideOfView)> MatchBracketsChangedSignal;
			typedef boost::signals2::signal<void(const Caret&, const SelectedRegion&)> MotionSignal;
			typedef boost::signals2::signal<void(const Caret&)> SelectionShapeChangedSignal;	// bad naming :(
			SignalConnector<CharacterInputSignal> characterInputSignal() BOOST_NOEXCEPT;
			SignalConnector<InputModeChangedSignal> inputModeChangedSignal() BOOST_NOEXCEPT;
			SignalConnector<MatchBracketsChangedSignal> matchBracketsChangedSignal() BOOST_NOEXCEPT;
			SignalConnector<MotionSignal> motionSignal() BOOST_NOEXCEPT;
			SignalConnector<SelectionShapeChangedSignal> selectionShapeChangedSignal() BOOST_NOEXCEPT;
			/// @}

		private:
			void _select(const kernel::Position& a, const TextHit& c);
			void adjustInputMethodCompositionWindow();
			bool canPastePlatformData() const;
			void checkMatchBrackets();
			void documentAboutToBeChanged(const kernel::DocumentChange& change) override;
			void documentChanged(const kernel::DocumentChange& change) override;
			void fireCaretMoved(const SelectedRegion& regionBeforeMotion);
			void internalExtendSelection(void (*algorithm)(void));
			void prechangeDocument();
			SelectedRegion selection() const BOOST_NOEXCEPT;
			void updateVisualAttributes();
			// VisualPoint
			void aboutToMove(TextHit& to) override;
			void moved(const TextHit& from) BOOST_NOEXCEPT override;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			LRESULT handleInputEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
			void onImeComposition(WPARAM wp, LPARAM lp, bool& consumed);
			LRESULT onImeRequest(WPARAM command, LPARAM lp, bool& consumed);
#endif
			// VisualPoint.MotionSignal
			void pointMoved(const VisualPoint& self, const TextHit& oldHit);
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document, const kernel::DocumentChange& change) override;
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change) override;
			// detail.InputMethodEventHandler
			void commitString(widgetapi::event::InputMethodEvent& event) BOOST_NOEXCEPT override;
			void preeditChanged(widgetapi::event::InputMethodEvent& event) BOOST_NOEXCEPT override;
			void preeditEnded() BOOST_NOEXCEPT override;
			void preeditStarted() BOOST_NOEXCEPT override;
			// detail.InputMethodQueryEventHandler
			std::pair<const StringPiece, StringPiece::const_iterator> querySurroundingText() const BOOST_NOEXCEPT override;
		private:
			boost::optional<TextHit> anchor_;
			std::unique_ptr<CaretPainter> painter_;
#if BOOST_OS_WINDOWS
			LCID clipboardLocale_;
#endif // BOOST_OS_WINDOWS
			CharacterInputSignal characterInputSignal_;
			InputModeChangedSignal inputModeChangedSignal_;
			MatchBracketsChangedSignal matchBracketsChangedSignal_;
			MotionSignal motionSignal_;
			SelectionShapeChangedSignal selectionShapeChangedSignal_;
			boost::signals2::connection anchorMotionSignalConnection_;
#ifdef ASCENSION_USE_SYSTEM_CARET
			boost::signals2::connection viewportResizedConnection_, viewportScrolledConnection_;
#endif // ASCENSION_USE_SYSTEM_CARET
			bool overtypeMode_;
			bool autoShow_;		// true if show itself when movements
			MatchBracketsTrackingMode matchBracketsTrackingMode_;
			struct Shape {
				std::unique_ptr<graphics::Image> image;
				boost::geometry::model::d2::point_xy<std::uint16_t> alignmentPoint;
				Shape() BOOST_NOEXCEPT;
			} shapeCache_;
			struct Context {
				bool yanking;			// true when right after pasted by using clipboard ring, and waiting for next cycle of ring
				std::unique_ptr<VirtualBox> selectedRectangle;	// for rectangular selection. null when the selection is linear
				bool typing;			// true when inputCharacter called (see prechangeDocument)
				boost::optional<TextHit> anchorDestination;
				bool inputMethodCompositionActivated, inputMethodComposingCharacter;
				boost::optional<kernel::Position> lastTypedPosition;	// the position the caret input character previously
				boost::optional<std::pair<kernel::Position, kernel::Position>> matchBrackets;	// matched brackets' positions. boost.none for none
				Context() BOOST_NOEXCEPT;
			} context_;
		};

		/// @defgroup functions_related_to_selection Free Functions Related-to Selection of @c Caret
		/// @{
		/**
		 * Copies the selected content to the clipboard.
		 * If the caret does not have a selection, this function does nothing.
		 * @param caret The caret gives the selection
		 * @param useKillRing Set @c true to send to the kill ring, not only the system clipboard
		 * @throw ClipboardException The clipboard operation failed
		 * @throw std#bad_alloc Internal memory allocation failed
		 */
		void copySelection(Caret& caret, bool useKillRing);
		/**
		 * Copies and deletes the selected text. If the selection is empty, this function does nothing.
		 * @param caret The caret gives the selection
		 * @param useKillRing Set @c true to send also the kill ring
		 * @return false if the change was interrupted
		 * @throw ClipboardException The clipboard operation failed
		 * @throw std#bad_alloc Internal memory allocation failed
		 * @throw ... Any exceptions @c Document#replace throws
		 */
		void cutSelection(Caret& caret, bool useKillRing);
		bool isPointOverSelection(const Caret& caret, const graphics::Point& p);
		bool isSelectionEmpty(const Caret& caret) BOOST_NOEXCEPT;
		boost::optional<boost::integer_range<Index>> selectedRangeOnLine(const Caret& caret, Index line);
		boost::optional<boost::integer_range<Index>> selectedRangeOnVisualLine(const Caret& caret, const graphics::font::VisualLine& line);
		boost::optional<boost::integer_range<Index>> selectedRangeOnVisualLine(Caret& caret,
			const graphics::font::VisualLine& line, const graphics::font::UseCalculatedLayoutTag&);
		String selectedString(const Caret& caret, const text::Newline& newline = text::Newline::USE_INTRINSIC_VALUE);
		std::basic_ostream<Char>& selectedString(const Caret& caret,
			std::basic_ostream<Char>& out, const text::Newline& newline = text::Newline::USE_INTRINSIC_VALUE);
		void selectWord(Caret& caret);
		/// @}

		/// @defgroup functions_change_document_by_using_caret Free Functions Change the Document by using @c Caret
		/// @{
		void breakLine(Caret& at, bool inheritIndent, std::size_t newlines /* = 1 */);
		void eraseSelection(Caret& caret);
		void insertRectangle(Caret& caret, const String& text);
		void indentBySpaces(Caret& caret, bool rectangle, long level = 1);
		void indentByTabs(Caret& caret, bool rectangle, long level = 1);
		void newLine(Caret& caret, std::size_t newlines = 1);
		bool transposeCharacters(Caret& caret);
		bool transposeLines(Caret& caret);
//		bool transposeParagraphs(Caret& caret);
//		bool transposeSentences(Caret& caret);
		bool transposeWords(Caret& caret);
		/// @}


		// inline implementations /////////////////////////////////////////////////////////////////

		/// Returns the anchor of the selection.
		inline locations::PointProxy Caret::anchor() const BOOST_NOEXCEPT {
			return locations::PointProxy(textArea(), boost::get_optional_value_or(anchor_, hit()));
		}

		/// Returns the neighborhood to the beginning of the document among the anchor and this point.
		inline locations::PointProxy Caret::beginning() const BOOST_NOEXCEPT {
			return locations::PointProxy(textArea(), std::min(hit(), anchor().hit));
		}

		/**
		 * Returns the rectangle selection.
		 * @return the virtual box represents the rectangle selection
		 * @throw IllegalStateException the selection is not rectangle.
		 */
		inline const VirtualBox& Caret::boxForRectangleSelection() const {
			if(!isSelectionRectangle())
				throw IllegalStateException("The selection is not rectangle.");
			 return *context_.selectedRectangle;
		}

		/**
		 * Sets the new auto-show mode.
		 * @param enable set @c true to enable the mode
		 * @return this caret
		 * @see #isAutoShowEnabled
		 */
		inline Caret& Caret::enableAutoShow(bool enable /* = true */) BOOST_NOEXCEPT {
			autoShow_ = enable;
			return *this;
		}

		/// Returns the neighborhood to the end of the document among the anchor and this point.
		inline locations::PointProxy Caret::end() const BOOST_NOEXCEPT {
			return locations::PointProxy(textArea(), std::max(hit(), anchor().hit));
		}

		/// Returns @c true if the point will be shown automatically when moved. Default is @c true.
		inline bool Caret::isAutoShowEnabled() const BOOST_NOEXCEPT {return autoShow_;}

		/// Returns @c true if the caret is in overtype mode.
		inline bool Caret::isOvertypeMode() const BOOST_NOEXCEPT {return overtypeMode_;}

		/// Returns @c true if the selection is rectangle.
		inline bool Caret::isSelectionRectangle() const BOOST_NOEXCEPT {
			return context_.selectedRectangle.get() != nullptr;
		}

		/// キャレット位置の括弧と対応する括弧の位置を返す (@a first が対括弧、@a second がキャレット周辺の括弧)
		inline const boost::optional<std::pair<kernel::Position, kernel::Position>>& Caret::matchBrackets() const BOOST_NOEXCEPT {
			return context_.matchBrackets;
		}

		/// Returns the matched braces tracking mode.
		inline Caret::MatchBracketsTrackingMode Caret::matchBracketsTrackingMode() const BOOST_NOEXCEPT {
			return matchBracketsTrackingMode_;
		}

		/**
		 * @fn ascension::viewer::Caret::paste
		 * Replaces the selected text by the content of the clipboard.
		 * This method inserts undo boundaries at the beginning and the end.
		 * @note When using the kill-ring, this method may exit in defective condition.
		 * @param useKillRing Set @c true to use the kill ring
		 * @throw ClipboardException The clipboard operation failed
		 * @throw ClipboardException(DV_E_FORMATETC) The current clipboard format is not supported
		 * @throw IllegalStateException @a useKillRing was @c true but the kill-ring was not available
		 * @throw std#bad_alloc Internal memory allocation failed
		 * @throw ... Any exceptions @c kernel#Document#replace throws
		 */

		/// Returns the selected region.
		inline SelectedRegion Caret::selectedRegion() const BOOST_NOEXCEPT {
			return selection();	// kernel.Document is only declared in this header file
		}

		/**
		 * Returns @c true if the caret is shown.
		 * @see #hide, show
		 */
		inline BOOST_CONSTEXPR bool Caret::shows() const BOOST_NOEXCEPT {
			return painter_.get() != nullptr && painter_->shows();
		}

		/**
		 * Tracks the match bracket.
		 * @param mode the tracking mode
		 * @return this caret
		 */
		inline Caret& Caret::trackMatchBrackets(MatchBracketsTrackingMode mode) {
			if(mode != matchBracketsTrackingMode_) {
				matchBracketsTrackingMode_ = mode;
				checkMatchBrackets();
			}
			return *this;
		}

		/// Returns @c true if the selection of the given caret is empty.
		inline bool isSelectionEmpty(const Caret& caret) BOOST_NOEXCEPT {
			return boost::empty(caret.selectedRegion());
		}

		/**
		 * Returns the selected text string.
		 * @param caret The caret gives a selection
		 * @param newline The newline representation for multiline selection. If the selection is
		 *                rectangular, this value is ignored and the document's newline is used instead
		 * @return The text string
		 */
		inline String selectedString(const Caret& caret,
				const text::Newline& newline /* = Newline::USE_INTRINSIC_VALUE */) {
			std::basic_ostringstream<Char> ss;
			selectedString(caret, ss, newline);
			return ss.str();
		}
	} // namespace viewer

	namespace kernel {
		template<>
		struct DocumentAccess<viewer::Caret> {
			static Document& get(viewer::Caret& p) BOOST_NOEXCEPT {
				return p.document();
			}
		};

		template<>
		struct PositionAccess<viewer::Caret> {
			static Position get(viewer::Caret& p) BOOST_NOEXCEPT {
				return viewer::insertionPosition(p);
			}
		};

		template<>
		struct DocumentAccess<const viewer::Caret> {
			static const Document& get(const viewer::Caret& p) BOOST_NOEXCEPT {
				return p.document();
			}
		};

		template<>
		struct PositionAccess<const viewer::Caret> {
			static Position get(const viewer::Caret& p) BOOST_NOEXCEPT {
				return viewer::insertionPosition(p);
			}
		};

		/**
		 * @overload
		 * @note There is no @c offsetInLine for @c viewer#Caret.
		 */
		BOOST_CONSTEXPR inline Index line(const viewer::Caret& caret) BOOST_NOEXCEPT {
			return line(caret.hit().characterIndex());
		}
	}
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
