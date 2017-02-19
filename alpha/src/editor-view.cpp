/**
 * @file editor-view.cpp
 * @author exeal
 * @date 2008 Separated from buffer.cpp)
 * @date 2008-2009, 2014-2015
 */

#include "application.hpp"
#include "buffer-list.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
//#include <ascension/graphics/native-conversion.hpp>
#include <ascension/graphics/paint.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-range.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/log.hpp>
#include <ascension/presentation/presentative-text-renderer.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-area-rendering-strategy.hpp>
//#include <ascension/viewer/widgetapi/widget.hpp>

namespace alpha {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK) && defined(ASCENSION_TEXT_VIEWER_IS_GTK_SCROLLABLE)
	namespace {
		static const char GLIBMM_CUSTOM_TYPE_NAME[] = "alpha.EditorView";
	}
#endif

	/// Default constructor.
	EditorView::EditorView(std::shared_ptr<Buffer> buffer) :
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK) && defined(ASCENSION_TEXT_VIEWER_IS_GTK_SCROLLABLE)
			Glib::ObjectBase(GLIBMM_CUSTOM_TYPE_NAME),
#endif
			ascension::viewer::TextViewer(buffer), buffer_(buffer) {
		document()->bookmarker().addListener(*this);
//		caretObject_.reset(new CaretProxy(caret()));
	}

	/// Copy-constructor.
	EditorView::EditorView(const EditorView& other) :
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK) && defined(ASCENSION_TEXT_VIEWER_IS_GTK_SCROLLABLE)
			Glib::ObjectBase(GLIBMM_CUSTOM_TYPE_NAME),
#endif
			ascension::viewer::TextViewer(other), buffer_(other.buffer_) {
	}

	/// Destructor.
	EditorView::~EditorView() {
		document()->bookmarker().removeListener(*this);
	}

	/// @see BookmarkListener#bookmarkChanged
	void EditorView::bookmarkChanged(ascension::Index line) {
		textArea()->redrawLine(line);
	}

	/// @see BookmarkListener#bookmarkCleared
	void EditorView::bookmarkCleared() {
		ascension::viewer::widgetapi::scheduleRedraw(*this, false);
	}

	/// @see TextViewer#drawIndicatorMargin
	void EditorView::drawIndicatorMargin(ascension::Index line, ascension::graphics::PaintContext& context, const ascension::graphics::Rectangle& rect) {
#if 0
		if(document()->bookmarker().isMarked(line)) {
			// draw a bookmark indication mark
			namespace gfx = ascension::graphics;
			const ascension::NumericRange<ascension::graphics::Scalar> xrange(gfx::geometry::range<0>(rect) | ascension::adaptors::ordered());
			const auto yrange(
				ascension::nrange(
					(gfx::geometry::top(rect) * 2 + gfx::geometry::bottom(rect)) / 3, (gfx::geometry::top(rect) + gfx::geometry::bottom(rect) * 2) / 3));
			const auto r(gfx::geometry::make<gfx::Rectangle>(std::make_pair(xrange, yrange)));
			auto gradient(
				std::make_shared<gfx::LinearGradient>(
					boost::geometry::make_zero<gfx::Point>(),
					gfx::geometry::make<gfx::Point>((
						gfx::geometry::_x = gfx::geometry::dx(r), gfx::geometry::_y = static_cast<gfx::Scalar>(0)))));

			// get themed colors
			if(Glib::RefPtr<Gtk::StyleContext> styleContext = get_style_context()) {
				const Gtk::StateFlags state = Gtk::STATE_FLAG_ACTIVE | Gtk::STATE_FLAG_SELECTED;
				gradient->addColorStop(0, ascension::graphics::fromNative<ascension::graphics::Color>(styleContext->get_background_color(state)));
				gradient->addColorStop(1, ascension::graphics::fromNative<ascension::graphics::Color>(styleContext->get_color(state)));
				context.setFillStyle(gradient);
				context.fillRectangle(r);
			}
		}
#endif
	}

	/// @see TextViewer#focusAboutToBeLost
	void EditorView::focusAboutToBeLost(ascension::viewer::widgetapi::event::Event& event) {
		ascension::viewer::TextViewer::focusAboutToBeLost(event);
	}

	/// @see TextViewer#focusGained
	void EditorView::focusGained(ascension::viewer::widgetapi::event::Event& event) {
		ascension::viewer::TextViewer::focusGained(event);
		BufferList::instance().select(*document());
	}

	/// @see TextViewer#initialized
	void EditorView::initialized() BOOST_NOEXCEPT {
		std::unique_ptr<ascension::presentation::PresentativeTextRenderer> renderer(
			new ascension::presentation::PresentativeTextRenderer(document()->presentation(), boost::geometry::make_zero<ascension::graphics::Dimension>()));
		renderer->setStrategy(std::unique_ptr<ascension::viewer::TextAreaRenderingStrategy>(new ascension::viewer::TextAreaRenderingStrategy(*textArea())));
		textArea()->setTextRenderer(std::move(renderer));
	}

	/// @see TextViewer#keyPressed
	void EditorView::keyPressed(ascension::viewer::widgetapi::event::KeyInput& input) {
#ifndef ALPHA_NO_AMBIENT
		// disable default keyboard bindings
#else
		return ascension::viewer::TextViewer::keyPressed(input);
#endif
	}
