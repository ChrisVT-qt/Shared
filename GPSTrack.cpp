// GPSTrack.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "MessageLogger.h"
#include "GPSTrack.h"
#include "Map.h"
#include "StringHelper.h"

// Qt includes
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>

// System includes
#include <cmath>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Default constructor
GPSTrack::GPSTrack()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
GPSTrack::~GPSTrack()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Instance
GPSTrack * GPSTrack::FromFile(const QString & mcrFilename)
{
    CALL_IN(QString("mcrFilename=%1")
        .arg(CALL_SHOW(mcrFilename)));

    // Check if file exists
    if (!QFile::exists(mcrFilename))
    {
        const QString reason =
            tr("File \"%1\" does not exist. Fatal.").arg(mcrFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }




    CALL_OUT("");
    return instance;
}



///////////////////////////////////////////////////////////////////////////////
// Import from NMEA 0183 format (Canon EOS 5d Mark IV)
QPair < QString, QList < int > > GPSTrack::ImportFromNMEAFile(
    const QString mcFilename)
{
    // Default color
    Preferences * p = Preferences::Instance();
    const int r = p -> GetValue("GPSTrack_DefaultTrackColorR").toInt();
    const int g = p -> GetValue("GPSTrack_DefaultTrackColorG").toInt();
    const int b = p -> GetValue("GPSTrack_DefaultTrackColorB").toInt();
    QColor default_color(r, g, b);
    
    // Return value
    QPair < QString, QList < int > > ret;
    
    
    // Open file
    QFile log_file(mcFilename);
    if (!log_file.open(QIODevice::ReadOnly))
    {
        ret.first = 
            tr("File \"%1\" exists, but opening it failed for some reason.")
                .arg(mcFilename);
        return ret;
    }
    QTextStream log_stream(&log_file);
 
    // Canon's NMEA 0183 format
    //
    // @CanonGPS/ver1.0/wgs-84/Canon EOS 5D Mark IV/012021001302/fe5c
    // $GPGGA,194900.452,3754.6073,N,12215.6291,W,1,05,2.7,212.0,M,,,,0000*14
    // $GPRMC,194900.452,A,3754.6073,N,12215.6291,W,,,100916,,,A*7B
    // ...
    // 
    // Format description can be found at
    // http://www.gpsinformation.org/dale/nmea.htm
    // (retrieved 16 Oct 2016)

    // Interpretation
    //
    // @CanonGPS/ver1.0/wgs-84/Canon EOS 5D Mark IV/012021001302/fe5c
    //   012021001302: Body serial number
    //   fe5c: Unknown; potentially checksum
    // 
    // $GPGGA,[A],[B],[C],[D],[E],[F],[G],[H],[I],[J],[K],[L],[M],[N]*[O]
    // $GPGGA,194900.452,3754.6073,N,12215.6291,W,1,05,2.7,212.0,M,,,,0000*14
    //   GPGGA: Essential fix data which provide 3D location and accuracy data.
    //   [A]: Time of this sentence in UTC; example is 19:49:00.452 UTC
    //   [B],[C]: Latitude in format ddmm.mmmm; example is 37 deg 54.6073' N
    //   [D],[E]: Longitude in format d?ddmm.mmm; example is 122 deg 14.6291' W
    //   [F]: Quality; 1 is for GPS (SPS).
    //   [G]: Number of satallites being tracked; 5 in the example
    //   [H]: Horizontal dilution (inaccurracy); need to research
    //   [I],[J]: Altitude above sea level; example is 212.0 m
    //   [K]: Height of geoid above WGS-84 ellipsoid; empty
    //   [L]: Time in seconds since last DGPS update; empty
    //   [M]: DGPS station number ID; empty
    //   [N]: Unknown
    //   [O]: Checksum; ignored
    
    // $GPRMC,[A],[P],[B],[C],[D],[E],[Q],[R],[S],[T],[U],[V]*[W]
    // $GPRMC,194900.452,A,3754.6073,N,12215.6291,W,,,100916,,,A*7B
    //   [P]: Status; "A" is for active, "V" for void.
    //   [Q]: Speed over ground in knots; empty
    //   [R]: Track angle in degrees true; empty
    //   [S]: Date in format ddmmyy; 10 Sep 2016 in example
    //   [T],[U]: Magnetic variation
    //   [V]: Unknown
    //   [W]: Checksum; ignored

    // Read header
    // @CanonGPS/ver1.0/wgs-84/Canon EOS 5D Mark IV/012021001302/fe5c
    const QString line = log_stream.readLine();
    QRegularExpression format("@CanonGPS/ver1.0/wgs-84/([^/]+)/([^/]+)/.*");
    QRegularExpressionMatch match = format.match(line);
    if (!match.hasMatch())
    {
        ret.first = tr("File \"%1\" does not seem to be a Canon NMEA file: "
            "Invalid header data \"%1\".").arg(mcFilename).arg(line);
        return ret;
    }

    // Track ID
    int track_id = m_NextId++;
    
    // Add to return value
    ret.second << track_id;
    
    // Initialize some stuff
    m_IdToTitle[track_id] = QString();
    m_IdToDateTime[track_id] = QString();
    m_IdToColor[track_id] = default_color;
    m_IdToOpacity[track_id] = int(0.6*255);
    m_IdToWaypointColor[track_id] = default_color;
    m_IdToWaypointOpacity[track_id] = 255;
    QList < int > track_waypoint_ids;
    
    // Abbreviation
    GPSWaypoint * way = GPSWaypoint::Instance();
    
    // Loop through log file
    QString old_time;
    double longitude = 0;
    double latitude = 0;
    double elevation = 0;
    QString date;
    QString time;
    while (!log_stream.atEnd())
    {
        // Read a line from file
        const QString line = log_stream.readLine();
        
        // Temporary data
        QString this_time;
        QString this_date;
        double this_latitude = 0.;
        double this_longitude = 0.;
        double this_elevation = 0.;
        
        // Check what it is
        // $GPGGA,194900.452,3754.6073,N,12215.6291,W,1,05,2.7,212.0,M,,,,
        //   0000*14
        format = QRegularExpression(
            "\\$GPGGA,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),"
            "([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),"
            "([^\\*]*)\\*(.*)");
        match = format.match(line);
        if (match.hasMatch())
        {
            // Time
            const QString gga_time = match.captured(1);
            QRegularExpression gga_format(
                "([01][0-9]|2[0-3])([0-5][0-9])([0-5][0-9])(\\.[0-9]*)?");
            QRegularExpressionMatch gga_match = gga_format.match(gga_time);
            if (!gga_match.hasMatch())
            {
                ret.first = tr("Invalid time format in GGA sentence: \"%1\".")
                    .arg(gga_time);
                return ret;
            }
            this_time = QString("%1:%2:%3")
                .arg(gga_match.captured(1),
                     gga_match.captured(2),
                     gga_match.captured(3));
            
            // Latitude
            const QString gga_latitude = match.captured(2);
            const QString gga_latitude_hemisphere = match.captured(3);
            gga_format =
                QRegularExpression("([0-9]+)([0-5][0-9](\\.[0-9]*)?)");
            gga_match = gga_format.match(gga_latitude);
            if (!gga_match.hasMatch())
            {
                ret.first = tr("Invalid latitude format in GGA sentence: "
                    "\"%1\".").arg(gga_latitude);
                return ret;
            }
            this_latitude = gga_match.captured(1).toDouble()
                + gga_match.captured(2).toDouble()/60.;
            if (gga_latitude_hemisphere == "N")
            {
                // Nothing to do.
            } else if (gga_latitude_hemisphere == "S")
            {
                this_latitude = -this_latitude;
            } else
            {
                ret.first = tr("Invalid latitude hemisphere in GGA sentence: "
                    "\"%1\".").arg(gga_latitude_hemisphere);
                return ret;
            }
            
            // Longitude
            const QString gga_longitude = match.captured(4);
            const QString gga_longitude_hemisphere = match.captured(5);
            gga_match = gga_format.match(gga_longitude);
            if (!gga_match.hasMatch())
            {
                ret.first = tr("Invalid longitude format in GGA sentence: "
                    "\"%1\".").arg(gga_longitude);
                return ret;
            }
            this_longitude = gga_match.captured(1).toDouble()
                + gga_match.captured(2).toDouble()/60.;
            if (gga_longitude_hemisphere == "E")
            {
                // Nothing to do.
            } else if (gga_longitude_hemisphere == "W")
            {
                this_longitude = -this_longitude;
            } else
            {
                ret.first = tr("Invalid longitude hemisphere in GGA sentence: "
                    "\"%1\".").arg(gga_longitude_hemisphere);
                return ret;
            }
            
            // Elevation
            this_elevation = match.captured(9).toDouble();
            const QString gga_elevation_units = match.captured(10);
            if (gga_elevation_units != "M")
            {
                ret.first = tr("Invalid elevation units in GGA sentence: "
                    "\"%1\".").arg(gga_elevation_units);
                return ret;
            }
            
            // Update consolidated information
            if (old_time.isEmpty() ||
                old_time == this_time)
            {
                old_time = this_time;
                time = this_time;
                longitude = this_longitude;
                latitude = this_latitude;
                elevation = this_elevation;
            }
        }
        
        // $GPRMC,194900.452,A,3754.6073,N,12215.6291,W,,,100916,,,A*7B
        format = QRegularExpression(
            "\\$GPRMC,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),"
            "([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),"
            "([^\\*]*)\\*(.*)");
        match = format.match(line);
        if (match.hasMatch())
        {
            // Time
            const QString rmc_time = match.captured(1);
            QRegularExpression rmc_format(
                "([01][0-9]|2[0-3])([0-5][0-9])([0-5][0-9])(\\.[0-9]*)?");
            QRegularExpressionMatch rmc_match = rmc_format.match(rmc_time);
            if (!rmc_match.hasMatch())
            {
                ret.first = tr("Invalid time format in RMC sentence: \"%1\".")
                    .arg(rmc_time);
                return ret;
            }
            this_time = tr("%1:%2:%3")
                .arg(rmc_match.captured(1),
                     rmc_match.captured(2),
                     rmc_match.captured(3));
            
            // Date
            const QString rmc_date = match.captured(9);
            rmc_format = QRegularExpression(
                "([0-2][0-9]|3[01])(0[1-9]|1[0-2])([0-9]{2})");
            rmc_match = rmc_format.match(rmc_date);
            if (!rmc_match.hasMatch())
            {
                ret.first = tr("Invalid date format in RMC sentence: \"%1\".")
                    .arg(rmc_date);
                return ret;
            }
            this_date = tr("20%1-%2-%3")
                .arg(rmc_match.captured(3),
                     rmc_match.captured(2),
                     rmc_match.captured(1));

            // Update consolidated information
            if (old_time.isEmpty() ||
                old_time == this_time)
            {
                old_time = this_time;
                date = this_date;
            }
        }
        
        // Check for new waypoint
        if (this_time != old_time)
        {
            const QString date_time = tr("%1 %2").arg(date).arg(time);
            if (m_IdToDateTime[track_id].isEmpty())
            {
                m_IdToDateTime[track_id] = date_time;
                m_IdToTitle[track_id] = tr("Hike on %1").arg(date_time);
            }
            
            // Create new waypoint
            const int waypoint_id = way -> CreateWaypoint(longitude, latitude,
                elevation, date_time);
            track_waypoint_ids << waypoint_id;
            
            // Prep next waypoint
            old_time = this_time;
            date = this_date;
            time = this_time;
            longitude = this_longitude;
            latitude = this_latitude;
            elevation = this_elevation;
        }
    }

    // Create last waypoint
    if (!old_time.isEmpty())
    {
        const QString date_time = tr("%1 %2").arg(date).arg(time);
        const int waypoint_id = way -> CreateWaypoint(longitude, latitude,
            elevation, date_time);
        track_waypoint_ids << waypoint_id;
    }

    // Add Waypoints to Track
    way -> AddWaypointsToTrack(track_waypoint_ids, track_id);

    // Database changed
    m_UnsavedChanges[track_id] = true;
    m_ExistsInDatabase[track_id] = false;

    // Track has been cratd
    emit TrackCreated(track_id);

    // Close input file
    log_file.close();
    
    // Done
    return ret;
}



// ===================================================================== Tracks



///////////////////////////////////////////////////////////////////////////////
// Check if Track exists
bool GPSTrack::DoesIdExist(const int mcTrackId) const
{
    return m_IdToTitle.contains(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Check if Tracks exist
bool GPSTrack::DoIdsExist(const QList < int > mcTrackIds) const
{
    // There's not a million of them, so go one by one
    for (int track_id : mcTrackIds)
    {
        if (!m_IdToTitle.contains(track_id))
        {
            return false;
        }
    }
    
    // All accounted for
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Get Track IDs
QList < int > GPSTrack::GetAllIds() const
{
    // Natural order by name
    return StringHelper::SortHash(m_IdToTitle);
}



///////////////////////////////////////////////////////////////////////////////
// Sort Track IDs (by title)
QList < int > GPSTrack::Sort(const QSet < int > mcTrackIds)
{
    // Sort Track IDs
    QHash < int, QString > track_title;
    for (int track_id : mcTrackIds)
    {
        // Check if Track exists
        if (!m_IdToTitle.contains(track_id))
        {
            MessageLogger::Error("GPSTrack::Sort()",
                tr("Unknown Track ID %1. Fatal.").arg(track_id));
            return QList < int >();
        }
        track_title[track_id] = m_IdToTitle[track_id];
    }
    return StringHelper::SortHash(track_title);
}



///////////////////////////////////////////////////////////////////////////////
// Delete Track
void GPSTrack::DeleteTrack(const int mcTrackId)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::DeleteTrack()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }
    
    // Delete associated Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    way -> DeleteWaypoints(way -> GetWaypointIdsForTrackId(mcTrackId));
    
    // Emit signal (before the fact)
    emit TrackWillBeDeleted(mcTrackId);
    
    // Delete Track
    m_IdToTitle.remove(mcTrackId);
    m_IdToDateTime.remove(mcTrackId);
    m_IdToColor.remove(mcTrackId);
    m_IdToOpacity.remove(mcTrackId);
    m_IdToWaypointColor.remove(mcTrackId);
    m_IdToWaypointOpacity.remove(mcTrackId);
    
    // Database updates
    m_UnsavedChanges.remove(mcTrackId);
    if (m_ExistsInDatabase[mcTrackId])
    {
        m_DeleteFromDatabase << mcTrackId;
    }
    m_ExistsInDatabase.remove(mcTrackId);

    // Emit signal (after the fact)
    emit TrackDeleted(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Get Track title
QString GPSTrack::GetTitle(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetTitle()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QString();
    }
    
    // Return title
    return m_IdToTitle[mcTrackId];
}



///////////////////////////////////////////////////////////////////////////////
// Set Track title
void GPSTrack::SetTitle(const int mcTrackId, const QString mcNewTitle)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::SetTitle()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }
    
    // Set new Track title
    m_IdToTitle[mcTrackId] = mcNewTitle;
    
    // Database changes
    m_UnsavedChanges[mcTrackId] = true;
    
    // Emit signal
    emit TrackChanged(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Get date/time
QString GPSTrack::GetDateTime(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetDateTime()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QString();
    }

    // Return date/time
    return m_IdToDateTime[mcTrackId];
}



///////////////////////////////////////////////////////////////////////////////
// Set date/time
void GPSTrack::SetDateTime(const int mcTrackId,
    const QString mcNewDateTime)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::SetDateTime()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }
    
    // Check if new date/time has correct format
    static const QRegularExpression regexpr(
        "^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01]) "
        "([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]");
    QRegularExpressionMatch match = regexpr.match(mcNewDateTime);
    if (!match.hasMatch())
    {
        MessageLogger::Error(CALL_METHOD,
            tr("Invalid date/time format \"%1\". Fatal.").arg(mcNewDateTime));
        return;
    }
    
    // Set new value
    m_IdToDateTime[mcTrackId] = mcNewDateTime;
    
    // Database changes
    m_UnsavedChanges[mcTrackId] = true;
    
    // Emit signal
    emit TrackChanged(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Set color
void GPSTrack::SetColor(const int mcTrackId, const QColor mcColor)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::SetColor()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }

    // Set new value
    m_IdToColor[mcTrackId] = mcColor;
    
    // Database changes
    m_UnsavedChanges[mcTrackId] = true;
    
    // Emit signal
    emit TrackChanged(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Get color
QColor GPSTrack::GetColor(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetColor()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QColor();
    }
    
    // Return value
    return m_IdToColor[mcTrackId];
}


    
///////////////////////////////////////////////////////////////////////////////
// Set opacity
void GPSTrack::SetOpacity(const int mcTrackId, const int mcOpacity)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::SetOpacity()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }
    
    // Opacity is between 0 and 255
    if (mcOpacity < 0 || mcOpacity > 255)
    {
        MessageLogger::Error("GPSTrack::SetOpacity()",
            tr("Invalid opacity value %1 (must be between 0 and 255). Fatal.")
                .arg(mcOpacity));
        return;
    }
    
    // Set new value
    m_IdToOpacity[mcTrackId] = mcOpacity;
    
    // Database changes
    m_UnsavedChanges[mcTrackId] = true;
    
    // Emit signal
    emit TrackChanged(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Get opacity
int GPSTrack::GetOpacity(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetOpacity()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return -1;
    }
    
    // Return value
    return m_IdToOpacity[mcTrackId];
}



