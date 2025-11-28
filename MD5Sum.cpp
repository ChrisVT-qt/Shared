// Copyright (C) 2025 Chris von Toerne
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

// MD5Sum.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "MD5Sum.h"
#include "MessageLogger.h"

// Qt includes
#include <QCryptographicHash>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QString>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Never to be instanciated
MD5Sum::MD5Sum()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Do nothing.

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
MD5Sum::~MD5Sum()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Do nothing.

    CALL_OUT("");
}



// ================================================================== MD5 Stuff



///////////////////////////////////////////////////////////////////////////////
// Compute MD5 sum
QString MD5Sum::ComputeMD5Sum(const QString mcFilename,
    const bool mcLookUp)
{
    CALL_IN(QString("mcFilename=%1, mcLookUp=%2")
        .arg(CALL_SHOW(mcFilename),
             CALL_SHOW(mcLookUp)));

    QFileInfo file_info(mcFilename);
    const qint64 file_size = file_info.size();

    // Look up if we are supposed to
    if (!mcLookUp ||
        !m_FilesizeToFilenameToMD5Sum.contains(file_size) ||
        !m_FilesizeToFilenameToMD5Sum[file_size].contains(mcFilename))
    {
        // Open file
        QFile in_file(mcFilename);
        
        // Check if that was successful
        if (!in_file.open(QIODevice::ReadOnly))
        {
            // Nope
            const QString reason =
                QObject::tr("File \"%1\" could not be opened.")
                    .arg(mcFilename);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }
        
        // Read entire file
        const QByteArray data = in_file.readAll();
        const QString hash =
            QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();

        // Store it.
        m_FilesizeToFilenameToMD5Sum[file_size][mcFilename] = hash;
    }

    // Return MD5 sum
    CALL_OUT("");
    return m_FilesizeToFilenameToMD5Sum[file_size][mcFilename];
}



///////////////////////////////////////////////////////////////////////////////
// Compute MD5 sum
QString MD5Sum::ComputeMD5Sum(const QByteArray & mcrData)
{
    CALL_IN(QString("mcrData=%1")
        .arg(CALL_SHOW(mcrData)));

    const QString md5sum =
        QCryptographicHash::hash(mcrData, QCryptographicHash::Md5).toHex();
    CALL_OUT("");
    return md5sum;
}



///////////////////////////////////////////////////////////////////////////////
// MD5 Sum cache
QHash < qint64, QHash < QString, QString > >
    MD5Sum::m_FilesizeToFilenameToMD5Sum;
