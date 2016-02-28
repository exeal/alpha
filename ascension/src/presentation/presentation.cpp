/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2015
 */

#include <ascension/corelib/numeric-range-algorithm/includes.hpp>
#include <ascension/presentation/hyperlink/hyperlink.hpp>
#include <ascension/presentation/hyperlink/hyperlink-detector.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/presentation/single-styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <boost/foreach.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/find.hpp>


namespace ascension {
	namespace graphics {
		/// A transparent object whose all components are zero.
		const Color Color::TRANSPARENT_BLACK(0, 0, 0, 0);
		/// An opaque object whose all components are zero.
		const Color Color::OPAQUE_BLACK(0, 0, 0, 0xff);
		/// An opaque object whose all components are 255.
		const Color Color::OPAQUE_WHITE(0xff, 0xff, 0xff, 0xff);

	}

	namespace presentation {
		namespace styles {
/*			namespace linewidthkeywords {
				// TODO: these value are changed later.
				const Length THIN(0.05f, Length::EM_HEIGHT);
				const Length MEDIUM(0.10f, Length::EM_HEIGHT);
				const Length THICK(0.20f, Length::EM_HEIGHT);
			}
*/		}


		// Presentation ///////////////////////////////////////////////////////////////////////////////////////////////

		namespace {
			template<typename CacheList>
			inline void bringToFrontInCacheList(CacheList& list, typename CacheList::iterator i) {
				list.splice(std::begin(list), list, i);
			}

			template<typename CacheList, typename T>
			inline void pushToCacheList(CacheList& list, boost::flyweight<T> object, typename CacheList::size_type maximumSize) {
				const auto found(boost::find(list, object));
				if(found != boost::const_end(list))
					return bringToFrontInCacheList(list, found);
				list.push_front(object);
				const auto newSize = list.size();
				if(newSize == maximumSize + 1)
					list.pop_back();
				else if(newSize > maximumSize + 1)
					list.resize(maximumSize);
			}

			typedef boost::flyweight<
				boost::flyweights::key_value<
					styles::SpecifiedValue<TextToplevelStyle>::type,
					styles::ComputedValue<TextToplevelStyle>::type
				>
			> CachedComputedTextToplevelStyle;

			typedef boost::flyweight<
				boost::flyweights::key_value<
					styles::SpecifiedValue<TextLineStyle>::type,
					styles::ComputedValue<TextLineStyle>::type
				>
			> CachedComputedTextLineStyle;

			typedef boost::flyweight<
				boost::flyweights::key_value<
					ComputedTextRunStyle::ConstructionParameters,
					styles::ComputedValue<TextRunStyle>::type
				>
			> CachedComputedTextRunStyle;

			typedef boost::flyweight<
				boost::flyweights::key_value<
					ComputedTextRunStyle::ConstructionParametersAsRoot,
					styles::ComputedValue<TextRunStyle>::type
				>
			> CachedComputedTextRunStyleAsRoot;

			const std::size_t MAXIMUM_COMPUTED_TEXT_RUNS_CACHE_SIZE = 1000;
			const std::size_t MAXIMUM_COMPUTED_TEXT_LINES_CACHE_SIZE = 100;
		}

		struct Presentation::ComputedStyles {
			CachedComputedTextRunStyleAsRoot forRuns;
			CachedComputedTextLineStyle forLines;
			CachedComputedTextToplevelStyle forToplevel;
			std::list<CachedComputedTextRunStyle> cacheForRuns;
			std::list<CachedComputedTextLineStyle> cacheForLines;

			ComputedStyles() :
				forRuns(ComputedTextRunStyle::ConstructionParametersAsRoot(&INITIAL_SPECIFIED_TEXT_RUN_STYLE_, styles::HANDLE_AS_ROOT)),
				forLines(styles::SpecifiedValue<TextLineStyle>::type()),
				forToplevel(styles::SpecifiedValue<TextToplevelStyle>::type()) {}

		private:
			static const SpecifiedTextRunStyle INITIAL_SPECIFIED_TEXT_RUN_STYLE_;
		};
		const SpecifiedTextRunStyle Presentation::ComputedStyles::INITIAL_SPECIFIED_TEXT_RUN_STYLE_;