///////////////////////////////////////////////////////////////////////////////
// Set Waypoint color
void GPSTrack::SetWaypointColor(const int mcTrackId, const QColor mcColor)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::SetWaypointColor()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }

    // Set new value
    m_IdToWaypointColor[mcTrackId] = mcColor;
    
    // Database changes
    m_UnsavedChanges[mcTrackId] = true;
    
    // Emit signal
    emit TrackChanged(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Get Waypoint color
QColor GPSTrack::GetWaypointColor(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetWaypointColor()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QColor();
    }
    
    // Return value
    return m_IdToWaypointColor[mcTrackId];
}



///////////////////////////////////////////////////////////////////////////////
// Set Waypoint opacity
void GPSTrack::SetWaypointOpacity(const int mcTrackId,
    const int mcOpacity)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::SetWaypointOpacity()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }

    // Opacity is between 0 and 255
    if (mcOpacity < 0 || mcOpacity > 255)
    {
        MessageLogger::Error("GPSTrack::SetWaypointOpacity()",
            tr("Invalid opacity value %1 (must be between 0 and 255). Fatal.")
                .arg(mcOpacity));
        return;
    }

    // Set new value
    m_IdToWaypointOpacity[mcTrackId] = mcOpacity;
    
    // Database changes
    m_UnsavedChanges[mcTrackId] = true;
    
    // Emit signal
    emit TrackChanged(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Get Waypoint opacity
int GPSTrack::GetWaypointOpacity(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetWaypointOpacity()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return -1;
    }
    
    // Return value
    return m_IdToWaypointOpacity[mcTrackId];
}



