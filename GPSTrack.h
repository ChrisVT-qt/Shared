// GPSTrack.h
// Class definition file

// Just include once
#ifndef GPSTRACK_H
#define GPSTRACK_H

// Qt includes
#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>

// Class definition
class GPSTrack
    : public QObject
{
    Q_OBJECT
    
    
    
    // ============================================================== Lifecycle
private:
	// Default constructor
	GPSTrack();

public:
	// Destructor
	virtual ~GPSTrack();

    // Instance
    static GPSTrack * FromFile(const QString & mcrFilename);



    // ================================================================= Tracks
public:
    // Get waypoint latitudes
    QList < double > GetLWaypointsLatitude() const;
private:
    QList < double > m_Latitude;
public:
    // Get waypoint longitudes
    QList < double > GetLWaypointsLongitude() const;
private:
    QList < double > m_Longitude;
public:
    // Get waypoint elevations
    QList < double > GetLWaypointsElevation() const;
private:
    QList < double > m_Elevation;

public:
    // Track start and end time
    QPair < QString, QString > GetTimeRange() const;
    
    // Get durations (in seconds)
    int GetDurationInSeconds() const;
    
    // Get distance (in meters)
    double GetDistanceInMeters() const;
};

#endif