		struct Presentation::Hyperlinks {
			Index lineNumber;
			std::unique_ptr<hyperlink::Hyperlink*[]> hyperlinks;	// TODO: This should be container class.
			std::size_t numberOfHyperlinks;
		};

		/**
		 * @class ascension::presentation::Presentation
		 * A bridge between the document and visual styled text.
		 *
		 * <h3>The Default (Reading) Direction</h3>
		 *
		 * This value is used only when the all other rules declared @c styles#Direction style property as 'unset'. All
		 * other rules take precedence over the default direction. You can set this value by calling
		 * @c #setDefaultDirection method. The default value is @c ASCENSION_DEFAULT_TEXT_READING_DIRECTION.
		 *
		 * @note This class is not intended to be subclassed.
		 * @see kernel#Document, kernel#DocumentPartitioner
		 */

		/**
		 * @typedef ascension::presentation::Presentation::ComputedTextToplevelStyleChangedSignal
		 * The signal which gets emitted when the computed text toplevel style of @c Presentation was changed.
		 * @param presentation The @c Presentation object
		 * @param previouslyDeclared The declared style used before the change
		 * @param previouslyComputed The computed style used before the change
		 * @see #computedTextToplevelStyle, #computedTextToplevelStyleChangedSignal, #declaredTextToplevelStyle,
		 *      #setDeclaredTextToplevelStyle
		 */

		/**
		 * Constructor.
		 * @param document The target document
		 */
		Presentation::Presentation(kernel::Document& document) BOOST_NOEXCEPT : document_(document),
				defaultDirection_(ASCENSION_DEFAULT_TEXT_READING_DIRECTION), computedStyles_(new ComputedStyles) {
			setDeclaredTextToplevelStyle(std::shared_ptr<const DeclaredTextToplevelStyle>());
			document_.addListener(*this);
		}
		
		/// Destructor.
		Presentation::~Presentation() BOOST_NOEXCEPT {
			document_.removeListener(*this);
			clearHyperlinksCache();
		}

		/**
		 * Registers the text line color specifier.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param specifier The specifier to register
		 * @throw NullPointerException @a specifier is @c null
		 */
		void Presentation::addTextLineColorSpecifier(std::shared_ptr<TextLineColorSpecifier> specifier) {
			if(specifier.get() == nullptr)
				throw NullPointerException("specifier");
			textLineColorSpecifiers_.push_back(specifier);
		}

		void Presentation::clearHyperlinksCache() BOOST_NOEXCEPT {
			BOOST_FOREACH(Hyperlinks* p, hyperlinks_) {
				for(size_t j = 0; j < p->numberOfHyperlinks; ++j)
					delete p->hyperlinks[j];
				delete p;
			}
			hyperlinks_.clear();
		}

		/// Returns the "Computed Value" of @c TextLineStyle in the toplevel style declared by
		/// @c #setDeclaredTextToplevelStyle method.
		const ComputedTextLineStyle& Presentation::computedTextLineStyle() const BOOST_NOEXCEPT {
			return computedStyles_->forLines.get();
		}

		/// Returns the "Computed Value" of @c TextRunStyle in the toplevel style declared by
		/// @c #setDeclaredTextToplevelStyle method.
		const ComputedTextRunStyle& Presentation::computedTextRunStyle() const BOOST_NOEXCEPT {
			return computedStyles_->forRuns.get();
		}

		/// Returns the "Computed Value" of the toplevel style declared by @c #setDeclaredTextToplevelStyle method.
		const ComputedTextToplevelStyle& Presentation::computedTextToplevelStyle() const BOOST_NOEXCEPT {
			return computedStyles_->forToplevel.get();
		}

		/// Returns @c ComputedTextToplevelStyleChanged signal.
		SignalConnector<Presentation::ComputedTextToplevelStyleChanged> Presentation::computedTextToplevelStyleChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(computedTextToplevelStyleChangedSignal_);
		}

