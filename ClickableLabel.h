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

    /** \brief Destructor
      */
    virtual ~ClickableLabel();



    // ============================================================ Clicking
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
};

#endif

