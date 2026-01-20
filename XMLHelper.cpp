// XMLHelper.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "MessageLogger.h"
#include "XMLHelper.h"



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Default constructor (never to be called from outside)
XMLHelper::XMLHelper()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
XMLHelper::~XMLHelper()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Get a particular element
QDomElement XMLHelper::SearchElement(const QDomElement mcParentElement,
    const QString mcTagName, const QString mcAttribute,
    const QString mcAttributeValue)
{
    CALL_IN(QString("mcParentElement=%1, mcTagName=%2, mcAttribute=%3, "
        "mcAttributeValue=%4")
        .arg(CALL_SHOW(mcParentElement),
             CALL_SHOW(mcTagName),
             CALL_SHOW(mcAttribute),
             CALL_SHOW(mcAttributeValue)));

    QList < QDomElement > to_check;
    to_check << mcParentElement;
    while (!to_check.isEmpty())
    {
        // Check if this is <[tag]> if not attribute and values are provided
        QDomElement element = to_check.takeFirst();

        // Has a particular tag
        if (element.tagName() == mcTagName &&
            mcAttribute.isEmpty() &&
            mcAttributeValue.isEmpty())
        {
            CALL_OUT("");
            return element;
        }

        // Has a particular tag and a particular attribute
        if (element.tagName() == mcTagName &&
            element.hasAttribute(mcAttribute) &&
            mcAttributeValue.isEmpty())
        {
            CALL_OUT("");
            return element;
        }

        // Has a particular tag, a particular attribute, and the attribute
        // has a particular value
        if (element.tagName() == mcTagName &&
            mcAttribute.isEmpty() &&
            mcAttributeValue.isEmpty())
        {
            CALL_OUT("");
            return element;
        }

        // Check if this is <[tag] [attribute]=[value]>
        if (element.tagName() == mcTagName &&
            element.hasAttribute(mcAttribute) &&
            element.attribute(mcAttribute) == mcAttributeValue)
        {
            CALL_OUT("");
            return element;
        }

        // Check children
        for (QDomNode child_node = element.firstChild();
             !child_node.isNull();
             child_node = child_node.nextSibling())
        {
            if (child_node.isElement())
            {
                to_check << child_node.toElement();
            }
        }
    }

    // Not found
    CALL_OUT("");
    return QDomElement();
}



///////////////////////////////////////////////////////////////////////////////
// Get a particular child element
QDomElement XMLHelper::NavigateToChildElement(const QDomElement mcParentElement,
    const QList < QStringList > mcPath)
{
    CALL_IN(QString("mcParentElement=%1, mcPath=%2")
        .arg(CALL_SHOW(mcParentElement),
             CALL_SHOW(mcPath)));

    // Check if parent exists
    if (mcParentElement.isNull())
    {
        const QString reason(tr("Parent element provided is null."));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDomElement();
    }

    // Check if we have three element in each path item
    for (const QStringList & item : mcPath)
    {
        if (item.size() != 3)
        {
            const QString reason(tr("Path items shall have three strings; "
                "this one does not: %1.")
                .arg(CALL_SHOW(item)));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QDomElement();
        }
    }

    // Navigate to target element
    QDomElement target = mcParentElement;
    for (const QStringList & item : mcPath)
    {
        const QString & tag = item[0];
        const QString & attribute = item[1];
        const QString & value = item[2];
        bool found = false;
        for (QDomElement child = target.firstChildElement(tag);
             !child.isNull();
             child = child.nextSiblingElement(tag))
        {
            if (child.attribute(attribute, "") == value)
            {
                found = true;
                target = child;
                break;
            }
        }
        if (!found)
        {
            // No necessarily an error; up to calling method
            CALL_OUT("");
            return QDomElement();
        }
    }

    CALL_OUT("");
    return target;
}



