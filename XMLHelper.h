// XMLHelper.h
// Class definition file

/** \file
  * \todo Add Doxygen information
  */

// Just include once
#ifndef XMLHELPER_H
#define XMLHELPER_H

// Qt includes
#include <QDomElement>
#include <QObject>
#include <QSet>

// Class definition
class XMLHelper :
    public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:
    // Default constructor (never to be called from outside)
    XMLHelper();

public:
    // Destructor
    ~XMLHelper();



    // ============================================================== Functions
public:
    // Get a particular element
    static QDomElement SearchElement(const QDomElement mcParentElement,
        const QString mcTagName, const QString mcAttribute = "",
        const QString mcAttributeValue = "");

    // Get a particular child element
    static QDomElement NavigateToChildElement(QDomElement mParentElement,
        const QList < QStringList > mcPath);

    // Get all element matching a particular pattern
    static QList < QDomElement > FindAllMatchingElements(
        QDomElement mParentElement, const QString mcTagName,
        const QString mcAttribute = "", const QString mcAttributeValue = "");

    // Convert the contents of a tag to HTML
    static QString ConvertToHTML(const QDomElement mcElement,
        const QSet < QString > mcSuppressTags = QSet < QString >());

    // Pretty-print XML
    static QString PrettyPrintXML(const QDomDocument mcXML,
        const QString & mcrIndent = " ");
    static QString PrettyPrintXML(const QDomElement mcXML,
        const QString & mcrIndent = " ");

private:
    static void PrettyPrintXML_Rec(const QDomElement mcElement,
        const QString & mcrCurrentIndentation, const QString & mcrIndent,
        QString & mrOutput);

public:
    // Copy DOM to another document
    static bool Copy(const QDomElement mcSourceDOM, QDomElement mDOMParent,
        const bool mcIgnoreSourceTagName = false);

    // Parse XML and append DOM to parent
    static bool AppendXML(QDomElement & mrParent, const QString & mcrXML);

    // All attributes
    static QSet < QString > GetAllAttributes(QDomElement & mrParent);

    // Strip leding DOCTYPE tag
    static QString StripDocType(QString mXML);
};

#endif
