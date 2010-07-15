/**
 * @file presentation.hpp
 * Provides classes define appearance and presentation of a text editor user interface.
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 */

#ifndef ASCENSION_PRESENTATION_HPP
#define ASCENSION_PRESENTATION_HPP
#include "document.hpp"
#ifdef ASCENSION_WINDOWS
#	include <manah/win32/windows.hpp>	// COLORREF
#endif // ASCENSION_WINDOWS

namespace ascension {

	namespace viewers {class TextViewer;}

	namespace rules {class URIDetector;}

	namespace presentation {

		/// @c Color provides colors based on RGB values.
		class Color : public manah::FastArenaObject<Color> {
		public:
			/// Creates
			Color() /*throw()*/ : valid_(false) {}
			/// Creates a color value based on RGB values.
			Color(byte red, byte green, byte blue, byte alpha = 255) /*throw()*/
				: red_(red << 8), green_(green << 8), blue_(blue << 8), alpha_(alpha << 8), valid_(true) {}
#ifdef ASCENSION_WINDOWS
			/// Creates an object from Win32 @c COLORREF value.
			static Color fromCOLORREF(COLORREF value) /*throw()*/ {return Color(
				static_cast<byte>(value & 0xff), static_cast<byte>((value >> 8) & 0xff), static_cast<byte>((value >> 16) & 0xff));}
			COLORREF asCOLORREF() const /*throw()*/ {return RGB(red(), green(), blue());}
#endif // ASCENSION_WINDOWS
			/// Returns the blue color component of this color.
			byte blue() const /*throw()*/ {return blue_ >> 8;}
			/// Returns the green color component of this color.
			byte green() const /*throw()*/ {return green_ >> 8;}
			/// Returns the red color component of this color.
			byte red() const /*throw()*/ {return red_ >> 8;}
			/// Returns the alpha value of this color.
			byte alpha() const /*throw()*/ {return alpha_ >> 8;}
			/// Returns @c true if this color is transparent.
			bool isTransparent() const /**/ {return alpha() == 0;}
			/// Equality operator.
			bool operator==(const Color& other) const /*throw()*/ {return valid_ == other.valid_
				&& (!valid_ || (red() == other.red() && green() == other.green() && blue() == other.blue() && alpha() == other.alpha()));}
			/// Inequality operator.
			bool operator!=(const Color& other) const /*throw()*/ {return !(*this == other);}
		private:
			ushort red_, green_, blue_, alpha_;
			bool valid_;
		};

		/// Foreground color and background.
		struct Colors {
			Color foreground;	///< Color of foreground (text).
			Color background;	///< Color of background.
			/**
			 * Constructor initializes the each colors.
			 * @param foregroundColor foreground color
			 * @param backgroundColor background color
			 */
			explicit Colors(const Color& foregroundColor = Color(),
				const Color& backgroundColor = Color()) /*throw()*/ : foreground(foregroundColor), background(backgroundColor) {}
		};

		struct Length {
			double value;	///< Value of the length.
			enum Unit {
				EM_HEIGHT,		///< The font size of the relevant font.
				X_HEIGHT,		///< The x-height of the relevant font.
				PIXELS,			///< Pixels, relative to the viewing device.
				INCHES,			///< Inches -- 1 inch is equal to 2.54 centimeters.
				CENTIMETERS,	///< Centimeters.
				MILLIMETERS,	///< Millimeters.
				POINTS,			///< Points -- the point used by CSS 2.1 are equal to 1/72nd of an inch.
				PICAS,			///< Picas --1 pica is equal to 12 points.
				DIPS,			///< Device independent pixels. 1 DIP is equal to 1/96th of an inch.
				PERCENTAGE,		///< Percentage.
				INHERIT			///< Inherits from the parent.
			} unit;	///< Unit of the length.

			/// Default constructor.
			Length() /*throw()*/ : unit(INHERIT) {}
			/// Constructor.
			explicit Length(double value, Unit unit = PIXELS) : value(value), unit(unit) {}
		};

