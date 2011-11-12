/**
 * @file caret.hpp
 * Defines the classes in the point-hierarchy related to visual presentation.
 * @author exeal
 * @date 2003-2008 (was point.hpp)
 * @date 2008 (separated from point.hpp)
 * @date 2009-2011
 */

#ifndef ASCENSION_CARET_HPP
#define ASCENSION_CARET_HPP
//#include <ascension/graphics/rendering.hpp>				// graphics.VisualLinesListener
#include <ascension/corelib/text/identifier-syntax.hpp>	// text.IdentifierSyntax
#include <ascension/viewer/caret-observers.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <ascension/viewer/viewer-observers.hpp>
#include <ascension/viewer/visual-point.hpp>
#ifdef ASCENSION_COMPILER_GCC
#	include <unknwn.h>	// IUnknown, OLESTR, ...
#endif // ASCENSION_COMPILER_GCC
#include <objidl.h>		// IDataObject

namespace ascension {

	namespace viewers {

		namespace utils {
			HRESULT createTextObjectForSelectedString(const Caret& caret, bool rtf, IDataObject*& content);
			std::pair<HRESULT, String> getTextFromDataObject(IDataObject& data, bool* rectangle = 0);
		}

		class VirtualBox;

		/// Clipboard Win32 API was failed.
		class ClipboardException : public std::runtime_error {
		public:
			explicit ClipboardException(HRESULT hr);
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

			// constructor
			explicit Caret(TextViewer& viewer, const kernel::Position& position = kernel::Position(0, 0));
			~Caret();
			// listeners
			void addListener(CaretListener& listener);
			void addCharacterInputListener(CharacterInputListener& listener);
			void addStateListener(CaretStateListener& listener);
			void removeListener(CaretListener& listener);
			void removeCharacterInputListener(CharacterInputListener& listener);
			void removeStateListener(CaretStateListener& listener);
			// attributes : the anchor and the caret
			const VisualPoint& anchor() const /*throw()*/;
			const VisualPoint& beginning() const /*throw()*/;
			Caret& enableAutoShow(bool enable = true) /*throw()*/;
			const VisualPoint& end() const /*throw()*/;
			bool isAutoShowEnabled() const /*throw()*/;
			// attributes : selection
			const VirtualBox& boxForRectangleSelection() const;
			bool isSelectionRectangle() const /*throw()*/;
			kernel::Region selectedRegion() const /*throw()*/;
			// attribute : shape
			void setShaper(std::tr1::shared_ptr<CaretShaper> shaper) /*throw()*/;
			// attributes : character input
			bool isOvertypeMode() const /*throw()*/;
			Caret& setOvertypeMode(bool overtype) /*throw()*/;
			// attributes : clipboard
			bool canPaste(bool useKillRing) const /*throw()*/;
#ifdef ASCENSION_OS_WINDOWS
			LCID clipboardLocale() const /*throw()*/;
			LCID setClipboardLocale(LCID newLocale);
#endif // SCENSION_OS_WINDOWS
			// attributes : matched braces
			const std::pair<kernel::Position, kernel::Position>& matchBrackets() const /*throw()*/;
			MatchBracketsTrackingMode matchBracketsTrackingMode() const /*throw()*/;
			Caret& trackMatchBrackets(MatchBracketsTrackingMode mode);
			// selection manipulations
			void beginRectangleSelection();
			void clearSelection();
			void endRectangleSelection();
			void extendSelection(const kernel::Position& to);
			void extendSelection(const VerticalDestinationProxy& to);
			void paste(bool useKillRing);
			void replaceSelection(const StringPiece& text, bool rectangleInsertion = false);
			void select(const kernel::Region& region);
			void select(const kernel::Position& anchor, const kernel::Position& caret);
			// text manipulation
			bool inputCharacter(CodePoint cp, bool validateSequence = true, bool blockControls = true);
			// visualization updates
			void resetVisualization();
			void updateLocation();

		private:
			void adjustInputMethodCompositionWindow();
			void checkMatchBrackets();
			void fireCaretMoved(const kernel::Region& oldRegion);
			void internalExtendSelection(void (*algorithm)(void));
			void prechangeDocument();
			void update(const kernel::DocumentChange& change);
			void updateVisualAttributes();
			// VisualPoint
			void aboutToMove(kernel::Position& to);
			void moved(const kernel::Position& from) /*throw()*/;
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
				SelectionAnchor(TextViewer& viewer, const kernel::Position& position) /*throw()*/ :
					VisualPoint(viewer, position), positionBeforeUpdate_() {adaptToDocument(false);}
				void beginInternalUpdate(const kernel::DocumentChange& change) /*throw()*/ {
					assert(!isInternalUpdating()); positionBeforeUpdate_ = position();
					adaptToDocument(true); update(change); adaptToDocument(false);}
				void endInternalUpdate() /*throw()*/ {
					assert(isInternalUpdating()); positionBeforeUpdate_ = kernel::Position();}
				bool isInternalUpdating() const /*throw()*/ {return positionBeforeUpdate_ != kernel::Position();}
				const kernel::Position& positionBeforeInternalUpdate() const /*throw()*/ {
					assert(isInternalUpdating()); return positionBeforeUpdate_;}
			private:
				using kernel::Point::adaptToDocument;
				kernel::Position positionBeforeUpdate_;
			} * anchor_;
#ifdef ASCENSION_OS_WINDOWS
			LCID clipboardLocale_;
#endif // SCENSION_OS_WIND
			detail::Listeners<CaretListener> listeners_;
			detail::Listeners<CharacterInputListener> characterInputListeners_;
			detail::Listeners<CaretStateListener> stateListeners_;
			bool overtypeMode_;
			bool autoShow_;		// true if show itself when movements
			MatchBracketsTrackingMode matchBracketsTrackingMode_;
			std::tr1::shared_ptr<CaretShaper> shaper_;
			struct Shape {
				std::auto_ptr<graphics::Image> image;
				presentation::ReadingDirection readingDirection;
				graphics::Scalar measure;
				Shape() /*throw()*/;
			} shapeCache_;
			struct Context {
				bool yanking;			// true when right after pasted by using clipboard ring, and waiting for next cycle of ring
				bool leaveAnchorNext;	// true if should leave the anchor at the next movement
				bool leadingAnchor;		// true if in anchor_->moveTo calling, and ignore pointMoved
				std::auto_ptr<VirtualBox> selectedRectangle;	// for rectangular selection. null when the selection is linear
				bool typing;			// true when inputCharacter called (see prechangeDocument)
				bool inputMethodCompositionActivated, inputMethodComposingCharacter;
				kernel::Position lastTypedPosition;	// the position the caret input character previously or INVALID_POSITION
				kernel::Region regionBeforeMoved;
				std::pair<kernel::Position, kernel::Position> matchBrackets;	// matched brackets' positions. Position() for none
				Context() /*throw()*/;
			} context_;
		};