///////////////////////////////////////////////////////////////////////////////
// Get Waypoint IDs
QList < int > GPSTrack::GetWaypointIds(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetWaypointIds()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QList < int >();
    }
    
    // Return title
    return GPSWaypoint::Instance() -> GetWaypointIdsForTrackId(mcTrackId);
}



///////////////////////////////////////////////////////////////////////////////
// Create new Track with given Waypoints
int GPSTrack::CreateNewTrackWithWaypointIds(
    const QList < int > mcWaypointIds)
{
    // Default color
    Preferences * p = Preferences::Instance();
    const int r = p -> GetValue("GPSTrack_DefaultTrackColorR").toInt();
    const int g = p -> GetValue("GPSTrack_DefaultTrackColorG").toInt();
    const int b = p -> GetValue("GPSTrack_DefaultTrackColorB").toInt();
    QColor default_color(r, g, b);

    // Do nothing for an empty set of Waypoints
    if (mcWaypointIds.isEmpty())
    {
        MessageLogger::Error("GPSTrack::CreateNewTrackWithWaypointIds()",
            tr("Empty list of Waypoint IDs. Fatal."));
        return INVALID_ID;
    }
    
    // Check if Waypoint IDs exist
    QHash < int, QString > tmp_ids;
    QSet < int > affected_tracks;
    GPSWaypoint * way = GPSWaypoint::Instance();
    for (int waypoint_id : mcWaypointIds)
    {
        if (!way -> DoesIdExist(waypoint_id))
        {
            MessageLogger::Error("GPSTrack::CreateNewTrackWithWaypointIds()",
                tr("Unknown Waypoint ID %1. Fatal.").arg(waypoint_id));
            return INVALID_ID;
        }
        tmp_ids[waypoint_id] = way -> GetDateTime(waypoint_id);
        affected_tracks << way -> GetTrackId(waypoint_id);
    }
    const QList < int > sorted_ids = StringHelper::SortHash(tmp_ids);
    
    // Create new Track
    const int new_track_id = m_NextId++;
    m_IdToTitle[new_track_id] = tr("Track from Selected Waypoints");
    m_IdToDateTime[new_track_id] = way -> GetDateTime(mcWaypointIds.first());
    m_IdToColor[new_track_id] = default_color;
    m_IdToOpacity[new_track_id] = int(0.6*255);
    m_IdToWaypointColor[new_track_id] = default_color;
    m_IdToWaypointOpacity[new_track_id] = 255;
    
    // Add Waypoints
    way -> AddWaypointsToTrack(sorted_ids, new_track_id);
    
    // Changes
    for (int track_id : affected_tracks)
    {
        // Check if the Track has any Waypoints left
        const QList < int > remaining_waypoint_ids =
            way -> GetWaypointIdsForTrackId(track_id);
        if (remaining_waypoint_ids.isEmpty())
        {
            // No. Delete it.
            DeleteTrack(track_id);
        } else
        {
            // Yes. Track changed.
            emit TrackChanged(track_id);
        }
    }
    emit TrackCreated(new_track_id);
    
    // Done
    return new_track_id;
}