		struct Border {
			enum Style {
				NONE, HIDDEN, DOTTED, DASHED, SOLID,
				DOT_DASH, DOT_DOT_DASH,
				DOUBLE, GROOVE, RIDGE, INSET, OUTSET,
				INHERIT
			};
			struct Part {
				Style style;
				Color color;	// if is Color(), same as the foreground
//				??? width;
			} top, right, bottom, left;
		};

		struct BaselineAlignment {
/*			enum DominantBaseline {
				DOMINANT_BASELINE_AUTO,
				DOMINANT_BASELINE_USE_SCRIPT,
				DOMINANT_BASELINE_NO_CHANGE,
				DOMINANT_BASELINE_RESET_SIZE,
				DOMINANT_BASELINE_ALPHABETIC,
				DOMINANT_BASELINE_HANGING,
				DOMINANT_BASELINE_IDEOGRAPHIC,
				DOMINANT_BASELINE_MATHEMATICAL,
				DOMINANT_BASELINE_CENTRAL,
				DOMINANT_BASELINE_MIDDLE,
				DOMINANT_BASELINE_TEXT_AFTER_EDGE,
				DOMINANT_BASELINE_TEXT_DEFORE_EDGE
			} dominantBaseline;
			enum AlignmentBaseline {
				ALIGNMENT_BASELINE_BASELINE,
				ALIGNMENT_BASELINE_USE_SCRIPT,
				ALIGNMENT_BASELINE_BEFORE_EDGE,
				ALIGNMENT_BASELINE_TEXT_BEFORE_EDGE,
				ALIGNMENT_BASELINE_AFTER_EDGE,
				ALIGNMENT_BASELINE_TEXT_AFTER_EDGE,
				ALIGNMENT_BASELINE_CENTRAL,
				ALIGNMENT_BASELINE_MIDDLE,
				ALIGNMENT_BASELINE_IDEOGRAPHIC,
				ALIGNMENT_BASELINE_ALPHABETIC,
				ALIGNMENT_BASELINE_HANGING,
				ALIGNMENT_BASELINE_MATHEMATICAL
			} alignmentBaseline;
			enum AlignmentAdjustEnums {
				ALIGNMENT_ADJUST_AUTO,
				ALIGNMENT_ADJUST_BASELINE,
				ALIGNMENT_ADJUST_BEFORE_EDGE,
				ALIGNMENT_ADJUST_TEXT_BEFORE_EDGE,
				ALIGNMENT_ADJUST_MIDDLE,
				ALIGNMENT_ADJUST_CENTRAL,
				ALIGNMENT_ADJUST_AFTER_EDGE,
				ALIGNMENT_ADJUST_TEXT_AFTER_EDGE,
				ALIGNMENT_ADJUST_IDEOGRAPHIC,
				ALIGNMENT_ADJUST_ALPHABETIC,
				ALIGNMENT_ADJUST_HANGING,
				ALIGNMENT_ADJUST_MATHEMATICAL
			};
			boost::any alignmentAdjust;
			enum BaselineShiftEnums {
				BASELINE_SHIFT_BASELINE,
				BASELINE_SHIFT_SUB,
				BASELINE_SHIFT_SUPER
			};
			boost::any baselineShift;
*/		};

		struct FontProperties {
			enum Weight {
				NORMAL_WEIGHT = 400, BOLD = 700, BOLDER, LIGHTER,
				THIN = 100, EXTRA_LIGHT = 200, ULTRA_LIGHT = 200, LIGHT = 300, 
				MEDIUM = 500, SEMI_BOLD = 600, DEMI_BOLD = 600,
				EXTRA_BOLD = 800, ULTRA_BOLD = 800, BLACK = 900, HEAVY = 900, INHERIT_WEIGHT
			} weight;
			enum Stretch {
				NORMAL_STRETCH, WIDER, NARROWER, ULTRA_CONDENSED, EXTRA_CONDENSED, CONDENSED, SEMI_CONDENSED,
				SEMI_EXPANDED, EXPANDED, EXTRA_EXPANDED, ULTRA_EXPANDED, INHERIT_STRETCH
			} stretch;
			enum Style {
				NORMAL_STYLE, ITALIC, OBLIQUE, INHERIT_STYLE
			} style;
			double size;	///< Font size in DIP. Zero means inherit the parent.

