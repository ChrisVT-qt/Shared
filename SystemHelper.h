// AppleHelper.h
// Class definition file

/** \file
  * \todo Add Doxygen information
  */

// Just include once
#ifndef SYSTEMHELPER_H
#define SYSTEMHELPER_H

// Qt includes
#include <QHash>
#include <QObject>
#include <QString>

// Class definition
class SystemHelper:
    public QObject
{
    Q_OBJECT
    
    
    
    // ============================================================== Lifecycle
private:
	// No instance should exist.
	SystemHelper();

public:
	// Destructor
	virtual ~SystemHelper();

    

    // ============================================================== Functions
public:
    // Determine file MIME type
    static QString GetMIMEType(const QString & mcrFilename);
    static QString GetMIMEType(const QByteArray & mcrData);

    // Get various Apple xattr attributes
    /** \todo Implement on Windows, Linux
      */
    static const QHash < QString, QString > GetAdditionalFileInfo(
        const QString mcFilename);
    
    // Get user name
    /** \todo Test on Windows, Linux
      */
    static const QString GetUserName();

    // Check if system is run with "dark" palette
    /** \todo Implement on Windows, Linux
      */
    static bool IsDarkMode();
};

#endif