#if 0
	/// @see Caret#MatchBracketsChangedSignal
	void EditorView::matchBracketsChanged(const ascension::viewer::Caret& self, const boost::optional<std::pair<ascension::kernel::Position, ascension::kernel::Position>>& previouslyMatchedBrackets, bool outsideOfView) {
		// TODO: indicate if the pair is outside of the viewer.
	}
#endif

#ifndef ALPHA_NO_AMBIENT
	namespace {
		void extendSelection(ascension::viewer::Caret& caret, boost::python::object to) {
			if(boost::python::extract<const ascension::kernel::Position&>(to).check())
				caret.extendSelectionTo(static_cast<ascension::kernel::Position>(boost::python::extract<ascension::kernel::Position>(to)));
			else if(boost::python::extract<const ascension::viewer::VisualDestinationProxy&>(to).check())
				caret.extendSelectionTo(static_cast<ascension::viewer::VisualDestinationProxy>(boost::python::extract<ascension::viewer::VisualDestinationProxy>(to)));
			else if(boost::python::extract<const ascension::kernel::Point&>(to).check())
				caret.extendSelectionTo(static_cast<const ascension::kernel::Point&>(boost::python::extract<const ascension::kernel::Point&>(to)).position());
			else {
				::PyErr_BadArgument();
				boost::python::throw_error_already_set();
			}
		}
		template<typename Result>
		Result extractTwoAxes(const boost::python::object& o) {
			boost::python::extract<Result> typed(o);
			if(typed.check())
				return typed;
			const boost::python::ssize_t length = boost::python::len(o);
			if(::PySequence_Check(o.ptr()) == 0 || length == -1)
				::PyErr_BadArgument();
			else if(length != 2)
				::PyErr_BadArgument();	// TODO: More suitable exception?
			else
				return Result(o[0], o[1]);
			boost::python::throw_error_already_set();
		}

		template<typename Range>
		inline boost::python::object makePythonRange(const Range& range) {
			/*static*/ boost::python::object rangeClass(boost::python::eval("range"));
			return rangeClass(*boost::begin(range), *boost::end(range));
		}

		template<const ascension::viewer::VisualPoint& (ascension::viewer::Caret::*procedure)() const>
		ascension::kernel::Position positionOfCaret(const ascension::viewer::Caret& c) {
			return (c.*procedure)().position();
		}
//		boost::python::object selectedTextEditor(const EditorWindow& window) {
//			return window.visibleView().asTextEditor();
//		}
	}

	ALPHA_EXPOSE_PROLOGUE(5)
		boost::python::scope scope(ambient::Interpreter::instance().toplevelPackage());

		typedef ascension::graphics::PhysicalTwoAxes<boost::python::object/*ssize_t*/> /*Integral*/PhysicalTwoAxes;
		boost::python::class_<PhysicalTwoAxes>("PhysicalTwoAxes", boost::python::init<>())
			.def(boost::python::init<PhysicalTwoAxes::value_type, PhysicalTwoAxes::value_type>(boost::python::args("x", "y")))
			.add_property("x",
				ambient::makeFunctionPointer([](PhysicalTwoAxes& self) -> PhysicalTwoAxes::value_type {
					return self.x();
				}),
				ambient::makeFunctionPointer([](PhysicalTwoAxes& self, PhysicalTwoAxes::const_reference v) {
					self.x() = v;
				}))
			.add_property("y",
				ambient::makeFunctionPointer([](PhysicalTwoAxes& self) -> PhysicalTwoAxes::value_type {
					return self.y();
				}),
				ambient::makeFunctionPointer([](PhysicalTwoAxes& self, PhysicalTwoAxes::const_reference v) {
					self.y() = v;
				}))
			.def(boost::python::self += boost::python::self)
			.def(boost::python::self + boost::python::self)
			.def(boost::python::self -= boost::python::self)
			.def(boost::python::self - boost::python::self);

		typedef ascension::presentation::FlowRelativeTwoAxes<boost::python::object/*ssize_t*/> /*Integral*/FlowRelativeTwoAxes;
		boost::python::class_<FlowRelativeTwoAxes>("FlowRelativeTwoAxes", boost::python::init<>())
			.def(boost::python::init<FlowRelativeTwoAxes::value_type, FlowRelativeTwoAxes::value_type>(boost::python::args("bpd", "ipd")))
			.add_property("bpd",
				ambient::makeFunctionPointer([](FlowRelativeTwoAxes& self) -> FlowRelativeTwoAxes::value_type {
					return self.bpd();
				}),
				ambient::makeFunctionPointer([](FlowRelativeTwoAxes& self, FlowRelativeTwoAxes::const_reference v) {
					self.bpd() = v;
				}))
			.add_property("ipd",
				ambient::makeFunctionPointer([](FlowRelativeTwoAxes& self) -> FlowRelativeTwoAxes::value_type {
					return self.ipd();
				}),
				ambient::makeFunctionPointer([](FlowRelativeTwoAxes& self, FlowRelativeTwoAxes::const_reference v) {
					self.ipd() = v;
				}))
			.def(boost::python::self += boost::python::self)
			.def(boost::python::self + boost::python::self)
			.def(boost::python::self -= boost::python::self)
			.def(boost::python::self - boost::python::self);

		boost::python::class_<ascension::viewer::VisualDestinationProxy>("_VisualDestinationProxy", boost::python::no_init);

		boost::python::class_<ascension::kernel::Point>("Point", boost::python::init<Buffer&, const ascension::kernel::Position&>())
			.add_property("adapts_to_buffer", &ascension::kernel::Point::adaptsToDocument,
				boost::python::make_function(&ascension::kernel::Point::adaptToDocument, boost::python::return_value_policy<boost::python::reference_existing_object>()))
			.add_property("buffer", boost::python::make_function(
				ambient::makeFunctionPointer([](const ascension::kernel::Point& p) -> const Buffer& {
					return static_cast<const Buffer&>(p.document());
				}),
				boost::python::return_internal_reference<>()))
