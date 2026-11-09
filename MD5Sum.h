// MD5Sum.h
// Class definition file

// Just include once
#pragma once

// Qt includes
#include <QByteArray>
#include <QHash>
#include <QString>

// Class definition
class MD5Sum
{
    // ============================================================== Lifecycle
private:
    // Never to be instanciated
    MD5Sum();
    
public:
    // Destructor
    ~MD5Sum();
    
    
    
    // ============================================================== MD5 Stuff
public:
    // Compute MD5 sum
    static QString ComputeMD5Sum(const QString mcFilename,
        const bool mcLookUp = true);
    static QString ComputeMD5Sum(const QByteArray & mcrData);

private:
    // MD5 Sum cache
    static QHash < qint64, QHash < QString, QString > >
        m_FilesizeToFilenameToMD5Sum;
};