		namespace {
			template<typename CascadedValues, typename ParentComputedValues, typename SpecifiedValues>
			struct SpecifiedValuesFromCascadedValues {
				SpecifiedValuesFromCascadedValues(const CascadedValues& cascadedValues,
					const ParentComputedValues& parentComputedValues, typename SpecifiedValues& specifiedValues) :
					cascadedValues(cascadedValues), parentComputedValues(parentComputedValues), specifiedValues(specifiedValues) {}
				template<typename Property, typename SpecifiedValue>
				void operator()(boost::fusion::pair<Property, SpecifiedValue>& p) const {
					handle<Property, SpecifiedValue, ParentComputedValues>(p);
				}
				template<typename Property, typename SpecifiedValue>
				void operator()(boost::fusion::pair<FlowRelativeFourSides<Property>, FlowRelativeFourSides<SpecifiedValue>>& p) const {
					handle<Property, SpecifiedValue, ParentComputedValues>(p);
				}
				const CascadedValues& cascadedValues;
				const ParentComputedValues& parentComputedValues;
				mutable SpecifiedValues& specifiedValues;	// mutable because of boost.fusion.for_each
			private:
				template<typename Property, typename SpecifiedValue, typename P>
				void handle(boost::fusion::pair<Property, SpecifiedValue>& p,
						typename std::enable_if<!std::is_same<P, styles::HandleAsRoot>::value>::type* = nullptr) const {
					styles::specifiedValueFromCascadedValue<Property>(
						boost::fusion::at_key<Property>(cascadedValues),
						boost::fusion::at_key<Property>(parentComputedValues), p.second);
				}
				template<typename Property, typename SpecifiedValue, typename P>
				void handle(boost::fusion::pair<Property, SpecifiedValue>& p,
						typename std::enable_if<std::is_same<P, styles::HandleAsRoot>::value>::type* = nullptr) const {
					styles::specifiedValueFromCascadedValue<Property>(
						boost::fusion::at_key<Property>(cascadedValues), styles::HANDLE_AS_ROOT, p.second);
				}
				template<typename Property, typename SpecifiedValue, typename P>
				void handle(boost::fusion::pair<FlowRelativeFourSides<Property>, FlowRelativeFourSides<SpecifiedValue>>& p,
						typename std::enable_if<!std::is_same<P, styles::HandleAsRoot>::value>::type* = nullptr) const {
					auto& cascadedSideValues = boost::fusion::at_key<FlowRelativeFourSides<Property>>(cascadedValues);
					auto& parentComputedSideValues = boost::fusion::at_key<FlowRelativeFourSides<Property>>(parentComputedValues);
					for(std::size_t i = 0; i < cascadedSideValues.size(); ++i)
						styles::specifiedValueFromCascadedValue<Property>(cascadedSideValues[i], parentComputedSideValues[i], p.second[i]);
				}
				template<typename Property, typename SpecifiedValue, typename P>
				void handle(boost::fusion::pair<FlowRelativeFourSides<Property>, FlowRelativeFourSides<SpecifiedValue>>& p,
						typename std::enable_if<std::is_same<P, styles::HandleAsRoot>::value>::type* = nullptr) const {
					auto& cascadedSideValues = boost::fusion::at_key<FlowRelativeFourSides<Property>>(cascadedValues);
					for(std::size_t i = 0; i < cascadedSideValues.size(); ++i)
						styles::specifiedValueFromCascadedValue<Property>(cascadedSideValues[i], styles::HANDLE_AS_ROOT, p.second[i]);
				}
			};

			template<typename CascadedValues, typename ParentComputedValues, typename SpecifiedValues>
			inline void specifiedValuesFromCascadedValues(const CascadedValues& cascadedValues,
					const ParentComputedValues& parentComputedValues, SpecifiedValues& specifiedValues) {
				static_assert(boost::fusion::traits::is_sequence<CascadedValues>::value, "'cascadedValues' is not boost.fusion sequence.");
				static_assert(boost::fusion::traits::is_sequence<SpecifiedValues>::value, "'specifiedValues' is not boost.fusion sequence.");
				static_assert(boost::fusion::traits::is_sequence<ParentComputedValues>::value
					|| std::is_same<typename std::decay<ParentComputedValues>::type, styles::HandleAsRoot>::value,
					"'parentComputedValues' is neither boost.fusion sequence nor 'styles.handleAsRoot'.");
//				static_assert(std::is_same<ParentComputedValues, typename styles::ComputedValue<Styles>::type>::value
//					|| std::is_same<typename std::decay<ParentComputedValues>::type, styles::HandleAsRoot>::value, "");
				const SpecifiedValuesFromCascadedValues<CascadedValues, ParentComputedValues, SpecifiedValues> compressor(cascadedValues, parentComputedValues, specifiedValues);
				boost::fusion::for_each(specifiedValues, compressor);
			}
		}

