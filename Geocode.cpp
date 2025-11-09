// Geocode.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "Geocode.h"
#include "secrets/GoogleSecrets.h"
#include "MessageLogger.h"
#include "Preferences.h"

// Qt includes
#include <QDebug>
#include <QDomElement>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>

// System include
#include <cmath>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Private constructor - called throug Instance()
Geocode::Geocode()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Initialize IDs
    m_NextSearchId = 0;
    m_NextResultId = 0;
    
    // Net access
    m_DownloadManager = new QNetworkAccessManager(this);
	connect (m_DownloadManager, SIGNAL(finished(QNetworkReply *)),
	    this, SLOT(RequestCompleted(QNetworkReply *)));
	connect (m_DownloadManager,
	    SIGNAL(sslErrors(QNetworkReply *, const QList < QSslError >)),
	    this, 
	    SLOT(SSLErrorOccurred(QNetworkReply *, const QList < QSslError >)));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Instance
Geocode * Geocode::Instance()
{
    CALL_IN("");

    if (!m_Instance)
    {
        m_Instance = new Geocode();
    }
    
    CALL_OUT("");
    return m_Instance;
}



///////////////////////////////////////////////////////////////////////////////
// Instance
Geocode * Geocode::m_Instance = nullptr;



///////////////////////////////////////////////////////////////////////////////
// Destructor
Geocode::~Geocode()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do.

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Invalid search ID
const int Geocode::INVALID_ID = -1;


	
// ===================================================================== Access



///////////////////////////////////////////////////////////////////////////////
// Search
int Geocode::SearchAddress(const QString mcAddress)
{
    CALL_IN(QString("mcAddress=%1")
        .arg(CALL_SHOW(mcAddress)));

    // Search ID
    int this_search_id = m_NextSearchId;
    m_NextSearchId++;

    // Encode address
    QString address = mcAddress;
    address.replace("+", "%2B");
    address.replace(" ", "+");
    address.replace("&", "%26");
    
    // Create request
    const QString request_text = QString("https://maps.googleapis.com/maps/"
        "api/geocode/xml?"
        "address=%1&"
        "key=%2")
        .arg(address,
             GOOGLE_API_KEY);
    
    // Read from URL
    QNetworkRequest request;
    request.setUrl(QUrl(request_text));
    request.setAttribute(QNetworkRequest::User, this_search_id);
    m_DownloadManager -> get(request);
    
    // Done
    CALL_OUT("");
    return this_search_id;
}



///////////////////////////////////////////////////////////////////////////////
// Get address for coordinates
int Geocode::SearchCoordinates(const double mcLongitude,
    const double mcLatitude)
{
    CALL_IN(QString("mcLongitude=%1, mcLatitude=%2")
        .arg(CALL_SHOW(mcLongitude),
             CALL_SHOW(mcLatitude)));

    // Search ID
    int this_search_id = m_NextSearchId;
    m_NextSearchId++;

    // Create request
    const QString request_text = QString("https://maps.googleapis.com/maps/"
        "api/geocode/xml?"
        "latlng=%1,%2&"
        "key=%3")
        .arg(mcLatitude)
        .arg(mcLongitude)
        .arg(GOOGLE_API_KEY);
    
    // Read from URL
    QNetworkRequest request;
    request.setUrl(QUrl(request_text));
    request.setAttribute(QNetworkRequest::User, this_search_id);
    m_DownloadManager -> get(request);
    
    // Done
    CALL_OUT("");
    return this_search_id;
}



///////////////////////////////////////////////////////////////////////////////
// Coordinates
QPair < double, double > Geocode::GetCoordinates(const int mcId)
{
    CALL_IN(QString("mcId=%1")
        .arg(CALL_SHOW(mcId)));

    if (!m_ResultIdToLongitude.contains(mcId) ||
        !m_ResultIdToLatitude.contains(mcId))
    {
        // No or incomplete data
        const QString reason = tr("No coordinates for results ID %1.")
            .arg(QString::number(mcId));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPair < double, double >(NAN, NAN);
    }
    
    // Successful
    CALL_OUT("");
    return QPair < double, double >(m_ResultIdToLongitude[mcId],
        m_ResultIdToLatitude[mcId]);
}



