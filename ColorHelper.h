// ColorHelper.h

/** \namespace ColorHelper
  * Home of convenience functions simplying working with colors
  */

#ifndef COLORHELPER_H
#define COLORHELPER_H

// Qt includes
#include <QColor>

namespace ColorHelper
{
    /** \brief Blend two colors to create a third one
      * \param mcrColor1 First color
      * \param mcrColor2 First color
      * \param mcRatio Mixing ratio, needs to be between 0 and 1. \c mcrColor1
      * will be returned for \c mcRatio=0, and \c mcrColor2 for \c mcRatio=1.
      * \return The blended color
      */
    QColor Blend(const QColor & mcrColor1, const QColor & mcrColor2,
        const double mcRatio);

    /** \brief Convert a color to a HTML color
      * \param mcrColor The color to be converted
      * \return The HTML representation of the color
      */
    QString ToHTML(const QColor & mcrColor);

    /** \brief Convert a HTML color to a QColor
      * \param mcrHTMLColor The color to be converted
      * \return The QColor representation of the color
      */
    QColor FromHTML(const QString & mcrHTMLColor);
}

#endif
