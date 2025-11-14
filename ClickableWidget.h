// ClickableWidget.h
// Class definition file

/** \class ClickableWidget
  * This class simplifies GUI interactions by emitting signals when clicked,
  * dragged into, dropped on etc.
  *
  * This works also with child widgets.
  */

// Just include once
#ifndef CLICKABLEWIDGET_H
#define CLICKABLEWIDGET_H

// Qt includes
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPoint>
#include <QWidget>

// Class definition
class ClickableWidget
    : public QWidget
{
    Q_OBJECT
    
    
    
    // ============================================================== Lifecycle
public:
    /** \brief Constructor
      */
	ClickableWidget();

    /** \brief Destructor
      */
    virtual ~ClickableWidget();
    
    
    
    // ============================================================ Drag & Drop
protected:
    /** \brief Called when user drags something into the widget
      * \param mpEvent Details of what's being dragged
      */
    virtual void dragEnterEvent(QDragEnterEvent * mpEvent);

    /** \brief Called when user moves what's being dragged
      * \param mpEvent Details of what's being dragged
      */
    virtual void dragMoveEvent(QDragMoveEvent * mpEvent);

    /** \brief Called when user drags something of of the widget
      * \param mpEvent Details of what's being dragged
      */
    void dragLeaveEvent(QDragLeaveEvent * mpEvent);

    /** \brief Called when user drops something into the widget
      * \param mpEvent Details of what's being dropped
      */
    void dropEvent(QDropEvent * mpEvent);

    /** \brief Called when user presses a mouse button inside the widget
      * \param mpEvent Details of the mouse click
      */
    virtual void mousePressEvent(QMouseEvent * mpEvent);

    /** \brief Clicked widget (used for double clicks)
      */
    QWidget * m_ClickedWidget;

    /** \brief Called when the mouse moves inside the widget
      * \param mpEvent Details of the move
      */
    virtual void mouseMoveEvent(QMouseEvent * mpEvent);

    /** \brief Called when user releases the mouse button
      * \param mpEvent Details of the mouse release event
      */
    virtual void mouseReleaseEvent(QMouseEvent * mpEvent);

    /** \brief Called when user double-clicks inside the widget
      * \param mpEvent Details of the double click
      */
    virtual void mouseDoubleClickEvent(QMouseEvent * mpEvent);
    
    /** \brief Drag start position
      */
    QPoint m_DragStartPosition;

    /** \brief Widget that started the drag ("from widget")
      */
    QWidget * m_DragStartWidget;

    /** \brief Widget that the current drag is hovering over ("to widget")
      */
    QWidget * m_DragCurrentWidget;

    // Context menu
    virtual void contextMenuEvent(QContextMenuEvent * mpEvent);



    // ================================================================ Signals
signals:
    /** \brief Emitted when widget is single-clicked
      * \param mpWidget Widget being clicked
      */
    void SingleClicked(QWidget * mpWidget);
    
    /** \brief Emitted when widget is double-clicked
      * \param mpWidget Widget being double-clicked
      */
    void DoubleClicked(QWidget * mpWidget);
    
    /** \brief Emitted when content has been dragged from one widget to another
      * \param mpStartWidget Widget that initiated the drag
      * \param mpEndWidget Widget that was dragged into
      */
    void Dragging(QWidget * mpStartWidget, QWidget * mpEndWidget);
    
    /** \brief Emitted when hovering over a widget
      * \param mpWidget Widget being hovered
      */
    void HoveringOver(QWidget * mpWidget);
    
    /** \brief Emitted when something was dropped onto the widget
      * \param mcpMimeData Content of the drop
      * \param mpWidget Widget that was dropped into
      */
    void DroppedOn(const QMimeData * mcpMimeData, QWidget * mpWidget);

    /** \brief Emitted when context menu is opened
      * \param mpWidget Widget being hovered
      */
    void ContextMenu(QWidget * mpWidget, const QPoint & mcrPosition);



    // =================================================================== Misc
public:
    /** \brief Draw current widget as "selected"
      * \param mcSelectedState \c true is widget is selected, \c false if it
      * isn't.
      */
    void SetSelected(const bool mcSelectedState);
};

#endif

