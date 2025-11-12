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

// NavigatedTextFile.cpp
// Class implementation

// Project includes
#include "CallTracer.h"
#include "MessageLogger.h"
#include "NavigatedTextFile.h"

// Qt includes
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
NavigatedTextFile::NavigatedTextFile(const QString mcFilename)
{
    CALL_IN(QString("mcFilename=%1")
        .arg(CALL_SHOW(mcFilename)));
    REGISTER_INSTANCE;

    // Initilize current line
    m_LineNumber = 0;
    
    // Open file
    QFile input_file(mcFilename);
    if (!input_file.open(QIODevice::ReadOnly))
    {
        m_IsOpen = false;
        const QString reason = tr("File \"%1\" could not be opened.")
            .arg(mcFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }
    
    // Filename
    m_Filename = mcFilename;
    
    // Read the whole thing
    const int max_size_mb = 200;
    m_FileContent = input_file.read(max_size_mb * 1024 * 1024);
    
    // Check if it was "the whole thing"
    if (!input_file.atEnd())
    {
        const QString reason =
            tr("Read maximum acceptable range (%1MB), but file has more data.")
                .arg(max_size_mb);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }
    
    // Split up in lines
    int index = 0;
    m_LineFirstCharacter << index;
    while (index < m_FileContent.size())
    {
        if (m_FileContent[index] != '\n' &&
            m_FileContent[index] != '\r')
        {
            index++;
            continue;
        }
        if (m_FileContent.size() > index + 1 &&
            m_FileContent.at(index) == '\r' &&
            m_FileContent.at(index+1) == '\n')
        {
            m_FileContent[index] = '\0';
            index++;
            continue;
        }
        m_FileContent[index] = '\0';
        index++;
        if (index < m_FileContent.size())
        {
            m_LineFirstCharacter << index;
        }
    }
    
    // Successfully opened file
    m_IsOpen = true;
    
    // At start
    m_LineNumber = 0;

    CALL_OUT("");
}
    


///////////////////////////////////////////////////////////////////////////////
// Destructor
NavigatedTextFile::~NavigatedTextFile()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do.

    CALL_OUT("");
}
   


// ===================================================================== Access



///////////////////////////////////////////////////////////////////////////////
// Get current line number
const char * NavigatedTextFile::GetCurrentLine()
{
    CALL_IN("");

    // Return valid line
    if (m_LineNumber < m_LineFirstCharacter.size())
    {
        CALL_OUT("");
        return m_FileContent.data() + m_LineFirstCharacter[m_LineNumber];
    }
    
    // Error
    const QString reason = tr("%1: End of list reached.")
        .arg(m_Filename);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return nullptr;
}



///////////////////////////////////////////////////////////////////////////////
// Read a particular line
const char * NavigatedTextFile::GetLine(const int mcLineNumber)
{
    CALL_IN(QString("mcLineNumber=%1")
        .arg(CALL_SHOW(mcLineNumber)));

    // Check if we have a file
    if (!m_IsOpen)
    {
        const QString reason = tr("No file has been read.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }

    // Check if this is a valid line number
    if (mcLineNumber < 0 ||
        mcLineNumber >= m_LineFirstCharacter.size())
    {
        const QString reason = tr("Invalid line number %1 (should be 0 to %2)")
            .arg(QString::number(mcLineNumber),
                 QString::number(m_LineFirstCharacter.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }

    // We actually have that line.
    CALL_OUT("");
    return m_FileContent.data() + m_LineFirstCharacter[mcLineNumber];
}



///////////////////////////////////////////////////////////////////////////////
// Read current line
const char * NavigatedTextFile::ReadLine()
{
    CALL_IN("");

    // Return valid line
    if (m_LineNumber < m_LineFirstCharacter.size())
    {
        CALL_OUT("");
        return m_FileContent.data() + m_LineFirstCharacter[m_LineNumber++];
    }
    
    // Error
    const QString reason = tr("%1: End of list reached.")
        .arg(m_Filename);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return nullptr;
}



///////////////////////////////////////////////////////////////////////////////
// Get number of lines
int NavigatedTextFile::GetNumberOfLines() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_LineFirstCharacter.size();
}



///////////////////////////////////////////////////////////////////////////////
// Move current line to a define line number
bool NavigatedTextFile::MoveTo(const int mcLineNumber)
{
    CALL_IN(QString("mcLineNumber=%1")
        .arg(CALL_SHOW(mcLineNumber)));

    if (mcLineNumber >= 0 &&
        mcLineNumber < m_LineFirstCharacter.size())
    {
        m_LineNumber = mcLineNumber;
        CALL_OUT("");
        return true;
    }
    
    // Error
    const QString reason = tr("%1: Line number exceeds number of lines.")
        .arg(m_Filename);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Move current line forward
bool NavigatedTextFile::Advance(const int mcNumberOfLines)
{
    CALL_IN(QString("mcNumberOfLines=%1")
        .arg(CALL_SHOW(mcNumberOfLines)));

    // Return valid line
    if (m_LineNumber + mcNumberOfLines >= 0 &&
        m_LineNumber + mcNumberOfLines < m_LineFirstCharacter.size())
    {
        m_LineNumber += mcNumberOfLines;
        CALL_OUT("");
        return true;
    }
    
    // Error
    const QString reason = tr("%1: End of list reached.")
        .arg(m_Filename);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Move current line back
bool NavigatedTextFile::Rewind(const int mcNumberOfLines)
{
    CALL_IN(QString("mcNumberOfLines=%1")
        .arg(CALL_SHOW(mcNumberOfLines)));

    // Return valid line
    if (m_LineNumber - mcNumberOfLines >= 0 &&
        m_LineNumber - mcNumberOfLines < m_LineFirstCharacter.size())
    {
        m_LineNumber -= mcNumberOfLines;
        CALL_OUT("");
        return true;
    }
    
    // Error
    const QString reason = tr("%1: Start of list reached.")
        .arg(m_Filename);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Move to the end of the file
void NavigatedTextFile::MoveToEnd()
{
    CALL_IN("");

    m_LineNumber = m_LineFirstCharacter.size();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Check if current line is at the end
bool NavigatedTextFile::AtEnd()
{
    CALL_IN("");

    CALL_OUT("");
    return (m_LineNumber >= m_LineFirstCharacter.size());
}



///////////////////////////////////////////////////////////////////////////////
// Current position
int NavigatedTextFile::GetCurrentLineNumber() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_LineNumber;
}



///////////////////////////////////////////////////////////////////////////////
// Check if file has been successfully read
bool NavigatedTextFile::IsOpen()
{
    CALL_IN("");

    CALL_OUT("");
    return m_IsOpen;
}


///////////////////////////////////////////////////////////////////////////////
// Filename
QString NavigatedTextFile::GetFilename() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_Filename;
}



// ====================================================================== Debug



///////////////////////////////////////////////////////////////////////////////
// Dump for debugging purposes
void NavigatedTextFile::Dump() const
{
    CALL_IN("");

    QString dump;
    dump = tr("Filename: %1\n"
        "Current Line: %2\n"
        "Content:\n")
            .arg(m_Filename,
                 QString::number(m_LineNumber));
    for (int line_nr = 0; line_nr < m_LineFirstCharacter.size(); line_nr++)
    {
        dump += QString("%1: %2\n")
            .arg(QString::number(line_nr),
                 m_FileContent.data() + m_LineFirstCharacter[line_nr]);
    }
    qDebug().noquote() << dump;

    CALL_OUT("");
}
