/**
 * @file caret.hpp
 * Defines the classes in the point-hierarchy related to visual presentation.
 * @author exeal
 * @date 2003-2008 was point.hpp
 * @date 2008 separated from point.hpp
 * @date 2009-2013
 */

#ifndef ASCENSION_CARET_HPP
#define ASCENSION_CARET_HPP
#include <ascension/corelib/text/identifier-syntax.hpp>	// text.IdentifierSyntax
#include <ascension/viewer/caret-observers.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <ascension/viewer/viewer-observers.hpp>
#include <ascension/viewer/visual-point.hpp>
#include <ascension/viewer/widgetapi/drag-and-drop.hpp>
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
#	include <ascension/win32/com/smart-pointer.hpp>	// win32.com.SmartPointer
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

namespace ascension {

	namespace graphics {
		namespace font {
			struct VisualLine;
		}
	}

	namespace viewers {
		namespace utils {
			/**
			 * Creates an MIME data object represents the selected content.
			 * @param caret The caret gives the selection
			 * @param rtf Set @c true if the content is available as Rich Text Format. This feature
			 *            is not implemented yet and the parameter is ignored
			 * @return The MIME data object
			 */
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			win32::com::SmartPointer<widgetapi::NativeMimeData>
#else
			std::unique_ptr<widgetapi::NativeMimeData>
#endif
				createMimeDataForSelectedString(const Caret& caret, bool rtf);
			/**
			 * Returns the text content from the given MIME data.
			 * @param data The MIME data
			 * @return A pair of the following values:
			 * @retval first The text string
			 * @retval second @c true if the content is rectangle
			 * @throw std#bad_alloc
			 */
			std::pair<String, bool> getTextFromMimeData(const widgetapi::NativeMimeData& data);
		}

		class VirtualBox;

		/// Clipboard Win32 API was failed.
		class ClipboardException : public std::system_error {
		public:
#ifdef ASCENSION_OS_WINDOWS
			typedef HRESULT NativeErrorCode;
#else
			typedef void NativeErrorCode;
#endif
		public:
			explicit ClipboardException(NativeErrorCode);
		};
	}