			/// Constructor.
			explicit FontProperties(Weight weight = INHERIT_WEIGHT,
				Stretch stretch = INHERIT_STRETCH, Style style = INHERIT_STYLE, double size = 0)
				: weight(weight), stretch(stretch), style(style), size(size) {}
			/// Equality operator.
			bool operator==(const FontProperties& other) const /*throw()*/ {
				return weight == other.weight && stretch == other.stretch && style == other.style && size == other.size;}
			/// Inequality operator.
			bool operator!=(const FontProperties& other) const /*throw()*/ {return !(*this == other);}
		};




		struct TypographyProperties {};

		struct Decorations {
			enum Style {NONE, SOLID, DOTTED, DAHSED, INHERIT};
			struct {
				Color color;	// if is Color(), same as the foreground
				Style style;
			} overline, strikethrough, baseline, underline;
		};

		enum TextTransform {
			CAPITALIZE, UPPERCASE, LOWERCASE, NONE, TEXT_TRANSFORM_INHERIT
		};

		/// Visual style settings of a text run.
		struct RunStyle : public manah::FastArenaObject<RunStyle> {
			Color foreground;	///< Foreground color.
			Color background;	///< Background color.
			Border border;		///< Border of a text run.
			BaselineAlignment baselineAlignment;
			String fontFamily;	///< Family name. Empty means inherit the parent.
			FontProperties fontProperties;
			double fontSizeAdjust;	///< in DIP unit. 0.0 means 'none', negative value means 'inherit'.
			std::locale locale;
			TypographyProperties typographyProperties;
			Decorations decorations;
			Length letterSpacing;	/// Letter spacing in DIP. Default is 0.
			Length wordSpacing;		/// Word spacing in DIP. Default is 0.
			TextTransform textTransform;
//			RubyProperties rubyProperties;
//			Effects effects;
			bool shapingEnabled;	/// Set @c false to disable shaping.

			/// Default constructor.
			RunStyle() : letterSpacing(0), wordSpacing(0), textTransform(), shapingEnabled(true) {}
			RunStyle& resolveInheritance(const RunStyle& base);
		};

		struct StyledRun {
			/// The beginning column in the line of the text range which the style applies.
			length_t column;
			/// The style of the text run.
			std::tr1::shared_ptr<const RunStyle> style;
			/// Default constructor.
			StyledRun() {}
			/// Constructor initializes the all members.
			StyledRun(length_t column, std::tr1::shared_ptr<const RunStyle> style) : column(column), style(style) {}
		};

		class IStyledRunIterator {
		public:
			/// Destructor.
			virtual ~IStyledRunIterator() /*throw()*/ {}
			/// Returns the current styled run.
			virtual const StyledRun& current() const = 0;
			/// Returns @c true if the iterator addresses the end of the range.
			virtual bool isDone() const = 0;
			/// Moves the iterator to the next styled run or throws @c IllegalStateException.
			virtual void next() = 0;
		};

		/**
		 * @c TextAlignment describes horizontal alignment of a paragraph. This implements
		 * 'text-align' property in CSS 3
		 * (http://www.w3.org/TR/2007/WD-css3-text-20070306/#text-align).
		 * @see resolveTextAlignment, LineStyle#alignment, LineStyle#lastSublineAlignment
		 */
		enum TextAlignment {
			/// The text is aligned to the start edge of the paragraph.
			ALIGN_START,
			/// The text is aligned to the end edge of the paragraph.
			ALIGN_END,
			/// The text is aligned to the left of the paragraph.
			ALIGN_LEFT,
			/// The text is aligned to the right of the paragraph.
			ALIGN_RIGHT,
			/// The text is aligned to the center of the paragraph.
			/// Some methods which take @c ParagraphAlignment don't accept this value.
			ALIGN_CENTER,
			/// The text is justified according to the method specified @c Justification value.
			JUSTIFY,
			/// The alignment is automatically determined.
			/// Some methods which take @c TextAlignment don't accept this value.
			INHERIT_TEXT_ALIGNMENT
		};

