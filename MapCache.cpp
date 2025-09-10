// MapCache.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "secrets/GoogleSecrets.h"
#include "MessageLogger.h"
#include "Map.h"
#include "MapCache.h"
#include "Preferences.h"

// Qt includes
#include <QDebug>
#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QStringList>
#include <QTimer>

// System includes
#include <cmath>

// Deploy
#include "Deploy.h"



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
MapCache::MapCache()
{
    CALL_IN("");

    // Initialize net access
    m_DownloadManager = new QNetworkAccessManager(this);
	connect (m_DownloadManager, SIGNAL(finished(QNetworkReply *)),
	    this, SLOT(DownloadedCompleted(QNetworkReply *)));

    // Initialize cached files
    InitCache();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
MapCache::~MapCache()
{
    CALL_IN("");

    // Delete NetworkAccessManager
    delete m_DownloadManager;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get singleton instance
MapCache * MapCache::Instance()
{
    CALL_IN("");

    // Check if we already have an instance
    if (!m_Instance)
    {
        // No. Create one.
        m_Instance = new MapCache();
    }
    
    // Return instance
    CALL_OUT("");
    return m_Instance;
}



///////////////////////////////////////////////////////////////////////////////
// Instance
MapCache * MapCache::m_Instance = nullptr;



// !!! Paw: Replace MapCache_CacheDirectory with Map:Storage:CacheDirectory
// !!! Paw: Replace MapCache_GoogleAPIKey with Map:Authentication:Google:APIKey



///////////////////////////////////////////////////////////////////////////////
// Initialize Preferences
bool MapCache::InitPreferences()
{
    CALL_IN("");

    // Preferences tag
    Preferences * p = Preferences::Instance();
    
    // Set values
    p -> AddValidTag("Map:Storage:CacheDirectory");
    p -> SetDefaultValue("Map:Storage:CacheDirectory",
        QDir::homePath() + "/Documents/eDiary");

    // Done
    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Initialize prefs
const bool MapCache::m_PreferencesInitialized = MapCache::InitPreferences();



// ===================================================================== Access



///////////////////////////////////////////////////////////////////////////////
// Read cached files
void MapCache::InitCache()
{
    CALL_IN("");

    // Get cache directory
    const QString cache_dir =
        Preferences::Instance() -> GetValue("Map:Storage:CacheDirectory");

    // Make sure this directory exists
#ifdef Q_OS_MACOS
    QDir root_dir("/");
    root_dir.mkpath(cache_dir);
    CALL_OUT("");
#else
    const QString reason =
        tr("Directory not specified for platforms other than Mac OS.");
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
#endif
}



///////////////////////////////////////////////////////////////////////////////
// Shrink Cache
void MapCache::CheckRamCacheSize()
{
    CALL_IN("");

    // Cache limits
    const int cache_lower_bound = 100;
    const int cache_upper_bound = 200;
    
    // Check cache size
    if (m_UsageMapTypes.size() <= cache_upper_bound)
    {
        // Cache is small enough.
        CALL_OUT("");
        return;
    }
    
    // Remove tiles until it's small enough.
    while (m_UsageMapTypes.size() > cache_lower_bound)
    {
        // Get oldest MapTile
        const QString map_type = m_UsageMapTypes.takeFirst();
        const int level = m_UsageLevel.takeFirst();
        const int x = m_UsageX.takeFirst();
        const int y = m_UsageY.takeFirst();
        if (!m_MapCache[map_type][level][x].contains(y))
        {
            // Has already been deleted.
            continue;
        }
        
        // Remove Pixmap
        m_MapCache[map_type][level][x].remove(y);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get map tile
void MapCache::ObtainMapTile(const int mcZoomLevel, const QString mcMapType,
    const int mcLeft, const int mcTop)
{
    CALL_IN(QString("mcZoomLevel=%1, mcMapType=%2, mcLeft=%3, mcTop=%4")
        .arg(CALL_SHOW(mcZoomLevel),
             CALL_SHOW(mcMapType),
             CALL_SHOW(mcLeft),
             CALL_SHOW(mcTop)));

    // Check if map type is valid
    if (mcMapType != "terrain" &&
        mcMapType != "roadmap" &&
        mcMapType != "satellite")
    {
        // Unknown map type
        const QString reason = tr("Invalid map type \"%1\". Fatal.")
            .arg(mcMapType);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }

    // Check if zoom level is valid
    if (mcZoomLevel < Map::GetMinZoomLevel(mcMapType) ||
        mcZoomLevel > Map::GetMaxZoomLevel(mcMapType))
    {
        const QString reason =
            tr("Invalid zoom level %1. Valid zoom levels are %2 to %3. Fatal.")
                .arg(QString::number(mcZoomLevel),
                     QString::number(Map::GetMinZoomLevel(mcMapType)),
                     QString::number(Map::GetMaxZoomLevel(mcMapType)));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }
    
    // Check if coordinates are valid
    const QPair < int, int > max_coordinates =
        Map::GetMaxCoordinates(mcZoomLevel);
    if (mcLeft < -max_coordinates.first ||
        mcLeft > max_coordinates.first)
    {
        const QString reason =
            tr("Invalid left pixel coordinate %1 for zoom level %2. "
                "Needs to be between -%3 and %3. Fatal.")
                .arg(QString::number(mcLeft),
                     QString::number(mcZoomLevel),
                     QString::number(max_coordinates.first));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }
    if (mcTop < -max_coordinates.second ||
        mcTop > max_coordinates.second)
    {
        const QString reason =
            tr("Invalid top pixel coordinate %1 for zoom level %2. "
                "Needs to be between -%3 and %3. Fatal.")
                .arg(QString::number(mcTop),
                     QString::number(mcZoomLevel),
                     QString::number(max_coordinates.second));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }

    // Counts as a valid request
    m_Stats_NumRequests++;
    
    // === Check if we have the tile already
    if (m_MapCache.contains(mcMapType) &&
        m_MapCache[mcMapType].contains(mcZoomLevel) &&
        m_MapCache[mcMapType][mcZoomLevel].contains(mcLeft) &&
        m_MapCache[mcMapType][mcZoomLevel][mcLeft].contains(mcTop))
    {
        // Tile is in memory already
        const QPixmap tile = m_MapCache[mcMapType][mcZoomLevel][mcLeft][mcTop];
        emit MapTileObtained(mcMapType, mcZoomLevel, mcLeft, mcTop, tile, 
            true, false);

        // Count cache hit
        m_Stats_NumCacheHits++;

        CALL_OUT("");
        return;
    }

    // === Don't have it cached in RAM. Check if it's on disk already
    const QString cache_dir =
        Preferences::Instance() -> GetValue("Map:Storage:CacheDirectory");
    const QString filename = QString("%1/%2/%3/%4_%5.png")
        .arg(cache_dir,
             mcMapType,
             QString::number(mcZoomLevel),
             QString::number(mcLeft),
             QString::number(mcTop));
    if (QFile::exists(filename))
    {
        // Load tile
        const QPixmap tile(filename);
        
        // Put in cache
        m_MapCache[mcMapType][mcZoomLevel][mcLeft][mcTop] = tile;
        
        // Done
        emit MapTileObtained(mcMapType, mcZoomLevel, mcLeft, mcTop, tile, 
            true, false);

        // Still a cache hit
        m_Stats_NumCacheHits++;

        CALL_OUT("");
        return;
    }
    
    // Tile needs to be obtained from Google.
    m_Stats_NumRetrieve++;

    // === Check if we can scale a lower resolution (temporary picture)
    
    // Top has "+500" because tile orientation is bottom to top 
    // (just like the mathematical orientation) and pixels are top to
    // bottom, so the "top" of the tile is the "bottom" of the pixels.
    const int scaled_left = (mcLeft - (abs(mcLeft) % 1000)) / 2;
    const int scaled_top = ((mcTop + 500) - (abs(mcTop + 500) % 1000)) / 2; 
    if (mcZoomLevel > Map::GetMinZoomLevel(mcMapType) &&
        m_MapCache.contains(mcMapType) &&
        m_MapCache[mcMapType].contains(mcZoomLevel - 1) &&
        m_MapCache[mcMapType][mcZoomLevel-1].contains(scaled_left) &&
        m_MapCache[mcMapType][mcZoomLevel-1][scaled_left].contains(
            scaled_top))
    {
        // Get lower resolution tile
        const QImage original_tile =
            m_MapCache[mcMapType][mcZoomLevel-1][scaled_left][scaled_top]
                .toImage();
        
        // Get the part of the up-scaled lower resolution tile that we need
        const int dx = abs(mcLeft) % 1000;
        const int dy = abs(mcTop) % 1000;
        const QImage original_scaled = original_tile.scaled(
            original_tile.width()*2, original_tile.height()*2);
        const QPixmap temporary_tile =
            QPixmap::fromImage(original_scaled).copy(dx, dy, 500, 500);
        
        // Scaled version isn't cached.
        emit MapTileObtained(mcMapType, mcZoomLevel, mcLeft, mcTop,
            temporary_tile, true, true);
        
        // Keep going, actual (full resolution) tile needs to be obtained.
    }
    
    // === Fetch tile
    
    // Check if the tile is already being fetched
    const QString marker = QString("%1_%2_%3_%4")
        .arg(mcMapType,
             QString::number(mcZoomLevel),
             QString::number(mcLeft),
             QString::number(mcTop));
    if (m_BeingObtained.contains(marker))
    {
        // Yup. Nothing to do.
        CALL_OUT("");
        return;
    }
    
    // Get it from Google
    m_BeingObtained << marker;
    
    // Check for partial tiles (edges & corners)
    const QPair < int, int > max_coords = Map::GetMaxCoordinates(mcZoomLevel);
    const int max_x = max_coords.first;
    const int min_x = -max_coords.first;
    const int max_y = max_coords.second;
    const int min_y = -max_coords.second;
    int offset_x = 0;
    int offset_y = 0;
    int width = 500;
    int height = 500;
    if (mcLeft > max_x - 500)
    {
        offset_x = mcLeft - (max_x - 500);
        width = 500 - offset_x;
    } else if (mcLeft < min_x)
    {
        offset_x = mcLeft - min_x;
        width = 500 + offset_x;
    }
    if (mcTop > max_y)
    {
        offset_y = mcTop - max_y;
        height = 500 - offset_y;
    } else if (mcTop < min_y + 500)
    {
        offset_y = mcTop - (min_y + 500);
        height = 500 + offset_y;
    }
    
    // Construct URL
    // (Google lat/long is based on the center of the image)
    const int effective_x = mcLeft - offset_x + 320;
    const int effective_y = mcTop - offset_y - 320;
    const QPair < double, double > coordinates = 
        Map::ConvertPixelToLatLong(mcZoomLevel, effective_x, effective_y);
    const double latitude = coordinates.second;
    const double longitude = coordinates.first;
    const QString url_text =
        QString("http://maps.google.com/maps/api/staticmap?"
            "center=%1,%2&"
            "zoom=%3&"
            "size=640x640&"
            "maptype=%4&"
            "sensor=false&"
            "key=%5")
                .arg(latitude, 0, 'f', 10)
                .arg(longitude, 0, 'f', 10)
                .arg(QString::number(mcZoomLevel),
                     mcMapType,
                     GOOGLE_API_KEY);

    QNetworkRequest request;
    request.setAttribute(REQUEST_MAPTYPE, mcMapType);
    request.setAttribute(REQUEST_ZOOM_LEVEL, mcZoomLevel);
    request.setAttribute(REQUEST_X, mcLeft);
    request.setAttribute(REQUEST_Y, mcTop);
    request.setAttribute(REQUEST_OFFSET_X, offset_x);
    request.setAttribute(REQUEST_OFFSET_Y, offset_y);
    request.setAttribute(REQUEST_WIDTH, width);
    request.setAttribute(REQUEST_HEIGHT, height);
    request.setUrl(QUrl(url_text));
    m_DownloadManager -> get(request);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// User-defined attributes
const QNetworkRequest::Attribute MapCache::REQUEST_MAPTYPE = 
    QNetworkRequest::Attribute(QNetworkRequest::User);
const QNetworkRequest::Attribute MapCache::REQUEST_ZOOM_LEVEL = 
    QNetworkRequest::Attribute(QNetworkRequest::User + 1);
const QNetworkRequest::Attribute MapCache::REQUEST_X = 
    QNetworkRequest::Attribute(QNetworkRequest::User + 2);
const QNetworkRequest::Attribute MapCache::REQUEST_Y = 
    QNetworkRequest::Attribute(QNetworkRequest::User + 3);
const QNetworkRequest::Attribute MapCache::REQUEST_OFFSET_X = 
    QNetworkRequest::Attribute(QNetworkRequest::User + 4);
const QNetworkRequest::Attribute MapCache::REQUEST_OFFSET_Y = 
    QNetworkRequest::Attribute(QNetworkRequest::User + 5);
const QNetworkRequest::Attribute MapCache::REQUEST_WIDTH = 
    QNetworkRequest::Attribute(QNetworkRequest::User + 6);
const QNetworkRequest::Attribute MapCache::REQUEST_HEIGHT = 
    QNetworkRequest::Attribute(QNetworkRequest::User + 7);

    

///////////////////////////////////////////////////////////////////////////////
// Tile obtained
void MapCache::DownloadedCompleted(QNetworkReply * mpDownloadReply)
{
    // !!! No CALL_IN/CALL_OUT since this runs in a different thread,
    // !!! and call tracing isn't thread safe yet.

    // Get data
    QNetworkRequest request = mpDownloadReply -> request();
    const QString map_type = request.attribute(REQUEST_MAPTYPE).toString();
    const int zoom_level = request.attribute(REQUEST_ZOOM_LEVEL).toInt();
    const int left = request.attribute(REQUEST_X).toInt();
    const int top = request.attribute(REQUEST_Y).toInt();
    const int offset_x = request.attribute(REQUEST_OFFSET_X).toInt();
    const int offset_y = request.attribute(REQUEST_OFFSET_Y).toInt();
    const int width = request.attribute(REQUEST_WIDTH).toInt();
    const int height = request.attribute(REQUEST_HEIGHT).toInt();
    
    // Remove from list of tiles being obtained
    const QString marker = QString("%1_%2_%3_%4")
        .arg(map_type,
             QString::number(zoom_level),
             QString::number(left),
             QString::number(top));
    m_BeingObtained.remove(marker);
    
    // Check if activity was successful
    if (mpDownloadReply -> error() != QNetworkReply::NoError)
    {
        // Nope. Do nothing (we could report an error).
        return;
    }
    
    // Get data
    QPixmap tmp_pixmap;
    QByteArray data = mpDownloadReply -> readAll();
    mpDownloadReply -> deleteLater();
    const bool success = tmp_pixmap.loadFromData(data);
    if (!success)
    {
        // Didn't work for some reason. Do nothing.
        const QString reason = tr("Pixmap could not be decoded from data.");
        MessageLogger::Error(CALL_METHOD, reason);
        return;
    }
    
    // Check if Google limitation has been reached
    // (did change on Sep 20, 2013)

    // Check that worked BEFORE Sep 20, 2013
    if (tmp_pixmap.width() == 100 && tmp_pixmap.height() == 100)
    {
        const QPixmap tile(":/resources/Maps/GoogleQuota.png");
        emit MapTileObtained(map_type, zoom_level, left, top, tile, false,
            false);
        return;
    }

    // Check that works AFTER Sep 20, 2013
    if (tmp_pixmap.width() == 362 && tmp_pixmap.height() == 122)
    {
        const QPixmap tile(":/resources/Maps/GoogleQuota.png");
        emit MapTileObtained(map_type, zoom_level, left, top, tile, false,
            false);
        return;
    }
    
    // Save part of the tile
    QImage target_tile(500, 500, QImage::Format_RGB32);
    target_tile.fill(0);
    QPainter image_painter(&target_tile);
    image_painter.drawPixmap(
        qMax(-offset_x, 0), qMax(offset_y, 0),
        tmp_pixmap,
        qMax(offset_x, 0), qMax(-offset_y, 0),
        width, height);

    // Save to cache
    const QString cache_dir =
        Preferences::Instance() -> GetValue("Map:Storage:CacheDirectory");
    QDir().mkpath(QString("%1/%2/%3")
        .arg(cache_dir,
             map_type,
             QString::number(zoom_level)));
    QString filename = QString("%1/%2/%3/%4_%5.png")
        .arg(cache_dir,
             map_type,
             QString::number(zoom_level),
             QString::number(left),
             QString::number(top));
    target_tile.save(filename, "png");

    // Map time successfully obtained. Store in cache.
    m_MapCache[map_type][zoom_level][left][top] =
        QPixmap::fromImage(target_tile);
        
    // Remember usage
    m_UsageMapTypes << map_type;
    m_UsageLevel << zoom_level;
    m_UsageX << top;
    m_UsageY << left;
        
    // Check ram cache size
    CheckRamCacheSize();
    
    // Internal - no checks
    emit MapTileObtained(map_type, zoom_level, left, top,
        m_MapCache[map_type][zoom_level][left][top], false, false);
}



///////////////////////////////////////////////////////////////////////////////
// Delete MapTile from disk
bool MapCache::DeleteMapTile(const int mcZoomLevel,
    const QString mcMapType, const int mcLeft, const int mcTop)
{
    CALL_IN(QString("mcZoomLevel=%1, mcMapType=%2, mcLeft=%3, mcTop=%4")
        .arg(CALL_SHOW(mcZoomLevel),
             CALL_SHOW(mcMapType),
             CALL_SHOW(mcLeft),
             CALL_SHOW(mcTop)));

    // Check if map type is valid
    if (mcMapType != "terrain" &&
        mcMapType != "roadmap" &&
        mcMapType != "satellite")
    {
        // Unknown map type
        const QString reason = tr("Invalid map type \"%1\". Fatal.")
            .arg(mcMapType);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if zoom level is valid
    if (mcZoomLevel < Map::GetMinZoomLevel(mcMapType) ||
        mcZoomLevel > Map::GetMaxZoomLevel(mcMapType))
    {
        const QString reason =
            tr("Invalid zoom level %1. Valid zoom levels are %2 to %3. Fatal.")
                .arg(QString::number(mcZoomLevel),
                     QString::number(Map::GetMinZoomLevel(mcMapType)),
                     QString::number(Map::GetMaxZoomLevel(mcMapType)));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    
    // Check if coordinates are valid
    const QPair < int, int > max_coordinates =
        Map::GetMaxCoordinates(mcZoomLevel);
    if (mcLeft < -max_coordinates.first ||
        mcLeft > max_coordinates.first)
    {
        const QString reason =
            tr("Invalid left pixel coordinate %1 for zoom level %2. "
                "Needs to be between -%3 and %3. Fatal.")
                .arg(QString::number(mcLeft),
                     QString::number(mcZoomLevel),
                     QString::number(max_coordinates.first));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    if (mcTop < -max_coordinates.second ||
        mcTop > max_coordinates.second)
    {
        const QString reason =
            tr("Invalid top pixel coordinate %1 for zoom level %2. "
                "Needs to be between -%3 and %3. Fatal.")
                .arg(QString::number(mcTop),
                     QString::number(mcZoomLevel),
                     QString::number(max_coordinates.second));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Remove from disk
    const QString cache_dir =
        Preferences::Instance() -> GetValue("Map:Storage:CacheDirectory");
    const QString filename = QString("%1/%2/%3/%4_%5.png")
        .arg(cache_dir,
             mcMapType,
             QString::number(mcZoomLevel),
             QString::number(mcLeft),
             QString::number(mcTop));
    QFile::remove(filename);
    
    // Check if we have a MapTile
    if (m_MapCache.contains(mcMapType) &&
        m_MapCache[mcMapType].contains(mcZoomLevel) &&
        m_MapCache[mcMapType][mcZoomLevel].contains(mcLeft) &&
        m_MapCache[mcMapType][mcZoomLevel][mcLeft].contains(mcTop))
    {
        // Remove from cache
        m_MapCache[mcMapType][mcZoomLevel][mcLeft].remove(mcTop);
    }
    
    // Get it again from Google
    ObtainMapTile(mcZoomLevel, mcMapType, mcLeft, mcTop);
    
    // Success
    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Cache size
qint64 MapCache::GetCacheSize(const QString mcMapType, 
    const int mcZoomLevel)
{
    CALL_IN(QString("mcMapType=%1, mcZoomLevel=%2")
        .arg(CALL_SHOW(mcMapType),
             CALL_SHOW(mcZoomLevel)));

    // Check if maptype is valid
    if (mcMapType != "terrain" &&
        mcMapType != "roadmap" &&
        mcMapType != "satellite" &&
        !mcMapType.isEmpty())
    {
        // Invalid.
        const QString reason = tr("Unknown map type \"%1\". Fatal.")
            .arg(mcMapType);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }
    
    // Check for zoom level
    if (!mcMapType.isEmpty() &&
        mcZoomLevel != -1)
    {
        if (mcZoomLevel < Map::GetMinZoomLevel(mcMapType) ||
            mcZoomLevel > Map::GetMaxZoomLevel(mcMapType))
        {
            // Invalid.
            const QString reason =
                tr("Invalid zoom level %1 for map type \"%2\". Fatal.")
                    .arg(QString::number(mcZoomLevel),
                         mcMapType);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return -1;
        }
    }
    
    // Return value
    qint64 ret = 0;
    
    // Get cache directory
    const QString cache_dir =
        Preferences::Instance() -> GetValue("Map:Storage:CacheDirectory");

    // Terrain
    if (mcMapType == "terrain" ||
        mcMapType.isEmpty())
    {
        const int min_level = (mcZoomLevel == -1 ?
            Map::GetMinZoomLevel("terrain") : mcZoomLevel);
        const int max_level = (mcZoomLevel == -1 ?
            Map::GetMaxZoomLevel("terrain") : mcZoomLevel);
        for (int level = min_level; level <= max_level; level++)
        {
            // Base directory
            QDir zoom_dir(QString("%1/terrain/%2")
                .arg(cache_dir,
                     QString::number(level)));
            
            // Check if this directory exists
            if (!zoom_dir.exists())
            {
                // No. Nothing to do.
                continue;
            }
            
            // Loop files in this directory
            const QList < QFileInfo > all_infos =
                zoom_dir.entryInfoList(QDir::Files);
            for (const QFileInfo & info : all_infos)
            {
                ret += info.size();
            }
        }
    }
    
    // Roadmap
    if (mcMapType == "roadmap" ||
        mcMapType.isEmpty())
    {
        const int min_level = (mcZoomLevel == -1 ?
            Map::GetMinZoomLevel("roadmap") : mcZoomLevel);
        const int max_level = (mcZoomLevel == -1 ?
            Map::GetMaxZoomLevel("roadmap") : mcZoomLevel);
        for (int level = min_level; level <= max_level; level++)
        {
            // Base directory
            QDir zoom_dir(QString("%1/roadmap/%2")
                .arg(cache_dir,
                     QString::number(level)));
            
            // Check if this directory exists
            if (!zoom_dir.exists())
            {
                // No. Nothing to do.
                continue;
            }
            
            // Loop files in this directory
            const QList < QFileInfo > all_infos =
                zoom_dir.entryInfoList(QDir::Files);
            for (const QFileInfo & info : all_infos)
            {
                ret += info.size();
            }
        }
    }
    
    // Satellite
    if (mcMapType == "satellite" ||
        mcMapType.isEmpty())
    {
        const int min_level = (mcZoomLevel == -1 ?
            Map::GetMinZoomLevel("satellite") : mcZoomLevel);
        const int max_level = (mcZoomLevel == -1 ?
            Map::GetMaxZoomLevel("satellite") : mcZoomLevel);
        for (int level = min_level; level <= max_level; level++)
        {
            // Base directory
            QDir zoom_dir(QString("%1/satellite/%2")
                .arg(cache_dir,
                     QString::number(level)));
            
            // Check if this directory exists
            if (!zoom_dir.exists())
            {
                // No. Nothing to do.
                continue;
            }
            
            // Loop files in this directory
            const QList < QFileInfo > all_infos =
                zoom_dir.entryInfoList(QDir::Files);
            for (const QFileInfo & info : all_infos)
            {
                ret += info.size();
            }
        }
    }
    
    // Done
    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Map statistics
QString MapCache::MapStatistics()
{
    CALL_IN("");

    QString ret;
    ret = tr("%1 map elements requested (%2 in cache, %3 new)")
        .arg(QString::number(m_Stats_NumRequests),
             QString::number(m_Stats_NumCacheHits),
             QString::number(m_Stats_NumRetrieve));

    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Initialize statistics
int MapCache::m_Stats_NumRequests = 0;
int MapCache::m_Stats_NumCacheHits = 0;
int MapCache::m_Stats_NumRetrieve = 0;