///////////////////////////////////////////////////////////////////////////////
// Get all element matching a particular pattern
QList < QDomElement > XMLHelper::FindAllMatchingElements(
    QDomElement mParentElement, const QString mcTagName,
    const QString mcAttribute, const QString mcAttributeValue)
{
   CALL_IN(QString("mParentElement=%1, mcTagName=%2, mcAttribute=%3, "
    "mcAttributeValue=%4")
        .arg(CALL_SHOW(mParentElement),
             CALL_SHOW(mcTagName),
             CALL_SHOW(mcAttribute),
             CALL_SHOW(mcAttributeValue)));

    // Private method - no checks

    // Find matches
    QList < QDomElement > matches;
    for (QDomNode dom_child = mParentElement.firstChild();
         !dom_child.isNull();
         dom_child = dom_child.nextSibling())
    {
        if (!dom_child.isElement())
        {
            continue;
        }
        QDomElement dom_element = dom_child.toElement();
        if (dom_element.tagName() == mcTagName)
        {
            bool found = false;
            if (mcAttribute.isEmpty())
            {
                found = true;
            } else if (dom_element.hasAttribute(mcAttribute) &&
                       dom_element.attribute(mcAttribute) == mcAttributeValue)
            {
                found = true;
            }
            if (found)
            {
                matches << dom_element;
            }
        }
        matches << FindAllMatchingElements(dom_element,
            mcTagName, mcAttribute, mcAttributeValue);
    }

    CALL_OUT("");
    return matches;
}