	namespace viewers {
		// documentation is caret.cpp
		class Caret : public VisualPoint, public detail::InputEventHandler,
			public kernel::PointListener, public kernel::DocumentListener,
			public DisplaySizeListener, public ViewportListener {
		public:
			/// Mode of tracking match brackets.
			enum MatchBracketsTrackingMode {
				DONT_TRACK,						///< Does not track.
				TRACK_FOR_FORWARD_CHARACTER,	///< Tracks the bracket matches forward character.
				TRACK_FOR_SURROUND_CHARACTERS	///< Tracks the bracket matches backward character.
			};

		public:
			explicit Caret(TextViewer& viewer, const kernel::Position& position = kernel::Position(0, 0));
			~Caret();

			/// @name Listeners
			/// @{
			void addCharacterInputListener(CharacterInputListener& listener);
			void addInputPropertyListener(InputPropertyListener& listener);
			void addListener(CaretListener& listener);
			void addStateListener(CaretStateListener& listener);
			void removeCharacterInputListener(CharacterInputListener& listener);
			void removeInputPropertyListener(InputPropertyListener& listener);
			void removeListener(CaretListener& listener);
			void removeStateListener(CaretStateListener& listener);
			/// @}

			/// @name The Anchor and The Caret
			/// @{
			const VisualPoint& anchor() const BOOST_NOEXCEPT;
			const VisualPoint& beginning() const BOOST_NOEXCEPT;
			Caret& enableAutoShow(bool enable = true) BOOST_NOEXCEPT;
			const VisualPoint& end() const BOOST_NOEXCEPT;
			bool isAutoShowEnabled() const BOOST_NOEXCEPT;
			/// @}

			/// @name The Selection
			/// @{
			const VirtualBox& boxForRectangleSelection() const;
			bool isSelectionRectangle() const BOOST_NOEXCEPT;
			kernel::Region selectedRegion() const BOOST_NOEXCEPT;
			/// @}

			/// @name The Shape
			/// @{
			void setShaper(std::shared_ptr<CaretShaper> shaper) BOOST_NOEXCEPT;
			/// @}

			/// @name Character Input
			/// @{
			bool isOvertypeMode() const BOOST_NOEXCEPT;
			Caret& setOvertypeMode(bool overtype) BOOST_NOEXCEPT;
			/// @}

			/// @name Clipboard
			/// @{
			bool canPaste(bool useKillRing) const BOOST_NOEXCEPT;
#ifdef ASCENSION_OS_WINDOWS
			LCID clipboardLocale() const BOOST_NOEXCEPT;
			LCID setClipboardLocale(LCID newLocale);
#endif // SCENSION_OS_WINDOWS
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
			void extendSelectionTo(const kernel::Position& to);
			void extendSelectionTo(const VisualDestinationProxy& to);
			void paste(bool useKillRing);
			void replaceSelection(const StringPiece& text, bool rectangleInsertion = false);
			void select(const kernel::Region& region);
			void select(const kernel::Position& anchor, const kernel::Position& caret);
			/// @}

			/// @name Text Manipulation
			/// @{
			bool inputCharacter(CodePoint c, bool validateSequence = true, bool blockControls = true);
			/// @}

			/// @name Visualization Updates
			/// @{
			void resetVisualization();
			void updateLocation();
			/// @}

		private:
			void adjustInputMethodCompositionWindow();
			bool canPastePlatformData() const;
			void checkMatchBrackets();
			void fireCaretMoved(const kernel::Region& oldRegion);
			void internalExtendSelection(void (*algorithm)(void));
			void prechangeDocument();
			void update(const kernel::DocumentChange& change);
			void updateVisualAttributes();
			// VisualPoint
			void aboutToMove(kernel::Position& to);
			void moved(const kernel::Position& from) BOOST_NOEXCEPT;
			// detail.InputEventHandler
			void abortInput();
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			LRESULT handleInputEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
			void onChar(CodePoint c, bool& consumed);
			void onImeComposition(WPARAM wp, LPARAM lp, bool& consumed);
			LRESULT onImeRequest(WPARAM command, LPARAM lp, bool& consumed);
#endif
			// kernel.PointListener
			void pointMoved(const kernel::Point& self, const kernel::Position& oldPosition);
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// DisplaySizeListener
			void viewerDisplaySizeChanged();
			// ViewportListener
			void viewportChanged(bool horizontal, bool vertical);
		private:
			class SelectionAnchor : public VisualPoint {
			public:
				SelectionAnchor(TextViewer& viewer, const kernel::Position& position) BOOST_NOEXCEPT :
					VisualPoint(viewer, position), positionBeforeUpdate_() {adaptToDocument(false);}
				void beginInternalUpdate(const kernel::DocumentChange& change) BOOST_NOEXCEPT {
					assert(!isInternalUpdating()); positionBeforeUpdate_ = position();
					adaptToDocument(true); update(change); adaptToDocument(false);}
				void endInternalUpdate() BOOST_NOEXCEPT {
					assert(isInternalUpdating()); positionBeforeUpdate_ = boost::none;}
				bool isInternalUpdating() const BOOST_NOEXCEPT {return positionBeforeUpdate_;}
				const kernel::Position& positionBeforeInternalUpdate() const BOOST_NOEXCEPT {
					assert(isInternalUpdating()); return *positionBeforeUpdate_;}
			private:
				using kernel::Point::adaptToDocument;
				boost::optional<kernel::Position> positionBeforeUpdate_;
			};
			std::unique_ptr<SelectionAnchor> anchor_;
#ifdef ASCENSION_OS_WINDOWS
			LCID clipboardLocale_;
#endif // SCENSION_OS_WIND
			detail::Listeners<CaretListener> listeners_;
			detail::Listeners<CharacterInputListener> characterInputListeners_;
			detail::Listeners<CaretStateListener> stateListeners_;
			detail::Listeners<InputPropertyListener> inputPropertyListeners_;
			bool overtypeMode_;
			bool autoShow_;		// true if show itself when movements
			MatchBracketsTrackingMode matchBracketsTrackingMode_;
			std::shared_ptr<CaretShaper> shaper_;
			struct Shape {
				std::unique_ptr<graphics::Image> image;
				graphics::geometry::BasicPoint<std::uint16_t> alignmentPoint;
				Shape() BOOST_NOEXCEPT;
			} shapeCache_;
			struct Context {
				bool yanking;			// true when right after pasted by using clipboard ring, and waiting for next cycle of ring
				bool leaveAnchorNext;	// true if should leave the anchor at the next movement
				bool leadingAnchor;		// true if in anchor_->moveTo calling, and ignore pointMoved
				std::unique_ptr<VirtualBox> selectedRectangle;	// for rectangular selection. null when the selection is linear
				bool typing;			// true when inputCharacter called (see prechangeDocument)
				bool inputMethodCompositionActivated, inputMethodComposingCharacter;
				boost::optional<kernel::Position> lastTypedPosition;	// the position the caret input character previously
				boost::optional<kernel::Region> regionBeforeMoved;
				boost::optional<std::pair<kernel::Position, kernel::Position>> matchBrackets;	// matched brackets' positions. boost.none for none
				Context() BOOST_NOEXCEPT;
			} context_;
		};

