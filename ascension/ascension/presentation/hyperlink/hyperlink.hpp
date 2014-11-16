/**
 * @file hyperlink.hpp
 * Defines @c Hyperlink interface.
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2014 was presentation.hpp
 * @date 2014-11-16 Separated from presentation.hpp
 */

#ifndef ASCENSION_HYPERLINK_HPP
#define ASCENSION_HYPERLINK_HPP

#include <ascension/corelib/basic-types.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace presentation {
		/**
		 * Provides support for detecting and presenting hyperlinks in text editors. "Hyperlink" is invokable text
		 * segment in the document.
		 * @see Presentation#getHyperlinks, Presentation#setHyperlinkDetector
		 */
		namespace hyperlink {
			/// Represents a hyperlink.
			class Hyperlink {
			public:
				/// Destructor.
				virtual ~Hyperlink() BOOST_NOEXCEPT {}
				/// Returns the descriptive text of the hyperlink.
				virtual String description() const BOOST_NOEXCEPT = 0;
				/// Invokes the hyperlink.
				virtual void invoke() const BOOST_NOEXCEPT = 0;
				/// Returns the columns of the region of the hyperlink.
				const boost::integer_range<Index>& region() const BOOST_NOEXCEPT {return region_;}
			protected:
				/// Protected constructor takes the region of the hyperlink.
				explicit Hyperlink(const boost::integer_range<Index>& region) BOOST_NOEXCEPT : region_(region) {}
			private:
				const boost::integer_range<Index> region_;
			};
		}
	}
} // namespace ascension.presentation.hyperlink

#endif // !ASCENSION_HYPERLINK_HPP
