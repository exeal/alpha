/**
 * @file visual-point.hpp
 * Defines @c VisualPoint class and related stuffs.
 * @author exeal
 * @date 2003-2008 was point.hpp
 * @date 2008 separated from point.hpp
 * @date 2009-2011 was caret.hpp
 * @date 2011-10-02 separated from caret.hpp
 * @date 2011-2015
 */

#ifndef ASCENSION_VISUAL_POINT_HPP
#define ASCENSION_VISUAL_POINT_HPP
#include <ascension/kernel/point.hpp>
#include <ascension/graphics/font/visual-line.hpp>
#include <ascension/graphics/font/visual-lines-listener.hpp>
#include <ascension/graphics/geometry/common.hpp>
#include <ascension/viewer/detail/weak-reference-for-points.hpp>

namespace ascension {
	namespace viewer {
		namespace detail {
			class VisualDestinationProxyMaker;
		}

		class TextArea;
		class VisualPoint;

		/// See the documentation of @c kernel#locations namespace.
		class VisualDestinationProxy : private kernel::Position {
		public:
			bool crossesVisualLines() const BOOST_NOEXCEPT {return crossesVisualLines_;}
			const kernel::Position& position() const BOOST_NOEXCEPT {
				return static_cast<const kernel::Position&>(*this);
			}
		private:
			explicit VisualDestinationProxy(const Position& p, bool crossesVisualLines)
				: Position(p), crossesVisualLines_(crossesVisualLines) {}
			const bool crossesVisualLines_;
			friend class detail::VisualDestinationProxyMaker;
		};
	}
}

