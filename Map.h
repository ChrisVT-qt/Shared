// Map.h
// Class definition file

// Avoid duplicate includes
#ifndef MAP_H
#define MAP_H

// Qt includes
#include <QPixmap>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>

// Class definition
class Map :
    public QObject
{
    Q_OBJECT
    
    
    
    // ============================================================== Lifecycle
private:
    // Constructor
    Map(const QString mcMapType, const int mcZoomLevel, const int mcLeftPixel,
        const double mcTopPixel, const int mcWidth, const int mcHeight);

public:
    // Create
    static Map * NewMap(const QString mcMapType, const int mcZoomLevel,
        const int mcLeftPixel, const int mcTopPixel, const int mcWidth,
        const int mcHeight);

    // Destructor
    virtual ~Map();

private:
    QString m_MapType;
    int m_ZoomLevel;
    int m_Left;
    int m_Top;
    int m_Width;
    int m_Height;
    
    // Initialize Preferences
    static bool InitPreferences();
    
    // Initialize prefs
    static const bool m_PreferencesInitialized;
    
    
    
    // =================================================================== Misc
public:
    // Set new size
    void SetSize(const int mcNewWidth, const int mcNewHeight);

    // Move map
    void Move(const int mcDeltaX, const int mcDeltaY);

    // Start fetching
    void StartFetching();

    // Get tiles required for a certain area
    void UpdateArea(const int mcLeft, const int mcTop, const int mcWidth,
        const int mcHeight);

    // Set zoom level
    void SetZoomLevel(const int mcNewZoomLevel);
    
    // Get top left corner
    QPair < int, int > GetTopLeft() const;

    // Set new top left corner
    void SetTopLeft(const int mcNewLeft, const int mcNewTop);
    
    // Map type
    void SetMapType(const QString mcNewMapType);
    QString GetMapType() const;
    
    // Get (current) Image
    const QPixmap & GetImage() const;

private slots:
    // Tile obtained
    void TileObtained(const QString mcMapType, const int mcZoomLevel,
        const int mcX, const int mcY, const QPixmap mcPixmap,
        const bool mcFromCache, const bool mcIsScaled);
    
private:
    QPixmap m_Image;
    QSet < QString > m_TilesNeeded;
    bool m_AllTilesRequested;

signals:
    // Fetching starting
    void MapFetchingStarted();
    
    // Map image changed
    void MapImageChanged();
    
    // Map image final
    void MapImageFinal();
    
public:
    // Check if map is final
    bool IsFinal() const;
private:
    bool m_IsFinal;
    
    
    
    // ======================================================= Helper Functions
public:
    // Min zoom level
    static int GetMinZoomLevel(const QString mcMapType);

    // Max zoom level
    static int GetMaxZoomLevel(const QString mcMapType);
    
    // Convert coordinates to pixel counts
    static QPair < int, int > ConvertLatLongToPixel(
        const int mcZoomLevel, const double mcLongitude,
        const double mcLatitude);

    // Max coordinates (min coordinates are the same, just negative)
    static QPair < int, int > GetMaxCoordinates(const int mcZoomLevel);
    
    // Convert coordinates to pixel counts
    static QPair < double, double > ConvertPixelToLatLong(
        const int mcZoomLevel, const int mcX, const int mcY);

    // Get a reasonable scale in meters
    static QPair < int, int > MapScaleInMeters(const int mcZoomLevel,
        const double mcLatitude);
    
    // Get a reasonable scale in miles
    static QPair < int, int > MapScaleInMiles(const int mcZoomLevel,
        const double mcLatitude);

    // m in ft
    static const double M_IN_FT;

    // mile in ft
    static const double MILE_IN_FT;

    // Earth radius (in meters)
    static const double EARTH_RADIUS_M;

    // Convert longitude String to double
    static double ConvertLongitudeToDouble(const QString mcLongitude);
    
    // Convert latitude String to double
    static double ConvertLatitudeToDouble(const QString mcLatitude);

    // Convert double to longitude String
    static QString ConvertDoubleToLongitude(const double mcLongitude);
    
    // Convert double to latitude String
    static QString ConvertDoubleToLatitude(const double mcLatitude);
    
    // Distance between two points
    static double CalculateDistanceInMeters(
        const double mcFirstLongitude, const double mcFirstLatitude,
        const double mcSecondLongitude, const double mcSecondLatitude);

    // Direction of second point from first point
    static double Bearing(
        const double mcFirstLongitude, const double mcFirstLatitude,
        const double mcSecondLongitude, const double mcSecondLatitude);
};

#endif