		/**
		 * Returns the style of the specified text line.
		 * @param line The line number
		 * @param[out] result The computed text line style
		 * @throw BadPositionException @a line is outside of the document
		 * @throw NullPointerException Internal @c Length#value call may throw this exception
		 */
		const ComputedTextLineStyle& Presentation::computeTextLineStyle(Index line) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position::bol(line));

			const std::shared_ptr<const DeclaredTextLineStyle> declared(declaredTextLineStyle(line));
			const std::shared_ptr<const DeclaredTextLineStyle> cascaded(*styles::cascade(boost::make_iterator_range_n(&declared, 1)));
			SpecifiedTextLineStyle specified;
			specifiedValuesFromCascadedValues(*cascaded, computedTextLineStyle(), specified);
			const CachedComputedTextLineStyle computed(specified);
			pushToCacheList(computedStyles_->cacheForLines, computed, MAXIMUM_COMPUTED_TEXT_LINES_CACHE_SIZE);
			return computedStyles_->cacheForLines.front();
		}

		namespace {
			template<typename DataMemberType>
			inline styles::HandleAsRoot extractTextRunStyle(const styles::HandleAsRoot&, DataMemberType) {
				return styles::HANDLE_AS_ROOT;
			}

			template<typename DataMemberType>
			inline auto extractTextRunStyle(const styles::ComputedValue<TextRunStyle>::type& computed, DataMemberType pointer) -> decltype(computed.*pointer) {
				return computed.*pointer;
			}

			inline CachedComputedTextRunStyle convertSpecifiedTextRunStylesToComputed(
					styles::SpecifiedValue<TextRunStyle>::type& specifiedValues, const styles::ComputedValue<TextRunStyle>::type& parentComputedValues) {
				return CachedComputedTextRunStyle(std::make_tuple(&specifiedValues, &boost::fusion::at_key<styles::Color>(parentComputedValues.colors)));
			}

			inline CachedComputedTextRunStyleAsRoot convertSpecifiedTextRunStylesToComputed(
					styles::SpecifiedValue<TextRunStyle>::type& specifiedValues, const styles::HandleAsRoot&) {
				return CachedComputedTextRunStyleAsRoot(std::make_tuple(&specifiedValues, styles::HANDLE_AS_ROOT));
			}

			template<typename ComputedValuesOrHandleAsRoot>
			inline typename std::conditional<
				std::is_same<typename std::decay<ComputedValuesOrHandleAsRoot>::type, styles::HandleAsRoot>::value,
				CachedComputedTextRunStyleAsRoot, CachedComputedTextRunStyle
			>::type computeTextRunStyle(const styles::DeclaredValue<TextRunStyle>::type& declared, const ComputedValuesOrHandleAsRoot& parentComputed) {
				// get the "Cascaded Value"s
				const DeclaredTextRunStyle& cascaded = *styles::cascade(boost::make_iterator_range_n(&declared, 1));

				// ("Cascaded Value"s for the line) + ("Computed Value"s for the toplevel (as parent)) => "Specified Value"s
				styles::SpecifiedValue<TextRunStyle>::type specified;
				// TODO: Wow, ugly code...
				specifiedValuesFromCascadedValues(cascaded.colors,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::colors), specified.colors);
				specifiedValuesFromCascadedValues(cascaded.backgroundsAndBorders,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::backgroundsAndBorders), specified.backgroundsAndBorders);
				specifiedValuesFromCascadedValues(cascaded.basicBoxModel,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::basicBoxModel), specified.basicBoxModel);
				specifiedValuesFromCascadedValues(cascaded.fonts,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::fonts), specified.fonts);
				specifiedValuesFromCascadedValues(cascaded.inlineLayout,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::inlineLayout), specified.inlineLayout);
				specifiedValuesFromCascadedValues(cascaded.text,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::text), specified.text);
				specifiedValuesFromCascadedValues(cascaded.textDecoration,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::textDecoration), specified.textDecoration);
				specifiedValuesFromCascadedValues(cascaded.writingModes,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::writingModes), specified.writingModes);
				specifiedValuesFromCascadedValues(cascaded.auxiliary,
					extractTextRunStyle(parentComputed, &ComputedTextRunStyle::auxiliary), specified.auxiliary);

				// "Specified Value"s => "Computed Value"s
				return convertSpecifiedTextRunStylesToComputed(specified, parentComputed);
			}
		}

		/**
		 * Computes and returns the text run style of the specified text line.
		 * @param line The line number
		 * @return The computed text run style
		 * @throw BadPositionException @a line is outside of the document
		 */
		const ComputedTextRunStyle& Presentation::computeTextRunStyleForLine(Index line) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position::bol(line));

			const CachedComputedTextRunStyle computed(computeTextRunStyle(*declaredTextLineStyle(line)->runsStyle(), computedTextRunStyle()));
			pushToCacheList(computedStyles_->cacheForRuns, computed, MAXIMUM_COMPUTED_TEXT_RUNS_CACHE_SIZE);
			return computedStyles_->cacheForRuns.front().get();
		}

		namespace {		
			class ComputedStyledTextRunIteratorImpl : public ComputedStyledTextRunIterator {
			public:
				ComputedStyledTextRunIteratorImpl(
						std::unique_ptr<DeclaredStyledTextRunIterator> declaration,
						const styles::ComputedValue<TextRunStyle>::type& computedParentStyles,
						std::list<CachedComputedTextRunStyle>& cacheList)
						: declaration_(std::move(declaration)), computedParentStyles_(computedParentStyles), cacheList_(cacheList) {
				}
				// ComputedStyledTextRunIterator
				bool isDone() const override BOOST_NOEXCEPT {
					return declaration_->isDone();
				}
				void next() override {
					return declaration_->next();
				}
				kernel::Position position() const override {
					return declaration_->position();
				}
				const ComputedTextRunStyle& style() const override {
					const CachedComputedTextRunStyle computed(computeTextRunStyle(*declaration_->style(), computedParentStyles_));
					pushToCacheList(cacheList_, computed, MAXIMUM_COMPUTED_TEXT_RUNS_CACHE_SIZE);
					return cacheList_.front().get();
				}

			private:
				std::unique_ptr<DeclaredStyledTextRunIterator> declaration_;
				const styles::ComputedValue<TextRunStyle>::type& computedParentStyles_;
				std::list<CachedComputedTextRunStyle>& cacheList_;
			};
		}

		/**
		 * Returns the styles of the text runs in the specified line.
		 * @param line The line 
		 * @return An iterator enumerates the styles of the text runs in the line, or @c null if the line has no styled
		 *         text runs
		 * @throw BadPositionException @a line is outside of the document
		 */
		std::unique_ptr<ComputedStyledTextRunIterator> Presentation::computeTextRunStyles(Index line) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position::bol(line));
			if(textRunStyleDeclarator_.get() != nullptr) {
				std::unique_ptr<DeclaredStyledTextRunIterator> declaredRunStyles(textRunStyleDeclarator_->declareTextRunStyle(line));
				if(declaredRunStyles.get() != nullptr)
					return std::unique_ptr<ComputedStyledTextRunIterator>(
						new ComputedStyledTextRunIteratorImpl(std::move(declaredRunStyles), computeTextRunStyleForLine(line), computedStyles_->cacheForRuns));
			}
			return std::unique_ptr<ComputedStyledTextRunIterator>(
				new SingleStyledTextRunIterator<ComputedStyledTextRunIterator>(
					kernel::Region::makeSingleLine(line, boost::irange<Index>(0, document().lineLength(line))), computedStyles_->forRuns.get()));
		}

		/**
		 * Computes the writing mode.
		 * @param line The line to test. If this is @c boost#none, this method returns the entire writing mode
		 * @return The computed writing mode value
		 * @throw IndexOutOfBoundsException @a line is invalid
		 */
		WritingMode Presentation::computeWritingMode(boost::optional<Index> line /* = boost::none */) const {
			const BlockFlowDirection writingMode = boost::fusion::at_key<styles::WritingMode>(computedTextToplevelStyle());
			if(line == boost::none)
				return WritingMode(boost::fusion::at_key<styles::Direction>(computedTextLineStyle()),
					writingMode, boost::fusion::at_key<styles::TextOrientation>(computedTextLineStyle()));
			else {
				// compute 'direction' and 'text-orientation' properties
				const std::shared_ptr<const DeclaredTextLineStyle> declared(declaredTextLineStyle(boost::get(line)));
#if 0
				const styles::DeclaredValue<TextLineStyle>::type& cascaded = styles::cascade(declared);
#else
				const styles::DeclaredValue<TextLineStyle>::type& cascaded = *declared;
#endif

				styles::SpecifiedValue<styles::Direction>::type specifiedDirection;
				const auto& cascadedDirection(boost::fusion::at_key<styles::Direction>(cascaded));
				const auto& computedParentDirection(boost::fusion::at_key<styles::Direction>(computedTextLineStyle()));
				static_assert(!std::is_same<std::decay<decltype(cascadedDirection)>::type, boost::mpl::void_>::value, "");
				static_assert(!std::is_same<std::decay<decltype(computedParentDirection)>::type, boost::mpl::void_>::value, "");
				styles::specifiedValueFromCascadedValue<styles::Direction>(
					cascadedDirection,
					[&computedParentDirection]() {
						return computedParentDirection;
					},
					specifiedDirection);

				styles::SpecifiedValue<styles::TextOrientation>::type specifiedTextOrientation;
				const auto& cascadedTextOrientation(boost::fusion::at_key<styles::TextOrientation>(cascaded));
				const auto& computedParentTextOrientation(boost::fusion::at_key<styles::TextOrientation>(computedTextLineStyle()));
				static_assert(!std::is_same<std::decay<decltype(cascadedTextOrientation)>::type, boost::mpl::void_>::value, "");
				static_assert(!std::is_same<std::decay<decltype(computedParentTextOrientation)>::type, boost::mpl::void_>::value, "");
				styles::specifiedValueFromCascadedValue<styles::TextOrientation>(
					cascadedTextOrientation,
					[&computedParentTextOrientation]() {
						return computedParentTextOrientation;
					},
					specifiedTextOrientation);

				styles::ComputedValue<styles::Direction>::type computedDirection(
					styles::computeAsSpecified<styles::Direction>(specifiedDirection));
				styles::ComputedValue<styles::TextOrientation>::type computedTextOrientation(
					styles::computeAsSpecified<styles::TextOrientation>(specifiedTextOrientation));
				return WritingMode(computedDirection, writingMode, computedTextOrientation);
			}		
		}

		/// @internal
		inline std::shared_ptr<const DeclaredTextLineStyle> Presentation::declaredTextLineStyle(Index line) const {
			std::shared_ptr<const DeclaredTextLineStyle> p;
			if(textLineStyleDeclarator_.get() != nullptr)
				p = textLineStyleDeclarator_->declareTextLineStyle(line);
			if(p.get() == nullptr)
				p = std::shared_ptr<const DeclaredTextLineStyle>(&DeclaredTextLineStyle::unsetInstance(), boost::null_deleter());
			assert(p.get() != nullptr);
			return p;
		}

		/// Returns the document to which the presentation connects.
		const kernel::Document& Presentation::document() const BOOST_NOEXCEPT {
			return document_;
		}

		/// Returns the document to which the presentation connects.
		kernel::Document& Presentation::document() BOOST_NOEXCEPT {
			return document_;
		}
		
		/// @see kernel#DocumentListener#documentAboutToBeChanged
		void Presentation::documentAboutToBeChanged(const kernel::Document& document) {
			// TODO: not implemented.
		}
		
		/// @see kernel#DocumentListener#documentChanged
		void Presentation::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
			auto erasedLines(change.erasedRegion().lines()), insertedLines(change.insertedRegion().lines());
			erasedLines.advance_end(-1);
			insertedLines.advance_end(-1);
			for(std::list<Hyperlinks*>::iterator i(std::begin(hyperlinks_)), e(std::end(hyperlinks_)); i != e; ) {
				const Index line = (*i)->lineNumber;
				if(line == *boost::const_begin(insertedLines) || includes(erasedLines, line)) {
					for(size_t j = 0; j < (*i)->numberOfHyperlinks; ++j)
						delete (*i)->hyperlinks[j];
					delete *i;
					i = hyperlinks_.erase(i);
					continue;
				} else {
					if(line >= *boost::const_end(erasedLines) && !boost::empty(erasedLines))
						(*i)->lineNumber -= boost::size(erasedLines);
					if(line >= *boost::const_end(insertedLines) && !boost::empty(insertedLines))
						(*i)->lineNumber += boost::size(insertedLines);
				}
				++i;
			}
		}

		/**
		 * Returns the hyperlinks in the specified line.
		 * @param line the line number
		 * @param[out] numberOfHyperlinks the number of the hyperlinks
		 * @return the hyperlinks or @c null
		 * @throw BadPositionException @a line is outside of the document
		 */
		const hyperlink::Hyperlink* const* Presentation::getHyperlinks(Index line, size_t& numberOfHyperlinks) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position::bol(line));
			else if(hyperlinkDetector_.get() == nullptr) {
				numberOfHyperlinks = 0;
				return nullptr;
			}
		
			// find in the cache
			for(std::list<Hyperlinks*>::iterator i(std::begin(hyperlinks_)), e(std::end(hyperlinks_)); i != e; ++i) {
				if((*i)->lineNumber == line) {
					Hyperlinks& result = **i;
					if(&result != hyperlinks_.front()) {
						// bring to the front
						hyperlinks_.erase(i);
						hyperlinks_.push_front(&result);
					}
					numberOfHyperlinks = result.numberOfHyperlinks;
					return result.hyperlinks.get();
				}
			}
		
			// not found in the cache
			if(hyperlinks_.size() == ASCENSION_HYPERLINKS_CACHE_SIZE) {
				delete hyperlinks_.back();
				hyperlinks_.pop_back();
			}
			std::vector<hyperlink::Hyperlink*> temp;
			for(Index offsetInLine = 0, eol = document_.lineLength(line); offsetInLine < eol; ) {
				std::unique_ptr<hyperlink::Hyperlink> h(hyperlinkDetector_->nextHyperlink(document_, line, boost::irange(offsetInLine, eol)));
				if(h.get() == nullptr)
					break;
				// check result
				const boost::integer_range<Index>& r(h->region());
				if(*boost::const_begin(r) < offsetInLine)
					break;
				offsetInLine = *boost::const_end(r);
				temp.push_back(h.release());
			}
			std::unique_ptr<Hyperlinks> newItem(new Hyperlinks);
			newItem->lineNumber = line;
			newItem->hyperlinks.reset(new hyperlink::Hyperlink*[numberOfHyperlinks = newItem->numberOfHyperlinks = temp.size()]);
			boost::copy(temp, newItem->hyperlinks.get());
			hyperlinks_.push_front(newItem.release());
			return hyperlinks_.front()->hyperlinks.get();
		}

		/**
		 * Removes the specified text line color specifier.
		 * @param specifier The director to remove
		 */
		void Presentation::removeTextLineColorSpecifier(TextLineColorSpecifier& specifier) BOOST_NOEXCEPT {
			for(std::list<std::shared_ptr<TextLineColorSpecifier>>::iterator
					i(std::begin(textLineColorSpecifiers_)), e(std::end(textLineColorSpecifiers_)); i != e; ++i) {
				if(i->get() == &specifier) {
					textLineColorSpecifiers_.erase(i);
					return;
				}
			}
		}

		/**
		 * Sets the default direction.
		 */
		void Presentation::setDefaultDirection(ReadingDirection direction) {
			if(direction != defaultDirection_) {
				defaultDirection_ = direction;
			}
		}
		
		/**
		 * Sets the hyperlink detector.
		 * @param newDirector The director. Set @c null to unregister
		 */
		void Presentation::setHyperlinkDetector(std::shared_ptr<hyperlink::HyperlinkDetector> newDetector) BOOST_NOEXCEPT {
			hyperlinkDetector_ = newDetector;
			clearHyperlinksCache();
		}

		/**
		 * Sets the line style declarator.
		 * @param newDeclarator The declarator. @c null to unregister
		 */
		void Presentation::setTextLineStyleDeclarator(std::shared_ptr<TextLineStyleDeclarator> newDeclarator) BOOST_NOEXCEPT {
			textLineStyleDeclarator_ = newDeclarator;
		}
		
		/**
		 * Sets the text run style declarator.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param newDirector The declarator. @c null to unregister
		 */
		void Presentation::setTextRunStyleDeclarator(std::shared_ptr<TextRunStyleDeclarator> newDeclarator) BOOST_NOEXCEPT {
			textRunStyleDeclarator_ = newDeclarator;
		}

		/**
		 * Sets the declared text toplevel line style.
		 * @param newStyle The style to set
		 * @see #declaredTextToplevelStyle, ComputedTextToplevelStyleListener
		 */
		void Presentation::setDeclaredTextToplevelStyle(std::shared_ptr<const DeclaredTextToplevelStyle> newStyle) {
			// compute TextToplevelStyle
			std::shared_ptr<const DeclaredTextToplevelStyle> newlyDeclaredToplevelStyles = (newStyle.get() != nullptr) ? newStyle
				: std::shared_ptr<const DeclaredTextToplevelStyle>(&DeclaredTextToplevelStyle::unsetInstance(), boost::null_deleter());
#if 0
			const DeclaredTextToplevelStyle& newlyCascadedToplevelStyles = styles::cascade(newlyDeclaredToplevelStyles);
#else
			const DeclaredTextToplevelStyle& newlyCascadedToplevelStyles = *newlyDeclaredToplevelStyles;
#endif
			styles::SpecifiedValue<TextToplevelStyle>::type newlySpecifiedToplevelStyles;
			specifiedValuesFromCascadedValues(newlyCascadedToplevelStyles, styles::HANDLE_AS_ROOT, newlySpecifiedToplevelStyles);
			CachedComputedTextToplevelStyle newlyComputedToplevelStyles(newlySpecifiedToplevelStyles);

			// compute TextLineStyle
#if 0
			const DeclaredTextLineStyle& newlyCascadedLineStyles = styles::cascade(newlyDeclaredToplevelStyles->linesStyle());
#else
			const DeclaredTextLineStyle& newlyCascadedLineStyles = *newlyDeclaredToplevelStyles->linesStyle();
#endif
			styles::SpecifiedValue<TextLineStyle>::type newlySpecifiedLineStyles;
			specifiedValuesFromCascadedValues(newlyCascadedLineStyles, styles::HANDLE_AS_ROOT, newlySpecifiedLineStyles);
			CachedComputedTextLineStyle newlyComputedLineStyles(newlySpecifiedLineStyles);

			// compute TextRunStyle
			CachedComputedTextRunStyleAsRoot newlyComputedRunStyles(
				computeTextRunStyle(*newlyDeclaredToplevelStyles->linesStyle()->runsStyle(), styles::HANDLE_AS_ROOT));

			// commit
			std::shared_ptr<const DeclaredTextToplevelStyle> previouslyDeclared(declaredTextToplevelStyle_);
			boost::flyweight<ComputedTextToplevelStyle> previouslyComputed(computedStyles_->forToplevel);
			using boost::swap;
			using std::swap;
			swap(newlyDeclaredToplevelStyles, declaredTextToplevelStyle_);
			swap(newlyComputedToplevelStyles, computedStyles_->forToplevel);
			swap(newlyComputedLineStyles, computedStyles_->forLines);
			swap(newlyComputedRunStyles, computedStyles_->forRuns);
			if(previouslyDeclared.get() != nullptr)
				computedTextToplevelStyleChangedSignal_(*this, *previouslyDeclared, previouslyComputed.get());
		}
		
		/**
		 * Returns the colors of the specified text line.
		 * @param line The line
		 * @param[out] foreground The foreground color of the line. Unspecified if an invalid value
		 * @param[out] background The background color of the line. Unspecified if an invalid value
		 * @throw BadPositionException @a line is outside of the document
		 */
		void Presentation::textLineColors(Index line,
				boost::optional<graphics::Color>& foreground, boost::optional<graphics::Color>& background) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position::bol(line));
			TextLineColorSpecifier::Priority highestPriority = 0, p;
			boost::optional<graphics::Color> f, g;
			BOOST_FOREACH(const std::shared_ptr<TextLineColorSpecifier>& s, textLineColorSpecifiers_) {
				p = s->specifyTextLineColors(line, f, g);
				if(p > highestPriority) {
					highestPriority = p;
					foreground = f;
					background = g;
				}
			}
		}
	}
}
