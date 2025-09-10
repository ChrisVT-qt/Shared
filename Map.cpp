// Map.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "MessageLogger.h"
#include "Map.h"
#include "MapCache.h"
#include "Preferences.h"

// Qt includes
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpression>

// System includes
#include <cmath>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
Map::Map(const QString mcMapType, const int mcZoomLevel, const int mcLeft,
    const double mcTop, const int mcWidth, const int mcHeight)
{
    CALL_IN(QString("mcMapType=%1, mcZoomLevel=%2, mcLeft=%3, "
        "mcTop=%4, mcWidth=%5, mcHeight=%6")
            .arg(CALL_SHOW(mcMapType),
                 CALL_SHOW(mcZoomLevel),
                 CALL_SHOW(mcLeft),
                 CALL_SHOW(mcTop),
                 CALL_SHOW(mcWidth),
                 CALL_SHOW(mcHeight)));

    // Store parameters
    m_MapType = mcMapType;
    m_ZoomLevel = mcZoomLevel;
    m_Left = mcLeft;
    m_Top = mcTop;
    m_Width = mcWidth;
    m_Height = mcHeight;
    
    // Create Image
    m_Image = QPixmap(m_Width, m_Height);
    QPainter painter(&m_Image);
    painter.fillRect(0, 0, m_Width, m_Height, Qt::gray);
    
    // Definitely not final
    m_IsFinal = false;
    
    // Connect signal
    MapCache * m = MapCache::Instance();
    connect (m, SIGNAL(MapTileObtained(const QString, const int, const int,
            const int, const QPixmap, const bool, const bool)),
        this, SLOT(TileObtained(const QString, const int, const int,
            const int, const QPixmap, const bool, const bool)));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Create
Map * Map::NewMap(const QString mcMapType, const int mcZoomLevel,
    const int mcLeft, const int mcTop, const int mcWidth, const int mcHeight)
{
    CALL_IN(QString("mcMapType=%1, mcZoomLevel=%2, mcLeft=%3, "
        "mcTop=%4, mcWidth=%5, mcHeight=%6")
            .arg(CALL_SHOW(mcMapType),
                 CALL_SHOW(mcZoomLevel),
                 CALL_SHOW(mcLeft),
                 CALL_SHOW(mcTop),
                 CALL_SHOW(mcWidth),
                 CALL_SHOW(mcHeight)));

    // Check if map type is valid
    if (mcMapType != "terrain" &&
        mcMapType != "roadmap" &&
        mcMapType != "satellite")
    {
        const QString reason = tr("Invalid map type \"%1\". Fatal.")
            .arg(mcMapType);
        MessageLogger::Error(CALL_METHOD,
            reason);
        CALL_OUT(reason);
        return nullptr;
    }
    
    // Check if zoom level is valid
    if (mcZoomLevel < GetMinZoomLevel(mcMapType) ||
        mcZoomLevel > GetMaxZoomLevel(mcMapType))
    {
        const QString reason =
            tr("Invalid zoom level %1. Valid zoom levels are %2 to %3. Fatal.")
                .arg(QString::number(mcZoomLevel),
                     QString::number(GetMinZoomLevel(mcMapType)),
                     QString::number(GetMaxZoomLevel(mcMapType)));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }
    
    // Check if top and left pixel make sense
    const QPair < double, double > coordinates = 
        ConvertPixelToLatLong(mcZoomLevel, mcLeft, mcTop);
    const double longitude = coordinates.first;
    const double latitude = coordinates.second;
    if (latitude < -90.0 || latitude > 90.0)
    {
        const QString reason =
            tr("Invalid latidude %1. Latitude (y) must be between -90 and "
                "90. Fatal.")
                .arg(QString::number(latitude));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }
    if (longitude < -180.0 || longitude > 180.0)
    {
        const QString reason =
            tr("Invalid longitude %1. Longitude (x) must be between -180 "
                "and 180. Fatal.")
                .arg(QString::number(longitude));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }
    
    // Check if resolution makes sense
    if (mcWidth < 1 || mcHeight < 1)
    {
        const QString reason = tr("Invalid resolution %1 x %2. Fatal.")
            .arg(QString::number(mcWidth),
                 QString::number(mcHeight));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }
    
    // Do it.
    Map * new_map =
        new Map(mcMapType, mcZoomLevel, mcLeft, mcTop, mcWidth, mcHeight);
    CALL_OUT("");
    return new_map;
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
Map::~Map()
{
    CALL_IN("");

    // Nothing to do.

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Initialize Preferences
bool Map::InitPreferences()
{
    CALL_IN("");

    // Preferences tag
    Preferences * p = Preferences::Instance();
    
    // Set values
    p -> AddValidTag("Map:DistanceUnits");
    p -> SetDefaultValue("Map:DistanceUnits", "miles_feet");

    p -> AddValidTag("Map:Default:Type");
    p -> SetDefaultValue("Map:Default:Type", "terrain");

    p -> AddValidTag("Map:Default:ZoomLevel");
    p -> SetDefaultValue("Map:Default:ZoomLevel", "3");

    p -> AddValidTag("Map:Default:CenterLongitude");
    p -> SetDefaultValue("Map:Default:CenterLongitude", "0.0");

    p -> AddValidTag("Map:Default:CenterLatitude");
    p -> SetDefaultValue("Map:Default:CenterLatitude", "21.0");
    
    // Done
    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Initialize prefs
const bool Map::m_PreferencesInitialized = Map::InitPreferences();



// ======================================================================= Misc



///////////////////////////////////////////////////////////////////////////////
// Set new size
void Map::SetSize(const int mcNewWidth, const int mcNewHeight)
{
    CALL_IN(QString("mcNewWidth=%1, mcNewHeight=%2")
        .arg(CALL_SHOW(mcNewWidth),
             CALL_SHOW(mcNewHeight)));

    // Check width/height makes sense
    if (mcNewWidth < 1 || mcNewHeight < 1)
    {
        const QString reason = tr("Invalid resolution %1 x %2. Fatal.")
            .arg(QString::number(mcNewWidth),
                 QString::number(mcNewHeight));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }
    
    // Deltas
    const int dx = (mcNewWidth - m_Width) / 2;
    const int dy = (mcNewHeight - m_Height) / 2;
    
    // Source and destination coordinates
    const int dest_left = qMax(dx, 0);
    const int dest_top = qMax(dy, 0);
    const int source_left = qMax(-dx, 0);
    const int source_top = qMax(-dy, 0);
    const int source_width = qMin(m_Width, mcNewWidth);
    const int source_height = qMin(m_Height, mcNewHeight);
    
    // Create new Image
    QPixmap new_image = QPixmap(m_Width + 2*dx, m_Height + 2*dy);
    QPainter image_painter(&new_image);
    image_painter.fillRect(0, 0, m_Width + 2*dx, m_Height + 2*dy, Qt::gray);
    image_painter.drawPixmap(dest_left, dest_top, m_Image,
        source_left, source_top, source_width, source_height);
    m_Image = new_image;
    m_Left -= dx;
    m_Top += dy;
    m_Width += 2*dx;
    m_Height += 2*dy;
    
    // Get Tiles if needed
    if (dx > 0)
    {
        // NW, NE, W, E, SW, SE
        UpdateArea(m_Left, m_Top, dx, m_Height);
        UpdateArea(m_Left + m_Width - dx, m_Top, dx, m_Height);
        if (dy > 0)
        {
            // N, S
            UpdateArea(m_Left + dx, m_Top, m_Width + 2*dx, dy);
            UpdateArea(m_Left + dx, m_Top - m_Height + dy, m_Width + 2*dx, dy);
        }
    } else
    {
        if (dy > 0)
        {
            // NW, N, NE, SW, S, SE
            UpdateArea(m_Left, m_Top, m_Width, dy);
            UpdateArea(m_Left, m_Top - m_Height + dy, m_Width, dy);
        }
    }
    
    // Image changed
    emit MapImageChanged();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Move map
void Map::Move(const int mcDeltaX, const int mcDeltaY)
{
    CALL_IN(QString("mcDeltaX=%1, mcDeltaY=%2")
        .arg(CALL_SHOW(mcDeltaX),
             CALL_SHOW(mcDeltaY)));

    // Source and destination coordinates
    const int dest_left = qMax(mcDeltaX, 0);
    const int dest_top = qMax(-mcDeltaY, 0);
    const int source_left = qMax(-mcDeltaX, 0);
    const int source_top = qMax(mcDeltaY, 0);
    const int source_width = m_Width - abs(mcDeltaX);
    const int source_height = m_Height - abs(mcDeltaY);
    
    // Create new Image
    QPixmap new_image = QPixmap(m_Width, m_Height);
    QPainter image_painter(&new_image);
    image_painter.fillRect(0, 0, m_Width, m_Height, Qt::gray);
    if (source_width > 0 && source_height > 0)
    {
        image_painter.drawPixmap(dest_left, dest_top, m_Image,
            source_left, source_top, source_width, source_height);
    }
    m_Image = new_image;
    m_Left -= mcDeltaX;
    m_Top -= mcDeltaY;
    
    // Get Tiles if needed
    if (mcDeltaX > 0)
    {
        // NW, W, SW
        UpdateArea(m_Left, m_Top, mcDeltaX, m_Height);
        if (mcDeltaY > 0)
        {
            // S, SE
            UpdateArea(m_Left + mcDeltaX, m_Top - m_Height + mcDeltaY,
                m_Width - mcDeltaX, mcDeltaY);
        } else
        {
            // N, NE
            UpdateArea(m_Left + mcDeltaX, m_Top + mcDeltaY,
                m_Width - mcDeltaX, -mcDeltaY);
        }
    } else
    {
        // NE, E, SE
        UpdateArea(m_Left + m_Width + mcDeltaX, m_Top, -mcDeltaX, m_Height);
        if (mcDeltaY > 0)
        {
            // SW, S
            UpdateArea(m_Left, m_Top - m_Height + mcDeltaY,
                m_Width + mcDeltaX, mcDeltaY);
        } else
        {
            // NW, N
            UpdateArea(m_Left, m_Top + mcDeltaY,
                m_Width + mcDeltaX, -mcDeltaY);
        }
    }
    
    // Image changed
    emit MapImageChanged();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Start fetching
void Map::StartFetching()
{
    CALL_IN("");

    UpdateArea(m_Left, m_Top, m_Width, m_Height);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get Tiles required for a certain area
void Map::UpdateArea(const int mcLeft, const int mcTop, const int mcWidth,
    const int mcHeight)
{
    CALL_IN(QString("mcLeft=%1, mcTop=%2, mcWidth=%3, mcHeight=%4")
        .arg(CALL_SHOW(mcLeft),
             CALL_SHOW(mcTop),
             CALL_SHOW(mcWidth),
             CALL_SHOW(mcHeight)));

    // Limits
    const int visible_west = mcLeft;
    const int visible_east = mcLeft + mcWidth;
    const int visible_north = mcTop;
    const int visible_south = mcTop - mcHeight;

    // Number of tiles needed
    const int fetch_west =
        visible_west - (visible_west % 500) - (visible_west > 0 ? 0 : 500);
    int tmp = visible_east + 500;
    const int fetch_east = tmp - (tmp % 500) - (tmp > 0 ? 0 : 500);
    const int fetch_south =
        visible_south - (visible_south % 500) - (visible_south > 0 ? 0 : 500);
    tmp = visible_north + 500;
    const int fetch_north = tmp - (tmp % 500) - (tmp > 0 ? 0 : 500);

    // Abbreviation
    MapCache * mc = MapCache::Instance();

    // Get MapTiles
    m_AllTilesRequested = false;
    for (int dy = fetch_south + 500; dy <= fetch_north; dy += 500)
    {
        for (int dx = fetch_west; dx < fetch_east; dx += 500)
        {
            const QString name = QString("%1_%2_%3_%4")
                .arg(m_MapType,
                    QString::number(m_ZoomLevel),
                    QString::number(dx),
                    QString::number(dy));
            if (m_TilesNeeded.contains(name))
            {
                // Already being obtained - will be alright
                continue;
            }
            
            // We will need to obtain this tile
            m_TilesNeeded << name;
            
            // Obtain tile
            mc -> ObtainMapTile(m_ZoomLevel, m_MapType, dx, dy);
        }
    }
    m_AllTilesRequested = true;
    
    // Check if we're done already
    if (m_TilesNeeded.isEmpty())
    {
        m_IsFinal = true;
        emit MapImageFinal();
    }

    // Let world know if we have to obtain tiles
    emit MapFetchingStarted();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Set zoom level
void Map::SetZoomLevel(const int mcNewZoomLevel)
{
    CALL_IN(QString("mcNewZoomLevel=%1")
        .arg(CALL_SHOW(mcNewZoomLevel)));

    // Check if zoom level is valid
    if (mcNewZoomLevel < GetMinZoomLevel(m_MapType) ||
        mcNewZoomLevel > GetMaxZoomLevel(m_MapType))
    {
        const QString reason =
            tr("Invalid zoom level %1. Valid zoom levels are %2 to %3. Fatal.")
                .arg(QString::number(mcNewZoomLevel),
                     QString::number(GetMinZoomLevel(m_MapType)),
                     QString::number(GetMaxZoomLevel(m_MapType)));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }
    
    // Check if there is actually any change in zoom level
    if (mcNewZoomLevel == m_ZoomLevel)
    {
        // No - don't do anything
        CALL_OUT("");
        return;
    }

    // Keep center where it was
    if (mcNewZoomLevel > m_ZoomLevel)
    {
        const int old_center_x = m_Left + m_Width/2;
        const int new_center_x =
            old_center_x * (1 << (mcNewZoomLevel - m_ZoomLevel));
        m_Left = new_center_x - m_Width/2;

        const int old_center_y = m_Top - m_Height/2;
        const int new_center_y =
            old_center_y * (1 << (mcNewZoomLevel - m_ZoomLevel));
        m_Top = new_center_y + m_Height/2;
    } else
    {
        const int old_center_x = m_Left + m_Width/2;
        const int new_center_x =
            old_center_x / (1 << (m_ZoomLevel - mcNewZoomLevel));
        m_Left = new_center_x - m_Width/2;

        const int old_center_y = m_Top - m_Height/2;
        const int new_center_y =
            old_center_y / (1 << (m_ZoomLevel - mcNewZoomLevel));
        m_Top = new_center_y + m_Height/2;
    }
    
    // Set everything
    m_ZoomLevel = mcNewZoomLevel;
    m_TilesNeeded.clear();
    
    // Rebuild map
    m_Image = QPixmap(m_Width, m_Height);
    {
        QPainter painter(&m_Image);
        painter.fillRect(0, 0, m_Width, m_Height, Qt::gray);
    }
    emit MapImageChanged();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get top left corner
QPair < int, int > Map::GetTopLeft() const
{
    // !!! Map::GetTopLeft() should be renamed; coordinates switched

    CALL_IN("");

    CALL_OUT("");
    return QPair < int, int >(m_Left, m_Top);
}



///////////////////////////////////////////////////////////////////////////////
// Set new top left corner
void Map::SetTopLeft(const int mcNewLeft, const int mcNewTop)
{
    // !!! Map::SetTopLeft() should be renamed; coordinates switched
    // !!! Map::SetTopLeft() Check if pixel coordinates are valid (depends on
    // !!! zoom level)

    CALL_IN(QString("mcNewLeft=%1, mcNewTop=%2")
        .arg(CALL_SHOW(mcNewLeft),
             CALL_SHOW(mcNewTop)));

    // Set everything
    m_Left = mcNewLeft;
    m_Top = mcNewTop;
    
    // Rebuild map
    m_Image = QPixmap(m_Width, m_Height);
    {
        QPainter painter(&m_Image);
        painter.fillRect(0, 0, m_Width, m_Height, Qt::gray);
    }
    emit MapImageChanged();
    UpdateArea(m_Left, m_Top, m_Width, m_Height);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Map type
void Map::SetMapType(const QString mcNewMapType)
{
    CALL_IN(QString("mcNewMapType=%1")
        .arg(CALL_SHOW(mcNewMapType)));

    // Check if map type is valid
    if (mcNewMapType != "terrain" &&
        mcNewMapType != "roadmap" &&
        mcNewMapType != "satellite")
    {
        const QString reason = tr("Invalid map type \"%1\". Not changed.")
            .arg(mcNewMapType);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }
    
    // Check if this is actually a change
    if (mcNewMapType == m_MapType)
    {
        // No.
        CALL_OUT("");
        return;
    }
    
    // Change
    m_MapType = mcNewMapType;
    
    // Rebuild map
    m_Image = QPixmap(m_Width, m_Height);
    {
        QPainter painter(&m_Image);
        painter.fillRect(0, 0, m_Width, m_Height, Qt::gray);
    }
    m_TilesNeeded.clear();
    emit MapImageChanged();
    UpdateArea(m_Left, m_Top, m_Width, m_Height);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Map type
QString Map::GetMapType() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_MapType;
}



///////////////////////////////////////////////////////////////////////////////
// Get (current) Image
const QPixmap & Map::GetImage() const
{
    CALL_IN("");

    // PERFORMANCE COMMENT:
    // Passing this as a reference is about 5ms faster than passing it by
    // value. This is about 30% of a MapWidget redraw.
    CALL_OUT("");
    return m_Image;
}



///////////////////////////////////////////////////////////////////////////////
// Tile obtained
void Map::TileObtained(const QString mcMapType, const int mcZoomLevel,
    const int mcX, const int mcY, const QPixmap mcPixmap,
    const bool mcFromCache, const bool mcIsScaled)
{
    CALL_IN(QString("mcMapType=%1, mcZoomLevel=%2, mcX=%3, mcY=%4, "
        "mcPixmap=%5, mcFromCache=%6, mcIsScaled=%7")
            .arg(CALL_SHOW(mcMapType),
                CALL_SHOW(mcZoomLevel),
                CALL_SHOW(mcX),
                CALL_SHOW(mcY),
                CALL_SHOW(mcPixmap),
                CALL_SHOW(mcFromCache),
                CALL_SHOW(mcIsScaled)));

    // Check if we're waiting for this tile in the first place
    const QString tile_key = QString("%1_%2_%3_%4")
        .arg(m_MapType,
            QString::number(mcZoomLevel),
            QString::number(mcX),
            QString::number(mcY));
    if (!m_TilesNeeded.contains(tile_key))
    {
        // Nope.
        CALL_OUT("");
        return;
    }
    
    // Check if we need this tile
    if (mcMapType != m_MapType ||
        mcZoomLevel != m_ZoomLevel)
    {
        CALL_OUT("");
        return;
    }

    // Area covered by the tile (map coordinates)
    const int tile_x0 = mcX;
    const int tile_x1 = mcX + 500;
    const int tile_y0 = mcY;
    const int tile_y1 = mcY - 500;
    
    // Map region (map coordinates)
    const int map_x0 = m_Left;
    const int map_x1 = m_Left + m_Width;
    const int map_y0 = m_Top;
    const int map_y1 = m_Top - m_Height;
    
    // Region of intersection (map coordinates)
    const int int_x0 = qMax(tile_x0, map_x0);
    const int int_x1 = qMin(tile_x1, map_x1);
    const int int_y0 = qMin(tile_y0, map_y0);
    const int int_y1 = qMax(tile_y1, map_y1);
    const int int_width = int_x1 - int_x0;
    const int int_height = int_y0 - int_y1;
    
    // Check for intersection
    if (int_x0 > int_x1 ||
        int_y0 < int_y1)
    {
        // No intersection
        CALL_OUT("");
        return;
    }

    // Source region (Image coordinates)
    const int source_left = int_x0 - tile_x0;
    const int source_top = tile_y0 - int_y0;
    
    // Destination region (Image coordinates)
    const int dest_left = int_x0 - map_x0;
    const int dest_top = map_y0 - int_y0;
    
    // Copy to Image
    QPainter image_painter(&m_Image);
    image_painter.drawPixmap(dest_left, dest_top, mcPixmap,
        source_left, source_top, int_width, int_height);
    
    // Image was updated
    if (!mcFromCache)
    {
        emit MapImageChanged();
    }
    
    // Check for done
    if (!mcIsScaled)
    {
        m_TilesNeeded.remove(tile_key);
        if (m_TilesNeeded.isEmpty() &&
            m_AllTilesRequested)
        {
            m_IsFinal = true;
            emit MapImageFinal();
        }
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Check if map is final
bool Map::IsFinal() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_IsFinal;
}



// =========================================================== Helper Functions



///////////////////////////////////////////////////////////////////////////////
// Min zoom level
int Map::GetMinZoomLevel(const QString mcMapType)
{
    CALL_IN(QString("mcMapType=%1")
        .arg(CALL_SHOW(mcMapType)));

    // Check for known map types
    if (mcMapType == "terrain" ||
        mcMapType == "roadmap" ||
        mcMapType == "satellite")
    {
        CALL_OUT("");
        return 1;
    }
    
    // Unknown
    const QString reason = tr("Unknown map type \"%1\". Can't evaluate.")
        .arg(mcMapType);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return -1;
}



///////////////////////////////////////////////////////////////////////////////
// Max zoom level
int Map::GetMaxZoomLevel(const QString mcMapType)
{
    CALL_IN(QString("mcMapType=%1")
        .arg(CALL_SHOW(mcMapType)));

    // Check for known map types
    if (mcMapType == "terrain")
    {
        CALL_OUT("");
        return 15;
    }
    if (mcMapType == "roadmap" ||
        mcMapType == "satellite")
    {
        CALL_OUT("");
        return 20;
    }
    
    // Unknown
    const QString reason = tr("Unknown map type \"%1\". Can't evaluate.")
        .arg(mcMapType);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return -1;
}



///////////////////////////////////////////////////////////////////////////////
// Convert coordinates to pixel counts
QPair < int, int > Map::ConvertLatLongToPixel(
    const int mcZoomLevel, const double mcLongitude,
    const double mcLatitude)
{
    CALL_IN(QString("mcZoomLevel=%1, mcLongitude=%2, mcLatitude=%3")
        .arg(CALL_SHOW(mcZoomLevel),
             CALL_SHOW(mcLongitude),
             CALL_SHOW(mcLatitude)));

    // No checks on parameters - that should be done elsewhere
    
    // Convert (longitude, latitude) to (meters_x, meters_y)
    const double meters_x = mcLongitude / 360. * (2. * M_PI * EARTH_RADIUS_M);
    const double meters_y =
        log(tan((90. + mcLatitude) * M_PI / 360.)) * EARTH_RADIUS_M;
    
    // Convert (meters_x, meters_y) to (pixel_x, pixel_y)
    const double scale = pow(2., mcZoomLevel - 10);
    const double pixels_per_meter =
        728. * 360. / (2. * M_PI * EARTH_RADIUS_M) * scale;
    const double pixel_x = meters_x * pixels_per_meter;
    const double pixel_y = meters_y * pixels_per_meter;

    CALL_OUT("");
    return QPair < int, int >(pixel_x, pixel_y);
}



///////////////////////////////////////////////////////////////////////////////
// Max coordinates (min coordinates are the same, just negative)
QPair < int, int > Map::GetMaxCoordinates(const int mcZoomLevel)
{
    CALL_IN(QString("mcZoomLevel=%1")
        .arg(CALL_SHOW(mcZoomLevel)));

    // Latitude in this projection goes only up to 85 degrees.
    const QPair < int, int > max_coordinates =
        ConvertLatLongToPixel(mcZoomLevel, 180, 85);
    CALL_OUT("");
    return max_coordinates;
}



///////////////////////////////////////////////////////////////////////////////
// Convert coordinates to pixel counts
QPair < double, double > Map::ConvertPixelToLatLong(
    const int mcZoomLevel, const int mcX, const int mcY)
{
    CALL_IN(QString("mcZoomLevel=%1, mcX=%2, mcY=%3")
        .arg(CALL_SHOW(mcZoomLevel),
             CALL_SHOW(mcX),
             CALL_SHOW(mcY)));

    // No checks on parameters - that should be done elsewhere

    // Convert (pixel_x, pixel_y) to (meters_x, meters_y)
    const double scale = pow(2., mcZoomLevel - 10);
    const double pixels_per_meter =
        728. * 360. / (2. * M_PI * EARTH_RADIUS_M) * scale;
    const double meters_x = mcX / pixels_per_meter;
    const double meters_y = mcY / pixels_per_meter;
    
    // Convert (meters_x, meters_y) to (longitude, latitude)
    const double longitude = meters_x * 360. / (2. * M_PI * EARTH_RADIUS_M);
    const double latitude =
        atan(exp(meters_y / EARTH_RADIUS_M)) * 360. / M_PI - 90.;

    CALL_OUT("");
    return QPair < double, double >(longitude, latitude);
}



///////////////////////////////////////////////////////////////////////////////
// Get a reasonable scale in meters
QPair < int, int > Map::MapScaleInMeters(const int mcZoomLevel, 
    const double mcLatitude)
{
    CALL_IN(QString("mcZoomLevel=%1, mcLatitude=%2")
        .arg(CALL_SHOW(mcZoomLevel),
             CALL_SHOW(mcLatitude)));

    // The goal is to find something to too terribly far away from 100 pixels
    // that has some "even" distance (10km, 500m, 200m, ...)

    // We assume zoom level is okay
    // (cannot check anyway; we're missing the map type) 

    // Check latitude
    if (mcLatitude < -90 || mcLatitude > 90)
    {
        const QString reason =
            tr("Invalid latitude. Should be between -90 and 90 (inclusive). "
                "Fatal.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < int, int >(0, 0);
    }
    
    // Convert latitude to pixels
    const QPair < int, int > pixels_100 =
        ConvertLatLongToPixel(mcZoomLevel, 0, mcLatitude);
    
    // Convert 100 pixels to degrees
    const QPair < double, double > deg_100 =
        ConvertPixelToLatLong(mcZoomLevel, 100, pixels_100.second);
    
    // Get distance covered by 100 pixels
    const double f =
        2 * M_PI * EARTH_RADIUS_M * cos(mcLatitude * 2 * M_PI / 360.);
    const double dist_100 = fabs(deg_100.first / 360) * f;
    
    // Candidates for "even" distances
    const double log_dist = log10(dist_100);
    const double log_dist2 = log10(dist_100*2.);
    const double log_dist5 = log10(dist_100*5.);
    const double delta = fabs(log_dist - round(log_dist));
    const double delta2 = fabs(log_dist2 - round(log_dist2));
    const double delta5 = fabs(log_dist5 - round(log_dist5));
    double scale_dist = 0;
    if (delta == qMin(delta, qMin(delta2, delta5)))
    {
        scale_dist = pow(10., round(log_dist));
    } else if (delta2 == qMin(delta, qMin(delta2, delta5)))
    {
        scale_dist = pow(10., round(log_dist2)) / 2.;
    } else
    {
        scale_dist = pow(10., round(log_dist5)) / 5.;
    }
    
    // Convert scale distance back to degrees
    const double scale_deg = scale_dist / f * 360.;
    const QPair < int, int > scale_pixels =
        ConvertLatLongToPixel(mcZoomLevel, scale_deg, mcLatitude);
    
    // Done
    CALL_OUT("");
    return QPair < int, int >(int(scale_dist), scale_pixels.first);
}



///////////////////////////////////////////////////////////////////////////////
// Get a reasonable scale in miles
QPair < int, int > Map::MapScaleInMiles(const int mcZoomLevel,
    const double mcLatitude)
{
    CALL_IN(QString("mcZoomlevel=%1, mcLatitude=%2")
        .arg(CALL_SHOW(mcZoomLevel),
             CALL_SHOW(mcLatitude)));

    // The goal is to find something to too terribly far away from 100 pixels
    // that has some "even" distance (10km, 500m, 200m, ...)
    
    // We're accepting the zoom level
    // (can't check anyway since we're missing the map type)

    // Check latitude
    if (mcLatitude < -90 || mcLatitude > 90)
    {
        const QString reason =
            tr("Invalid latitude. Should be between -90 and 90 (inclusive). "
               "Fatal.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < int, int >(0, 0);
    }
    
    // Convert latitude to pixels
    const QPair < int, int > pixels_100 =
        ConvertLatLongToPixel(mcZoomLevel, 0, mcLatitude);
    
    // Convert 100 pixels to degrees
    const QPair < double, double > deg_100 =
        ConvertPixelToLatLong(mcZoomLevel, 100, pixels_100.second);
    
    // Get distance covered by 100 pixels, converted to feet
    const double f =
        2 * M_PI * EARTH_RADIUS_M * cos(mcLatitude * 2 * M_PI / 360.);
    double dist_100 = fabs(deg_100.first / 360) * f * M_IN_FT;
    
    // For less than 5000ft, use feet, otherwise use miles
    const double conversion_factor = (dist_100 < 5000 ? 1 : MILE_IN_FT);
    dist_100 /= conversion_factor;
        
    // Candidates for "even" distances
    const double log_dist = log10(dist_100);
    const double log_dist2 = log10(dist_100*2.);
    const double log_dist5 = log10(dist_100*5.);
    const double delta = fabs(log_dist - round(log_dist));
    const double delta2 = fabs(log_dist2 - round(log_dist2));
    const double delta5 = fabs(log_dist5 - round(log_dist5));
    double scale_dist = 0;
    if (delta == qMin(delta, qMin(delta2, delta5)))
    {
        scale_dist = pow(10., round(log_dist));
    } else if (delta2 == qMin(delta, qMin(delta2, delta5)))
    {
        scale_dist = pow(10., round(log_dist2)) / 2.;
    } else
    {
        scale_dist = pow(10., round(log_dist5)) / 5.;
    }
    
    // Convert scale distance back to degrees
    const double scale_deg =
        scale_dist / M_IN_FT * conversion_factor / f * 360.;
    const QPair < int, int > scale_pixels =
        ConvertLatLongToPixel(mcZoomLevel, scale_deg, mcLatitude);

    // Done
    CALL_OUT("");
    return QPair < int, int >(int(scale_dist * conversion_factor),
        scale_pixels.first);
}



///////////////////////////////////////////////////////////////////////////////
// Earth radius (in meters)
const double Map::EARTH_RADIUS_M = 6378137.;



///////////////////////////////////////////////////////////////////////////////
// mile in ft
const double Map::MILE_IN_FT = 5280.0;



///////////////////////////////////////////////////////////////////////////////
// m in ft
const double Map::M_IN_FT = 3.2808399;



///////////////////////////////////////////////////////////////////////////////
// Convert longitude String to double
double Map::ConvertLongitudeToDouble(const QString mcLongitude)
{
    CALL_IN(QString("mcLongitude=%1")
        .arg(CALL_SHOW(mcLongitude)));

    // Return value
    double ret = NAN;
    
    // Check for format
    
    // Like "-73.135"
    static const QRegularExpression numeric_only(
        "^([+\\-]?)([0-9]+(\\.[0-9]+)?)$");
    QRegularExpressionMatch numeric_match = numeric_only.match(mcLongitude);
    if (numeric_match.hasMatch())
    {
        const QString sign = numeric_match.captured(1);
        const QString value = numeric_match.captured(2);
        ret = value.toDouble();
        if (ret > 180.0)
        {
            // Out of range
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        CALL_OUT("");
        return (sign == "-" ? -ret:ret);
    }
    
    // Like "W 24.42"
    static const QRegularExpression hemisphere_form(
        "^([EW]) +([0-9]+(\\.[0-9]+)?)$");
    QRegularExpressionMatch hemisphere_match =
        hemisphere_form.match(mcLongitude);
    if (hemisphere_match.hasMatch())
    {
        const QString hemisphere = hemisphere_match.captured(1);
        const QString value = hemisphere_match.captured(2);
        ret = value.toDouble();
        if (ret > 180.0)
        {
            // Out of range
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        CALL_OUT("");
        return (hemisphere == tr("W") ? -ret:ret);
    }
    
    // Like "E 12d 13.102'"
    static const QRegularExpression long_minutes_only(
        QString("^([EW]) +([0-9]+)%1 +([0-9]+(\\.[0-9]+)?)'")
            .arg(QChar(0xb0)));
    QRegularExpressionMatch long_minutes_match =
        long_minutes_only.match(mcLongitude);
    if (long_minutes_match.hasMatch())
    {
        const QString hemisphere = long_minutes_match.captured(1);
        const double degrees = long_minutes_match.captured(2).toDouble();
        const double minutes = long_minutes_match.captured(3).toDouble();
        if (degrees > 180.0 ||
            minutes >= 60.0)
        {
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        ret = degrees + minutes/60.0;
        if (ret > 180.0)
        {
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        CALL_OUT("");
        return (hemisphere == tr("W") ? -ret:ret);
    }
    
    // Like "E 12d 13' 23.102""
    static const QRegularExpression long_form(
        QString("^([EW]) +([0-9]+)%1 +([0-9]+)' +([0-9]+(\\.[0-9]+)?)\\\"$")
            .arg(QChar(0xb0)));
    QRegularExpressionMatch long_match = long_form.match(mcLongitude);
    if (long_match.hasMatch())
    {
        const QString hemisphere = long_match.captured(1);
        const double degrees = long_match.captured(2).toDouble();
        const double minutes = long_match.captured(3).toDouble();
        const double seconds = long_match.captured(4).toDouble();
        if (degrees > 180.0 ||
            minutes >= 60.0 ||
            seconds >= 60.0)
        {
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        ret = degrees + minutes/60.0 + seconds/3600.0;
        if (ret > 180.0)
        {
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }

        CALL_OUT("");
        return (hemisphere == tr("W") ? -ret:ret);
    }
    
    // Invalid.
    const QString reason = tr("Invalid");
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return NAN;
}



///////////////////////////////////////////////////////////////////////////////
// Convert latitude String to double
double Map::ConvertLatitudeToDouble(const QString mcLatitude)
{
    CALL_IN(QString("mcLatitude=%1")
        .arg(CALL_SHOW(mcLatitude)));

    // Return value
    double ret = NAN;
    
    // Check for format
    
    // Like "-73.135"
    static const QRegularExpression numeric_only(
        "^([+\\-]?)([0-9]+(\\.[0-9]+)?)$");
    QRegularExpressionMatch numeric_match = numeric_only.match(mcLatitude);
    if (numeric_match.hasMatch())
    {
        const QString sign = numeric_match.captured(1);
        const QString value = numeric_match.captured(2);
        ret = value.toDouble();
        if (ret > 90.0)
        {
            // Out of range
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        CALL_OUT("");
        return (sign == "-" ? -ret:ret);
    }
    
    // Like "N 24.42"
    static const QRegularExpression hemisphere_form(
        "^([NS]) +([0-9]+(\\.[0-9]+)?)$");
    QRegularExpressionMatch hemisphere_match =
        hemisphere_form.match(mcLatitude);
    if (hemisphere_match.hasMatch())
    {
        const QString hemisphere = hemisphere_match.captured(1);
        const QString value = hemisphere_match.captured(2);
        ret = value.toDouble();
        if (ret > 90.0)
        {
            // Out of range
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        CALL_OUT("");
        return (hemisphere == "S" ? -ret:ret);
    }
    
    // Like "N 12d 13.102'"
    static const QRegularExpression long_minutes_only(
        QString("^([NS]) +([0-9]+)%1 +([0-9]+(\\.[0-9]+)?)'")
            .arg(QChar(0xb0)));
    QRegularExpressionMatch long_minutes_match =
        long_minutes_only.match(mcLatitude);
    if (long_minutes_match.hasMatch())
    {
        const QString hemisphere = long_minutes_match.captured(1);
        const double degrees = long_minutes_match.captured(2).toDouble();
        const double minutes = long_minutes_match.captured(3).toDouble();
        if (degrees > 90.0 ||
            minutes >= 60.0)
        {
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        ret = degrees + minutes/60.0;
        if (ret > 90.0)
        {
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        CALL_OUT("");
        return (hemisphere == "S" ? -ret:ret);
    }

    // Like "N 12d 13' 23.102""
    static const QRegularExpression long_form(
        QString("^([NS]) +([0-9]+)%1 +([0-9]+)' +([0-9]+(\\.[0-9]+)?)\\\"$")
            .arg(QChar(0xb0)));
    QRegularExpressionMatch long_match = long_form.match(mcLatitude);
    if (long_match.hasMatch())
    {
        const QString hemisphere = long_match.captured(1);
        const double degrees = long_match.captured(2).toDouble();
        const double minutes = long_match.captured(3).toDouble();
        const double seconds = long_match.captured(4).toDouble();
        if (degrees > 90.0 ||
            minutes >= 60.0 ||
            seconds >= 60.0)
        {
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        ret = degrees + minutes/60.0 + seconds/3600.0;
        if (ret > 90.0)
        {
            const QString reason = tr("Out of range");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return NAN;
        }
        CALL_OUT("");
        return (hemisphere == "S" ? -ret:ret);
    }
    
    // Invalid.
    const QString reason = tr("Invalid");
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return NAN;
}



///////////////////////////////////////////////////////////////////////////////
// Convert double to longitude String
QString Map::ConvertDoubleToLongitude(const double mcLongitude)
{
    CALL_IN(QString("mcLongitude=%1")
        .arg(CALL_SHOW(mcLongitude)));

    // Check for validity
    if (!(mcLongitude == mcLongitude) ||
        fabs(mcLongitude) > 180.0)
    {
        const QString reason = tr("Out of range");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // Parts
    const QString hemisphere = (mcLongitude > 0 ? tr("E") : tr("W"));
    double rest = fabs(mcLongitude);
    int degrees = int(rest);
    rest = (rest - degrees) * 60;
    int minutes = int(rest);
    double seconds = (rest - minutes) * 60;

    // Some weird roundoff stuff
    if (round(seconds * 1000) == 60 * 1000)
    {
        minutes++;
        seconds = 0.0;
    }
    if (minutes == 60)
    {
        degrees++;
        minutes = 0;
    }
    
    // Return value
    CALL_OUT("");
    return QString("%1 %2%3 %4' %5\"")
        .arg(hemisphere)
        .arg(degrees, 3, 10, QChar('0'))
        .arg(QChar(0xb0))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 6, 'f', 3, QChar('0'));
}



///////////////////////////////////////////////////////////////////////////////
// Convert double to latitude String
QString Map::ConvertDoubleToLatitude(const double mcLatitude)
{
    CALL_IN(QString("mcLatitude=%1")
        .arg(CALL_SHOW(mcLatitude)));

    // Check for validity
    if (!(mcLatitude == mcLatitude) ||
        fabs(mcLatitude) > 90.0)
    {
        const QString reason = tr("Out of range");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // Parts
    const QString hemisphere = (mcLatitude > 0 ? "N" : "S");
    double rest = fabs(mcLatitude);
    int degrees = int(rest);
    rest = (rest - degrees) * 60;
    int minutes = int(rest);
    double seconds = (rest - minutes) * 60;
    
    // Some weird roundoff stuff
    if (round(seconds * 1000) == 60 * 1000)
    {
        minutes++;
        seconds = 0.0;
    }
    if (minutes == 60)
    {
        degrees++;
        minutes = 0;
    }

    // Return value
    CALL_OUT("");
    return QString("%1 %2%3 %4' %5\"")
        .arg(hemisphere)
        .arg(degrees, 2, 10, QChar('0'))
        .arg(QChar(0xb0))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 6, 'f', 3, QChar('0'));
}



///////////////////////////////////////////////////////////////////////////////
// Distance between two points
double Map::CalculateDistanceInMeters(const double mcFirstLongitude,
    const double mcFirstLatitude, const double mcSecondLongitude,
    const double mcSecondLatitude)
{
    CALL_IN(QString("mcFirstLongitude=%1, mcFirstLatitude=%2, "
        "mcSecondLongitude=%3, mcSecondLatitude=%4")
        .arg(CALL_SHOW(mcFirstLongitude),
             CALL_SHOW(mcFirstLatitude),
             CALL_SHOW(mcSecondLongitude),
             CALL_SHOW(mcSecondLatitude)));

    // Convert to radians
    const double long_1 = mcFirstLongitude / 180. * M_PI;
    const double long_2 = mcSecondLongitude / 180. * M_PI;
    const double lat_1 = mcFirstLatitude / 180. * M_PI;
    const double lat_2 = mcSecondLatitude / 180. * M_PI;

    // This uses Haversine formula, see e.g.
    // http://en.wikipedia.org/wiki/Haversine_formula
    const double delta_lat = lat_2 - lat_1;
    const double delta_long = long_2 - long_1;
    const double sin_delta_lat = sin(delta_lat/2.);
    const double sin_delta_long = sin(delta_long/2.);
    const double a = sin_delta_lat * sin_delta_lat +
        cos(lat_1) * cos(lat_2) * sin_delta_long * sin_delta_long;

    CALL_OUT("");
    return 2. * EARTH_RADIUS_M * asin(sqrt(a));
}



///////////////////////////////////////////////////////////////////////////////
// Direction of second point from first point
double Map::Bearing(
    const double mcFirstLongitude, const double mcFirstLatitude,
    const double mcSecondLongitude, const double mcSecondLatitude)
{
    CALL_IN(QString("mcFirstLongitude=%1, mcFirstLatitude=%2, "
        "mcSecondLongitude=%3, mcSecondLatitude=%4")
        .arg(CALL_SHOW(mcFirstLongitude),
             CALL_SHOW(mcFirstLatitude),
             CALL_SHOW(mcSecondLongitude),
             CALL_SHOW(mcSecondLatitude)));

    // Convert to radians
    const double long_1 = mcFirstLongitude / 180. * M_PI;
    const double long_2 = mcSecondLongitude / 180. * M_PI;
    const double lat_1 = mcFirstLatitude / 180. * M_PI;
    const double lat_2 = mcSecondLatitude / 180. * M_PI;

    // Bearing formula from GuteFrage.net:
    // https://www.gutefrage.net/frage/
    //      richtung-zu-einer-bestimmen-gps-koordinate-bestimmen
    double zeta = acos(sin(lat_1) * sin(lat_2) + cos(lat_1) * cos(lat_2) *
        cos(long_2 - long_1));
    double alpha = acos((sin(lat_2) - sin(lat_1) * cos(zeta)) /
        (cos(lat_1) * sin(zeta)));

    CALL_OUT("");
    return alpha / M_PI * 180.;
}
