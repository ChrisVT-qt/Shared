// MapCache.h
// Class definition file

// Avoid duplicate includes
#ifndef MAPCACHE_H
#define MAPCACHE_H

// Qt includes
#include <QHash>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPair>
#include <QPixmap>
#include <QSet>
#include <QString>

// Class definition
class MapCache :
    public QObject
{
    Q_OBJECT
    
    
    
    // ============================================================== Lifecycle
private:
    // Constructor
    MapCache();

public:
    // Destructor
    virtual ~MapCache();

    // Get singleton instance
    static MapCache * Instance();
    
private:
    // Instance
    static MapCache * m_Instance;

    // Initialize Preferences
    static bool InitPreferences();
    static const bool m_PreferencesInitialized;
    
    
    
    // ================================================================= Access
public:
    // Read cached files
    void InitCache();

private:
    // Hashes map type, level, x coordinate, y coordiate to tile.
    QHash < QString, QHash < int, QHash < int, QHash < int, QPixmap > > > >
        m_MapCache;

    // Check ram cache size
    void CheckRamCacheSize();
    
    // Cache usage
    QList < QString > m_UsageMapTypes;
    QList < int > m_UsageLevel;
    QList < int > m_UsageX;
    QList < int > m_UsageY;
    
public:
    // Get map tile
    void ObtainMapTile(const int mcZoomLevel, const QString mcMapType,
        const int mcLeft, const int mcTop);
    
private:
    QNetworkAccessManager * m_DownloadManager;
    QSet < QString > m_BeingObtained;
    
    // User-defined attributes
    static const QNetworkRequest::Attribute REQUEST_MAPTYPE;
    static const QNetworkRequest::Attribute REQUEST_ZOOM_LEVEL;
    static const QNetworkRequest::Attribute REQUEST_X;
    static const QNetworkRequest::Attribute REQUEST_Y;
    static const QNetworkRequest::Attribute REQUEST_OFFSET_X;
    static const QNetworkRequest::Attribute REQUEST_OFFSET_Y;
    static const QNetworkRequest::Attribute REQUEST_WIDTH;
    static const QNetworkRequest::Attribute REQUEST_HEIGHT;

private slots:
    // Tile obtained
    void DownloadedCompleted(QNetworkReply * mpDownloadReply);    
    
signals:
    // Let everybody know that a MapTime has been obtained
    void MapTileObtained(const QString mcMapType, const int mcZoomLevel,
        const int mcX, const int mcY, const QPixmap mcPixmap,
        const bool mcFromCache, const bool mcIsScaled);
    
public:
    // Delete MapTile from disk
    bool DeleteMapTile(const int mcZoomLevel, const QString mcMapType,
        const int mcLeft, const int mcTop);
    
    // Cache size
    static qint64 GetCacheSize(const QString mcMapType = QString(),
        const int mcZoomLevel = -1);

    // Map statistics
    static QString MapStatistics();
private:
    static int m_Stats_NumRequests;
    static int m_Stats_NumCacheHits;
    static int m_Stats_NumRetrieve;
};

#endif
