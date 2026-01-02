// Geocode.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "Geocode.h"
#include "Map.h"
#include "MessageLogger.h"
#include "secrets/GoogleSecrets.h"

// Qt includes
#include <QDebug>
#include <QDomElement>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Private constructor - called throug Instance()
Geocode::Geocode()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
Geocode::~Geocode()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do.

    CALL_OUT("");
}


	
// ===================================================================== Access


///////////////////////////////////////////////////////////////////////////////
// Convert geoinformation type to human readable form
QString Geocode::ToHumanReadable(
    const Geocode::GeoInformationType mcInformation)
{
    CALL_IN(QString("mcInformation=%1")
        .arg("..."));

    // Set up mapper
    static QHash < GeoInformationType, QString > information_mapper;
    if (information_mapper.isEmpty())
    {
        information_mapper[Info_FormattedAddress] = "formatted address";
        information_mapper[Info_Locality] = "locality";
        information_mapper[Info_AdminLevel1] = "admin level 1";
        information_mapper[Info_AdminLevel2] = "admin level 2";
        information_mapper[Info_AdminLevel3] = "admin level 3";
        information_mapper[Info_PostalCode] = "postal code";
        information_mapper[Info_Country] = "country";
        information_mapper[Info_Longitude] = "longitude";
        information_mapper[Info_PrettyLongitude] = "longitude (pretty)";
        information_mapper[Info_Latitude] = "latitude";
        information_mapper[Info_PrettyLatitude] = "latitude (pretty)";
    }

    if (information_mapper.contains(mcInformation))
    {
        CALL_OUT("");
        return information_mapper[mcInformation];
    } else
    {
        const QString reason = tr("Unknown information type.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    // We never get here
}

#if 0
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
#endif



///////////////////////////////////////////////////////////////////////////////
// Get address for coordinates
QHash < Geocode::GeoInformationType, QString > Geocode::GetGeoInformation(
    const double mcLongitude, const double mcLatitude)
{
    CALL_IN(QString("mcLongitude=%1, mcLatitude=%2")
        .arg(CALL_SHOW(mcLongitude),
             CALL_SHOW(mcLatitude)));

    // Create request
    const QString request_text = QString("https://maps.googleapis.com/maps/"
        "api/geocode/xml?"
        "latlng=%1,%2&"
        "key=%3")
        .arg(mcLatitude)
        .arg(mcLongitude)
        .arg(GOOGLE_API_KEY);

    // Obtain information
    QNetworkAccessManager network_manager;
    QEventLoop event_loop;
    connect (&network_manager, &QNetworkAccessManager::finished,
        &event_loop, &QEventLoop::quit);
    QNetworkReply * reply = network_manager.get(QNetworkRequest(request_text));
    event_loop.exec();

    // Check if error occurred
    if (reply -> error() != QNetworkReply::NoError)
    {
        delete reply;
        const QString reason =
            tr("An error occurred while downloading geo information image "
                "from \"%1\".")
                .arg(request_text);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < GeoInformationType, QString >();
    }

    // No error
    QString xml(reply -> readAll());
    delete reply;

    // Decode reply

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
    QHash < GeoInformationType, QString > geo;

    // Parse XML DOM
    QDomDocument doc;
    const QDomDocument::ParseResult result = doc.setContent(xml);
    if (!result)
    {
        const QString reason = tr("Error parsing XML response: \"%1\". "
            "This should be a rare problem.")
            .arg(result.errorMessage);
        MessageLogger::Error(CALL_METHOD, reason);
        return QHash < GeoInformationType, QString >();
    }

    QDomElement dom_root = doc.documentElement();
    if (dom_root.tagName() != "GeocodeResponse")
    {
        const QString reason = tr("Root tag is not <GeocodeResponse>.");
        MessageLogger::Error(CALL_METHOD, reason);
        return QHash < GeoInformationType, QString >();
    }

    // Check status
    QDomElement status = dom_root.firstChildElement("status");
    if (status.isNull())
    {
        const QString reason =
            tr("Root tag <GeocodeResponse> is missing a <status> tag.");
        MessageLogger::Error(CALL_METHOD, reason);
        return QHash < GeoInformationType, QString >();
    }
    const QString status_text = status.text();
    if (status_text != "OK")
    {
        QDomElement dom_error_message =
            dom_root.firstChildElement("error_message");
        if (dom_error_message.isNull())
        {
            const QString reason =
                tr("Root tag <GeocodeResponse> is missing a <error_message> "
                    "tag even though there was an error.");
            MessageLogger::Error(CALL_METHOD, reason);
            return QHash < GeoInformationType, QString >();
        }
        const QString error_message_text = dom_error_message.text();

        const QString reason = tr("Request failed with status \"%1\": %2.")
            .arg(status_text,
                error_message_text);
        MessageLogger::Error(CALL_METHOD, reason);
        return QHash < GeoInformationType, QString >();
    }

    // Loop results
    for (QDomElement dom_result = dom_root.firstChildElement("result");
        !dom_result.isNull();
        dom_result = dom_result.nextSiblingElement("result"))
    {
        // We ignore <type>

        // Formatted address
        QDomElement dom_formatted_address =
            dom_result.firstChildElement("formatted_address");
        if (dom_formatted_address.isNull())
        {
            const QString reason = tr("No <formatted_address> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }
        geo[Info_FormattedAddress] = dom_formatted_address.text();

        // Loop address components
        for (QDomElement component =
                dom_result.firstChildElement("address_component");
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
                    geo[Info_Locality] = long_name_text;
                    continue;
                }
                if (type.text() == "administrative_area_level_1")
                {
                    geo[Info_AdminLevel1] = long_name_text;
                    continue;
                }
                if (type.text() == "administrative_area_level_2")
                {
                    geo[Info_AdminLevel2] = long_name_text;
                    continue;
                }
                if (type.text() == "administrative_area_level_3")
                {
                    geo[Info_AdminLevel3] = long_name_text;
                    continue;
                }
                if (type.text() == "postal_code")
                {
                    geo[Info_PostalCode] = long_name_text;
                    continue;
                }
                if (type.text() == "country")
                {
                    geo[Info_Country] = long_name_text;
                    continue;
                }

                // Ignore remaining types
            }
        }

        // Geometry
        QDomElement dom_geometry = dom_result.firstChildElement("geometry");
        if (dom_geometry.isNull())
        {
            const QString reason = tr("No <geometry> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }

        // Location
        QDomElement dom_location = dom_geometry.firstChildElement("location");
        if (dom_location.isNull())
        {
            const QString reason = tr("No <location> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }

        // Latitude
        QDomElement dom_lat = dom_location.firstChildElement("lat");
        if (dom_lat.isNull())
        {
            const QString reason = tr("No <lat> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }
        const double latitude = dom_lat.text().toDouble();
        geo[Info_Latitude] = QString::number(latitude, 'f', 10);
        geo[Info_PrettyLatitude] = Map::ConvertDoubleToLatitude(latitude);

        // Longitude
        QDomElement dom_lng = dom_location.firstChildElement("lng");
        if (dom_lng.isNull())
        {
            const QString reason = tr("No <lng> tag.");
            MessageLogger::Error(CALL_METHOD, reason);
            continue;
        }
        const double longitude = dom_lng.text().toDouble();
        geo[Info_Longitude] = QString::number(longitude, 'f', 10);
        geo[Info_PrettyLongitude] = Map::ConvertDoubleToLongitude(longitude);

        // We ignore <location_type>
        // We ignore <viewport>
        // We ignore <bounds>

        // Next result

        // !!We stick with the first result
        break;
    }

    CALL_OUT("");
    return geo;
}