///////////////////////////////////////////////////////////////////////////////
// Get Track IDs that are at least partially in a certain rangs
QList < int > GPSTrack::GetPartiallyVisibleTracks(
    const double mcMinLongitude, const double mcMaxLongitude,
    const double mcMinLatitude, const double mcMaxLatitude) const
{
    // Return value
    QList < int > ret;
    
    // Loop Tracks
    GPSWaypoint * way = GPSWaypoint::Instance();
    for (int track_id : m_IdToTitle.keys())
    {
        // Loop Waypoints
        const QList < int > waypoint_ids =
            way -> GetWaypointIdsForTrackId(track_id);
        for (int waypoint_id : waypoint_ids)
        {
            // Check if within range
            const QPair < double, double > long_lat = 
                way -> GetPosition(waypoint_id);
            if (long_lat.first >= mcMinLongitude &&
                long_lat.first <= mcMaxLongitude &&
                long_lat.second >= mcMinLatitude &&
                long_lat.second <= mcMaxLatitude)
            {
                ret << track_id;
                break;
            }
            
            // Next Waypoint
        }
        
        // Next Track
    }
    
    // Done
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Track start and end time
QPair < QString, QString > GPSTrack::GetTimeRange(
    const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetTimeRange()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QPair < QString, QString >();
    }
    
    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids = 
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        MessageLogger::Error("GPSTrack::GetTimeRange()",
            tr("Track %1 has no Waypoints. Fatal.").arg(mcTrackId));
        return QPair < QString, QString >();
    }
    
    // Return time range
    const QString first_time = way -> GetDateTime(waypoint_ids.first());
    const QString last_time = way -> GetDateTime(waypoint_ids.last());
    return QPair < QString, QString >(first_time, last_time);
}



