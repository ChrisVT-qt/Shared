// ClickableLabel.h
// Class definition file

/** \class ClickableLabel
  * This class simplifies GUI interactions by emitting signals when clicked
  */

// Just include once
#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

// Qt includes
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPoint>

// Class definition
class ClickableLabel
    : public QLabel
{
    Q_OBJECT



    // ============================================================== Lifecycle
public:
    /** \brief Constructor
      */
    ClickableLabel();

    /** \brief Constructor
      */
    ClickableLabel(const QString & mcrText);

    /** \brief Destructor
      */
    virtual ~ClickableLabel();



    // =============================================================== Clicking
protected:
    /** \brief Called when user presses a mouse button inside the widget
      * \param mpEvent Details of the mouse click
      */
    virtual void mousePressEvent(QMouseEvent * mpEvent);

signals:
    /** \brief Emitted when widget is single-clicked
      */
    void SingleClicked();

protected:
    /** \brief Called when user double-clicks inside the widget
      * \param mpEvent Details of the double click
      */
    virtual void mouseDoubleClickEvent(QMouseEvent * mpEvent);

signals:
    /** \brief Emitted when widget is double-clicked
      */
    void DoubleClicked();



    // ============================================================ Drag & Drop
protected:
    /** \brief Called when user drags something into the widget
      * \param mpEvent Details of the drag event
      */
    virtual void dragEnterEvent(QDragEnterEvent * mpEvent);

    /** \brief Called when user drags something out of the widget
      * \param mpEvent Details of the drag event
      */
    virtual void dragLeaveEvent(QDragLeaveEvent * mpEvent);

    /** \brief Called when user drops something on the widget
      * \param mpEvent Details of the drop event
      */
    virtual void dropEvent(QDropEvent * mpEvent);

signals:
    /** \brief Emitted when user dropped something on the widget
      */
    void DroppedURIs(const QMimeData * mcpMIMEData);
};

#endif

