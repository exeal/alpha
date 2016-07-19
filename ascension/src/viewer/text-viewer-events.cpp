/**
 * @file text-viewer.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2016-02-08 Separated from text-viewer.cpp.
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/geometry/algorithms/normalize.hpp>
#include <ascension/graphics/geometry/algorithms/size.hpp>
#include <ascension/graphics/geometry/algorithms/within.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/mouse-input-strategy.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/viewer/visual-locations.hpp>

namespace ascension {
	namespace viewer {
		void TextViewer::doShowContextMenu(void* nativeEvent) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			const Gdk::Event abstractEvent(Glib::wrap(static_cast<GdkEvent*>(nativeEvent)));
			bool byKeyboard;
			switch(abstractEvent.gobj()->type) {
				case Gdk::BUTTON_RELEASE:
					byKeyboard = false;
					break;
				case Gdk::KEY_RELEASE:
					byKeyboard = true;
					break;
				default:
					return;
			}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			const MSG& message = *static_cast<const MSG*>(nativeEvent);
			const auto globalLocation(win32::makeMouseLocation<boost::geometry::model::d2::point_xy<WORD>>(message.lParam));
			const bool byKeyboard = graphics::geometry::x(globalLocation) == 0xffffu && graphics::geometry::y(globalLocation) == 0xffffu;
#endif

			if(!allowsMouseInput() && !byKeyboard)	// however, may be invoked by other than the mouse...
				return;
			utils::closeCompletionProposalsPopup(*this);
//			texteditor::abortIncrementalSearch(document());

			graphics::Point location;
			widgetapi::event::LocatedUserInput::MouseButton buttons;
			widgetapi::event::KeyboardModifiers modifiers;

			// invoked by the keyboard
			if(byKeyboard) {
				// MSDN says "the application should display the context menu at the location of the current selection."
				location = modelToView(*this, textArea()->caret()->hit());
				// TODO: Support RTL and vertical window layout.
				graphics::geometry::y(location) +=
					widgetapi::createRenderingContext(*this)->fontMetrics(textArea()->textRenderer()->defaultFont())->cellHeight() + 1;
				if(!graphics::geometry::within(location, textArea()->contentRectangle()))
					location = graphics::geometry::make<graphics::Point>((graphics::geometry::_x = 1.0f, graphics::geometry::_y = 1.0f));
			} else {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				double x, y;
				if(!abstractEvent.get_coords(x, y))
					return;	// hmm...
				Gdk::ModifierType state;
				if(!abstractEvent.get_state(state))
					return;
				location = graphics::geometry::make<graphics::Point>((
					graphics::geometry::_x = static_cast<graphics::Scalar>(x), graphics::geometry::_y = static_cast<graphics::Scalar>(y)));
				static const Gdk::ModifierType NATIVE_BUTTON_MASK = Gdk::BUTTON1_MASK | Gdk::BUTTON2_MASK | Gdk::BUTTON3_MASK | Gdk::BUTTON4_MASK | Gdk::BUTTON5_MASK;
				buttons = !byKeyboard ? (state & NATIVE_BUTTON_MASK) : widgetapi::event::LocatedUserInput::NO_BUTTON;
				modifiers = widgetapi::event::KeyboardModifiers::fromNative(state & ~NATIVE_BUTTON_MASK);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				location = graphics::geometry::make<graphics::Point>((
					graphics::geometry::_x = graphics::geometry::x(globalLocation), graphics::geometry::_y = graphics::geometry::y(globalLocation)));
				widgetapi::mapFromGlobal(*this, location);
				buttons = widgetapi::event::LocatedUserInput::NO_BUTTON;
				modifiers = win32::makeModifiers();
#endif
			}

			// ignore if the point is over the scroll bars
			const graphics::Rectangle localBounds(widgetapi::bounds(*this, false));
			if(!graphics::geometry::within(location, localBounds))
				return;

			return showContextMenu(widgetapi::event::LocatedUserInput(location, buttons, modifiers), nativeEvent);
		}

		/// @internal Calls @c #mouseDoubleClicked.
		void TextViewer::fireMouseDoubleClicked(widgetapi::event::MouseButtonInput& input) {
			if(allowsMouseInput())
				mouseDoubleClicked(input);
		}

		/// @internal Calls @c #mouseMoved.
		void TextViewer::fireMouseMoved(widgetapi::event::LocatedUserInput& input) {
			restoreHiddenCursor();
			if(allowsMouseInput())
				mouseMoved(input);
		}

		/// @internal Calls @c #mousePressed.
		void TextViewer::fireMousePressed(widgetapi::event::MouseButtonInput& input) {
			restoreHiddenCursor();
			if(allowsMouseInput())
				mousePressed(input);
		}

		/// @internal Calls @c #mouseReleased.
		void TextViewer::fireMouseReleased(widgetapi::event::MouseButtonInput& input) {
			if(allowsMouseInput() || input.button() == widgetapi::event::LocatedUserInput::BUTTON3_DOWN)
				restoreHiddenCursor();
			if(allowsMouseInput())
				mouseReleased(input);
		}

		/// @internal Calls @c #mouseTripleClicked.
		void TextViewer::fireMouseTripleClicked(widgetapi::event::MouseButtonInput& input) {
			if(allowsMouseInput())
				mouseTripleClicked(input);
		}

		/// @internal Calls @c #mouseWheelChanged.
		void TextViewer::fireMouseWheelChanged(widgetapi::event::MouseWheelInput& input) {
			restoreHiddenCursor();
			if(allowsMouseInput())
				mouseWheelChanged(input);
		}

		namespace {
			void handleDirectionalKey(TextViewer& viewer, graphics::PhysicalDirection direction, const widgetapi::event::KeyboardModifiers& modifiers) {
				if(const auto renderer = viewer.textArea()->textRenderer()) {
					using namespace ascension::texteditor::commands;
					static kernel::Position(*const nextCharacterLocation)
						(const kernel::locations::PointProxy&, Direction, kernel::locations::CharacterUnit, Index) = kernel::locations::nextCharacter;

					const auto abstractDirection = presentation::mapDirection<presentation::FlowRelativeDirection>(renderer->writingModes(), direction);
					const Direction logicalDirection =
						(abstractDirection == presentation::FlowRelativeDirection::AFTER || abstractDirection == presentation::FlowRelativeDirection::END) ?
							Direction::FORWARD : Direction::BACKWARD;
					switch(boost::native_value(abstractDirection)) {
						case presentation::FlowRelativeDirection::BEFORE:
						case presentation::FlowRelativeDirection::AFTER:
							if((modifiers & widgetapi::event::KeyboardModifiers(std::make_tuple(widgetapi::event::SHIFT_DOWN, widgetapi::event::ALT_DOWN)).flip()).none()) {
								if(!modifiers.test(widgetapi::event::ALT_DOWN))
									makeCaretMovementCommand(viewer, &locations::nextVisualLine,
										logicalDirection, modifiers.test(widgetapi::event::SHIFT_DOWN))();
								else if(modifiers.test(widgetapi::event::SHIFT_DOWN))
									makeRowSelectionExtensionCommand(viewer, &locations::nextVisualLine, logicalDirection)();
							}
							break;
						case presentation::FlowRelativeDirection::START:
						case presentation::FlowRelativeDirection::END:
							if((modifiers & widgetapi::event::KeyboardModifiers(std::make_tuple(widgetapi::event::CONTROL_DOWN, widgetapi::event::SHIFT_DOWN, widgetapi::event::ALT_DOWN)).flip()).none()) {
								if(!modifiers.test(widgetapi::event::ALT_DOWN)) {
									if(modifiers.test(widgetapi::event::CONTROL_DOWN))
										makeCaretMovementCommand(viewer, &kernel::locations::nextWord,
											logicalDirection, modifiers.test(widgetapi::event::SHIFT_DOWN))();
									else
										makeCaretMovementCommand(viewer, nextCharacterLocation,
											logicalDirection, modifiers.test(widgetapi::event::SHIFT_DOWN))();
								} else if(modifiers.test(widgetapi::event::SHIFT_DOWN)) {
									if(modifiers.test(widgetapi::event::CONTROL_DOWN))
										makeRowSelectionExtensionCommand(viewer, &kernel::locations::nextWord, logicalDirection)();
									else
										makeRowSelectionExtensionCommand(viewer, nextCharacterLocation, logicalDirection)();
								}
							}
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
				}
			}
		}

		/// Invoked when a key has been pressed.
		void TextViewer::keyPressed(widgetapi::event::KeyInput& input) {
			if(const auto mouseInputStrategy = textArea_->mouseInputStrategy().lock())
				mouseInputStrategy->interruptMouseReaction(true);

			// TODO: This code is temporary. The following code provides a default implementation of
			// TODO: "key combination to command" map.
			using namespace ascension::viewer::widgetapi::event;
			using namespace ascension::texteditor::commands;
			static kernel::Position(*const nextCharacterLocation)(const kernel::locations::PointProxy&, Direction, kernel::locations::CharacterUnit, Index) = kernel::locations::nextCharacter;
//			if(hasModifier<UserInput::ALT_DOWN>(input)) {
//				if(!hasModifier<UserInput::SHIFT_DOWN>(input)
//						|| (input.keyboardCode() != VK_LEFT && input.keyboardCode() != VK_UP
//						&& input.keyboardCode() != VK_RIGHT && input.keyboardCode() != VK_DOWN))
//					return false;
//			}
			switch(input.keyboardCode()) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_BackSpace:
				case GDK_KEY_F16:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Backspace:
				case Qt::Key_F16:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_BACK:
				case VK_F16:
#endif
					if(!input.hasModifierOtherThan(widgetapi::event::SHIFT_DOWN))
						CharacterDeletionCommand(*this, Direction::BACKWARD)();
					else if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						WordDeletionCommand(*this, Direction::BACKWARD)();
					else if(!input.hasModifierOtherThan(std::make_tuple(widgetapi::event::SHIFT_DOWN, widgetapi::event::ALT_DOWN)) && input.hasModifier(widgetapi::event::ALT_DOWN))
						UndoCommand(*this, input.hasModifier(widgetapi::event::SHIFT_DOWN))();
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Clear:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Clear:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_CLEAR:
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						EntireDocumentSelectionCreationCommand(*this)();
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Return:
				case GDK_KEY_KP_Enter:
				case GDK_KEY_ISO_Enter:
				case GDK_KEY_3270_Enter:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Enter:
				case Qt::Key_Return:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_RETURN:
#endif
					if(!input.hasModifierOtherThan(widgetapi::event::SHIFT_DOWN))
						NewlineCommand(*this)();
					else if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						NewlineCommand(*this, Direction::BACKWARD)();
					else if(input.modifiers() == widgetapi::event::KeyboardModifiers(std::make_tuple(widgetapi::event::CONTROL_DOWN, widgetapi::event::SHIFT_DOWN)))
						NewlineCommand(*this, Direction::FORWARD)();	
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Escape:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Escape:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_ESCAPE:
#endif
					if(input.modifiers() == 0)
						CancelCommand(*this)();
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Page_Up:	// 'GDK_KEY_Prior' has same value
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_PageUp:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_PRIOR:
#endif
					if(!input.hasModifierOtherThan(widgetapi::event::SHIFT_DOWN))
						makeCaretMovementCommand(*this, &locations::nextPage, Direction::BACKWARD, input.hasModifier(widgetapi::event::SHIFT_DOWN))();
					else if(input.modifiers() == widgetapi::event::CONTROL_DOWN) {
						try {
							textArea()->viewport()->scrollBlockFlowPage(+1);
						} catch(...) {
						}
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Page_Down:	// 'GDK_KEY_Next' has same value
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_PageDown:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_NEXT:
#endif
					if(!input.hasModifierOtherThan(widgetapi::event::SHIFT_DOWN))
						makeCaretMovementCommand(*this, &locations::nextPage, Direction::FORWARD, input.hasModifier(widgetapi::event::SHIFT_DOWN))();
					else if(input.modifiers() == widgetapi::event::CONTROL_DOWN) {
						try {
							textArea()->viewport()->scrollBlockFlowPage(-1);
						} catch(...) {
						}
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Home:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Home:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_HOME:
#endif
					if(!input.hasModifierOtherThan(std::make_tuple(widgetapi::event::SHIFT_DOWN, widgetapi::event::CONTROL_DOWN))) {
						if(input.hasModifier(widgetapi::event::CONTROL_DOWN))
							makeCaretMovementCommand(*this, &kernel::locations::beginningOfDocument, input.hasModifier(widgetapi::event::SHIFT_DOWN))();
						else
							makeCaretMovementCommand(*this, &locations::beginningOfVisualLine, input.hasModifier(widgetapi::event::SHIFT_DOWN))();
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_End:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_End:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_END:
#endif
					if(!input.hasModifierOtherThan(std::make_tuple(widgetapi::event::SHIFT_DOWN, widgetapi::event::CONTROL_DOWN))) {
						if(input.hasModifier(widgetapi::event::CONTROL_DOWN))
							makeCaretMovementCommand(*this, &kernel::locations::endOfDocument, input.hasModifier(widgetapi::event::SHIFT_DOWN))();
						else
							makeCaretMovementCommand(*this, &locations::endOfVisualLine, input.hasModifier(widgetapi::event::SHIFT_DOWN))();
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Left:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Left:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_LEFT:
#endif
					handleDirectionalKey(*this, graphics::PhysicalDirection::LEFT, input.modifiers());
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Up:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Up:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_UP:
#endif
					handleDirectionalKey(*this, graphics::PhysicalDirection::TOP, input.modifiers());
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Right:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Right:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_RIGHT:
#endif
					handleDirectionalKey(*this, graphics::PhysicalDirection::RIGHT, input.modifiers());
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Down:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Down:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_DOWN:
#endif
					handleDirectionalKey(*this, graphics::PhysicalDirection::BOTTOM, input.modifiers());
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Insert:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Insert:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_INSERT:
#endif
					if(!input.hasModifierOtherThan(std::make_tuple(widgetapi::event::SHIFT_DOWN, widgetapi::event::CONTROL_DOWN))) {
						if(input.hasModifier(widgetapi::event::SHIFT_DOWN))
							PasteCommand(*this, input.hasModifier(widgetapi::event::CONTROL_DOWN))();
						else if(input.hasModifier(widgetapi::event::CONTROL_DOWN))
							copySelection(*textArea()->caret(), true);
						else
							OvertypeModeToggleCommand(*this)();
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Delete:
				case GDK_KEY_KP_Delete:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Delete:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_DELETE:
#endif
					if(input.modifiers() == widgetapi::event::KeyboardModifiers())
						CharacterDeletionCommand(*this, Direction::FORWARD)();
					else if(input.modifiers() == widgetapi::event::SHIFT_DOWN)
						cutSelection(*textArea()->caret(), true);
					else if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						WordDeletionCommand(*this, Direction::FORWARD)();
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_A:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_A:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'A':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						EntireDocumentSelectionCreationCommand(*this)();	// ^A -> Select All
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_C:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_C:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'C':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						copySelection(*textArea()->caret(), true);	// ^C -> Copy
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_H:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_H:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'H':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						CharacterDeletionCommand(*this, Direction::BACKWARD)(), true;	// ^H -> Backspace
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_I:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_I:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'I':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						CharacterInputCommand(*this, 0x0009u)();	// ^I -> Tab
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_J:
				case GDK_KEY_M:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_J:
				case Qt::Key_M:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'J':
				case 'M':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						NewlineCommand(*this, boost::none)();	// ^J or ^M -> New Line
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_V:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_V:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'V':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						PasteCommand(*this, false)();	// ^V -> Paste
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_X:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_X:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'X':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						cutSelection(*textArea()->caret(), true);	// ^X -> Cut
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Y:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Y:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'Y':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						UndoCommand(*this, true)();	// ^Y -> Redo
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Z:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Z:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'Z':
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN)
						UndoCommand(*this, false)();	// ^Z -> Undo
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_KP_5:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_5:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_NUMPAD5:
#endif
					if(input.modifiers() == widgetapi::event::CONTROL_DOWN) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
						if(hasModifier<Qt::KeypadModifier>(input))
#endif
						EntireDocumentSelectionCreationCommand(*this)();
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_F12:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_F12:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_F12:
#endif
					if(input.modifiers() == widgetapi::event::KeyboardModifiers(std::make_tuple(widgetapi::event::CONTROL_DOWN | widgetapi::event::SHIFT_DOWN)))
						CodePointToCharacterConversionCommand(*this)();
					break;

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Undo:
					UndoCommand(*this, false)();
					break;
				case GDK_KEY_Redo:
					UndoCommand(*this, true)();
					break;
//				case GDK_KEY_Shift_L:
//					if(input.hasModifier(widgetapi::event::CONTROL_DOWN) && configuration_.readingDirection == presentation::RIGHT_TO_LEFT)
//						writingModeProvider_->setDefaultDirection(presentation::LEFT_TO_RIGHT);
//					break;
//				case GDK_KEY_Shift_R:
//					if(input.hasModifier(widgetapi::event::CONTROL_DOWN) && configuration_.readingDirection == presentation::LEFT_TO_RIGHT)
//						writingModeProvider_->setDefaultDirection(presentation::RIGHT_TO_LEFT);
//					break;
				case GDK_KEY_Copy:
					copySelection(*textArea()->caret(), true);
					break;
				case GDK_KEY_Cut:
					cutSelection(*textArea()->caret(), true);
					break;
				case GDK_KEY_Paste:
					PasteCommand(*this, false)();
					break;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Copy:
					copySelection(textArea().caret(), true);
					break;
				case Qt::Key_Cut:
					cutSelection(textArea().caret(), true);
					break;
				case Qt::Key_Paste:
					PasteCommand(*this, false)();
					break;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_SHIFT:
					if(input.hasModifier(widgetapi::event::CONTROL_DOWN)) {
						if(::GetKeyState(VK_LSHIFT) < 0 && configuration_.readingDirection == RIGHT_TO_LEFT)
							presentation().setDefaultDirection(LEFT_TO_RIGHT);
						else if(::GetKeyState(VK_RSHIFT) < 0 && configuration_.readingDirection == LEFT_TO_RIGHT)
							presentation().setDefaultDirection(RIGHT_TO_LEFT);
						}
					break;
#endif
				default:
					return input.ignore();
			}
			return input.consume();
		}

		/// Invoked when a key has been released.
		void TextViewer::keyReleased(widgetapi::event::KeyInput& input) {
			if(input.hasModifier(widgetapi::event::ALT_DOWN)) {
				restoreHiddenCursor();
				if(const auto mouseInputStrategy = textArea()->mouseInputStrategy().lock())
					mouseInputStrategy->interruptMouseReaction(true);
			}
			return input.ignore();
		}

		/// Invoked when the mouse button has been double-clicked.
		void TextViewer::mouseDoubleClicked(widgetapi::event::MouseButtonInput& input) {
			if(const auto mis = mouseInputStrategy(input.location()))
				mis->mouseButtonInput(MouseInputStrategy::DOUBLE_CLICKED, input, *this);
		}

		/// Invoked when the mouse cursor has been moved onto a widget.
		void TextViewer::mouseMoved(widgetapi::event::LocatedUserInput& input) {
			if(const auto mis = mouseInputStrategy(input.location()))
				mis->mouseMoved(input, *this);
		}

		/// Invoked when a mouse button has been pressed on a widget.
		void TextViewer::mousePressed(widgetapi::event::MouseButtonInput& input) {
			if(const auto mis = mouseInputStrategy(input.location()))
				mis->mouseButtonInput(MouseInputStrategy::PRESSED, input, *this);
		}

		/// Invoked when a mouse button has been released on a widget.
		void TextViewer::mouseReleased(widgetapi::event::MouseButtonInput& input) {
			if(const auto mis = mouseInputStrategy(input.location()))
				mis->mouseButtonInput(MouseInputStrategy::RELEASED, input, *this);
		}

		/// Invoked when the mouse button has been triple-clicked. 
		void TextViewer::mouseTripleClicked(widgetapi::event::MouseButtonInput& input) {
			if(const auto mis = mouseInputStrategy(input.location()))
				mis->mouseButtonInput(MouseInputStrategy::TRIPLE_CLICKED, input, *this);
		}

		/// Invoked when the mouse wheel is rotated.
		void TextViewer::mouseWheelChanged(widgetapi::event::MouseWheelInput& input) {
			if(const auto mis = mouseInputStrategy(input.location()))
				mis->mouseWheelRotated(input, *this);
		}

		/// @see Widget#paint
		void TextViewer::paint(graphics::PaintContext& context) {
			if(!isFrozen()) {	// skip if frozen
				graphics::Rectangle scheduledBounds(context.boundsToPaint());
				if(!graphics::geometry::isEmpty(graphics::geometry::normalize(scheduledBounds))) {	// skip if the region to paint is empty
					if(const auto ta = textArea()) {
						const auto canvas(ta->allocationRectangle());
						context.save();
						context.beginPath().rectangle(canvas).clip();
						context.translate(graphics::geometry::left(canvas), graphics::geometry::top(canvas));
						ta->paint(context);
						context.restore();
					}
				}
			}
		}

		/// @see Widget#resized
		void TextViewer::resized(const graphics::Dimension&) {
			utils::closeCompletionProposalsPopup(*this);
			if(widgetapi::Proxy<widgetapi::Window> window = widgetapi::window(*this)) {
				if(widgetapi::isMinimized(window))
					return;
			}
			if(textArea_.get() == nullptr)
				return;
			updateTextAreaAllocationRectangle();
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			// notify the tooltip
			win32::AutoZeroSize<TOOLINFOW> ti;
			const graphics::Rectangle viewerBounds(widgetapi::bounds(*this, false));
			ti.hwnd = handle().get();
			ti.uId = 1;
			ti.rect = viewerBounds;
			::SendMessageW(toolTip_.get(), TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			if(contentAssistant() != 0)
				contentAssistant()->viewerBoundsChanged();
		}
	}
}