///////////////////////////////////////////////////////////////////////////////
// Get durations (in seconds)
int GPSTrack::GetDurationInSeconds(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetDurationInSeconds()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return -1;
    }
    
    // Check if Track has sufficient Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.size() < 2)
    {
        // No, but for our purpose here, that's okay.
        return 0;
    }
    
    // Return time range
    const QString first_time = way -> GetDateTime(waypoint_ids.first());
    const QString last_time = way -> GetDateTime(waypoint_ids.last());

    // Convert to time
    const QDateTime start =
        QDateTime::fromString(first_time, "yyyy-MM-dd hh:mm:ss");
    const QDateTime end =
        QDateTime::fromString(last_time, "yyyy-MM-dd hh:mm:ss");
    return int(start.secsTo(end));
}



///////////////////////////////////////////////////////////////////////////////
// Get distance (in meters)
double GPSTrack::GetDistanceInMeters(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetDistanceInMeters()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return nan("");
    }
    
    // Try and lookup
    if (m_DistanceInMeters.contains(mcTrackId))
    {
        return m_DistanceInMeters[mcTrackId];
    }

    // Check if Track has sufficient Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.size() < 2)
    {
        // No, but for our purpose here, that's okay.
        return 0.0;
    }
    
    // Return value
    double distance = 0.0;
    
    // Calculate distance covered
    double previous_lat = nan("");
    double previous_long = nan("");
    for (int waypoint_id : waypoint_ids)
    {
        const QPair < double, double > this_long_lat =
            way -> GetPosition(waypoint_id);
        if (previous_lat == previous_lat)
        {
            // We do have a previous Waypoint
            distance += Map::CalculateDistanceInMeters(previous_long,
                previous_lat, this_long_lat.first, this_long_lat.second);
        }
        previous_long = this_long_lat.first;
        previous_lat = this_long_lat.second;
    }

    // Save in cache
    m_DistanceInMeters[mcTrackId] = distance;
    
    // Return distance
    return distance;
}



///////////////////////////////////////////////////////////////////////////////
// Get distance (in ft)
double GPSTrack::GetDistanceInFeet(const int mcTrackId) const
{
    return GetDistanceInMeters(mcTrackId) * Map::M_IN_FT;
}



///////////////////////////////////////////////////////////////////////////////
// Get (cumulative) distance for all waypoints
QList < double > GPSTrack::GetTrackDistanceInMeters(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetTrackDistance()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QList < double >();
    }

    // Get Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    
    // Return value
    QList < double > ret;
    
    // Current distance
    double distance = 0.0;
    
    // Calculate distance covered
    double previous_lat = nan("");
    double previous_long = nan("");
    for (int waypoint_id : waypoint_ids)
    {
        const QPair < double, double > this_long_lat =
            way -> GetPosition(waypoint_id);
        if (previous_lat == previous_lat)
        {
            // We do have a previous Waypoint
            distance += Map::CalculateDistanceInMeters(previous_long,
                previous_lat, this_long_lat.first, this_long_lat.second);
        }
        previous_long = this_long_lat.first;
        previous_lat = this_long_lat.second;
        ret << distance;
    }
    
    // Return distance
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Get (cumulative) distance for all waypoints
QList < double > GPSTrack::GetTrackDistanceInFeet(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetTrackDistance()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QList < double >();
    }
    
    // Get Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    
    // Return value
    QList < double > ret;
    
    // Current distance
    double distance = 0.0;
    
    // Calculate distance covered
    double previous_lat = nan("");
    double previous_long = nan("");
    for (int waypoint_id : waypoint_ids)
    {
        const QPair < double, double > this_long_lat =
            way -> GetPosition(waypoint_id);
        if (previous_lat == previous_lat)
        {
            // We do have a previous Waypoint
            distance += Map::CalculateDistanceInMeters(previous_long,
                previous_lat, this_long_lat.first, this_long_lat.second);
        }
        previous_long = this_long_lat.first;
        previous_lat = this_long_lat.second;
        ret << distance * Map::M_IN_FT;
    }
    
    // Return distance
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Get elevation for all waypoints
QList < double > GPSTrack::GetTrackElevationInMeters(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetTrackElevation()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QList < double >();
    }
    
    // Get Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
        
    // Return value
    QList < double > ret;
    for (int waypoint_id : waypoint_ids)
    {
        ret << way -> GetElevation(waypoint_id);
    }
    
    // Return elevation
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Get elevation for all waypoints
QList < double > GPSTrack::GetTrackElevationInFeet(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetTrackElevation()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QList < double >();
    }
    
    // Get Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
        
    // Return value
    QList < double > ret;
    for (int waypoint_id : waypoint_ids)
    {
        ret << way -> GetElevation(waypoint_id) * Map::M_IN_FT;
    }
    
    // Return elevation
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Get elapsed time for all waypoints
QList < double > GPSTrack::GetElapsedTime(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetElsapsedTime()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QList < double >();
    }
    
    // Get Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
        
    // Check if it's empty
    if (waypoint_ids.isEmpty())
    {
        return QList < double >();
    }
    
    // Start time
    const int first_id = waypoint_ids.first();
    const double start_time = QDateTime::fromString(
        way -> GetDateTime(first_id), "yyyy-MM-dd hh:mm:ss")
            .toSecsSinceEpoch();
    
    // Return value
    QList < double > ret;
    
    // Calculate distance covered
    for (int waypoint_id : waypoint_ids)
    {
        const double this_time = QDateTime::fromString(
            way -> GetDateTime(waypoint_id), "yyyy-MM-dd hh:mm:ss")
                .toSecsSinceEpoch();
        ret << int(this_time - start_time);
    }
    
    // Return distance
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Get minimum elevation (in meters)
double GPSTrack::GetMinimumElevationInMeters(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetMinimumElevationInMeters()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return nan("");
    }
    
    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        return nan("");
    }

    // Get mininum elevation
    double min_elev = nan("");
    for (int waypoint_id : waypoint_ids)
    {
        const double this_elev = way -> GetElevation(waypoint_id);
        if (min_elev == min_elev &&
            this_elev == this_elev)
        {
            min_elev = qMin(min_elev, this_elev);
        } else
        {
            if (this_elev == this_elev)
            {
                min_elev = this_elev;
            }
        }
    }
    
    // Done
    return min_elev;
}