		/// @defgroup functions_related_to_selection Free Functions Related-to Selection of @c Caret
		/// @{
		void copySelection(Caret& caret, bool useKillRing);
		void cutSelection(Caret& caret, bool useKillRing);
		bool isPointOverSelection(const Caret& caret, const graphics::Point& p);
		bool isSelectionEmpty(const Caret& caret) BOOST_NOEXCEPT;
		boost::optional<boost::integer_range<Index>> selectedRangeOnLine(const Caret& caret, Index line);
		boost::optional<boost::integer_range<Index>> selectedRangeOnVisualLine(const Caret& caret, const graphics::font::VisualLine& line);
		String selectedString(const Caret& caret, const text::Newline& newline = text::Newline::USE_INTRINSIC_VALUE);
		std::basic_ostream<Char>& selectedString(const Caret& caret,
			std::basic_ostream<Char>& out, const text::Newline& newline = text::Newline::USE_INTRINSIC_VALUE);
		void selectWord(Caret& caret);
		/// @}

		/// @defgroup functions_change_document_by_using_caret Free Functions Change the Document by using @c Caret
		/// @{
		void breakLine(Caret& at, bool inheritIndent, std::size_t newlines /* = 1 */);
		void eraseSelection(Caret& caret);
		void insertRectangle(Caret& caret, const Char* first, const Char* last);
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
		inline const VisualPoint& Caret::anchor() const BOOST_NOEXCEPT {return *anchor_;}

		/// Returns the neighborhood to the beginning of the document among the anchor and this point.
		inline const VisualPoint& Caret::beginning() const BOOST_NOEXCEPT {
			return std::min(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));
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
		inline const VisualPoint& Caret::end() const BOOST_NOEXCEPT {
			return std::max(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));
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
		 * Selects the specified region.
		 * @param region the region. @a pos1 member is the anchor, @a pos2 member is the caret
		 */
		inline void Caret::select(const kernel::Region& region) {select(region.first, region.second);}

		/// Returns the selected region.
		inline kernel::Region Caret::selectedRegion() const BOOST_NOEXCEPT {
			return kernel::Region(*anchor_, position());
		}

		/**
		 * Sets the caret shaper.
		 * @param shaper The new caret shaper
		 */
		inline void Caret::setShaper(std::shared_ptr<CaretShaper> shaper) {shaper_ = shaper;}

		/**
		 * Tracks the match bracket.
		 * @param mode the tracking mode
		 * @return this caret
		 */
		inline Caret& Caret::trackMatchBrackets(MatchBracketsTrackingMode mode) BOOST_NOEXCEPT {
			if(mode != matchBracketsTrackingMode_) {
				matchBracketsTrackingMode_ = mode;
				checkMatchBrackets();
			}
			return *this;
		}

		/// Returns @c true if the selection of the given caret is empty.
		inline bool isSelectionEmpty(const Caret& caret) BOOST_NOEXCEPT {
			return caret.selectedRegion().isEmpty();
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

	} // namespace viewers
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
