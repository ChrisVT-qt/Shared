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
	// Destructor
	virtual ~Geocode();
	
    
	
	// ================================================================= Access
public:
    enum GeoInformationType {
        Info_FormattedAddress,
        Info_Locality,
        Info_AdminLevel1,
        Info_AdminLevel2,
        Info_AdminLevel3,
        Info_PostalCode,
        Info_Country,
        Info_Longitude,
        Info_PrettyLongitude,
        Info_Latitude,
        Info_PrettyLatitude
    };
    static QString ToHumanReadable(const GeoInformationType mcInformation);

#if 0
    // Search address
    static QHash < GeoInformationType, QString > SearchAddress(
        const QString mcAddress);
#endif

    // Get information about a location
    static QHash < GeoInformationType, QString > GetGeoInformation(
        const double mcLongitude, const double mcLatitude);
};

#endif

