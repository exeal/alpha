/**
 * @file presentation-reconstructor.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-2014
 * @see partition-presentation-reconstructor.hpp
 */

#ifndef ASCENSION_PRESENTATION_RECONSTRUCTOR_HPP
#define ASCENSION_PRESENTATION_RECONSTRUCTOR_HPP

#include <ascension/kernel/partition.hpp>	// kernel.ContentType
#include <boost/config.hpp>	// BOOST_NOEXCEPT
#include <map>
#include <memory>

namespace ascension {
	namespace presentation {
		class PartitionPresentationReconstructor;
		class Presentation;
		struct DeclaredStyledTextRunIterator;

		/**
		 * Interface for objects which declare style of text runs in a text line.
		 * @see Presentation#setTextRunStyleDeclarator, StyledTextRunIterator, TextLineStyleDeclarator
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