///////////////////////////////////////////////////////////////////////////////
// Convert the contents of a tag to HTML
QString XMLHelper::ConvertToHTML(const QDomElement mcElement,
    const QSet < QString > mcSuppressTags)
{
    CALL_IN(QString("mcElement=%1")
        .arg(CALL_SHOW(mcElement)));

    // Make sure we have an element
    if (mcElement.isNull())
    {
        const QString reason(tr("Element provided is null."));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    // No checks on mcSuppressTags

    // Gather content
    QStringList content;
    for (QDomNode child = mcElement.firstChild();
         !child.isNull();
         child = child.nextSibling())
    {
        if (child.isText())
        {
            content << child.toText().data().trimmed();
            continue;
        }
        if (child.isElement())
        {
            QDomElement element = child.toElement();
            QString sub_content = ConvertToHTML(element, mcSuppressTags);
            const QString tag = element.tagName();
            if (mcSuppressTags.contains(tag))
            {
                content << sub_content;
            } else
            {
                QDomNamedNodeMap attributes = element.attributes();
                QStringList attributes_txt;
                for (int index = 0;
                     index < attributes.size();
                     index++)
                {
                    QDomAttr attribute = attributes.item(index).toAttr();
                    attributes_txt << QString(" %1=\"%2\"")
                        .arg(attribute.name(),
                             attribute.value());
                }
                content << QString("<%1%2>%3</%1>")
                    .arg(element.tagName(),
                         attributes_txt.join(""),
                         sub_content);
            }
            continue;
        }
    }

    CALL_OUT("");
    return content.join("");
}



///////////////////////////////////////////////////////////////////////////////
// Pretty-print XML
QString XMLHelper::PrettyPrintXML(const QDomDocument mcXML,
    const QString & mcrIndent)
{
    CALL_IN(QString("mcXML=%1, mcrIndent=%2")
        .arg(CALL_SHOW(mcXML),
             CALL_SHOW(mcrIndent)));

    QString xml;
    PrettyPrintXML_Rec(mcXML.documentElement(), "", mcrIndent, xml);

    CALL_OUT("");
    return xml;
}



///////////////////////////////////////////////////////////////////////////////
// Pretty-print XML
QString XMLHelper::PrettyPrintXML(const QDomElement mcXML,
    const QString & mcrIndent)
{
    CALL_IN(QString("mcXML=%1, mcrIndent=%2")
        .arg(CALL_SHOW(mcXML),
             CALL_SHOW(mcrIndent)));

    QString xml;
    PrettyPrintXML_Rec(mcXML, "", mcrIndent, xml);

    CALL_OUT("");
    return xml;
}



///////////////////////////////////////////////////////////////////////////////
void XMLHelper::PrettyPrintXML_Rec(const QDomElement mcElement,
    const QString & mcrCurrentIndentation, const QString & mcrIndent,
    QString & mrOutput)
{
    CALL_IN(QString("mcElement=%1, mcrCurrentIndentation=%2, mcrIndent=%3, "
        "mrOutput=%4")
        .arg(CALL_SHOW(mcElement),
             CALL_SHOW(mcrCurrentIndentation),
             CALL_SHOW(mcrIndent),
             CALL_SHOW(mrOutput)));

    // == This tag
    QStringList attributes;
    QDomNamedNodeMap all_attributes = mcElement.attributes();
    for (int attr_index = 0;
         attr_index < all_attributes.count();
         attr_index++)
    {
        const QDomAttr dom_attribute =
            all_attributes.item(attr_index).toAttr();
        attributes += QString("%1=\"%2\"")
            .arg(dom_attribute.name(),
                 dom_attribute.value());
    }
    std::sort(attributes.begin(), attributes.end());
    const bool has_children = mcElement.hasChildNodes();
    mrOutput += QString("%1<%2%3%4%5>\n")
        .arg(mcrCurrentIndentation,
             mcElement.tagName(),
             attributes.isEmpty() ? "" : " ",
             attributes.join(" "),
             has_children ? "" : "/");

    if (has_children)
    {
        // == Subtags
        QString indent = mcrCurrentIndentation + mcrIndent;
        for (QDomNode dom_child = mcElement.firstChild();
             !dom_child.isNull();
             dom_child = dom_child.nextSibling())
        {
            if (dom_child.isText())
            {
                mrOutput += QString("%1%2\n")
                    .arg(indent,
                         dom_child.toText().data());
            } else if (dom_child.isElement())
            {
                PrettyPrintXML_Rec(dom_child.toElement(), indent, mcrIndent,
                    mrOutput);
            }
        }

        // == Close tag
        mrOutput += QString("%1</%2>\n")
            .arg(mcrCurrentIndentation,
                 mcElement.tagName());
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Copy DOM to another document
bool XMLHelper::Copy(const QDomElement & mcrSourceDOM,
    QDomElement & mrDOMParent, const bool mcIgnoreSourceTagName)
{
    CALL_IN(QString("mcSourceDOM=%1, mDOMParent=%2, mcIgnoreSourceTagName=%3")
        .arg(CALL_SHOW(mcrSourceDOM),
             CALL_SHOW(mrDOMParent),
             CALL_SHOW(mcIgnoreSourceTagName)));

    // Check if we have a source
    if (mcrSourceDOM.isNull())
    {
        const QString reason = tr("Null DOM provided as source.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Create this tag in destination parent
    QDomDocument dest_doc = mrDOMParent.ownerDocument();
    QDomElement dom_element;
    if (!mcIgnoreSourceTagName)
    {
        dom_element = dest_doc.createElement(mcrSourceDOM.tagName());
        mrDOMParent.appendChild(dom_element);
    } else
    {
        dom_element = mrDOMParent;
    }

    // Attributes
    if (!mcIgnoreSourceTagName)
    {
        QDomNamedNodeMap all_attributes = mcrSourceDOM.attributes();
        for (int attr_index = 0;
             attr_index < all_attributes.count();
             attr_index++)
        {
            const QDomAttr dom_attribute =
                all_attributes.item(attr_index).toAttr();
            dom_element.setAttribute(dom_attribute.name(),
                dom_attribute.value());
        }
    }

    // Child nodes
    for (QDomNode dom_child = mcrSourceDOM.firstChild();
         !dom_child.isNull();
         dom_child = dom_child.nextSibling())
    {
        if (dom_child.isText())
        {
            const QString text = dom_child.toText().data();
            QDomText dom_text = dest_doc.createTextNode(text);
            dom_element.appendChild(dom_text);
            continue;
        }
        if (dom_child.isElement())
        {
            const QDomElement dom_source_element = dom_child.toElement();
            const bool success = Copy(dom_source_element, dom_element);
            if (!success)
            {
                // Error has been reported elsewhere.
                CALL_OUT("");
                return false;
            }
        }

        // Ignore all other nodes
    }

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Copy HTML DOM to another document
bool XMLHelper::CopyHTML(const QDomElement & mcrSourceDOM,
    QDomElement & mrDOMParent, const bool mcIgnoreSourceTagName)
{
    CALL_IN(QString("mcSourceDOM=%1, mDOMParent=%2, mcIgnoreSourceTagName=%3")
        .arg(CALL_SHOW(mcrSourceDOM),
             CALL_SHOW(mrDOMParent),
             CALL_SHOW(mcIgnoreSourceTagName)));

    // It's not going to change
    static QSet < QString > known_html_tags = GetKnownHTMLTags();

    // Check if we have a source
    if (mcrSourceDOM.isNull())
    {
        const QString reason = tr("Null DOM provided as source.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Create this tag in destination parent
    QDomDocument dest_doc = mrDOMParent.ownerDocument();
    QDomElement dom_element;
    if (!mcIgnoreSourceTagName)
    {
        const QString tag = mcrSourceDOM.tagName();
        if (!known_html_tags.contains(tag))
        {
            const QString reason =
                tr("Top level tag <%1> is not a valid HTML tag.")
                .arg(tag);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        dom_element = dest_doc.createElement(tag);
        mrDOMParent.appendChild(dom_element);
    } else
    {
        dom_element = mrDOMParent;
    }

    // Attributes
    if (!mcIgnoreSourceTagName)
    {
        QDomNamedNodeMap all_attributes = mcrSourceDOM.attributes();
        for (int attr_index = 0;
             attr_index < all_attributes.count();
             attr_index++)
        {
            const QDomAttr dom_attribute =
                all_attributes.item(attr_index).toAttr();
            dom_element.setAttribute(dom_attribute.name(),
                dom_attribute.value());
        }
    }

    // Child nodes
    for (QDomNode dom_child = mcrSourceDOM.firstChild();
         !dom_child.isNull();
         dom_child = dom_child.nextSibling())
    {
        if (dom_child.isText())
        {
            QString text = dom_child.toText().data();
            text = EncodeHTMLEntities(text);
            QDomText dom_text = dest_doc.createTextNode(text);
            dom_element.appendChild(dom_text);
            continue;
        }
        if (dom_child.isElement())
        {
            const QDomElement dom_source_element = dom_child.toElement();
            const QString tag = dom_source_element.tagName();
            if (!known_html_tags.contains(tag))
            {
                const QString reason =  tr("Tag <%1> is not a valid HTML tag.")
                    .arg(tag);
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return false;
            }
            const bool success = CopyHTML(dom_source_element, dom_element);
            if (!success)
            {
                // Error has been reported elsewhere.
                CALL_OUT("");
                return false;
            }
        }

        // Ignore all other nodes
    }

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Encode HTML entities for XML
QString XMLHelper::EncodeHTMLEntities(QString mHTMLText)
{
    CALL_IN(QString("mHTMLText=%1")
        .arg(CALL_SHOW(mHTMLText)));

    mHTMLText.replace("&#039;", "'");

    CALL_OUT("");
    return mHTMLText;
}



///////////////////////////////////////////////////////////////////////////////
// Parse XML and append DOM to parent
bool XMLHelper::AppendXML(QDomElement & mrParent, const QString & mcrXML)
{
    CALL_IN(QString("mrParent=%1, mcrXML=%2")
        .arg(CALL_SHOW(mrParent),
             CALL_SHOW(mcrXML)));

    // Check for valid parent
    if (mrParent.isNull())
    {
        const QString reason = tr("No parent element given.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check for content
    if (mcrXML.isEmpty())
    {
        const QString reason = tr("XML is empty.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Parse XML
    QDomDocument dom_root("stuff");
    const QString content = QString("<content>%1</content>")
        .arg(mcrXML);
    if (!dom_root.setContent(content))
    {
        const QString reason = tr("XML cannot be parsed: %1")
            .arg(mcrXML);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Append all children to parent
    QDomElement dom_content = dom_root.firstChildElement("content");
    for (QDomNode child = dom_content.firstChild();
       !child.isNull();
       child = child.nextSibling())
    {
        // We need to clone the child node because if we don't, the current
        // node will get moved to the new parent, and there will be no
        // nextSibling() since we're now looping children of the new parent
        // instead of the old one.
        mrParent.appendChild(child.cloneNode(true));
    }

    // Success
    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// All attributes
QSet < QString > XMLHelper::GetAllAttributes(const QDomElement & mrElement)
{
    CALL_IN(QString("mrElement=%1")
        .arg(CALL_SHOW(mrElement)));

    // Check for valid element
    if (mrElement.isNull())
    {
        const QString reason = tr("No element given.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QSet < QString >();
    }

    QSet < QString > attributes;
    QDomNamedNodeMap all_attributes = mrElement.attributes();
    for (int attr_index = 0;
         attr_index < all_attributes.count();
         attr_index++)
    {
        const QDomAttr dom_attribute =
            all_attributes.item(attr_index).toAttr();
        attributes += dom_attribute.name();
    }

    CALL_OUT("");
    return attributes;
}



///////////////////////////////////////////////////////////////////////////////
// Strip leading DOCTYPE tag
QString XMLHelper::StripDocType(QString mXML)
{
    CALL_IN(QString("mXML=%1")
        .arg(CALL_SHOW(mXML)));

    mXML.replace("\n", "\\n");
    static const QRegularExpression format_doctype("^<!DOCTYPE [^>]+>(.*)$");
    QRegularExpressionMatch match_doctype = format_doctype.match(mXML);
    if (match_doctype.hasMatch())
    {
        mXML = match_doctype.captured(1).trimmed();
    }
    mXML.replace("\\n", "\n");

    CALL_OUT("");
    return mXML;
}



///////////////////////////////////////////////////////////////////////////////
// Check for proper nesting
QString XMLHelper::CheckProperNesting(const QString & mcrXML)
{
    CALL_IN(QString("mcrXML=%1")
        .arg(CALL_SHOW(mcrXML)));

    // Eliminate compact tags
    // (Careful: "/" can be part of the attributes, such as f_stop="f/2.8")
    QString xml = mcrXML;
    xml.replace("\n", "");
    static const QRegularExpression format_split_compact(
        "(.*)<([^>]+)/>(.*)");
    while (true)
    {
        QRegularExpressionMatch match_split_compact =
            format_split_compact.match(xml);
        if (!match_split_compact.hasMatch())
        {
            break;
        }
        xml = match_split_compact.captured(1) +
            match_split_compact.captured(3);
    }

    // Eliminate attributes (in complete tags)
    static const QRegularExpression format_split_attributes(
        "([^<]*)<([^ >]+)( [^>]+)?>(.*)");
    QString tags_only;
    while (true)
    {
        QRegularExpressionMatch match_split_attributes =
            format_split_attributes.match(xml);
        if (!match_split_attributes.hasMatch())
        {
            break;
        }
        tags_only += "<" + match_split_attributes.captured(2) + ">";
        xml = match_split_attributes.captured(4);
    }
    // Result looks like:
    // <text><title></title></text>
    xml = tags_only;

    // Eliminate open/close pairs
    if (xml.left(1) == "<")
    {
        xml = xml.mid(1);
    }
    if (xml.right(1) == ">")
    {
        xml = xml.left(xml.length() - 1);
    }
    QStringList tags = xml.split("><");
    bool pair_found = true;
    while (pair_found)
    {
        pair_found = false;
        for (int remove_idx = 0;
            remove_idx < tags.size() - 1;
            remove_idx++)
        {
            if (tags[remove_idx + 1] == "/" + tags[remove_idx])
            {
                pair_found = true;
                tags.removeAt(remove_idx);
                tags.removeAt(remove_idx);
                break;
            }
        }
    }

    // Result looks like:
    // ("text", "body")

    // Not properly nested if there are left overs
    if (!tags.isEmpty())
    {
        const QString result = tr("A tag hasn't been closed: <%1>")
            .arg(tags.join("><"));
        CALL_OUT("");
        return result;
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Get text components
QString XMLHelper::GetText(const QDomElement & mcrElement)
{
    CALL_IN(QString("mcrElement=%1")
        .arg(CALL_SHOW(mcrElement)));

    // Check if we actually have an element
    if (mcrElement.isNull())
    {
        const QString reason = tr("No element provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    // Tranverse DOM
    QStringList text_parts;
    GetText_Rec(mcrElement, text_parts);
    const QString text = text_parts.join("");

    CALL_OUT("");
    return text;
}



///////////////////////////////////////////////////////////////////////////////
// Get text components
void XMLHelper::GetText_Rec(const QDomElement & mcrElement,
    QStringList & mrText)
{
    CALL_IN(QString("mcrElement=%1")
        .arg(CALL_SHOW(mcrElement)));

    for (QDomNode dom_child = mcrElement.firstChild();
         !dom_child.isNull();
         dom_child = dom_child.nextSibling())
    {
        if (dom_child.isText())
        {
            mrText << dom_child.toText().data();
            continue;
        }
        if (!dom_child.isElement())
        {
            continue;
        }
        QDomElement dom_child_element = dom_child.toElement();
        GetText_Rec(dom_child_element, mrText);
    }

    CALL_OUT("");
}


///////////////////////////////////////////////////////////////////////////////
// Get HTML components
QString XMLHelper::GetHTML(const QDomElement & mcrElement)
{
    CALL_IN(QString("mcrElement=%1")
        .arg(CALL_SHOW(mcrElement)));

    // Check if we actually have an element
    if (mcrElement.isNull())
    {
        const QString reason = tr("No element provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    // Tranverse DOM
    QStringList html_parts;
    GetHTML_Rec(mcrElement, html_parts);
    const QString html = html_parts.join("");

    CALL_OUT("");
    return html;
}



///////////////////////////////////////////////////////////////////////////////
// Get HTML components
void XMLHelper::GetHTML_Rec(const QDomElement & mcrElement,
    QStringList & mrHTML)
{
    CALL_IN(QString("mcrElement=%1")
        .arg(CALL_SHOW(mcrElement)));

    static QSet < QString > known_html_tags = GetKnownHTMLTags();

    for (QDomNode dom_child = mcrElement.firstChild();
         !dom_child.isNull();
         dom_child = dom_child.nextSibling())
    {
        if (dom_child.isText())
        {
            mrHTML << dom_child.toText().data();
            continue;
        }
        if (!dom_child.isElement())
        {
            continue;
        }
        QDomElement dom_child_element = dom_child.toElement();
        const QString tag = dom_child_element.tagName();
        if (known_html_tags.contains(tag))
        {
            // Open tag (with attributes)
            QString opening = "<" + tag;
            const QSet < QString > attributes = GetAllAttributes(mcrElement);
            for (auto attr_iterator = attributes.begin();
                 attr_iterator != attributes.end();
                 attr_iterator++)
            {
                const QString & attribute = *attr_iterator;
                opening += QString(" %1=\"%2\"")
                    .arg(attribute,
                         mcrElement.attribute(attribute));
            }
            opening += ">";
            mrHTML << opening;
        }

        // Process child elements regardless of being HTML tags or not
        GetHTML_Rec(dom_child_element, mrHTML);

        if (known_html_tags.contains(tag))
        {
            // Close the tag we opened above
            const QString closing = QString("</%1>")
                .arg(tag);
            mrHTML << closing;
        }
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get known HTML tags
QSet < QString > XMLHelper::GetKnownHTMLTags()
{
    CALL_IN("");

    const QSet < QString > known_tags({
        "a",
        "b",
        "br",
        "code",
        "div",
        "font",
        "hr",
        "i",
        "li",
        "ol",
        "p",
        "s",
        "span",
        "table",
        "td",
        "tr",
        "u",
        "ul",
        "pre",
        "style"});

    CALL_OUT("");
    return known_tags;
}