		/**
		 * Orientation of the text layout.
		 * @see LineStyle#readingDirection
		 */
		enum ReadingDirection {
			LEFT_TO_RIGHT,				///< The text is left-to-right.
			RIGHT_TO_LEFT,				///< The text is right-to-left.
			INHERIT_READING_DIRECTION	///< 
		};

		struct NumberSubstitution {
			/// Specifies how to apply number substitution on digits and related punctuation.
			enum Method {
				/// The substitution method should be determined based on the locale.
				FROM_LOCALE,
				/// The number shapes depend on the context.
				CONTEXTUAL,
				/// No substitution is performed. Characters U+0030..0039 are always rendered as
				/// nominal numeral shapes (European numbers).
				NONE,
				/// Numbers are rendered using the national number shapes.
				NATIONAL,
				/// Numbers are rendered using the traditional shapes for the specified locale.
				TRADITIONAL
			} method;	///< The substitution method.
			/// The name of the locale to be used.
			std::string localeName;
			/// Whether to ignore user override.
			bool ignoreUserOverride;

			/// Default constructor.
			NumberSubstitution() /*throw()*/ : method(NONE), ignoreUserOverride(false) {}
		};

		struct LineStyle {
			/// The reading direction of the line. Default value is @c INHERIT_READING_DIRECTION.
			ReadingDirection readingDirection;
			/// The text alignment of the line. Default value is @c ALIGN_START.
			TextAlignment alignment;
			/// The number substitution setting. Default value is built by the default constructor.
			NumberSubstitution numberSubstitution;

			LineStyle() /*throw()*/;
		};

		/**
		 * Interface for objects which direct style of a line.
		 * @see Presentation#setLineStyleDirector
		 */
		class ILineStyleDirector {
		public:
			/// Destructor.
			virtual ~ILineStyleDirector() /*throw()*/ {}
		private:
			/**
			 * Queries the style of the line.
			 * @param line the line to be queried
			 * @return the style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::tr1::shared_ptr<const LineStyle> queryLineStyle(length_t line) const = 0;
			friend class Presentation;
		};

		/**
		 * Interface for objects which direct style of text runs in a line.
		 * @see Presentation#setTextRunStyleDirector
		 */
		class ITextRunStyleDirector {
		public:
			/// Destructor.
			virtual ~ITextRunStyleDirector() /*throw()*/ {}
		private:
			/**
			 * Queries the style of the line.
			 * @param line the line to be queried
			 * @return the style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::auto_ptr<IStyledRunIterator> queryTextRunStyle(length_t line) const = 0;
			friend class Presentation;
		};

		/**
		 * Interface for objects which direct color of a line.
		 * @see Presentation#addLineColorDirector
		 */
		class ILineColorDirector {
		public:
			/// Priority.
			typedef uchar Priority;
			/// Destructor.
			virtual ~ILineColorDirector() /*throw()*/ {}
		private:
			/**
			 * Queries the color of the line.
			 * @param line the line to be queried
			 * @param[out] the color of the line. if this is invalid color, line color is not set
			 * @return the priority
			 */
			virtual Priority queryLineColor(length_t line, Colors& color) const = 0;
			friend class Presentation;
		};

		/***/
		class ITextViewerListListener {
		private:
			/***/
			virtual void textViewerListChanged(Presentation& presentation) = 0;
			friend class Presentation;
		};

		/// @internal
		namespace internal {
			class ITextViewerCollection {
			private:
				virtual void addTextViewer(viewers::TextViewer& viewer) /*throw()*/ = 0;
				virtual void removeTextViewer(viewers::TextViewer& viewer) /*throw()*/ = 0;
				friend class viewers::TextViewer;
			};
		}

