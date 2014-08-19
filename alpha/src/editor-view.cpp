/**
 * @file editor-view.cpp
 * @author exeal
 * @date 2008 Separated from buffer.cpp)
 * @date 2008-2009, 2014
 */

#include "application.hpp"
#include "buffer-list.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/text-editor/command.hpp>	// ascension.texteditor.commands.IncrementalSearchCommand
#include <ascension/viewer/caret.hpp>
#include <boost/range/algorithm/replace_if.hpp>
#include <glibmm/i18n.h>

namespace alpha {
	/// Constructor.
	EditorView::EditorView(ascension::presentation::Presentation& presentation) : ascension::viewers::TextViewer(presentation), visualColumnStartValue_(1) {
		document().bookmarker().addListener(*this);
//		caretObject_.reset(new CaretProxy(caret()));
	}

	/// Copy-constructor.
	EditorView::EditorView(const EditorView& other) : ascension::viewers::TextViewer(other), visualColumnStartValue_(other.visualColumnStartValue_) {
		document().bookmarker().addListener(*this);
//		caretObject_.reset(new CaretProxy(caret()));
	}

	/// Destructor.
	EditorView::~EditorView() {
		document().bookmarker().removeListener(*this);
	}

	/// Begins incremental search.
	void EditorView::beginIncrementalSearch(ascension::searcher::TextSearcher::Type type, ascension::Direction direction) {
		ascension::texteditor::commands::IncrementalFindCommand(*this, type, direction, this)();
	}

	/// @see BookmarkListener#bookmarkChanged
	void EditorView::bookmarkChanged(ascension::Index line) {
		redrawLine(line);
	}

	/// @see BookmarkListener#bookmarkCleared
	void EditorView::bookmarkCleared() {
		queue_draw();
	}

	/// @see TextViewer#drawIndicatorMargin
	void EditorView::drawIndicatorMargin(ascension::Index line, ascension::graphics::PaintContext& context, const ascension::graphics::Rectangle& rect) {
		if(document().bookmarker().isMarked(line)) {
			// draw a bookmark indication mark
			namespace gfx = ascension::graphics;
			auto xrange(gfx::geometry::range<0>(rect));
			xrange.advance_begin(+2);
			xrange.advance_end(-2);
			auto yrange(boost::irange((gfx::geometry::top(rect) * 2 + gfx::geometry::bottom(rect)) / 3, (gfx::geometry::top(rect) + gfx::geometry::bottom(rect) * 2) / 3));
			const gfx::Rectangle r(xrange, yrange);
			auto gradient(std::make_shared<gfx::LinearGradient>(boost::geometry::make_zero<gfx::Point>(), gfx::Point(gfx::geometry::_x = gfx::geometry::dx(r), gfx::geometry::_y = static_cast<gfx::Scalar>(0))));

			// get themed colors
			if(Glib::RefPtr<Gtk::StyleContext> styleContext = get_style_context()) {
				const Gtk::StateFlags state = Gtk::STATE_FLAG_ACTIVE | Gtk::STATE_FLAG_SELECTED;
				gradient->addColorStop(0, ascension::graphics::Color::from(styleContext->get_background_color(state)));
				gradient->addColorStop(1, ascension::graphics::Color::from(styleContext->get_color(state)));
				context.setFillStyle(gradient);
				context.fillRectangle(r);
			}
		}
	}

	/// @see TextViewer#focusAboutToBeLost
	void EditorView::focusAboutToBeLost(ascension::viewers::widgetapi::Event& event) {
		ascension::viewers::TextViewer::focusAboutToBeLost(event);
		BufferList::instance().editorSession().incrementalSearcher().end();
	}

	/// @see TextViewer#focusGained
	void EditorView::focusGained(ascension::viewers::widgetapi::Event& event) {
		ascension::viewers::TextViewer::focusGained(event);
		BufferList::instance().select(document());
	}