//			.add_property("excluded_from_restriction", &ascension::kernel::Point::isExcludedFromRestriction,
//				boost::python::make_function(&ascension::kernel::Point::excludeFromRestriction, boost::python::return_value_policy<boost::python::reference_existing_object>()))
			.add_property("gravity", &ascension::kernel::Point::gravity,
				boost::python::make_function(&ascension::kernel::Point::setGravity, boost::python::return_value_policy<boost::python::reference_existing_object>()))
			.add_property("position", boost::python::make_function(&ascension::kernel::Point::position, boost::python::return_value_policy<boost::python::copy_const_reference>()))
			.def("is_buffer_deleted", &ascension::kernel::Point::isDocumentDisposed)
			.def<ascension::kernel::Point& (ascension::kernel::Point::*)(const ascension::kernel::Position&)>(
				"move_to", &ascension::kernel::Point::moveTo, boost::python::return_value_policy<boost::python::reference_existing_object>());

		boost::python::class_<ascension::viewer::Caret, boost::python::bases<ascension::kernel::Point>, boost::noncopyable>("_Caret", boost::python::no_init)
			.add_property("anchor", &positionOfCaret<&ascension::viewer::Caret::anchor>)
			.add_property("beginning", &positionOfCaret<&ascension::viewer::Caret::beginning>)
			.add_property("end", &positionOfCaret<&ascension::viewer::Caret::end>)
			.add_property("selected_region", &ascension::viewer::Caret::selectedRegion)
			.def("begin_rectangle_selection", &ascension::viewer::Caret::beginRectangleSelection)
			.def("can_paste", &ascension::viewer::Caret::canPaste, boost::python::arg("use_killring") = false)
			.def("clear_selection", &ascension::viewer::Caret::clearSelection)
			.def("copy_selection", &ascension::viewer::copySelection)
			.def("cut_selection", &ascension::viewer::cutSelection)
			.def("delete_selection", &ascension::viewer::eraseSelection)
			.def("end_rectangle_selection", &ascension::viewer::Caret::endRectangleSelection)
			.def("extend_selection", &extendSelection)
			.def("input_character", &ascension::viewer::Caret::inputCharacter,
				(boost::python::arg("character"), boost::python::arg("validate_sequence") = true, boost::python::arg("block_controls") = true))
			.def("is_overtype_mode", &ascension::viewer::Caret::isOvertypeMode)
			.def("is_selection_empty", &ascension::viewer::isSelectionEmpty)
			.def("is_selection_rectangle", &ascension::viewer::Caret::isSelectionRectangle)
			.def("paste", &ascension::viewer::Caret::paste, boost::python::arg("use_killring") = false)
			.def("replace_selection", &ascension::viewer::Caret::replaceSelection, (boost::python::arg("text"), boost::python::arg("rectangle_insertion") = false))
			.def<void (ascension::viewer::Caret::*)(const ascension::kernel::Region&)>("select", &ascension::viewer::Caret::select)
			.def("select_word", &ascension::viewer::selectWord)
			.def<ascension::String (*)(const ascension::viewer::Caret&, const ascension::text::Newline&)>("selected_string", &ascension::viewer::selectedString, boost::python::arg("newline") = ascension::text::Newline::USE_INTRINSIC_VALUE)
			.def("set_overtype_mode", &ascension::viewer::Caret::setOvertypeMode, boost::python::arg("set") = true, boost::python::return_value_policy<boost::python::reference_existing_object>())