		/**
		 * Provides support for detecting and presenting hyperlinks in text editors. "Hyperlink" is
		 * invokable text segment in the document.
		 * @see Presentation#getHyperlinks, Presentation#setHyperlinkDetector
		 */
		namespace hyperlink {
			/// Represents a hyperlink.
			class IHyperlink {
			public:
				/// Destructor.
				virtual ~IHyperlink() /*throw()*/ {}
				/// Returns the descriptive text of the hyperlink.
				virtual String description() const /*throw()*/ = 0;
				/// Invokes the hyperlink.
				virtual void invoke() const /*throw()*/ = 0;
				/// Returns the columns of the region of the hyperlink.
				const Range<length_t>& region() const /*throw()*/ {return region_;}
			protected:
				/// Protected constructor takes the region of the hyperlink.
				explicit IHyperlink(const Range<length_t>& region) /*throw()*/ : region_(region) {}
			private:
				const Range<length_t> region_;
			};

			/// A @c HyperlinkDetector finds the hyperlinks in the document.
			class IHyperlinkDetector {
			public:
				/// Destructor.
				virtual ~IHyperlinkDetector() /*throw()*/ {}
				/**
				 * Returns the next hyperlink in the specified line.
				 * @param document the document
				 * @param line the line number
				 * @param range the column range in the line to search. @a range.beginning() can be
				 * the beginning of the found hyperlink
				 * @return the found hyperlink, or @c null if not found
				 */
				virtual std::auto_ptr<IHyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/ = 0;
			};

			/**
			 * URI hyperlink detector.
			 * @see rules#URIDetector, rules#URIRule
			 * @note This class is not intended to be subclassed.
			 */
			class URIHyperlinkDetector : public IHyperlinkDetector {
			public:
				URIHyperlinkDetector(const rules::URIDetector& uriDetector, bool delegateOwnership) /*throw()*/;
				~URIHyperlinkDetector() /*throw()*/;
				// IHyperlinkDetector
				std::auto_ptr<IHyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/;
			private:
				ascension::internal::StrategyPointer<const rules::URIDetector> uriDetector_;
			};

			/**
			 * @note This class is not intended to be subclassed.
			 */
			class CompositeHyperlinkDetector : public hyperlink::IHyperlinkDetector {
			public:
				~CompositeHyperlinkDetector() /*throw()*/;
				void setDetector(kernel::ContentType contentType, std::auto_ptr<hyperlink::IHyperlinkDetector> detector);
				// hyperlink.IHyperlinkDetector
				std::auto_ptr<IHyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/;
			private:
				std::map<kernel::ContentType, hyperlink::IHyperlinkDetector*> composites_;
			};
		} // namespace hyperlink