///////////////////////////////////////////////////////////////////////////////
// Formatted address
QString Geocode::GetFormattedAddress(const int mcId)
{
    CALL_IN(QString("mcId=%1")
        .arg(CALL_SHOW(mcId)));

    if (!m_ResultIdToFormattedAddress.contains(mcId))
    {
        CALL_OUT("");
        return QString();
    }
    
    // Successful
    CALL_OUT("");
    return m_ResultIdToFormattedAddress[mcId];
}



///////////////////////////////////////////////////////////////////////////////
// Locality
QString Geocode::GetLocality(const int mcId)
{
    CALL_IN(QString("mcId=%1")
        .arg(CALL_SHOW(mcId)));

    if (!m_ResultIdToLocality.contains(mcId))
    {
        CALL_OUT("");
        return QString();
    }
    
    // Successful
    CALL_OUT("");
    return m_ResultIdToLocality[mcId];
}



///////////////////////////////////////////////////////////////////////////////
// Administrative Level 2
QString Geocode::GetAdminLevel2(const int mcId)
{
    CALL_IN(QString("mcId=%1")
        .arg(CALL_SHOW(mcId)));

    if (!m_ResultIdToAdminLevel2.contains(mcId))
    {
        CALL_OUT("");
        return QString();
    }
    
    // Successful
    CALL_OUT("");
    return m_ResultIdToAdminLevel2[mcId];
}



///////////////////////////////////////////////////////////////////////////////
// Administrative Level 1
QString Geocode::GetAdminLevel1(const int mcId)
{
    CALL_IN(QString("mcId=%1")
        .arg(CALL_SHOW(mcId)));

    if (!m_ResultIdToAdminLevel1.contains(mcId))
    {
        CALL_OUT("");
        return QString();
    }
    
    // Successful
    CALL_OUT("");
    return m_ResultIdToAdminLevel1[mcId];
}



///////////////////////////////////////////////////////////////////////////////
// Country
QString Geocode::GetCountry(const int mcId)
{
    CALL_IN(QString("mcId=%1")
        .arg(CALL_SHOW(mcId)));

    if (!m_ResultIdToCountry.contains(mcId))
    {
        CALL_OUT("");
        return QString();
    }
    
    // Successful
    CALL_OUT("");
    return m_ResultIdToCountry[mcId];
}



