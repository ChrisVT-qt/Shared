// Geocode.h
// Class definition file

// Just include once
#ifndef GEOCODE_H
#define GEOCODE_H

// Qt includes
#include <QNetworkAccessManager>
#include <QObject>

// Class definition
class Geocode :
    public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:
	// Private constructor - called throug Instance()
	Geocode();

public:
    // Instance
    static Geocode * Instance();
private:
    static Geocode * m_Instance;

public:
	// Destructor
	virtual ~Geocode();
	
	// Invalid search ID
	static const int INVALID_ID;

private:
    QNetworkAccessManager * m_DownloadManager;
	
    
	
	// ================================================================= Access
public:
    // Search
	int SearchAddress(const QString mcAddress);
	
	// Get address for coordinates
	int SearchCoordinates(const double mcLongitude, const double mcLatitude);
	
	// Coordinates
	QPair < double, double > GetCoordinates(const int mcId);
	
	// Parts of the location
	QString GetFormattedAddress(const int mcId);
	QString GetLocality(const int mcId);
	QString GetAdminLevel2(const int mcId);
	QString GetAdminLevel1(const int mcId);
	QString GetCountry(const int mcId);

	// Remove search results from cache
	void DeleteSearch(const int mcSearchId);
	
private slots:
    // Net request completed
    void RequestCompleted(QNetworkReply * mpReply);
    
    // SSL errors
    void SSLErrorOccurred(QNetworkReply * mpReply,
        const QList < QSslError > mcErrors);
    
private:
    // Parsing XML response
    void ParseXml(const QString mcXml, int mcSearchId);

    // Search ID
    int m_NextSearchId;
    QHash < int, QList < int > > m_SearchIdToResultIds;
    
    // Cache
    int m_NextResultId;
	QHash < int, QString > m_ResultIdToFormattedAddress;
	QHash < int, QString > m_ResultIdToLocality;
	QHash < int, QString > m_ResultIdToPostalCode;
	QHash < int, QString > m_ResultIdToAdminLevel3;
	QHash < int, QString > m_ResultIdToAdminLevel2;
	QHash < int, QString > m_ResultIdToAdminLevel1;
	QHash < int, QString > m_ResultIdToCountry;
	QHash < int, double > m_ResultIdToLongitude;
	QHash < int, double > m_ResultIdToLatitude;
	
signals:
    // Search completed
    void SearchCompleted(const int mcSearchId, 
        const QList < int > mcResultIds);
};

#endif