///////////////////////////////////////////////////////////////////////////////
// Get minimum elevation (in feet)
double GPSTrack::GetMinimumElevationInFeet(const int mcTrackId) const
{
    return GetMinimumElevationInMeters(mcTrackId) * Map::M_IN_FT;
}



///////////////////////////////////////////////////////////////////////////////
// Get maximum elevation (in meters)
double GPSTrack::GetMaximumElevationInMeters(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetMaximumElevationInMeters()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return nan("");
    }
    
    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        return nan("");
    }

    // Get maxinum elevation
    double max_elev = nan("");
    for (int waypoint_id : waypoint_ids)
    {
        const double this_elev = way -> GetElevation(waypoint_id);
        if (max_elev == max_elev &&
            this_elev == this_elev)
        {
            max_elev = qMax(max_elev, this_elev);
        } else
        {
            if (this_elev == this_elev)
            {
                max_elev = this_elev;
            }
        }
    }
    
    // Done
    return max_elev;
}



///////////////////////////////////////////////////////////////////////////////
// Get maximum elevation (in feet)
double GPSTrack::GetMaximumElevationInFeet(const int mcTrackId) const
{
    return GetMaximumElevationInMeters(mcTrackId) * Map::M_IN_FT;
}



///////////////////////////////////////////////////////////////////////////////
// Get elevation gain (in meters)
double GPSTrack::GetElevationGainInMeters(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetElevationGainInMeters()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return nan("");
    }
    
    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        return 0;
    }
    
    // Get min and max elevation
    double min_elev = nan("");
    double max_elev = nan("");
    for (int waypoint_id : waypoint_ids)
    {
        const double this_elev = way -> GetElevation(waypoint_id);
        if (min_elev == min_elev &&
            this_elev == this_elev)
        {
            min_elev = qMin(min_elev, this_elev);
            max_elev = qMax(max_elev, this_elev);
        } else
        {
            if (this_elev == this_elev)
            {
                min_elev = this_elev;
                max_elev = this_elev;
            }
        }
    }
    
    // Done
    return (max_elev - min_elev);
}



///////////////////////////////////////////////////////////////////////////////
// Get elevation gain (in feet)
double GPSTrack::GetElevationGainInFeet(const int mcTrackId) const
{
    return GetElevationGainInMeters(mcTrackId) * Map::M_IN_FT;
}



///////////////////////////////////////////////////////////////////////////////
// Get total elevation gain (in meters)
double GPSTrack::GetTotalElevationGainInMeters(const int mcTrackId) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetTotalElevationGainInMeters()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return nan("");
    }
    
    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        return 0.;
    }
    
    // Calculate total elevation gain
    double ret = 0.;
    double last_elev = nan("");
    for (int waypoint_id : waypoint_ids)
    {
        const double this_elev = way -> GetElevation(waypoint_id);
        if (this_elev == this_elev)
        {
            if (last_elev == last_elev)
            {
                ret += qMax(this_elev - last_elev, 0.);
            }
            last_elev = this_elev;
        }
    }
    
    // Done
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Get total elevation gain (in feet)
double GPSTrack::GetTotalElevationGainInFeet(const int mcTrackId) const
{
    return GetTotalElevationGainInMeters(mcTrackId) * Map::M_IN_FT;
}



///////////////////////////////////////////////////////////////////////////////
// Get position (interpolated)
QPair < double, double > GPSTrack::GetPositionForDateTime(
    const int mcTrackId, const QString mcDateTime) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetPositionForDateTime()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QPair < double, double >();
    }

    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        MessageLogger::Error("GPSTrack::GetPositionForDateTime()",
            tr("Track %1 has no Waypoints. Fatal.").arg(mcTrackId));
        return QPair < double, double >();
    }
    
    // Check for correct date/time format
    static const QRegularExpression regexpr(
        "^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01]) "
        "([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]");
    QRegularExpressionMatch match = regexpr.match(mcDateTime);
    if (!match.hasMatch())
    {
        MessageLogger::Error(CALL_METHOD,
            tr("Invalid date/time format \"%1\". Fatal.").arg(mcDateTime));
        return QPair < double, double >();
    }
    
    // Find Waypoint before and after date requested
    int before_id = INVALID_ID;
    int after_id = INVALID_ID;
    for (int waypoint_id : waypoint_ids)
    {
        // Check if we are past the time
        const QString this_datetime = way -> GetDateTime(waypoint_id);
        if (this_datetime > mcDateTime)
        {
            after_id = waypoint_id;
            break;
        }
        
        // Otherwise, this is a good candidate for 
        before_id = waypoint_id;
    }
    
    // Check for problems
    if (before_id == INVALID_ID)
    {
        MessageLogger::Error("GPSTrack::GetPositionForDateTime()",
            tr("Time requested if before the entire track. Fatal.")
                .arg(mcDateTime));
        return QPair < double, double >();
    }
    if (after_id == INVALID_ID)
    {
        MessageLogger::Error("GPSTrack::GetPositionForDateTime()",
            tr("Time requested if after the entire track. Fatal.")
                .arg(mcDateTime));
        return QPair < double, double >();
    }
    
    // Get times
    const double time_before = QDateTime::fromString(
        way -> GetDateTime(before_id), "yyyy-MM-dd hh:mm:ss")
            .toSecsSinceEpoch();
    const double time_after = QDateTime::fromString(
        way -> GetDateTime(after_id), "yyyy-MM-dd hh:mm:ss")
            .toSecsSinceEpoch();
    const double time_requested = QDateTime::fromString(
        mcDateTime, "yyyy-MM-dd hh:mm:ss")
            .toSecsSinceEpoch();
    
    // Interpolate
    const QPair < double, double > before_long_lat = 
        way -> GetPosition(before_id);
    const QPair < double, double > after_long_lat = 
        way -> GetPosition(after_id);
    const double theta =
        (time_requested - time_before) / (time_after - time_before);
    const double longitude =
        (1. - theta) * before_long_lat.first + theta * after_long_lat.first;
    const double latitude =
        (1. - theta) * before_long_lat.second + theta * after_long_lat.second;
        
    // Return position
    return QPair < double, double >(longitude, latitude);
}



