/**
 * @file hyperlink-detector.hpp
 * Defines @c HyperlinkDetector interface and its implementations.
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2014 was presentation.hpp
 * @date 2014-11-16 Separated from presentation.hpp
 */

#ifndef ASCENSION_HYPERLINK_DETECTOR_HPP
#define ASCENSION_HYPERLINK_DETECTOR_HPP
#include <ascension/kernel/content-type.hpp>
#include <map>
#include <memory>

namespace ascension {
	namespace kernel {
		class Document;
	}

	namespace rules {
		class URIDetector;
	}

	namespace presentation {
		namespace hyperlink {
			class Hyperlink;

			/// A @c HyperlinkDetector finds the hyperlinks in the document.
			class HyperlinkDetector {
			public:
				/// Destructor.
				virtual ~HyperlinkDetector() BOOST_NOEXCEPT {}
				/**
				 * Returns the next hyperlink in the specified text line.
				 * @param document The document
				 * @param line The line number
				 * @param range The range of offsets in the line to search. @a range.begin() can be the beginning of
				 *              the found hyperlink
				 * @return The found hyperlink, or @c null if not found
				 */
				virtual std::unique_ptr<Hyperlink> nextHyperlink(const kernel::Document& document,
					Index line, const boost::integer_range<Index>& range) const BOOST_NOEXCEPT = 0;
			};

			/**
			 * URI hyperlink detector.
			 * @see rules#URIDetector, rules#URIRule
			 * @note This class is not intended to be subclassed.
			 */
			class URIHyperlinkDetector : public HyperlinkDetector {
			public:
				URIHyperlinkDetector(std::shared_ptr<const rules::URIDetector> uriDetector) BOOST_NOEXCEPT;
				~URIHyperlinkDetector() BOOST_NOEXCEPT;
				// HyperlinkDetector
				std::unique_ptr<Hyperlink> nextHyperlink(const kernel::Document& document,
					Index line, const boost::integer_range<Index>& range) const BOOST_NOEXCEPT override;
			private:
				std::shared_ptr<const rules::URIDetector> uriDetector_;
			};

			/**
			 * @note This class is not intended to be subclassed.
			 */
			class CompositeHyperlinkDetector : public HyperlinkDetector {
			public:
				~CompositeHyperlinkDetector() BOOST_NOEXCEPT;
				void setDetector(const kernel::ContentType& contentType, std::unique_ptr<HyperlinkDetector> detector);
				// hyperlink.HyperlinkDetector
				std::unique_ptr<Hyperlink> nextHyperlink(const kernel::Document& document,
					Index line, const boost::integer_range<Index>& range) const BOOST_NOEXCEPT override;
			private:
				std::map<kernel::ContentType, hyperlink::HyperlinkDetector*> composites_;
			};
		}
	}
} // namespace ascension.presentation.hyperlink

#endif // !ASCENSION_HYPERLINK_DETECTOR_HPP