		// free functions related to selection of Caret class
		void copySelection(Caret& caret, bool useKillRing);
		void cutSelection(Caret& caret, bool useKillRing);
		bool isPointOverSelection(const Caret& caret, const graphics::NativePoint& p);
		bool isSelectionEmpty(const Caret& caret) /*throw()*/;
		bool selectedRangeOnLine(const Caret& caret, length_t line, Range<length_t>& range);
		bool selectedRangeOnVisualLine(const Caret& caret, length_t line, length_t subline, Range<length_t>& range);
		String selectedString(const Caret& caret, text::Newline newline = text::NLF_RAW_VALUE);
		std::basic_ostream<Char>& selectedString(const Caret& caret,
			std::basic_ostream<Char>& out, text::Newline newline = text::NLF_RAW_VALUE);
		void selectWord(Caret& caret);

		// free functions change the document by using Caret class
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


		// inline implementations /////////////////////////////////////////////////////////////////

		/// Returns the anchor of the selection.
		inline const VisualPoint& Caret::anchor() const /*throw()*/ {return *anchor_;}

		/// Returns the neighborhood to the beginning of the document among the anchor and this point.
		inline const VisualPoint& Caret::beginning() const /*throw()*/ {
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
		inline Caret& Caret::enableAutoShow(bool enable /* = true */) /*throw()*/ {
			autoShow_ = enable;
			return *this;
		}

		/// Returns the neighborhood to the end of the document among the anchor and this point.
		inline const VisualPoint& Caret::end() const /*throw()*/ {
			return std::max(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));
		}

		/// Returns @c true if the point will be shown automatically when moved. Default is @c true.
		inline bool Caret::isAutoShowEnabled() const /*throw()*/ {return autoShow_;}

		/// Returns @c true if the caret is in overtype mode.
		inline bool Caret::isOvertypeMode() const /*throw()*/ {return overtypeMode_;}

		/// Returns @c true if the selection is rectangle.
		inline bool Caret::isSelectionRectangle() const /*throw()*/ {
			return context_.selectedRectangle.get() != 0;
		}

		/// キャレット位置の括弧と対応する括弧の位置を返す (@a first が対括弧、@a second がキャレット周辺の括弧)
		inline const std::pair<kernel::Position, kernel::Position>& Caret::matchBrackets() const /*throw()*/ {return context_.matchBrackets;}

		/// Returns the matched braces tracking mode.
		inline Caret::MatchBracketsTrackingMode Caret::matchBracketsTrackingMode() const /*throw()*/ {return matchBracketsTrackingMode_;}

		/**
		 * Selects the specified region.
		 * @param region the region. @a pos1 member is the anchor, @a pos2 member is the caret
		 */
		inline void Caret::select(const kernel::Region& region) {select(region.first, region.second);}

		/// Returns the selected region.
		inline kernel::Region Caret::selectedRegion() const /*throw()*/ {return kernel::Region(*anchor_, position());}

		/**
		 * Sets the caret shaper.
		 * @param shaper The new caret shaper
		 */
		inline void Caret::setShaper(std::tr1::shared_ptr<CaretShaper> shaper) {shaper_ = shaper;}

		/**
		 * Tracks the match bracket.
		 * @param mode the tracking mode
		 * @return this caret
		 */
		inline Caret& Caret::trackMatchBrackets(MatchBracketsTrackingMode mode) /*throw()*/ {
			if(mode != matchBracketsTrackingMode_) {
				matchBracketsTrackingMode_ = mode;
				checkMatchBrackets();
			}
			return *this;
		}

		/// Returns @c true if the selection of the given caret is empty.
		inline bool isSelectionEmpty(const Caret& caret) /*throw()*/ {return caret.selectedRegion().isEmpty();}

		/**
		 * Returns the selected text string.
		 * @param caret The caret gives a selection
		 * @param newline The newline representation for multiline selection. if the selection is
		 *                rectangular, this value is ignored and the document's newline is used instead
		 * @return The text string
		 */
		inline String selectedString(const Caret& caret, text::Newline newline /* = NLF_RAW_VALUE */) {
			std::basic_ostringstream<Char> ss;
			selectedString(caret, ss, newline);
			return ss.str();
		}

	} // namespace viewers
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
