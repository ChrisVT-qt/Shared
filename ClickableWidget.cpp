// Copyright (C) 2023 Chris von Toerne
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Contact the author by email: christian.vontoerne@gmail.com

// ClickableWidget.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "ClickableWidget.h"

// Qt includes
#include <QApplication>
#include <QDebug>
#include <QDragEnterEvent>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Default constructor
ClickableWidget::ClickableWidget()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Fill background (needed to show selection state)
    setAutoFillBackground(true);
    
    // Accepts drops
    setAcceptDrops(true);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
ClickableWidget::~ClickableWidget()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do.

    CALL_OUT("");
}

    
    
// ================================================================ Drag & Drop



///////////////////////////////////////////////////////////////////////////////
// Drag in...
void ClickableWidget::dragEnterEvent(QDragEnterEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // We accept files only.
    if (!mpEvent -> mimeData() -> hasFormat("text/uri-list"))
    {
        // Ignore event
        mpEvent -> ignore();
        CALL_OUT("");
        return;
    }
    
    // Accept event
    mpEvent -> acceptProposedAction();
    
    // Emit signal
    emit HoveringOver(childAt(mpEvent -> position().toPoint()));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// ... move
void ClickableWidget::dragMoveEvent(QDragMoveEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Accept event
    mpEvent -> acceptProposedAction();

    // Emit signal
    emit HoveringOver(childAt(mpEvent -> position().toPoint()));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// ... leave
void ClickableWidget::dragLeaveEvent(QDragLeaveEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Accept event
    mpEvent -> accept();

    // Emit signal
    emit HoveringOver(nullptr);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// ... and drop
void ClickableWidget::dropEvent(QDropEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Accept event
    mpEvent -> acceptProposedAction();

    // Emit event
    emit DroppedOn(mpEvent -> mimeData(),
        childAt(mpEvent -> position().toPoint()));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Clicked
void ClickableWidget::mousePressEvent(QMouseEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Check if left mousebutton is pressed
    if (!(mpEvent -> buttons() & Qt::LeftButton))
    {
        // Nope. We don't care, then.
        CALL_OUT("");
        return;
    }
    
    // Accept event
    mpEvent -> accept();

    // Check for widget under mouse pointer
    QWidget * clicked_widget = childAt(mpEvent -> pos()); 
    
    // Send signal if we got it
    if (clicked_widget)
    {
        emit SingleClicked(clicked_widget);
    }

    // Avoid invalid double clicks    
    m_ClickedWidget = clicked_widget;

    // We may start dragging.
    m_DragStartPosition = mpEvent -> pos();
    m_DragStartWidget = clicked_widget;
    m_DragCurrentWidget = clicked_widget;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Move
void ClickableWidget::mouseMoveEvent(QMouseEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Check if left mousebutton is pressed
    if (!(mpEvent -> buttons() & Qt::LeftButton))
    {
        // Nope. We don't care, then.
        CALL_OUT("");
        return;
    }
    
    // Check if we moved a short distance (until we start to drag)
    if ((mpEvent -> pos() - m_DragStartPosition).manhattanLength()
          < QApplication::startDragDistance())
    {
        // No. Do nothing. Little movements do not result in a drag.
        CALL_OUT("");
        return;
    }
    
    // Big ones do. So we drag.

    // Accept event
    mpEvent -> accept();
    
    // Current widget
    QWidget * current_widget = childAt(mpEvent -> pos());
    if (current_widget != m_DragCurrentWidget)
    {
        // Current target widget has changed
        m_DragCurrentWidget = current_widget;
        emit Dragging(m_DragStartWidget, m_DragCurrentWidget);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Mouse button was released
void ClickableWidget::mouseReleaseEvent(QMouseEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Accept event
    mpEvent -> accept();
    
    // Clear dragging start/end
    m_DragStartWidget = nullptr;
    m_DragCurrentWidget = nullptr;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Double-clicked
void ClickableWidget::mouseDoubleClickEvent(QMouseEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Check if it's the left mousebutton
    if (mpEvent -> button() != Qt::LeftButton)
    {
        // Nope. Do nothing.
        CALL_OUT("");
        return;
    }
    
    // Accept event
    mpEvent -> accept();

    // Check for widget under mouse pointer
    QWidget * clicked_widget = childAt(mpEvent -> pos()); 
    
    // Check if it's the same as the first clicked widget
    // (click - move - click is no double-click!)
    if (clicked_widget && clicked_widget == m_ClickedWidget)
    {
        emit DoubleClicked(clicked_widget);
    }

    CALL_OUT("");
}



// ======================================================================= Misc



///////////////////////////////////////////////////////////////////////////////
// Selected
void ClickableWidget::SetSelected(const bool mcSelectedState)
{
    CALL_IN(QString("mcSelectedState=%1")
        .arg(CALL_SHOW(mcSelectedState)));

    setBackgroundRole(mcSelectedState ? QPalette::Text : QPalette::Window);

    CALL_OUT("");
}