namespace ascension {
	namespace kernel {
		namespace locations {
			bool isEndOfVisualLine(const viewer::VisualPoint& p);
			bool isFirstPrintableCharacterOfLine(const viewer::VisualPoint& p);
			bool isFirstPrintableCharacterOfVisualLine(const viewer::VisualPoint& p);
			bool isLastPrintableCharacterOfLine(const viewer::VisualPoint& p);
			bool isLastPrintableCharacterOfVisualLine(const viewer::VisualPoint& p);
			bool isBeginningOfVisualLine(const viewer::VisualPoint& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy backwardPage(const viewer::VisualPoint& p, Index pages = 1);
			viewer::VisualDestinationProxy backwardVisualLine(const viewer::VisualPoint& p, Index lines = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			kernel::Position beginningOfVisualLine(const viewer::VisualPoint& p);
			kernel::Position contextualBeginningOfLine(const viewer::VisualPoint& p);
			kernel::Position contextualBeginningOfVisualLine(const viewer::VisualPoint& p);
			kernel::Position contextualEndOfLine(const viewer::VisualPoint& p);
			kernel::Position contextualEndOfVisualLine(const viewer::VisualPoint& p);
			kernel::Position endOfVisualLine(const viewer::VisualPoint& p);
			kernel::Position firstPrintableCharacterOfLine(const viewer::VisualPoint& p);
			kernel::Position firstPrintableCharacterOfVisualLine(const viewer::VisualPoint& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy forwardPage(const viewer::VisualPoint& p, Index pages = 1);
			viewer::VisualDestinationProxy forwardVisualLine(const viewer::VisualPoint& p, Index lines = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			kernel::Position lastPrintableCharacterOfLine(const viewer::VisualPoint& p);
			kernel::Position lastPrintableCharacterOfVisualLine(const viewer::VisualPoint& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy leftCharacter(
				const viewer::VisualPoint& p, CharacterUnit unit, Index characters = 1);
			boost::optional<kernel::Position> leftWord(const viewer::VisualPoint& p, Index words = 1);
			boost::optional<kernel::Position> leftWordEnd(const viewer::VisualPoint& p, Index words = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy nextPage(
				const viewer::VisualPoint& p, Direction direction, Index pages = 1);
			viewer::VisualDestinationProxy nextVisualLine(
				const viewer::VisualPoint& p, Direction direction, Index lines = 1);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy rightCharacter(
				const viewer::VisualPoint& p, CharacterUnit unit, Index characters = 1);
			boost::optional<Position> rightWord(const viewer::VisualPoint& p, Index words = 1);
			boost::optional<Position> rightWordEnd(const viewer::VisualPoint& p, Index words = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
		} // namespace locations
	} // namespace kernel

	namespace viewer {
		// documentation is visual-point.cpp
		class VisualPoint : public kernel::Point, public graphics::font::VisualLinesListener {
		public:
			/**
			 * The @c VisualPoint is not installed by a @c TextArea.
			 * @see VisualPoint#install, TextAreaDisposedException
			 */
			struct NotInstalledException : IllegalStateException {
				NotInstalledException();
			};
			/**
			 * The @c VisualPoint had been installed by the @c TextArea, but the text area has been disposed.
			 * @see kernel#DocumentDisposedException, VisualPoint
			 */
			struct TextAreaDisposedException : IllegalStateException {
				TextAreaDisposedException();
			};

			explicit VisualPoint(kernel::Document& document, const kernel::Position& position = kernel::Position::zero());
			explicit VisualPoint(TextArea& textArea, const kernel::Position& position = kernel::Position::zero());
			explicit VisualPoint(const kernel::Point& other);
			VisualPoint(const VisualPoint& other);
			virtual ~VisualPoint() BOOST_NOEXCEPT;

			/// @name Installation
			/// @{
			virtual void install(TextArea& textArea);
			bool isInstalled() const BOOST_NOEXCEPT;
			virtual void uninstall() BOOST_NOEXCEPT;
			/// @}

			/// @name Text Area
			/// @{
			bool isFullyAvailable() const BOOST_NOEXCEPT;
			bool isTextAreaDisposed() const;
			TextArea& textArea();
			const TextArea& textArea() const;
			/// @}

			/// @name Visual Positions
			/// @{
			Index offsetInVisualLine() const;
			const graphics::font::VisualLine& visualLine() const;
			/// @}

			/// @name Movement
			/// @{
			using kernel::Point::moveTo;
			void moveTo(const VisualDestinationProxy& to);
			/// @}

		protected:
			// kernel.Point
			virtual void moved(const kernel::Position& from) override BOOST_NOEXCEPT;
		private:
			void buildVisualLineCache();
			void rememberPositionInVisualLine();
			void throwIfNotFullyAvailable() const;
			// layout.VisualLinesListener
			void visualLinesDeleted(const boost::integer_range<Index>& lines,
				Index sublines, bool longestLineChanged) override BOOST_NOEXCEPT;
			void visualLinesInserted(const boost::integer_range<Index>& lines) override BOOST_NOEXCEPT;
			void visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) override BOOST_NOEXCEPT;

		private:
			std::shared_ptr<const detail::WeakReferenceForPoints<TextArea>::Proxy> textAreaProxy_;
			boost::optional<graphics::Scalar> positionInVisualLine_;	// see rememberPositionInVisualLine
			bool crossingLines_;	// true only when the point is moving across the different lines
			boost::optional<graphics::font::VisualLine> lineNumberCaches_;	// caches
			friend class TextArea;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			friend VisualDestinationProxy kernel::locations::backwardVisualLine(const VisualPoint& p, Index lines);
			friend VisualDestinationProxy kernel::locations::forwardVisualLine(const VisualPoint& p, Index lines);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			friend VisualDestinationProxy kernel::locations::nextVisualLine(const VisualPoint& p, Direction direction, Index lines);
		};

		namespace utils {
			// scroll
			void recenter(VisualPoint& p);
			void show(VisualPoint& p);
		}	// namespace utils

		namespace detail {
			using kernel::detail::identifierSyntax;
		}


		/**
		 * Returns @c true if the point has been installed and the @c TextArea is not disposed.
		 * @note This method does not check if the document has been disposed.
		 * @see #isInstalled, #isTextAreaDisposed
		 */
		inline bool VisualPoint::isFullyAvailable() const BOOST_NOEXCEPT {
			return textAreaProxy_.get() != nullptr && textAreaProxy_->get() != nullptr;
		}

		/**
		 * Returns @c true if the point has been installed by the @c TextArea.
		 * @see #install, #isFullyAvailable, #isTextAreaDisposed, #uninstall
		 */
		inline bool VisualPoint::isInstalled() const BOOST_NOEXCEPT {
			return textAreaProxy_.get() != nullptr;
		}

		/**
		 * Returns @c true if the text area which had installed the point has been disposed.
		 * @throw NotInstalledException The point is not installed
		 * @see #isFullyAvailable, #isInstalled
		 */
		inline bool VisualPoint::isTextAreaDisposed() const {
			if(!isInstalled())
				throw NotInstalledException();
			return textAreaProxy_->get() == nullptr;
		}

		/**
		 * Returns the text area which has installed this @c VisualPoint.
		 * @throw NotInstalledException
		 * @throw TextAreaDisposedException
		 */
		inline TextArea& VisualPoint::textArea() {
			throwIfNotFullyAvailable();
			return *textAreaProxy_->get();
		}

		/**
		 * Returns the text area which has installed this @c VisualPoint.
		 * @throw NotInstalledException
		 * @throw TextAreaDisposedException
		 */
		inline const TextArea& VisualPoint::textArea() const {
			throwIfNotFullyAvailable();
			return *textAreaProxy_->get();
		}

		/// @internal
		inline void VisualPoint::throwIfNotFullyAvailable() const {
			if(isTextAreaDisposed())	// may throw
				throw TextAreaDisposedException();
		}

		/**
		 * Returns the visual line numbers.
		 * @throw kernel#DocumentDisposedException
		 * @throw NotInstalledException
		 * @throw TextAreaDisposedException
		 */
		inline const graphics::font::VisualLine& VisualPoint::visualLine() const {
			throwIfNotFullyAvailable();
			if(lineNumberCaches_ == boost::none)
				const_cast<VisualPoint*>(this)->buildVisualLineCache();
			return boost::get(lineNumberCaches_);
		}

	} // namespace viewer
} // namespace ascension

#endif // !ASCENSION_VISUAL_POINT_HPP
