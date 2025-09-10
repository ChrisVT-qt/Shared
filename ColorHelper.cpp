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

// ColorHelper.cpp

// Project includes
#include "CallTracer.h"
#include "ColorHelper.h"
#include "MessageLogger.h"

// Qt includes
#include <QRegularExpression>



///////////////////////////////////////////////////////////////////////////////
// Blend two colors
QColor ColorHelper::Blend(const QColor & mcrColor1, const QColor & mcrColor2,
    const double mcRatio)
{
    CALL_IN(QString("mcrColor1=%1, mcrColor2=%2, mcRatio=%3")
        .arg(CALL_SHOW(mcrColor1),
             CALL_SHOW(mcrColor2),
             CALL_SHOW(mcRatio)));

    int r = mcrColor1.red()   * (1-mcRatio) + mcrColor2.red()   * mcRatio;
    int g = mcrColor1.green() * (1-mcRatio) + mcrColor2.green() * mcRatio;
    int b = mcrColor1.blue()  * (1-mcRatio) + mcrColor2.blue()  * mcRatio;

    CALL_OUT("");
    return QColor(r, g, b, 255);
}



///////////////////////////////////////////////////////////////////////////////
// Convert a color to a HTML color
QString ColorHelper::ToHTML(const QColor & mcrColor)
{
    CALL_IN(QString("mcrColor=%1")
        .arg(CALL_SHOW(mcrColor)));

    // Check for invalid/uninitialized colors
    if (!mcrColor.isValid())
    {
        const QString reason = QObject::tr("Color object is not valid.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    // Valid color
    const QString html = QString("#%1%2%3")
        .arg(mcrColor.red(), 2, 16, QChar('0'))
        .arg(mcrColor.green(), 2, 16, QChar('0'))
        .arg(mcrColor.blue(), 2, 16, QChar('0'));

    CALL_OUT("");
    return html;
}



///////////////////////////////////////////////////////////////////////////////
// Convert a HTML color to a QColor
QColor ColorHelper::FromHTML(const QString & mcrHTMLColor)
{
    CALL_IN(QString("mcrHTMLColor=%1")
        .arg(CALL_SHOW(mcrHTMLColor)));

    static const QRegularExpression format_color(
        "^#?([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})$");
    const QRegularExpressionMatch match_color =
        format_color.match(mcrHTMLColor.toLower());

    // Check for invalid format
    if (!match_color.hasMatch())
    {
        const QString reason = QObject::tr("\"%1\" is not a valid HTML color.")
            .arg(mcrHTMLColor);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QColor();
    }

    // Create color
    bool ok;
    const int color_r = match_color.captured(1).toInt(&ok, 16);
    const int color_g = match_color.captured(2).toInt(&ok, 16);
    const int color_b = match_color.captured(3).toInt(&ok, 16);
    QColor color(color_r, color_g, color_b);

    CALL_OUT("");
    return color;
}
