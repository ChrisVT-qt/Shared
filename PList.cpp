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

// PList.cpp
// Class implementation

// Project includes
#include "CallTracer.h"
#include "MessageLogger.h"
#include "PList.h"

// Qt includes
#include <QDateTime>
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>

// System
#ifdef __APPLE__
#include <sys/xattr.h>
#endif


// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
PList::PList()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Should never be instanciated

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
PList::~PList()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do.

    CALL_OUT("");
}



// =================================================================== External



///////////////////////////////////////////////////////////////////////////////
// Get XML version of the binary PList
QString PList::GetXML(const QString mcFilename, const QString mcItem)
{
    CALL_IN(QString("mcFilename=%1, mcItem=%2")
        .arg(CALL_SHOW(mcFilename),
             CALL_SHOW(mcItem)));

#ifdef __APPLE__
    // Binary PList
    const int required_size = getxattr(mcFilename.toLocal8Bit(), 
        mcItem.toLocal8Bit(), nullptr, 0, 0, 0);
    
    // Check no xattr data
    if (required_size <= 0)
    {
        CALL_OUT("");
        return QString();
    }
    
    // This file has xattr data
    m_PList = (char *)malloc(required_size);
    m_PList_Size = getxattr(mcFilename.toLocal8Bit(), mcItem.toLocal8Bit(),
        m_PList, required_size, 0, 0);
    if (m_PList_Size <= 0)
    {
        const QString reason = tr("Could not read xattr data");
        free(m_PList);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // Check format
    if (!CheckHeader() ||
        !CheckVersion())
    {
        free(m_PList);
        const QString reason =
            tr("Header or version did not complete successfully.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // Interpret trailer first
    if (!Parse_Trailer())
    {
        free(m_PList);
        const QString reason = tr("Trailer did not parse successfully.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // Create XML
    m_XML = "<plist>";
    
    // Root element position
    int position = m_OffsetTableStart + m_OffsetBytes * m_RootObject;
    const QPair < int, bool > new_position = ReadOffset(position);
    if (new_position.second == false)
    {
        free(m_PList);
        const QString reason = tr("Offset didn't read successfully.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    position = new_position.first;
    
    // Parse PList
    Parse_Element(position);
    m_XML += "</plist>";
    free(m_PList);
    m_PList = NULL;
    
    // Return XML
    CALL_OUT("");
    return m_XML;
#else
    return QString();
#endif
}



///////////////////////////////////////////////////////////////////////////////
// Values
char * PList::m_PList = NULL;
int PList::m_PList_Size = 0;
QString PList::m_XML = QString();



// ====================================================== Convenience Functions



///////////////////////////////////////////////////////////////////////////////
// Get file source
QString PList::GetItemSource(const QString mcFilename)
{
    CALL_IN(QString("mcFilename=%1")
        .arg(CALL_SHOW(mcFilename)));

    // Check if file exists
    if (!QFile::exists(mcFilename))
    {
        // Raise error
        const QString reason = tr("Could not find file \"%1\".")
            .arg(mcFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    const QString xml =
        GetXML(mcFilename, "com.apple.metadata:kMDItemWhereFroms");
    if (xml.isEmpty())
    {
        // No PList information available (totally fine)
        CALL_OUT("");
        return QString();
    }
    
    QDomDocument dom("stuff");
    const QDomDocument::ParseResult parse_result = dom.setContent(xml);
    if (!parse_result.errorMessage.isEmpty())
    {
        const QString reason =
            tr("PList XML data for file \"%1\" could not be parsed: %2")
                .arg(mcFilename,
                     xml);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // Get <plist><array><string>
    QDomElement dom_plist = dom.firstChildElement("plist");
    if (dom_plist.isNull())
    {
        const QString reason =
            tr("PList XML data for file \"%1\" does not seem to have a "
                "<plist> tag.")
                .arg(mcFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    QDomElement dom_array = dom_plist.firstChildElement("array");
    if (dom_array.isNull())
    {
        const QString reason =
            tr("PList XML data for file \"%1\" does not seem to have a "
                "<plist><array> tag.")
                .arg(mcFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    QDomElement dom_string = dom_array.firstChildElement("string");
    if (dom_string.isNull())
    {
        const QString reason =
            tr("PList XML data for file \"%1\" does not seem to have a "
                "<plist><array><string> tag.")
                .arg(mcFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // All good!
    CALL_OUT("");
    return dom_string.text();
}



// =================================================================== Internal



///////////////////////////////////////////////////////////////////////////////
// Check if header is fine
bool PList::CheckHeader()
{
    CALL_IN("");

    // HEADER
    // ->  magic number ("bplist")
    //     file format version
    
    // Check if we have reached the end
    const int magic_size = 6;
    if (magic_size > m_PList_Size)
    {
        const QString reason = tr("Too few PList data to check header");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    // Should be "bplist"
    const int result = strncmp(m_PList, "bplist", magic_size);

    CALL_OUT("");
    return (result == 0);
}



///////////////////////////////////////////////////////////////////////////////
// Check if version is fine
bool PList::CheckVersion()
{
    CALL_IN("");

    // HEADER
    //     magic number ("bplist")
    // ->  file format version

    // Check if we have reached the end
    if (6 + 2 > m_PList_Size)
    {
        const QString reason = tr("Too few PList data to check version");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    const QString version = QString(m_PList[6]) + QString(m_PList[6 + 1]);

    CALL_OUT("");
    return (version == "00" || version == "01");
}



///////////////////////////////////////////////////////////////////////////////
// Interpret last 32 bytes of the binary PList
bool PList::Parse_Trailer()
{
    CALL_IN("");

    // Trailer
    // The final 32 bytes of a binary plist have the following format:
    // 6 bytes of \x00 padding
    // a 1 byte integer which is the number of bytes for an offset value. 
    //      Valid values are 1, 2, 3, or 4. Offset values are encoded as 
    //      unsigned, big endian integers. Must be wide enough to encode the 
    //      offset of the offset table, not just the highest object offset.
    // a 1 byte integer which is the number of bytes for an object reference 
    //      number. Valid values are 1 or 2. Reference numbers are encoded as
    //      unsigned, big endian integers.
    // 4 bytes of \x00 padding
    // a 4 byte integer which is the number of objects in the plist
    // 4 bytes of \x00 padding
    // a 4 byte integer which is the reference number of the root object in
    //      the plist. This is usually zero.
    // 4 bytes of \x00 padding
    // a 4 byte integer which is the offset in the file of the start of the 
    //      offset table, named above as the third element in a binary plist 
    
    int pos = m_PList_Size - 32;
    if (pos < 0)
    {
        const QString reason = tr("Insufficient data for trailer.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    // 1) 6 bytes of \x00 padding
    if ((unsigned char)m_PList[pos] != 0 ||
        (unsigned char)m_PList[pos+1] != 0 ||
        (unsigned char)m_PList[pos+2] != 0 ||
        (unsigned char)m_PList[pos+3] != 0 ||
        (unsigned char)m_PList[pos+4] != 0 ||
        (unsigned char)m_PList[pos+5] != 0)
    {
        const QString reason =
            tr("Failed in first section (6 bytes of padding).");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    pos += 6;
    
    // 2) a 1 byte integer which is the number of bytes for an offset value. 
    //      Valid values are 1, 2, 3, or 4. Offset values are encoded as 
    //      unsigned, big endian integers. Must be wide enough to encode the 
    //      offset of the offset table, not just the highest object offset.
    m_OffsetBytes = (unsigned char)m_PList[pos];
    pos++;
    if (m_OffsetBytes < 1 ||
        m_OffsetBytes > 4)
    {
        const QString reason =
            tr("Failed in second section (bytes for an offset value).");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    // 3) a 1 byte integer which is the number of bytes for an object reference 
    //      number. Valid values are 1 or 2. Reference numbers are encoded as
    //      unsigned, big endian integers.
    m_ReferenceBytes = (unsigned char)m_PList[pos];
    pos++;
    if (m_ReferenceBytes < 1 ||
        m_ReferenceBytes > 2)
    {
        const QString reason =
            tr("Failed in third section (bytes for an object reference).");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    // 4) 4 bytes of \x00 padding
    if ((unsigned char)m_PList[pos] != 0 ||
        (unsigned char)m_PList[pos+1] != 0 ||
        (unsigned char)m_PList[pos+2] != 0 ||
        (unsigned char)m_PList[pos+3] != 0)
    {
        const QString reason =
            tr("Failed in fourth section (4 bytes of padding).");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    pos += 4;
    
    // 5) a 4 byte integer which is the number of objects in the plist
    QPair < int, bool > new_value = ReadBigEndianInteger(pos, 4);
    if (new_value.second == false)
    {
        const QString reason = tr("Big endian integer could not be read.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    m_NumObjects = new_value.first;
    
    // 6) 4 bytes of \x00 padding
    if ((unsigned char)m_PList[pos] != 0 ||
        (unsigned char)m_PList[pos+1] != 0 ||
        (unsigned char)m_PList[pos+2] != 0 ||
        (unsigned char)m_PList[pos+3] != 0)
    {
        const QString reason =
            tr("Failed in sixth section (4 bytes of padding).");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    pos += 4;

    // 7) a 4 byte integer which is the reference number of the root object in
    //      the plist. This is usually zero.
    new_value = ReadBigEndianInteger(pos, 4);
    if (new_value.second == false)
    {
        const QString reason = tr("Big endian integer could not be read.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    m_RootObject = new_value.first;
    
    // 8) 4 bytes of \x00 padding
    if ((unsigned char)m_PList[pos] != 0 ||
        (unsigned char)m_PList[pos+1] != 0 ||
        (unsigned char)m_PList[pos+2] != 0 ||
        (unsigned char)m_PList[pos+3] != 0)
    {
        const QString reason =
            tr("Failed in eighth section (4 bytes of padding).");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    pos += 4;
    
    // 9) a 4 byte integer which is the offset in the file of the start of the 
    //      offset table, named above as the third element in a binary plist 
    new_value = ReadBigEndianInteger(pos, 4);
    if (new_value.second == false)
    {
        const QString reason = tr("Big endian integer could not be read.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    m_OffsetTableStart = new_value.first;
    
    // Success
    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse element
bool PList::Parse_Element(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // OBJECT TABLE
    //      variable-sized objects
    // 
    //      Object Formats (marker byte followed by additional info in some
    //      cases)
    //      null    0000 0000
    //      bool    0000 1000               // false
    //      bool    0000 1001               // true
    //      fill    0000 1111               // fill byte
    //      int     0001 nnnn ...           // # of bytes is 2^nnnn, 
    //                                      //   big-endian bytes
    //      real    0010 nnnn ...           // # of bytes is 2^nnnn,
    //                                      //   big-endian bytes
    //      date    0011 0011 ...           // 8 byte float follows, 
    //                                      //   big-endian bytes
    //      data    0100 nnnn [int] ...     // nnnn is number of bytes unless
    //                                      //   1111 then int count follows,
    //                                      //   followed by bytes
    //      string  0101 nnnn [int] ...     // ASCII string, nnnn is # of
    //                                      //   chars, else 1111 then int
    //                                      //   count, then bytes
    //      string  0110 nnnn [int] ...     // Unicode string, nnnn is # of
    //                                      //   chars, else 1111 then int 
    //                                      //   count, then big-endian 2-byte
    //                                      //   uint16_t
    //              0111 xxxx               // unused
    //      uid     1000 nnnn ...           // nnnn+1 is # of bytes
    //              1001 xxxx               // unused
    //      array   1010 nnnn [int] objref* // nnnn is count, unless 1111, then
    //                                      //   int count follows
    //              1011 xxxx               // unused
    //      set     1100 nnnn [int] objref* // nnnn is count, unless 1111,
    //                                      //   then int count follows
    //      dict    1101 nnnn [int] keyref* objref* // nnnn is count, unless 
    //                                      //   1111, then int count follows
    //              1110 xxxx               // unused
    //              1111 xxxx               // unused

    unsigned int format = (unsigned char)(m_PList[mrPosition]);
    const int type = format/16;
    bool success = false;
    switch (type)
    {
    case 0x0:
        success = Parse_Singleton(mrPosition);
        break;
    case 0x1:
        success = Parse_Int(mrPosition);
        break;
    case 0x2:
        success = Parse_Real(mrPosition);
        break;
    case 0x3:
        success = Parse_Date(mrPosition);
        break;
    case 0x4:
        success = Parse_Data(mrPosition);
        break;
    case 0x5:
        success = Parse_String_ASCII(mrPosition);
        break;
    case 0x6:
        success = Parse_String_Unicode(mrPosition);
        break;
    case 0x8:
        success = Parse_UID(mrPosition);
        break;
    case 0xa:
        success = Parse_Array(mrPosition);
        break;
    case 0xc:
        success = Parse_Set(mrPosition);
        break;
    case 0xd:
        success = Parse_Dict(mrPosition);
        break;
    default:
        // Unused
        const QString reason = tr("Unknown element type %1 at position %2.")
            .arg(type,
                 mrPosition);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    CALL_OUT("");
    return success;
}



///////////////////////////////////////////////////////////////////////////////
// Parse singleton
bool PList::Parse_Singleton(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + 1 > m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    switch (typedata)
    {
    case 0x0:
        m_XML += "<null/>";
        break;
    case 0x8:
        m_XML += "<boolean>false</boolean>";
        break;
    case 0x9:
        m_XML += "<boolean>true</boolean>";
        break;
    case 0xf:
        m_XML += "<fill/>";
        break;
    default:
    {
        const QString reason = tr("Unknown singleton type %1 at position %2.")
            .arg(QString::number(typedata),
                 QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    }
    
    // Success!
    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse integer
bool PList::Parse_Int(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;
    
    // Check if we have reached the end
    if (mrPosition + (1<<typedata) >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for %2-byte integer.")
                .arg(QString::number(mrPosition),
                     QString::number(1<<typedata));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    if (typedata < 3)
    {
        QPair < int, bool > value =
            ReadBigEndianInteger(mrPosition, 1<<typedata);
        if (value.second == false)
        {
            const QString reason = tr("Big endian integer could not be read.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        m_XML += QString("<integer>%1</integer>\n").arg(value.first);
        CALL_OUT("");
        return true;
    } else
    {
        const QString reason = tr("Invalid integer size %1 at position %2.")
            .arg(QString::number(typedata),
                 QString::number(mrPosition-1));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // We never get here
}



///////////////////////////////////////////////////////////////////////////////
// Parse real value
bool PList::Parse_Real(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // !!! Untested

    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;
    
    // Check if we have reached the end
    if (mrPosition + (1<<typedata) >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for a %2-byte real.")
                .arg(QString::number(mrPosition),
                     QString::number(1<<typedata));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    double value;
    switch (typedata)
    {
    case 2:
        value = (float)m_PList[mrPosition];
        break;
    case 3:
        value = (double)m_PList[mrPosition];
        break;
    default:
    {
        const QString reason = tr("Invalid real size %1 at position %2.")
            .arg(QString::number(1<<typedata),
                 QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    }
    mrPosition += (1<<typedata);
    m_XML += QString("<real>%1</real>").arg(value);

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse date
bool PList::Parse_Date(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // !!! Does not yet work properly
    
    // Check if we have reached the end
    if (mrPosition + 1 + 8 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    if (typedata != 0x3)
    {
        const QString reason = tr("Invalid date indicator %1 at position %2.")
            .arg(QString::number(typedata),
                 QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    mrPosition++;

    // !!! This is the stuff that doesn't seem to work properly
    double secs_since_epoch;
    memcpy(&secs_since_epoch, m_PList + mrPosition, sizeof(secs_since_epoch));
    mrPosition += 8;
    
    QDateTime date = 
        QDateTime::fromString("2001-01-01 00:00:00", "yyyy-MM-dd hh:mm:ss");
    date = date.addSecs(secs_since_epoch);
    QString value = date.toString("yyyy-MM-dd hh:mm:ss");
    m_XML += QString("<date>%1</date>").arg(value);

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse data
bool PList::Parse_Data(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // !!! Not fully implemented, never encountered in the wild.
    
    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    int position = mrPosition;

    int num_bytes;
    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;
    if (typedata < 0xf)
    {
        num_bytes = typedata;
    } else
    {
        // Will change position
        const QPair < int, bool > new_num_bytes = ReadPListInt(mrPosition);
        if (new_num_bytes.second == false)
        {
            const QString reason = tr("Could not read PListInt.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        num_bytes = new_num_bytes.first;
    }
    
    // Check if we have reached the end
    if (mrPosition + num_bytes >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for %2 bytes of data.")
                .arg(QString::number(position),
                     QString::number(num_bytes));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    // !!! Do something with the data
    mrPosition += num_bytes;
    m_XML += QString("<data>%1</data>")
        .arg(tr("not implemented"));

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse ASCII string
bool PList::Parse_String_ASCII(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    int position = mrPosition;

    int num_bytes;
    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;
    if (typedata < 0xf)
    {
        num_bytes = typedata;
    } else
    {
        // Will change position
        const QPair < int, bool > new_num_bytes = ReadPListInt(mrPosition);
        if (new_num_bytes.second == false)
        {
            const QString reason = tr("Could not read PListInt.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        num_bytes = new_num_bytes.first;
    }
    
    // Check if we have reached the end
    if (mrPosition + num_bytes >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for %2 bytes of data.")
                .arg(QString::number(position),
                     QString::number(num_bytes));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    QString value = 
        QString::fromLocal8Bit(m_PList + mrPosition, num_bytes);
        
    // Make it XML-compatible
    value.replace("&", "&amp;");
    value.replace("<", "&lt;");
    value.replace(">", "&gt;");

    mrPosition += num_bytes;
    m_XML += QString("<string>%1</string>").arg(value);

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse unicode string
bool PList::Parse_String_Unicode(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    int position = mrPosition;

    int num_chars;
    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;
    if (typedata < 0xf)
    {
        num_chars = typedata;
    } else
    {
        // Will change position
        const QPair < int, bool > new_num_chars = ReadPListInt(mrPosition);
        if (new_num_chars.second == false)
        {
            const QString reason = tr("Could not read PListInt.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        num_chars = new_num_chars.first;
    }
    
    // Check if we have reached the end
    if (mrPosition + num_chars*2 >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for %2 bytes of data.")
                .arg(QString::number(position),
                     QString::number(num_chars*2));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    // Text is always big endian (no byte order marker)
    QString value;
    for (int offset = 0; offset < num_chars; offset++)
    {
        unsigned short single_char =
            ((unsigned char)(m_PList[mrPosition + offset*2]) << 8) +
            (unsigned char)(m_PList[mrPosition + offset*2 + 1]);
        value += QChar(single_char);
    }
    
    // Make it XML-compatible
    value.replace("&", "&amp;");
    value.replace("<", "&lt;");
    value.replace(">", "&gt;");

    mrPosition += num_chars*2;
    m_XML += QString("<string>%1</string>").arg(value);

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse UID
bool PList::Parse_UID(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // !!! Not fully implemented, never encountered in the wild.

    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    int position = mrPosition;

    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;
    const int size = typedata + 1;

    // Check if we have reached the end
    if (mrPosition + size >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for %2 bytes of UID.")
                .arg(QString::number(position),
                     QString::number(size));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // !!! Do something with the UID data
    mrPosition += size;
    m_XML += QString("<uid>%1</uid>")
        .arg(tr("not implemented"));

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse array of elements
bool PList::Parse_Array(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    int position = mrPosition;

    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;

    int size;
    if (typedata < 0xf)
    {
        size = typedata;
    } else
    {
        // Will change position
        const QPair < int, bool > new_size = ReadPListInt(mrPosition);
        if (new_size.second == false)
        {
            const QString reason = tr("Could not read PListInt.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        size = new_size.first;
    }

    // Check if we have reached the end
    if (mrPosition + size * m_ReferenceBytes >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for %2 bytes of "
                "references.")
                .arg(QString::number(position),
                     QString::number(size * m_ReferenceBytes));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    m_XML += "<array>";    
    for (int offset = 0; offset < size; offset++)
    {
        // Will change position
        const QPair < int, bool > ref_id = ReadReference(mrPosition);
        if (ref_id.second == false)
        {
            const QString reason = tr("Could not read reference.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        int position = m_OffsetTableStart + m_OffsetBytes * ref_id.first;
        const QPair < int, bool > new_position = ReadOffset(position);
        if (new_position.second == false)
        {
            const QString reason = tr("Could not read offset.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        position = new_position.first;
        const bool success = Parse_Element(position);
        if (success == false)
        {
            const QString reason = tr("Could not parse element.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
    }
    m_XML += "</array>";

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse set of elements
bool PList::Parse_Set(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    int position = mrPosition;

    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;

    int size;
    if (typedata < 0xf)
    {
        size = typedata;
    } else
    {
        // Will change position
        const QPair < int, bool > new_size = ReadPListInt(mrPosition);
        if (new_size.second == false)
        {
            const QString reason = tr("Could not read PListInt.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        size = new_size.first;
    }
    
    // Check if we have reached the end
    if (mrPosition + size * m_ReferenceBytes >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for %2 bytes of "
                "references.")
                .arg(QString::number(position),
                     QString::number(size * m_ReferenceBytes));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    m_XML += "<set>";    
    for (int offset = 0; offset < size; offset++)
    {
        // Will change position
        const QPair < int, bool > ref_id = ReadReference(mrPosition);
        if (ref_id.second == false)
        {
            const QString reason = tr("Could not read reference.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        int position = m_OffsetTableStart + m_OffsetBytes * ref_id.first;
        const QPair < int, bool > new_position = ReadOffset(position);
        if (new_position.second == false)
        {
            const QString reason = tr("Could not read offset.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        position = new_position.first;
        const bool success = Parse_Element(position);
        if (success == false)
        {
            const QString reason = tr("Could not parse element.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
    }
    m_XML += "</set>";

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Parse dictionary (key/object)
bool PList::Parse_Dict(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + 1 >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    int position = mrPosition;

    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;
    
    int num_pairs;
    if (typedata < 0xf)
    {
        num_pairs = typedata;
    } else
    {
        // Will change position
        const QPair < int, bool > new_num_pairs = ReadPListInt(mrPosition);
        if (new_num_pairs.second == false)
        {
            const QString reason = tr("Could not read PListInt.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        num_pairs = new_num_pairs.first;
    }
    // Check if we have reached the end
    if (mrPosition + 2 * num_pairs * m_ReferenceBytes >= m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1 for "
            "%2 bytes of reference pairs.")
            .arg(QString::number(position),
                 QString::number(2 * num_pairs * m_ReferenceBytes));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    m_XML += "<dict>";
    for (int count = 0; count < num_pairs; count++)
    {
        // Will change position
        
        // Key
        m_XML += "<key>";
        const QPair < int, bool > key = ReadReference(mrPosition);
        if (key.second == false)
        {
            const QString reason = tr("Could not read reference.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        int position = m_OffsetTableStart + m_OffsetBytes * key.first;
        QPair < int, bool > new_position = ReadOffset(position);
        if (new_position.second == false)
        {
            const QString reason = tr("Could not read offset.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        position = new_position.first;
        bool success = Parse_Element(position);
        if (success == false)
        {
            const QString reason = tr("Could not read element.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        m_XML += "</key>";
        
        // Object
        m_XML += "<object>";
        const QPair < int, bool > obj = ReadReference(mrPosition);
        if (obj.second == false)
        {
            const QString reason = tr("Could not read reference.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        position = m_OffsetTableStart + m_OffsetBytes * obj.first;
        new_position = ReadOffset(position);
        if (new_position.second == false)
        {
            const QString reason = tr("Could not read offset.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        position = new_position.first;
        success = Parse_Element(position);
        if (success == false)
        {
            const QString reason = tr("Could not read element.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
        m_XML += "</object>";
    }
    m_XML += "</dict>";
    
    // Success
    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Read a PList int (variable size)
QPair < int, bool > PList::ReadPListInt(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + 1 > m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < int, bool >(-1, false);
    }
    
    const int typedata = ((unsigned char)m_PList[mrPosition]) % 16;
    mrPosition++;
    
    // Check if we have reached the end
    if (mrPosition + (1<<typedata) >= m_PList_Size)
    {
        const QString reason =
            tr("Reached end of buffer at position %1 for a %2-byte integer")
                .arg(QString::number(mrPosition),
                     QString::number(1<<typedata));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < int, bool >(-1, false);
    }
    if (typedata < 3)
    {
        const QPair < int, bool > result =
            ReadBigEndianInteger(mrPosition, 1<<typedata);
        CALL_OUT("");
        return result;
    } else
    {
        const QString reason = tr("Invalid integer size %1 at position %2.")
            .arg(QString::number(1<<typedata),
                 QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < int, bool >(-1, false);
    }

    // We never get here
}



///////////////////////////////////////////////////////////////////////////////
// Read big endian integer (known size)
QPair < int, bool > PList::ReadBigEndianInteger(int & mrPosition, 
    const int mcSize)
{
    CALL_IN(QString("mrPosition=%1, mcSize=%2")
        .arg(CALL_SHOW(mrPosition),
             CALL_SHOW(mcSize)));

    // Check if we have reached the end
    if (mrPosition + mcSize > m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < int, bool >(-1, false);
    }

    int ret = 0;
    for (int byte = 0; byte < mcSize; byte++)
    {
        ret = (ret << 8) + (unsigned char)m_PList[mrPosition++];
    }
    
    // Success
    CALL_OUT("");
    return QPair < int, bool >(ret, true);
}



///////////////////////////////////////////////////////////////////////////////
// Read a reference (variable size)
QPair < int, bool > PList::ReadReference(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + m_ReferenceBytes > m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < int, bool >(-1, false);
    }

    int ret = 0;
    for (int byte = 0; byte < m_ReferenceBytes; byte++)
    {
        ret = (ret << 8) + (unsigned char)m_PList[mrPosition++];
    }
    
    // Success
    CALL_OUT("");
    return QPair < int, bool >(ret, true);;
}



///////////////////////////////////////////////////////////////////////////////
// Read offset (variable size)
QPair < int, bool > PList::ReadOffset(int & mrPosition)
{
    CALL_IN(QString("mrPosition=%1")
        .arg(CALL_SHOW(mrPosition)));

    // Check if we have reached the end
    if (mrPosition + m_OffsetBytes > m_PList_Size)
    {
        const QString reason = tr("Reached end of buffer at position %1.")
            .arg(QString::number(mrPosition));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < int, bool >(-1, false);
    }

    int ret = 0;
    for (int byte = 0; byte < m_OffsetBytes; byte++)
    {
        ret = (ret << 8) + (unsigned char)m_PList[mrPosition++];
    }
    
    // Success
    CALL_OUT("");
    return QPair < int, bool >(ret, true);;
}



///////////////////////////////////////////////////////////////////////////////
// More values
int PList::m_OffsetBytes = -1;
int PList::m_ReferenceBytes = -1;
int PList::m_NumObjects = -1;
int PList::m_RootObject = -1;
int PList::m_OffsetTableStart = -1;