///////////////////////////////////////////////////////////////////////////////
// Remove search results from cache
void Geocode::DeleteSearch(const int mcSearchId)
{
    CALL_IN(QString("mcSearchId=%1")
        .arg(CALL_SHOW(mcSearchId)));

    if (!m_SearchIdToResultIds.contains(mcSearchId))
    {
        // No data
        const QString reason = tr("Unknown search ID %1.")
            .arg(QString::number(mcSearchId));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }
    
    // Delete results
    for (const int result_id : m_SearchIdToResultIds[mcSearchId])
    {
        m_ResultIdToFormattedAddress.remove(result_id);
        m_ResultIdToLocality.remove(result_id);
        m_ResultIdToPostalCode.remove(result_id);
        m_ResultIdToAdminLevel3.remove(result_id);
        m_ResultIdToAdminLevel2.remove(result_id);
        m_ResultIdToAdminLevel1.remove(result_id);
        m_ResultIdToCountry.remove(result_id);
        m_ResultIdToLongitude.remove(result_id);
        m_ResultIdToLatitude.remove(result_id);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Net request completed
void Geocode::RequestCompleted(QNetworkReply * mpReply)
{
    // !!! Not tracking CALL_IN/CALL_OUT; CallTracer isn't thread-safe.

    // Get content
    QNetworkRequest request = mpReply -> request();
    const int search_id = request.attribute(QNetworkRequest::User).toInt();
    
    // Get data
    QByteArray data = mpReply -> readAll();
    mpReply -> deleteLater();
    
    // Parse
    ParseXml(QString(data), search_id);
}



///////////////////////////////////////////////////////////////////////////////
// SSL errors
void Geocode::SSLErrorOccurred(QNetworkReply * mpReply,
    const QList < QSslError > mcErrors)
{
    // !!! Not tracking CALL_IN/CALL_OUT; CallTracer isn't thread-safe.

    Q_UNUSED(mcErrors);
    
    // Ignore - We know what we're doing here,
    mpReply -> ignoreSslErrors();
}



///////////////////////////////////////////////////////////////////////////////
// Parsing XML response
void Geocode::ParseXml(const QString mcXml, const int mcSearchId)
{
    // !!! Not tracking CALL_IN/CALL_OUT; CallTracer isn't thread-safe.

    // XML schema explained at
    // https://developers.google.com/maps/documentation/geocoding/
    
    // <GeocodeResponse>
    //   <status>OK</status>
    //   <result>
    //     <type>locality</type>
    //     <type>political</type>
    //     <formatted_address>Lexington, KY, USA</formatted_address>
    //     <address_component>
    //       <long_name>Lexington</long_name>
    //       <short_name>Lexington</short_name>
    //       <type>locality</type>
    //       <type>political</type>
    //     </address_component>
    //     <address_component>
    //       ...
    //     </address_component>
    //     <geometry>
    //       <location>
    //         <lat>38.0405837</lat>
    //         <lng>-84.5037164</lng>
    //       </location>
    //       <location_type>APPROXIMATE</location_type>
    //       <viewport>
    //         <southwest>
    //           <lat>37.8452559</lat>
    //           <lng>-84.6604151</lng>
    //         </southwest>
    //         <northeast>
    //           <lat>38.2114040</lat>
    //           <lng>-84.2827150</lng>
    //         </northeast>
    //       </viewport>
    //       <bounds>
    //         <southwest>
    //           <lat>37.8452559</lat>
    //           <lng>-84.6604151</lng>
    //         </southwest>
    //         <northeast>
    //           <lat>38.2114040</lat>
    //           <lng>-84.2827150</lng>
    //         </northeast>
    //       </bounds>
    //     </geometry>
    //   </result>
    //   <result>
    //     ...
    //   </result>
    // </GeocodeResponse>
    
    // or
    
    // <GeocodeResponse>
    //   <status>REQUEST_DENIED</status>
    //   <error_message>The provided API key is invalid.</error_message>
    // </GeocodeResponse>


    // Initialize results
    m_SearchIdToResultIds[mcSearchId] = QList < int >();
    
    // Parse XML DOM
    QDomDocument xml;
    const QDomDocument::ParseResult result = xml.setContent(mcXml);
    if (!result)
    {
        const QString reason =
            tr("Error parsing XML response: \"%1\". This should be a rare problem.")
                .arg(result.errorMessage);
        MessageLogger::Error(CALL_METHOD, reason);
        emit SearchCompleted(mcSearchId, QList < int >());
        return;
    }
    
    QDomElement root = xml.documentElement();
    if (root.tagName() != "GeocodeResponse")
    {
        const QString reason = tr("Root tag is not <GeocodeResponse>.");
        MessageLogger::Error(CALL_METHOD, reason);
        emit SearchCompleted(mcSearchId, QList < int >());
        return;
    }

    // Check status
    QDomElement status = root.firstChildElement("status");
    if (status.isNull())
    {
        const QString reason =
            tr("Root tag <GeocodeResponse> is missing a <status> tag.");
        MessageLogger::Error(CALL_METHOD, reason);
        emit SearchCompleted(mcSearchId, QList < int >());
        return;
    }
    const QString status_text = status.text();
    if (status_text != "OK")
    {
        QDomElement error_message = root.firstChildElement("error_message");
        if (error_message.isNull())
        {
            const QString reason =
                tr("Root tag <GeocodeResponse> is missing a <error_message> "
                    "tag even though there was an error.");
            MessageLogger::Error(CALL_METHOD, reason);
            emit SearchCompleted(mcSearchId, QList < int >());
            return;
        }
        const QString error_message_text = error_message.text();
        
        const QString reason = tr("Request failed with status \"%1\": %2.")
            .arg(status_text,
                error_message_text);
        MessageLogger::Error(CALL_METHOD, reason);
        emit SearchCompleted(mcSearchId, QList < int >());
        return;
    }
    
    // Loop results
    for (QDomElement result = root.firstChildElement("result");
        !result.isNull();
        result = result.nextSiblingElement("result"))
    {
        // Result ID
        const int result_id = m_NextResultId;
        m_NextResultId++;
        
        // We ignore <type>
        
        // Formatted address
        QDomElement formatted_address = 
            result.firstChildElement("formatted_address");
        if (formatted_address.isNull())
        {
            const QString reason = tr("No <formatted_address> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }
        m_ResultIdToFormattedAddress[result_id] = formatted_address.text();

        // Loop address components
        for (QDomElement component =
                result.firstChildElement("address_component");
            !component.isNull();
            component =
                component.nextSiblingElement("address_component"))
        {
            // Long name
            QDomElement long_name = 
                component.firstChildElement("long_name");
            if (long_name.isNull())
            {
                const QString reason = tr("No <long_name> tag.");
                MessageLogger::Error(CALL_METHOD, reason);
                continue;
            }
            const QString long_name_text = long_name.text();

            // Type
            for (QDomElement type = component.firstChildElement("type");
                !type.isNull();
                type = type.nextSiblingElement("type"))
            {
                if (type.text() == "political")
                {
                    // Ignored
                    continue;
                }
                if (type.text() == "locality")
                {
                    m_ResultIdToLocality[result_id] = long_name_text;
                    continue;
                }
                if (type.text() == "administrative_area_level_1")
                {
                    m_ResultIdToAdminLevel1[result_id] = long_name_text;
                    continue;
                }
                if (type.text() == "administrative_area_level_2")
                {
                    m_ResultIdToAdminLevel2[result_id] = long_name_text;
                    continue;
                }
                if (type.text() == "administrative_area_level_3")
                {
                    m_ResultIdToAdminLevel3[result_id] = long_name_text;
                    continue;
                }
                if (type.text() == "postal_code")
                {
                    m_ResultIdToPostalCode[result_id] = long_name_text;
                    continue;
                }
                if (type.text() == "country")
                {
                    m_ResultIdToCountry[result_id] = long_name_text;
                    continue;
                }
                
                // Ignore remaining types
            }
        }
        
        // Geometry
        QDomElement geometry = result.firstChildElement("geometry");
        if (geometry.isNull())
        {
            const QString reason = tr("No <geometry> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }
        
        // Location
        QDomElement location = geometry.firstChildElement("location");
        if (location.isNull())
        {
            const QString reason = tr("No <location> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }
        
        // Latitude
        QDomElement lat = location.firstChildElement("lat");
        if (lat.isNull())
        {
            const QString reason = tr("No <lat> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }
        m_ResultIdToLatitude[result_id] = lat.text().toDouble();
        
        // Longitude
        QDomElement lng = location.firstChildElement("lng");
        if (lng.isNull())
        {
            const QString reason = tr("No <lng> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }
        m_ResultIdToLongitude[result_id] = lng.text().toDouble();

        // We ignore <location_type>
        // We ignore <viewport>
        // We ignore <bounds>

        m_SearchIdToResultIds[mcSearchId] << result_id;
        
        // Next result
    }

    // Done.
    emit SearchCompleted(mcSearchId, m_SearchIdToResultIds[mcSearchId]);
}