///////////////////////////////////////////////////////////////////////////////
// Get elevation (interpolated)
double GPSTrack::GetElevationForDateTime(const int mcTrackId,
    const QString mcDateTime) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetElevationForDateTime()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return nan("");
    }

    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        MessageLogger::Error("GPSTrack::GetElevationForDateTime()",
            tr("Track %1 has no Waypoints. Fatal.").arg(mcTrackId));
        return nan("");
    }
    
    // Check for correct date/time format
    static const QRegularExpression regexpr(
        "^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01]) "
        "([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]");
    QRegularExpressionMatch match = regexpr.match(mcDateTime);
    if (!match.hasMatch())
    {
        MessageLogger::Error(CALL_METHOD,
            tr("Invalid date/time format \"%1\". Fatal.").arg(mcDateTime));
        return nan("");
    }
    
    // Find Waypoint before and after date requested
    int before_id = INVALID_ID;
    int after_id = INVALID_ID;
    for (int waypoint_id : waypoint_ids)
    {
        // Check if we are past the time
        const QString this_datetime = way -> GetDateTime(waypoint_id);
        if (this_datetime > mcDateTime)
        {
            after_id = waypoint_id;
            break;
        }
        
        // Otherwise, this is a good candidate for 
        before_id = waypoint_id;
    }
    
    // Check for problems
    if (before_id == INVALID_ID)
    {
        MessageLogger::Error("GPSTrack::GetElevationForDateTime()",
            tr("Time requested is before the entire track. Fatal.")
                .arg(mcDateTime));
        return nan("");
    }
    if (after_id == INVALID_ID)
    {
        MessageLogger::Error("GPSTrack::GetElevationForDateTime()",
            tr("Time requested is after the entire track. Fatal.")
                .arg(mcDateTime));
        return nan("");
    }
    
    // Get times
    const double time_before = QDateTime::fromString(
        way -> GetDateTime(before_id), "yyyy-MM-dd hh:mm:ss")
            .toSecsSinceEpoch();
    const double time_after = QDateTime::fromString(
        way -> GetDateTime(after_id), "yyyy-MM-dd hh:mm:ss")
            .toSecsSinceEpoch();
    const double time_requested = QDateTime::fromString(
        mcDateTime, "yyyy-MM-dd hh:mm:ss")
            .toSecsSinceEpoch();
    
    // Interpolate
    const double theta = 
        (time_requested - time_before) / (time_after - time_before);
    const double before_elev = way -> GetElevation(before_id);
    const double after_elev = way -> GetElevation(after_id);
    const double elevation = (1. - theta) * before_elev + theta * after_elev;
        
    // Return elevation
    return elevation;
}



///////////////////////////////////////////////////////////////////////////////
// Get position (interpolated)
QPair < double, double > GPSTrack::GetPositionForDistanceInMeters(
    const int mcTrackId, const double mcDistance) const
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::GetPositionForDistanceInMeters()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QPair < double, double >(nan(""), nan(""));
    }

    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        MessageLogger::Error("GPSTrack::GetPositionForDistanceInMeters()",
            tr("Track %1 has no Waypoints. Fatal.").arg(mcTrackId));
        return QPair < double, double >(nan(""), nan(""));
    }
    
    // Check meaningful distance
    if (mcDistance < 0.0 ||
        mcDistance > GetDistanceInMeters(mcTrackId))
    {
        return QPair < double, double >(nan(""), nan(""));
    }
    
    // Try and lookup
    // (round to 1cm)
    const int lookup_distance = int(round(mcDistance * 100));
    if (m_PositionForDistanceMeters.contains(mcTrackId) &&
        m_PositionForDistanceMeters[mcTrackId].contains(lookup_distance))
    {
        return m_PositionForDistanceMeters[mcTrackId][lookup_distance];
    }
    
    // Work with rounded distance
    const double distance = lookup_distance * 0.01;
    
    // Find Waypoint before and after distance requested
    int before_id = INVALID_ID;
    int after_id = INVALID_ID;
    double before_distance = 0.;
    double after_distance = 0.;
    double current_distance = 0.;
    for (int waypoint_id : waypoint_ids)
    {
        // New distance
        if (before_id != INVALID_ID)
        {
            const QPair < double, double > before_pos =
                way -> GetPosition(before_id);
            const QPair < double, double > current_pos =
                way -> GetPosition(waypoint_id);
            current_distance += Map::CalculateDistanceInMeters(
                before_pos.first, before_pos.second,
                current_pos.first, current_pos.second);
        }
        
        // Check if we are past the distance
        if (current_distance >= distance)
        {
            after_id = waypoint_id;
            after_distance = current_distance;
            break;
        }
        
        // Otherwise, this is a good candidate for 
        before_id = waypoint_id;
        before_distance = current_distance;
    }
    
    // Check for problems
    if (before_id == INVALID_ID)
    {
        if (distance == 0.)
        {
            // For the trail start, we never get to set before_id above.
            before_id = after_id;
        } else
        {
            MessageLogger::Error("GPSTrack::GetPositionForDistanceInMeters()",
                tr("Distance requested is before the entire track. Fatal.")
                    .arg(distance));
            return QPair < double, double >(nan(""), nan(""));
        }
    }
    if (after_id == INVALID_ID)
    {
        MessageLogger::Error("GPSTrack::GetPositionForDistanceInMeters()",
            tr("Distance requested is after the entire track. Fatal.")
                .arg(distance));
        return QPair < double, double >(nan(""), nan(""));
    }
    
    // Interpolate
    const double theta = 
        (distance - before_distance) / (after_distance - before_distance);
    const QPair < double, double > before_pos = way -> GetPosition(before_id);
    const QPair < double, double > after_pos = way -> GetPosition(after_id);
    const double pos_long = (1. - theta) * before_pos.first +
        theta * after_pos.first;
    const double pos_lat = (1. - theta) * before_pos.second +
        theta * after_pos.second;
        
    // Cache result
    m_PositionForDistanceMeters[mcTrackId][lookup_distance] =
        QPair < double, double >(pos_long, pos_lat);
    
    // Return position
    return m_PositionForDistanceMeters[mcTrackId][lookup_distance];
}



