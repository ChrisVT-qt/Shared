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
#include "ClickableLabel.h"

// Qt includes
#include <QApplication>
#include <QDebug>
#include <QDragEnterEvent>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Default constructor
ClickableLabel::ClickableLabel()
    : QLabel()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
ClickableLabel::~ClickableLabel()
{
    CALL_IN("");

    // Nothing to do.

    CALL_OUT("");
}



// =================================================================== Clicking



///////////////////////////////////////////////////////////////////////////////
// Clicked
void ClickableLabel::mousePressEvent(QMouseEvent * mpEvent)
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

    emit SingleClicked();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Double-clicked
void ClickableLabel::mouseDoubleClickEvent(QMouseEvent * mpEvent)
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

    emit DoubleClicked();

    CALL_OUT("");
}
