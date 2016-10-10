/**
 * @file content-type.hpp
 * @author exeal
 * @date 2011-01-21 Separated from document.hpp.
 * @date 2016-10-05 Separated from partition.hpp.
 */

#ifndef ASCENSION_CONTENT_TYPE_HPP
#define ASCENSION_CONTENT_TYPE_HPP
#include <boost/operators.hpp>
#include <cstdint>
#include <utility>

namespace ascension {
	namespace text {
		class IdentifierSyntax;
	}

	namespace kernel {
		class Document;
		class Position;

		/// Content type of a document partition.
		class ContentType : /* public FastArenaObject<ContentType>, */ private boost::totally_ordered<ContentType> {
		public:
			bool operator==(const ContentType& other) const BOOST_NOEXCEPT;
			bool operator<(const ContentType& other) const BOOST_NOEXCEPT;
			static ContentType newValue() BOOST_NOEXCEPT;

			/// @name Special Values
			/// @{
			static const ContentType DEFAULT_CONTENT, PARENT_CONTENT;
			bool isSpecial() const BOOST_NOEXCEPT;
			/// @}

		private:
			explicit ContentType(std::uint32_t value) BOOST_NOEXCEPT;
			std::uint32_t value_;
			static const std::uint32_t MAXIMUM_SPECIAL_VALUE_ = 99;
		};

		ContentType contentType(const std::pair<const Document&, Position>& p);

		/**
		 * An @c ContentTypeInformationProvider provides the information about the document's content types.
		 * @see Document#setContentTypeInformation, Document#setContentTypeInformation
		 */
		class ContentTypeInformationProvider {
		public:
			/// Destructor.
			virtual ~ContentTypeInformationProvider() BOOST_NOEXCEPT {}
			/**
			 * Returns the identifier syntax for the specified content type.
			 * @param contentType The type of content
			 * @return The identifier syntax
			 */
			virtual const text::IdentifierSyntax& getIdentifierSyntax(const ContentType& contentType) const BOOST_NOEXCEPT = 0;
		};


		/// Returns @c true if this content type and @a other are same.
		inline bool ContentType::operator==(const ContentType& other) const BOOST_NOEXCEPT {
			return value_ == other.value_;
		}

		/// Returns @c true if this content type is less than @a other.
		inline bool ContentType::operator<(const ContentType& other) const BOOST_NOEXCEPT {
			return value_ < other.value_;
		}

		/// Returns @c true if this content type is special which is defined by @c ascension and for special use.
		inline bool ContentType::isSpecial() const BOOST_NOEXCEPT {
			return value_ <= MAXIMUM_SPECIAL_VALUE_;
		}
	}
} // namespace ascension.kernel

#endif // !ASCENSION_CONTENT_TYPE_HPP
