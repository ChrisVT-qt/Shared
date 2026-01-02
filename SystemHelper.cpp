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

// SystemHelper.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "MessageLogger.h"
#include "SystemHelper.h"

// Qt includes
#include <QDebug>
#include <QEventLoop>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPalette>

// Platform-specific includes: Apple
#if defined(__APPLE__)
#include "PList.h"
#include <sys/xattr.h>
#endif



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// No instance should exist.
SystemHelper::SystemHelper()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Nothing to do.

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
SystemHelper::~SystemHelper()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do.

    CALL_OUT("");
}

    

// ================================================================== Functions



///////////////////////////////////////////////////////////////////////////////
// Download file
QByteArray SystemHelper::Download(const QString & mcrURL)
{
    CALL_IN(QString("mcrURL=%1")
        .arg(CALL_SHOW(mcrURL)));

    // Download image
    QNetworkAccessManager network_manager;
    QEventLoop event_loop;
    connect (&network_manager, &QNetworkAccessManager::finished,
        &event_loop, &QEventLoop::quit);
    QNetworkReply * reply = network_manager.get(QNetworkRequest(mcrURL));
    event_loop.exec();

    // Check if error occurred
    if (reply -> error() != QNetworkReply::NoError)
    {
        delete reply;
        const QString reason =
            tr("An error occurred while downloading an image from \"%1\".")
                .arg(mcrURL);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QByteArray();
    }

    // No error
    QByteArray ret = reply -> readAll();
    delete reply;

    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Determine file MIME type
QString SystemHelper::GetMIMEType(const QString & mcrFilename)
{
    CALL_IN(QString("mcrFilename=%1")
        .arg(CALL_SHOW(mcrFilename)));

    QFileInfo file_info(mcrFilename);

    QMimeDatabase mime_database;
    const QMimeType mime_type = mime_database.mimeTypeForFile(file_info);
    if (mime_type.isValid())
    {
        CALL_OUT("");
        return mime_type.name();
    } else
    {
        // No MIME type
        return QString();
    }

    // We never get here
}



///////////////////////////////////////////////////////////////////////////////
// Determine file MIME type
QString SystemHelper::GetMIMEType(const QByteArray & mcrData)
{
    CALL_IN(QString("mcrData=%1")
        .arg(CALL_SHOW(mcrData)));

    QMimeDatabase mime_database;
    const QMimeType mime_type = mime_database.mimeTypeForData(mcrData);
    if (mime_type.isValid())
    {
        CALL_OUT("");
        return mime_type.name();
    } else
    {
        // No MIME type
        return QString();
    }

    // We never get here
}



///////////////////////////////////////////////////////////////////////////////
// Get various apple xattr attributes
const QHash < QString, QString > SystemHelper::GetAdditionalFileInfo(
    const QString mcFilename)
{
    CALL_IN(QString("mcFilename=%1")
        .arg(CALL_SHOW(mcFilename)));

    // Return value
    QHash < QString, QString > ret;
    
#if defined(__APPLE__)
    // Finder Comment
    char valuebuffer[1024];
    ssize_t si = getxattr(mcFilename.toLocal8Bit(),
        "com.apple.metadata:kMDItemFinderComment", valuebuffer, 1024, 0, 0);
    if (si > 0)
    {
        qDebug() << "======================================================";   
        qDebug() << tr("%1 has comment xattr!").arg(mcFilename);
        qDebug() << "======================================================";   
        for (int idx = 11; idx < 1024 && valuebuffer[idx] != char(0); idx++)
        {
            if (valuebuffer[idx] < (char)31)
            {
                valuebuffer[idx] = '\0';
                break;
            }
        }
        ret["comment"] = valuebuffer + 11;
        ret["comment"] = ret["comment"].trimmed();
    }
    
    // Item Source
    const QString source = PList::GetItemSource(mcFilename);
    if (!source.isEmpty())
    {
        ret["source"] = source;
    }

    // Label
    static QHash < unsigned char, QString > all_labels;
    if (all_labels.isEmpty())
    {
        all_labels[12] = tr("red");
        all_labels[14] = tr("orange");
        all_labels[10] = tr("yellow");
        all_labels[4] = tr("green");
        all_labels[8] = tr("blue");
        all_labels[6] = tr("purple");
        all_labels[2] = tr("gray");
    }
    si = getxattr(mcFilename.toLocal8Bit(),
        "com.apple.FinderInfo", valuebuffer, 1024, 0, 0);
    if (si > 0 && valuebuffer[0] == '\0')
    {
        ret["label"] = all_labels[(unsigned char)valuebuffer[9]];
    }
#endif
    
    // Return value
    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Get user name
const QString SystemHelper::GetUserName()
{
    CALL_IN("");

    QString user;

#if defined(__APPLE__)
    user = qgetenv("USER");
#elif defined(WIN32) || defined(_WIN32)
    char win_user_name[100];
    GetUserName(&win_user_name, sizeof(win_user_name));
    user = QString(win_user_name);
#endif

    CALL_OUT("");
    return user;
}



///////////////////////////////////////////////////////////////////////////////
// Check if system is run with "dark" palette
bool SystemHelper::IsDarkMode()
{
    CALL_IN("");

#if defined(__APPLE__)
    // Use window base color to tell

    // Older pre-macOS 26 (Tahoe)
    // const QColor system_light_bg(0xec, 0xec, 0xec);

    // macoOS 26 (Tahoe) with Liquid Glass
    const QColor system_light_bg(0xff, 0xff, 0xff);

    const QColor system_current_bg(QPalette().color(QPalette::Window));

    // Will not change while application is running
    // (or otherwise, we'd need to update colors for all widgets)
    static const bool is_light = (system_current_bg == system_light_bg);
#else
    static const bool is_light = true;
#endif

    CALL_OUT("");
    return !is_light;
}