	/// @see IncrementalSearchListener#incrementalSearchAborted
	void EditorView::incrementalSearchAborted(const ascension::kernel::Position& initialPosition) {
		incrementalSearchCompleted();
		caret().moveTo(initialPosition);
	}

	/// @see IncrementalSearchListener#incrementalSearchCompleted
	void EditorView::incrementalSearchCompleted() {
		Application::instance()->window().statusBar().pop();
	}

	/// @see IncrementalSearchListener#incrementalSearchPatternChanged
	void EditorView::incrementalSearchPatternChanged(ascension::searcher::IncrementalSearchCallback::Result result, int wrappingStatus) {

		const Glib::RefPtr<Application> app(Application::instance());
		ui::StatusBar& statusBar = app->window().statusBar();
		const ascension::searcher::IncrementalSearcher& isearch = BufferList::instance().editorSession().incrementalSearcher();
		const bool forward = isearch.direction() == ascension::Direction::FORWARD;
		Glib::ustring message;
		bool messageIsFormat = true;

		if(result == ascension::searcher::IncrementalSearchCallback::EMPTY_PATTERN) {
			caret().select(isearch.matchedRegion());
			message = forward ? _("Incremental search : (empty pattern)") : _("Reversal incremental search : (empty pattern)");
			messageIsFormat = false;
		} else if(result == ascension::searcher::IncrementalSearchCallback::FOUND) {
			caret().select(isearch.matchedRegion());
			message = forward ? _("Incremental search : %1") : _("Reversal incremental search : %1");
		} else {
			if(result == ascension::searcher::IncrementalSearchCallback::NOT_FOUND)
				message = forward ? _("Incremental search : %1 (not found)") : _("Reversal incremental search : %1 (not found)");
			else
				message = forward ? _("Incremental search : %1 (invalid pattern)") : _("Reversal incremental search : %1 (invalid pattern)");
			beep();
		}

		if(messageIsFormat) {
			message = Glib::ustring::compose(message, isearch.pattern());
			std::string temp(message);
			boost::replace_if(temp, std::bind(std::equal_to<std::string::value_type>(), '\t', std::placeholders::_1), ' ');
			message = temp;
		}
		statusBar.push(message);
//		if(boost::get_optional_value_or(app->readIntegerProfile("View", "applyMainFontToSomeControls"), 1) != 0)
//			statusBar.override_font(textRenderer().defaultFont()->describeProperties());
	}

	/// @see IncrementalSearchListener#incrementalSearchStarted
	void EditorView::incrementalSearchStarted(const ascension::kernel::Document&) {
	}

	/// @see TextViewer#keyPressed
	void EditorView::keyPressed(ascension::viewers::widgetapi::KeyInput& input) {
		// disable default keyboard bindings
//		return ascension::viewers::TextViewer::keyPressed(input);
	}
#if 0
	/// @see Caret#MatchBracketsChangedSignal
	void EditorView::matchBracketsChanged(const ascension::viewers::Caret& self, const boost::optional<std::pair<ascension::kernel::Position, ascension::kernel::Position>>& previouslyMatchedBrackets, bool outsideOfView) {
		// TODO: indicate if the pair is outside of the viewer.
	}
#endif

