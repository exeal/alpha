/**
 * @file presentation-reconstructor.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-2014
 */

#ifndef ASCENSION_PRESENTATION_RECONSTRUCTOR_HPP
#define ASCENSION_PRESENTATION_RECONSTRUCTOR_HPP

#include <ascension/kernel/document.hpp>	// kernel.ContentType, kernel.Region
#include <map>
#include <memory>							// std.shared_ptr, std.unique_ptr
#include <boost/range/irange.hpp>

namespace ascension {
	namespace presentation {
		class Presentation;
		struct DeclaredStyledTextRunIterator;
		class DeclaredTextRunStyle;

		/**
		 * Creates (reconstructs) styles of the document region. This is used by
		 * @c PresentationReconstructor class to manage the styles in the specified content type.
		 * @see PresentationReconstructor#setPartitionReconstructor
		 */
		class PartitionPresentationReconstructor {
		public:
			/// Destructor.
			virtual ~PartitionPresentationReconstructor() BOOST_NOEXCEPT {}
		private:
			/**
			 * Returns the styled text segments for the specified document region.
			 * @param region The region to reconstruct the new presentation
			 * @return The presentation or @c null (filled by the presentation's default style)
			 */
			virtual std::unique_ptr<DeclaredStyledTextRunIterator>
				presentation(const kernel::Region& region) const BOOST_NOEXCEPT = 0;
			friend class PresentationReconstructor;
		};

		/// Reconstructs document presentation with single text style.
		class SingleStyledPartitionPresentationReconstructor : public PartitionPresentationReconstructor {
		public:
			explicit SingleStyledPartitionPresentationReconstructor(std::shared_ptr<const DeclaredTextRunStyle> style) BOOST_NOEXCEPT;
		private:
			// PartitionPresentationReconstructor
			std::unique_ptr<DeclaredStyledTextRunIterator> presentation(const kernel::Region& region) const override;
		private:
			class Iterator;
			const std::shared_ptr<const DeclaredTextRunStyle> style_;
		};

		/**
		 * Interface for objects which declare style of text runs in a text line.
		 * @see Presentation#setTextRunStyleDeclarator, StyledTextRunIterator,
		 *      TextLineStyleDeclarator
		 */
		class TextRunStyleDeclarator {
		public:
			/// Destructor.
			virtual ~TextRunStyleDeclarator() BOOST_NOEXCEPT {}
		private:
			/**
			 * Returns the style of the text line.
			 * @param line The line to be queried
			 * @return The style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::unique_ptr<DeclaredStyledTextRunIterator> declareTextRunStyle(Index line) const = 0;
			friend class Presentation;
		};

		/**
		 * 
		 */
		class PresentationReconstructor : public TextRunStyleDeclarator {
		public:
			// constructors
			explicit PresentationReconstructor(Presentation& presentation) BOOST_NOEXCEPT;
			~PresentationReconstructor() BOOST_NOEXCEPT;
			// attribute
			void setPartitionReconstructor(kernel::ContentType contentType,
				std::unique_ptr<PartitionPresentationReconstructor> reconstructor);
		private:
			// TextRunStyleDeclarator
			std::unique_ptr<DeclaredStyledTextRunIterator> declareTextRunStyle(Index line) const override;
		private:
			class Iterator;
			Presentation& presentation_;
			std::map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors_;
		};

	}
} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_CONSTRUCTOR_HPP