		/**
		 * A bridge between the document and visual styled text.
		 * @note This class is not intended to be subclassed.
		 * @see Document, DocumentPartitioner, TextViewer
		 */
		class Presentation : public kernel::IDocumentListener, public internal::ITextViewerCollection {
			MANAH_NONCOPYABLE_TAG(Presentation);
		public:
			typedef std::set<viewers::TextViewer*>::iterator TextViewerIterator;
			typedef std::set<viewers::TextViewer*>::const_iterator TextViewerConstIterator;
			// constructors
			explicit Presentation(kernel::Document& document) /*throw()*/;
			~Presentation() /*throw()*/;
			// attributes
			void addTextViewerListListener(ITextViewerListListener& listener);
			kernel::Document& document() /*throw()*/;
			const kernel::Document& document() const /*throw()*/;
			const hyperlink::IHyperlink* const* getHyperlinks(length_t line, std::size_t& numberOfHyperlinks) const;
			void removeTextViewerListListener(ITextViewerListListener& listener);
			// styles
			std::tr1::shared_ptr<const LineStyle> defaultLineStyle() const /*throw()*/;
			std::tr1::shared_ptr<const RunStyle> defaultTextRunStyle() const /*throw()*/;
			Colors getLineColor(length_t line) const;
			void setDefaultLineStyle(std::tr1::shared_ptr<const LineStyle> newStyle);
			void setDefaultTextRunStyle(std::tr1::shared_ptr<const RunStyle> newStyle);
			std::tr1::shared_ptr<const LineStyle> lineStyle(length_t line) const;
			std::auto_ptr<IStyledRunIterator> textRunStyles(length_t line) const;
			// strategies
			void addLineColorDirector(std::tr1::shared_ptr<ILineColorDirector> director);
			void removeLineColorDirector(ILineColorDirector& director) /*throw()*/;
			void setHyperlinkDetector(hyperlink::IHyperlinkDetector* newDetector, bool delegateOwnership) /*throw()*/;
			void setLineStyleDirector(std::tr1::shared_ptr<ILineStyleDirector> newDirector) /*throw()*/;
			void setTextRunStyleDirector(std::tr1::shared_ptr<ITextRunStyleDirector> newDirector) /*throw()*/;
			// TextViewer enumeration
			TextViewerIterator firstTextViewer() /*throw()*/;
			TextViewerConstIterator firstTextViewer() const /*throw()*/;
			TextViewerIterator lastTextViewer() /*throw()*/;
			TextViewerConstIterator lastTextViewer() const /*throw()*/;
			std::size_t numberOfTextViewers() const /*throw()*/;
		private:
			void clearHyperlinksCache() /*throw()*/;
			// kernel.IDocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// internal.ITextViewerCollection
			void addTextViewer(viewers::TextViewer& viewer) /*throw()*/;
			void removeTextViewer(viewers::TextViewer& viewer) /*throw()*/;
		private:
			kernel::Document& document_;
			std::set<viewers::TextViewer*> textViewers_;
			static std::tr1::shared_ptr<const LineStyle> DEFAULT_LINE_STYLE;
			std::tr1::shared_ptr<const LineStyle> defaultLineStyle_;
			std::tr1::shared_ptr<const RunStyle> defaultTextRunStyle_;
			std::tr1::shared_ptr<ILineStyleDirector> lineStyleDirector_;
			std::tr1::shared_ptr<ITextRunStyleDirector> textRunStyleDirector_;
			std::list<std::tr1::shared_ptr<ILineColorDirector> > lineColorDirectors_;
			ascension::internal::Listeners<ITextViewerListListener> textViewerListListeners_;
			ascension::internal::StrategyPointer<hyperlink::IHyperlinkDetector> hyperlinkDetector_;
			struct Hyperlinks;
			mutable std::list<Hyperlinks*> hyperlinks_;
		};

		/**
		 * Creates (reconstructs) styles of the document region. This is used by
		 * @c PresentationReconstructor class to manage the styles in the specified content type.
		 * @see PresentationReconstructor#setPartitionReconstructor
		 */
		class IPartitionPresentationReconstructor {
		public:
			/// Destructor.
			virtual ~IPartitionPresentationReconstructor() /*throw()*/ {}
		private:
			/**
			 * Returns the styled text segments for the specified document region.
			 * @param region the region to reconstruct the new presentation
			 * @return the presentation or @c null (filled by the presentation's default style)
			 */
			virtual std::auto_ptr<IStyledRunIterator> getPresentation(const kernel::Region& region) const /*throw()*/ = 0;
			friend class PresentationReconstructor;
		};

		/// Reconstructs document presentation with single text style.
		class SingleStyledPartitionPresentationReconstructor : public IPartitionPresentationReconstructor {
			MANAH_UNASSIGNABLE_TAG(SingleStyledPartitionPresentationReconstructor);
		public:
			explicit SingleStyledPartitionPresentationReconstructor(std::tr1::shared_ptr<const RunStyle> style) /*throw()*/;
		private:
			// IPartitionPresentationReconstructor
			std::auto_ptr<IStyledRunIterator>
				getPresentation(length_t line, const Range<length_t>& columnRange) const /*throw()*/;
		private:
			class StyledRunIterator;
			const std::tr1::shared_ptr<const RunStyle> style_;
		};

		/**
		 * 
		 */
		class PresentationReconstructor : public ITextRunStyleDirector, public kernel::IDocumentPartitioningListener {
			MANAH_UNASSIGNABLE_TAG(PresentationReconstructor);
		public:
			// constructors
			explicit PresentationReconstructor(Presentation& presentation) /*throw()*/;
			~PresentationReconstructor() /*throw()*/;
			// attribute
			void setPartitionReconstructor(kernel::ContentType contentType,
				std::auto_ptr<IPartitionPresentationReconstructor> reconstructor);
		private:
			// ITextRunStyleDirector
			std::auto_ptr<IStyledRunIterator> queryTextRunStyle(length_t line) const;
			// kernel.IDocumentPartitioningListener
			void documentPartitioningChanged(const kernel::Region& changedRegion);
		private:
			class StyledRunIterator;
			Presentation& presentation_;
			std::map<kernel::ContentType, IPartitionPresentationReconstructor*> reconstructors_;
		};