	namespace {
		void extendSelection(ascension::viewers::Caret& caret, boost::python::object to) {
			if(boost::python::extract<const ascension::kernel::Position&>(to).check())
				caret.extendSelectionTo(static_cast<ascension::kernel::Position>(boost::python::extract<ascension::kernel::Position>(to)));
			else if(boost::python::extract<const ascension::viewers::VisualDestinationProxy&>(to).check())
				caret.extendSelectionTo(static_cast<ascension::viewers::VisualDestinationProxy>(boost::python::extract<ascension::viewers::VisualDestinationProxy>(to)));
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

		template<const ascension::viewers::VisualPoint& (ascension::viewers::Caret::*procedure)() const>
		ascension::kernel::Position positionOfCaret(const ascension::viewers::Caret& c) {
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

		typedef ascension::presentation::AbstractTwoAxes<boost::python::object/*ssize_t*/> /*Integral*/AbstractTwoAxes;
		boost::python::class_<AbstractTwoAxes>("AbstractTwoAxes", boost::python::init<>())
			.def(boost::python::init<AbstractTwoAxes::value_type, AbstractTwoAxes::value_type>(boost::python::args("bpd", "ipd")))
			.add_property("bpd",
				ambient::makeFunctionPointer([](AbstractTwoAxes& self) -> AbstractTwoAxes::value_type {
					return self.bpd();
				}),
				ambient::makeFunctionPointer([](AbstractTwoAxes& self, AbstractTwoAxes::const_reference v) {
					self.bpd() = v;
				}))
			.add_property("ipd",
				ambient::makeFunctionPointer([](AbstractTwoAxes& self) -> AbstractTwoAxes::value_type {
					return self.ipd();
				}),
				ambient::makeFunctionPointer([](AbstractTwoAxes& self, AbstractTwoAxes::const_reference v) {
					self.ipd() = v;
				}))
			.def(boost::python::self += boost::python::self)
			.def(boost::python::self + boost::python::self)
			.def(boost::python::self -= boost::python::self)
			.def(boost::python::self - boost::python::self);

		boost::python::class_<ascension::viewers::VisualDestinationProxy>("_VisualDestinationProxy", boost::python::no_init);

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

		boost::python::class_<ascension::viewers::Caret, boost::python::bases<ascension::kernel::Point>, boost::noncopyable>("_Caret", boost::python::no_init)
			.add_property("anchor", &positionOfCaret<&ascension::viewers::Caret::anchor>)
			.add_property("beginning", &positionOfCaret<&ascension::viewers::Caret::beginning>)
			.add_property("end", &positionOfCaret<&ascension::viewers::Caret::end>)
			.add_property("selected_region", &ascension::viewers::Caret::selectedRegion)
			.def("begin_rectangle_selection", &ascension::viewers::Caret::beginRectangleSelection)
			.def("can_paste", &ascension::viewers::Caret::canPaste, boost::python::arg("use_killring") = false)
			.def("clear_selection", &ascension::viewers::Caret::clearSelection)
			.def("copy_selection", &ascension::viewers::copySelection)
			.def("cut_selection", &ascension::viewers::cutSelection)
			.def("delete_selection", &ascension::viewers::eraseSelection)
			.def("end_rectangle_selection", &ascension::viewers::Caret::endRectangleSelection)
			.def("extend_selection", &extendSelection)
			.def("input_character", &ascension::viewers::Caret::inputCharacter,
				(boost::python::arg("character"), boost::python::arg("validate_sequence") = true, boost::python::arg("block_controls") = true))
			.def("is_overtype_mode", &ascension::viewers::Caret::isOvertypeMode)
			.def("is_selection_empty", &ascension::viewers::isSelectionEmpty)
			.def("is_selection_rectangle", &ascension::viewers::Caret::isSelectionRectangle)
			.def("paste", &ascension::viewers::Caret::paste, boost::python::arg("use_killring") = false)
			.def("replace_selection", &ascension::viewers::Caret::replaceSelection, (boost::python::arg("text"), boost::python::arg("rectangle_insertion") = false))
			.def<void (ascension::viewers::Caret::*)(const ascension::kernel::Region&)>("select", &ascension::viewers::Caret::select)
			.def("select_word", &ascension::viewers::selectWord)
			.def<ascension::String (*)(const ascension::viewers::Caret&, const ascension::text::Newline&)>("selected_string", &ascension::viewers::selectedString, boost::python::arg("newline") = ascension::text::Newline::USE_INTRINSIC_VALUE)
			.def("set_overtype_mode", &ascension::viewers::Caret::setOvertypeMode, boost::python::arg("set") = true, boost::python::return_value_policy<boost::python::reference_existing_object>())
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
				return ascension::graphics::font::pageSize<ascension::presentation::BlockFlowDirection>(*editor.textRenderer().viewport());
			}))
			.add_property("page_size_in_inline_flow_direction", ambient::makeFunctionPointer([](const EditorView& editor) -> ascension::graphics::font::TextViewportScrollOffset {
				return ascension::graphics::font::pageSize<ascension::presentation::ReadingDirection>(*editor.textRenderer().viewport());
			}))
			.add_property("scrollable_range_in_block_flow_direction", ambient::makeFunctionPointer([](const EditorView& editor) -> boost::python::object {
				return makePythonRange(ascension::graphics::font::scrollableRange<ascension::presentation::BlockFlowDirection>(*editor.textRenderer().viewport()));
			}))
			.add_property("scrollable_range_in_inline_flow_direction", ambient::makeFunctionPointer([](const EditorView& editor) -> boost::python::object {
				return makePythonRange(ascension::graphics::font::scrollableRange<ascension::presentation::ReadingDirection>(*editor.textRenderer().viewport()));
			}))
			.def("is_scroll_locked", ambient::makeFunctionPointer([](const EditorView& editor) {
				return editor.textRenderer().viewport()->isScrollLocked();
			}))
			.def("lock_scroll", ambient::makeFunctionPointer([](EditorView& editor) {
				editor.textRenderer().viewport()->lockScroll();
			}))
			.def("scroll", ambient::makeFunctionPointer([](EditorView& editor, boost::python::object offsets) {
				const boost::python::extract<AbstractTwoAxes> abstractOffsets(offsets);
				if(abstractOffsets.check()) {
					const AbstractTwoAxes ao(static_cast<AbstractTwoAxes>(abstractOffsets));
					const boost::python::extract<ascension::graphics::font::TextViewportSignedScrollOffset> ebpd(ao.bpd()), eipd(ao.ipd());
					if(ebpd.check() && eipd.check()) {
						const ascension::graphics::font::TextViewportSignedScrollOffset bpd(ebpd), ipd(eipd);
						return editor.textRenderer().viewport()->scroll(
#if 0
							ascension::presentation::makeAbstractTwoAxes((ascension::presentation::_bpd = bpd, ascension::presentation::_ipd = ipd)));
#else
							ascension::presentation::AbstractTwoAxes<ascension::graphics::font::TextViewportSignedScrollOffset>(ascension::presentation::_bpd = ebpd, ascension::presentation::_ipd = eipd));
#endif
					}
				}
				const boost::python::extract<PhysicalTwoAxes> physicalOffsets(offsets);
				if(physicalOffsets.check()) {
					const PhysicalTwoAxes po(static_cast<PhysicalTwoAxes>(physicalOffsets));
					const boost::python::extract<ascension::graphics::font::TextViewportSignedScrollOffset> ex(po.x()), ey(po.y());
					if(ex.check() && ey.check()) {
						const ascension::graphics::font::TextViewportSignedScrollOffset x(ex), y(ey);
						return editor.textRenderer().viewport()->scroll(
							ascension::graphics::makePhysicalTwoAxes((ascension::graphics::_x = x, ascension::graphics::_y = y)));
					}
				}
				::PyErr_BadArgument();
				boost::python::throw_error_already_set();
			}))
			.def("scroll_block_flow_page", ambient::makeFunctionPointer([](EditorView& editor, boost::python::ssize_t pages) {
				editor.textRenderer().viewport()->scrollBlockFlowPage(pages);
			}))
			.def("unlock_scroll", ambient::makeFunctionPointer([](EditorView& editor) {
				editor.textRenderer().viewport()->unlockScroll();
			}));
	ALPHA_EXPOSE_EPILOGUE()
}
