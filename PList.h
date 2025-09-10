// PList.h
// Class definition

/** \file
  * \todo Add Doxygen information
  */

#ifndef PLIST_H
#define PLIST_H

// Qt includes
#include <QObject>
#include <QPair>
#include <QString>

// Define class
class PList
    : public QObject
{
    Q_OBJECT
    
    

    // ============================================================== Lifecycle
private:
    // Constructor
    PList();

public:
    // Destructor
    virtual ~PList();
    

    
    // =============================================================== External
public:
    static QString GetXML(const QString mcFilename, const QString mcItem);
private:
    static char * m_PList;
    static int m_PList_Size;
    static QString m_XML;

    
    
    // ================================================== Convenience Functions
public:
    static QString GetItemSource(const QString mcFilename);


    
    // =============================================================== Internal
private:
    static bool CheckHeader();
    static bool CheckVersion();

    static bool Parse_Trailer();

    static bool Parse_Element(int & mrPosition);
    static bool Parse_Singleton(int & mrPosition);
    static bool Parse_Int(int & mrPosition);
    static bool Parse_Real(int & mrPosition);
    static bool Parse_Date(int & mrPosition);
    static bool Parse_Data(int & mrPosition);
    static bool Parse_String_ASCII(int & mrPosition);
    static bool Parse_String_Unicode(int & mrPosition);
    static bool Parse_UID(int & mrPosition);
    static bool Parse_Array(int & mrPosition);
    static bool Parse_Set(int & mrPosition);
    static bool Parse_Dict(int & mrPosition);
    
    static QPair < int, bool > ReadPListInt(int & mrPosition);
    static QPair < int, bool > ReadBigEndianInteger(int & mrPosition,
        const int mcSize);
    static QPair < int, bool > ReadReference(int & mrPosition);
    static QPair < int, bool > ReadOffset(int & mrPosition);

    static int m_OffsetBytes;
    static int m_ReferenceBytes;
    static int m_NumObjects;
    static int m_RootObject;
    static int m_OffsetTableStart;
};

#endif

