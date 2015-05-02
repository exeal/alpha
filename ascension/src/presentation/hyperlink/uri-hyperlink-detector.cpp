/**
 * @file uri-hyperlink-detector.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2014 was presentation.cpp
 * @date 2014-11-16 Separated from presentation.cpp
 */

#include <ascension/kernel/document.hpp>
#include <ascension/presentation/hyperlink/hyperlink.hpp>
#include <ascension/presentation/hyperlink/hyperlink-detector.hpp>
#include <ascension/rules/uri-detector.hpp>
#ifdef BOOST_OS_WINDOWS
#	include <shellapi.h>	// ShellExecuteW
#else
#endif

namespace ascension {
	namespace presentation {
		namespace {
			class URIHyperlink : public hyperlink::Hyperlink {
			public:
				explicit URIHyperlink(const boost::integer_range<Index>& region, const String& uri) BOOST_NOEXCEPT : hyperlink::Hyperlink(region), uri_(uri) {}
				String description() const BOOST_NOEXCEPT {
					static const Char PRECEDING[] = {0x202au, 0};
					static const Char FOLLOWING[] = {0x202cu, 0x0a,
						0x43, 0x54, 0x52, 0x4c, 0x20, 0x2b, 0x20, 0x63, 0x6c, 0x69, 0x63, 0x6b, 0x20,
						0x74, 0x6f, 0x20, 0x66, 0x6f, 0x6c, 0x6c, 0x6f, 0x77, 0x20, 0x74, 0x68, 0x65,
						0x20, 0x6c, 0x69, 0x6e, 0x6b, 0x2e, 0
					};	// "\x202c\nCTRL + click to follow the link."
					return PRECEDING + uri_ + FOLLOWING;
				}
				void invoke() const BOOST_NOEXCEPT {
#ifdef BOOST_OS_WINDOWS
					::ShellExecuteW(nullptr, nullptr, uri_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
#endif
				}
			private:
				const String uri_;
			};
		} // namespace @0

		namespace hyperlink {
			/**
			 * Constructor.
			 * @param uriDetector Can't be @c null
			 * @throw NullPointerException @a uriDetector is @c null
			 */
			URIHyperlinkDetector::URIHyperlinkDetector(std::shared_ptr<const rules::URIDetector> uriDetector) : uriDetector_(uriDetector) {
				if(uriDetector.get() == nullptr)
					throw NullPointerException("uriDetector");
			}
			
			/// Destructor.
			URIHyperlinkDetector::~URIHyperlinkDetector() BOOST_NOEXCEPT {
			}
			
			/// @see HyperlinkDetector#nextHyperlink
			std::unique_ptr<Hyperlink> URIHyperlinkDetector::nextHyperlink(
					const kernel::Document& document, Index line, const boost::integer_range<Index>& range) const {
				const String& s = document.line(line);
				if(*range.end() > s.length())
					throw std::out_of_range("range");
				const Char* bol = s.data();
				const StringPiece result(uriDetector_->search(StringPiece(bol + range.front(), range.size())));
				if(result.begin() != nullptr)
					return std::unique_ptr<Hyperlink>(new URIHyperlink(
						boost::irange<Index>(result.begin() - bol, result.end() - bol), String(result.begin(), result.end())));
				else
					return std::unique_ptr<Hyperlink>();
			}
		}
	}
}