///////////////////////////////////////////////////////////////////////////////
// Get position (interpolated)
QPair < double, double > GPSTrack::GetPositionForDistanceInFeet(
    const int mcTrackId, const double mcDistance) const
{
    return GetPositionForDistanceInMeters(mcTrackId,
        mcDistance / Map::M_IN_FT);
}



///////////////////////////////////////////////////////////////////////////////
// Direction vertical (normal) to the track
QPair < double, double >
    GPSTrack::CalculateNormalDirectionForDistanceInMeters(
        const int mcTrackId, const double mcDistance)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error(
            "GPSTrack::CalculateNormalDirectionForDistanceInMeters()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return QPair < double, double >(nan(""), nan(""));
    }

    // Check if Track is empty
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    if (waypoint_ids.isEmpty())
    {
        MessageLogger::Error(
            "GPSTrack::CalculateNormalDirectionForDistanceInMeters()",
            tr("Track %1 has no Waypoints. Fatal.").arg(mcTrackId));
        return QPair < double, double >(nan(""), nan(""));
    }
    
    // Check meaningful distance
    if (mcDistance < 0.0 ||
        mcDistance > GetDistanceInMeters(mcTrackId))
    {
        return QPair < double, double >(nan(""), nan(""));
    }
    
    // Try and lookup
    // (round to 1cm)
    const int lookup_distance = int(round(mcDistance * 100));
    if (m_NormalDirectionForDistanceInMeters.contains(mcTrackId) &&
        m_NormalDirectionForDistanceInMeters[mcTrackId].contains(
            lookup_distance))
    {
        return
            m_NormalDirectionForDistanceInMeters[mcTrackId][lookup_distance];
    }
    
    // Work with rounded distance
    const double distance = lookup_distance * 0.01;

    // Calculate tangent
    QPair < double, double > pos_back =
        GetPositionForDistanceInMeters(mcTrackId, distance - 5.);
    QPair < double, double > pos_ahead =
        GetPositionForDistanceInMeters(mcTrackId, distance + 5.);
    const QPair < int, int > xy_back =
        Map::ConvertLatLongToPixel(20, pos_back.first, pos_back.second);
    const QPair < int, int > xy_ahead =
        Map::ConvertLatLongToPixel(20, pos_ahead.first, pos_ahead.second);
    // Minus in y component because of orientation of y axis in Pixmap
    // (origin is top left, increasing values of y go down)
    QPair < double, double > tangent(
        .5 * (xy_ahead.first - xy_back.first),
        -.5 * (xy_ahead.second - xy_back.second));
    const double norm = sqrt(tangent.first * tangent.first +
        tangent.second * tangent.second);
    tangent.first /= norm;
    tangent.second /= norm;

    // Cache result
    m_NormalDirectionForDistanceInMeters[mcTrackId][lookup_distance] =
        QPair < double, double >(tangent.second, -tangent.first);
    
    // Return normal direction
    return m_NormalDirectionForDistanceInMeters[mcTrackId][lookup_distance];
}



///////////////////////////////////////////////////////////////////////////////
// Direction vertical (normal) to the track
QPair < double, double > GPSTrack::CalculateNormalDirectionForDistanceInFeet(
    const int mcTrackId, const double mcDistance)
{
    return CalculateNormalDirectionForDistanceInMeters(mcTrackId,
        mcDistance / Map::M_IN_FT);
}



///////////////////////////////////////////////////////////////////////////////
// Get closest Track (time)
int GPSTrack::GetClosestTimeTrackId(const QString mcDateTime)
{
    // Convert date/time
    const int reference_t = 
        QDateTime::fromString(mcDateTime, "yyyy-MM-dd hh:mm:ss")
            .toSecsSinceEpoch();
        
    // Find closest
    double closest_delta_t = 1e10;
    int closest_track_id = GPSTrack::INVALID_ID;
    GPSWaypoint * way = GPSWaypoint::Instance();
    for (int track_id : m_IdToTitle.keys())
    {
        const QList < int > waypoint_ids =
            way -> GetWaypointIdsForTrackId(track_id);
        const int waypoint_id = waypoint_ids.first();
        const QString waypoint_time = way -> GetDateTime(waypoint_id);
        const double this_t =
            QDateTime::fromString(waypoint_time, "yyyy-MM-dd hh:mm:ss")
                .toSecsSinceEpoch();
        const double this_delta_t = fabs(reference_t - this_t);
        if (this_delta_t < closest_delta_t)
        {
            closest_track_id = track_id;
            closest_delta_t = this_delta_t;
        }
    }
    
    // Done
    return closest_track_id;
}



///////////////////////////////////////////////////////////////////////////////
// Add time to track (i.e. all Waypoints)
void GPSTrack::AdjustTrackTime(const int mcTrackId, const int mcDeltaSeconds)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::AdjustTrackTime()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }
    
    // Update Waypoints
    GPSWaypoint * way = GPSWaypoint::Instance();
    const QList < int > waypoint_ids =
        way -> GetWaypointIdsForTrackId(mcTrackId);
    way -> AdjustDateTime(waypoint_ids, mcDeltaSeconds);
}



///////////////////////////////////////////////////////////////////////////////
// Track changed
void GPSTrack::EmitTrackChanged(const int mcTrackId)
{
    // Check if Track exists
    if (!m_IdToTitle.contains(mcTrackId))
    {
        MessageLogger::Error("GPSTrack::EmitTrackChanged()",
            tr("Unknown Track ID %1. Fatal.").arg(mcTrackId));
        return;
    }
    
    // Invalidate caches
    m_DistanceInMeters.remove(mcTrackId);
    m_PositionForDistanceMeters.remove(mcTrackId);
    m_NormalDirectionForDistanceInMeters.remove(mcTrackId);
    
    // Emit signal
    emit TrackChanged(mcTrackId);
}