/*			.def("show_automatically", &Caret::showAutomatically)
			.def("shows_automatically", &Caret::showsAutomatically)*/;

		boost::python::class_<EditorView, boost::noncopyable>("_TextEditor", boost::python::no_init)
			.add_property("buffer", boost::python::make_function(
				ambient::makeFunctionPointer([](const EditorView& editor) -> const Buffer& {
					return editor.document();
				}),
				boost::python::return_internal_reference<>()))
			.add_property("caret", &EditorView::asCaret)
			.add_property("page_size_in_block_flow_direction", ambient::makeFunctionPointer([](const EditorView& editor) -> ascension::graphics::font::TextViewportScrollOffset {
				return ascension::graphics::font::pageSize<ascension::presentation::BlockFlowDirection>(*editor.textArea().textRenderer().viewport());
			}))
			.add_property("page_size_in_inline_flow_direction", ambient::makeFunctionPointer([](const EditorView& editor) -> ascension::graphics::font::TextViewportScrollOffset {
				return ascension::graphics::font::pageSize<ascension::presentation::ReadingDirection>(*editor.textArea().textRenderer().viewport());
			}))
			.add_property("scrollable_range_in_block_flow_direction", ambient::makeFunctionPointer([](const EditorView& editor) -> boost::python::object {
				return makePythonRange(ascension::graphics::font::scrollableRange<ascension::presentation::BlockFlowDirection>(*editor.textArea().textRenderer().viewport()));
			}))
			.add_property("scrollable_range_in_inline_flow_direction", ambient::makeFunctionPointer([](const EditorView& editor) -> boost::python::object {
				return makePythonRange(ascension::graphics::font::scrollableRange<ascension::presentation::ReadingDirection>(*editor.textArea().textRenderer().viewport()));
			}))
			.def("is_scroll_locked", ambient::makeFunctionPointer([](const EditorView& editor) {
				return editor.textArea().textRenderer().viewport()->isScrollLocked();
			}))
			.def("lock_scroll", ambient::makeFunctionPointer([](EditorView& editor) {
				editor.textArea().textRenderer().viewport()->lockScroll();
			}))
			.def("scroll", ambient::makeFunctionPointer([](EditorView& editor, boost::python::object offsets) {
				const boost::python::extract<FlowRelativeTwoAxes> abstractOffsets(offsets);
				if(abstractOffsets.check()) {
					const FlowRelativeTwoAxes ao(static_cast<FlowRelativeTwoAxes>(abstractOffsets));
					const boost::python::extract<ascension::graphics::font::TextViewportSignedScrollOffset> ebpd(ao.bpd()), eipd(ao.ipd());
					if(ebpd.check() && eipd.check()) {
						const ascension::graphics::font::TextViewportSignedScrollOffset bpd(ebpd), ipd(eipd);
						return editor.textArea().textRenderer().viewport()->scroll(
#if 0
							ascension::presentation::makeFlowRelativeTwoAxes((ascension::presentation::_bpd = bpd, ascension::presentation::_ipd = ipd)));
#else
							ascension::presentation::FlowRelativeTwoAxes<ascension::graphics::font::TextViewportSignedScrollOffset>(ascension::presentation::_bpd = ebpd, ascension::presentation::_ipd = eipd));
#endif
					}
				}
				const boost::python::extract<PhysicalTwoAxes> physicalOffsets(offsets);
				if(physicalOffsets.check()) {
					const PhysicalTwoAxes po(static_cast<PhysicalTwoAxes>(physicalOffsets));
					const boost::python::extract<ascension::graphics::font::TextViewportSignedScrollOffset> ex(po.x()), ey(po.y());
					if(ex.check() && ey.check()) {
						const ascension::graphics::font::TextViewportSignedScrollOffset x(ex), y(ey);
						return editor.textArea().textRenderer().viewport()->scroll(
							ascension::graphics::makePhysicalTwoAxes((ascension::graphics::_x = x, ascension::graphics::_y = y)));
					}
				}
				::PyErr_BadArgument();
				boost::python::throw_error_already_set();
			}))
			.def("scroll_block_flow_page", ambient::makeFunctionPointer([](EditorView& editor, boost::python::ssize_t pages) {
				editor.textArea().textRenderer().viewport()->scrollBlockFlowPage(pages);
			}))
			.def("unlock_scroll", ambient::makeFunctionPointer([](EditorView& editor) {
				editor.textArea().textRenderer().viewport()->unlockScroll();
			}));
	ALPHA_EXPOSE_EPILOGUE()
#endif // !ALPHA_NO_AMBIENT
}