		/**
		 * Registers the line color director.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param director the director to register
		 * @throw NullPointerException @a director is @c null
		 */
		inline void Presentation::addLineColorDirector(std::tr1::shared_ptr<ILineColorDirector> director) {
			if(director.get() == 0) throw NullPointerException("director"); lineColorDirectors_.push_back(director);}

		/**
		 * Registers the text viewer list listener.
		 * @param listener the listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		inline void Presentation::addTextViewerListListener(ITextViewerListListener& listener) {textViewerListListeners_.add(listener);}

		/// Returns the default line style this object gives.
		inline std::tr1::shared_ptr<const LineStyle> Presentation::defaultLineStyle() const /*throw()*/ {return defaultLineStyle_;}

		/// Returns the default text run style this object gives.
		inline std::tr1::shared_ptr<const RunStyle> Presentation::defaultTextRunStyle() const /*throw()*/ {return defaultTextRunStyle_;}

		/// Returns the style of the specified line.
		inline std::tr1::shared_ptr<const LineStyle> Presentation::lineStyle(length_t line) const /*throw()*/ {
			std::tr1::shared_ptr<const LineStyle> style;
			if(lineStyleDirector_.get() != 0)
				style = lineStyleDirector_->queryLineStyle(line);
			return (style.get() != 0) ? style : defaultLineStyle();
		}

		/// Returns the number of text viewers.
		inline std::size_t Presentation::numberOfTextViewers() const /*throw()*/ {return textViewers_.size();}

		/**
		 * Removes the specified line color director.
		 * @param director the director to remove
		 */
		inline void Presentation::removeLineColorDirector(ILineColorDirector& director) /*throw()*/ {
			for(std::list<std::tr1::shared_ptr<ILineColorDirector> >::iterator
					i(lineColorDirectors_.begin()), e(lineColorDirectors_.end()); i != e; ++i) {
				if(i->get() == &director) {lineColorDirectors_.erase(i); return;}
			}
		}

		/**
		 * Removes the text viewer list listener.
		 * @param listener the listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		inline void Presentation::removeTextViewerListListener(ITextViewerListListener& listener) {textViewerListListeners_.remove(listener);}

		/// 
		inline TextAlignment defaultTextAlignment(const Presentation& presentation) {
			std::tr1::shared_ptr<const LineStyle> style(presentation.defaultLineStyle());
			return (style.get() != 0
				&& style->alignment != INHERIT_TEXT_ALIGNMENT) ? style->alignment : ASCENSION_DEFAULT_TEXT_ALIGNMENT;
		}

		///
		inline ReadingDirection defaultReadingDirection(const Presentation& presentation) {
			std::tr1::shared_ptr<const LineStyle> style(presentation.defaultLineStyle());
			return (style.get() != 0
				&& style->readingDirection != INHERIT_READING_DIRECTION) ? style->readingDirection : ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
		}

		/**
		 * Resolve an ambiguous text alignment value (@c ALIGN_START and @c ALIGN_END).
		 * @param value the text alignment to compute
		 * @param direction the reading direction
		 * @return the resolved text alignment
		 * @note This function does not resolve @c INHERIT_ALIGNMENT.
		 */
		inline TextAlignment resolveTextAlignment(TextAlignment value, ReadingDirection direction) {
			switch(value) {
				case ALIGN_START:
					return (direction == LEFT_TO_RIGHT) ? ALIGN_LEFT : ALIGN_RIGHT;
				case ALIGN_END:
					return (direction == LEFT_TO_RIGHT) ? ALIGN_RIGHT : ALIGN_LEFT;
				default:
					return value;
			}
		}

}} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_HPP
