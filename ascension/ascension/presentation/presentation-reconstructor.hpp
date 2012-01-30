/**
 * @file presentation-reconstructor.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012
 */

#ifndef ASCENSION_PRESENTATION_RECONSTRUCTOR_HPP
#define ASCENSION_PRESENTATION_RECONSTRUCTOR_HPP

#include <ascension/corelib/basic-types.hpp>	// std.tr1.shared_ptr, ...
#include <ascension/kernel/document.hpp>		// kernel.ContentType, kernel.Region

namespace ascension {

	namespace presentation {

		class Presentation;
		class StyledTextRunIterator;
		struct TextRunStyle;

		/**
		 * Creates (reconstructs) styles of the document region. This is used by
		 * @c PresentationReconstructor class to manage the styles in the specified content type.
		 * @see PresentationReconstructor#setPartitionReconstructor
		 */
		class PartitionPresentationReconstructor {
		public:
			/// Destructor.
			virtual ~PartitionPresentationReconstructor() /*throw()*/ {}
		private:
			/**
			 * Returns the styled text segments for the specified document region.
			 * @param region The region to reconstruct the new presentation
			 * @return The presentation or @c null (filled by the presentation's default style)
			 */
			virtual std::unique_ptr<StyledTextRunIterator> getPresentation(const kernel::Region& region) const /*throw()*/ = 0;
			friend class PresentationReconstructor;
		};

		/// Reconstructs document presentation with single text style.
		class SingleStyledPartitionPresentationReconstructor : public PartitionPresentationReconstructor {
			ASCENSION_UNASSIGNABLE_TAG(SingleStyledPartitionPresentationReconstructor);
		public:
			explicit SingleStyledPartitionPresentationReconstructor(std::shared_ptr<const TextRunStyle> style) /*throw()*/;
		private:
			// PartitionPresentationReconstructor
			std::unique_ptr<StyledTextRunIterator>
				getPresentation(Index line, const Range<Index>& columnRange) const /*throw()*/;
		private:
			class Iterator;
			const std::shared_ptr<const TextRunStyle> style_;
		};

		/**
		 * Interface for objects which direct style of text runs in a text line.
		 * @see Presentation#setTextRunStyleDirector
		 */
		class TextRunStyleDirector {
		public:
			/// Destructor.
			virtual ~TextRunStyleDirector() /*throw()*/ {}
		private:
			/**
			 * Queries the style of the text line.
			 * @param line The line to be queried
			 * @return The style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::unique_ptr<StyledTextRunIterator> queryTextRunStyle(Index line) const = 0;
			friend class Presentation;
		};

		/**
		 * 
		 */
		class PresentationReconstructor : public TextRunStyleDirector {
			ASCENSION_UNASSIGNABLE_TAG(PresentationReconstructor);
		public:
			// constructors
			explicit PresentationReconstructor(Presentation& presentation) /*throw()*/;
			~PresentationReconstructor() /*throw()*/;
			// attribute
			void setPartitionReconstructor(kernel::ContentType contentType,
				std::unique_ptr<PartitionPresentationReconstructor> reconstructor);
		private:
			// TextRunStyleDirector
			std::unique_ptr<StyledTextRunIterator> queryTextRunStyle(Index line) const;
		private:
			class Iterator;
			Presentation& presentation_;
			std::map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors_;
		};

	}
} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_CONSTRUCTOR_HPP
