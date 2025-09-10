// NavigatedTextFile.h
// Class definition

/** \file
  * \todo Add Doxygen information
  */

#ifndef NAVIGATEDTEXTFILE_H
#define NAVIGATEDTEXTFILE_H

// Qt includes
#include <QList>
#include <QObject>
#include <QString>

// Define class
class NavigatedTextFile:
    public QObject
{
    Q_OBJECT
    
    
    
    // ============================================================== Lifecycle
public:
    // Constructor
    NavigatedTextFile(const QString mcFilename);
    
    // Destructor
    ~NavigatedTextFile();
    
    
    
    // ================================================================= Access
public:
    const char * GetCurrentLine();
    const char * GetLine(const int mcLineNumber);
    const char * ReadLine();
    int GetNumberOfLines() const;
    bool MoveTo(const int mcLineNumber);
    bool Advance(const int mcNumberOfLines);
    bool Rewind(const int mcNumberOfLines);
    void MoveToEnd();
    bool AtEnd();
private:
    // Lines
    QList < int > m_LineFirstCharacter;
    QByteArray m_FileContent;

public:
    // Filename
    QString GetFilename() const;
private:
    QString m_Filename;
    
public:
    // Current position
    int GetCurrentLineNumber() const;
private:
    int m_LineNumber;

public:
    bool IsOpen();
private:
    bool m_IsOpen;



    // ================================================================== Debug
public:
    void Dump() const;
};

#endif

