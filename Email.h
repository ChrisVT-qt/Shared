// Email.h
// Class definition file

/** \class Email
  * Aims to provide a standard interface to emails
  *
  * Over the years, files containing emails have changed as the standard
  * and user needs evolved. Different email clients encode attachments with
  * varying degrees of details in varied ways, and a plethora of header
  * elements is used to provide context information of an email. There is also
  * a wide range of how to combine email addresses and real names, and date
  * formats. The goal of this class is to provide the ability to import
  * a text file containing one or more emails and to provide a standard
  * interface to access their data. You can also store all information in
  * a standardized XML format.
  *
  * The class has been tested with all of the author's email dating back to
  * the dark ages of the internet in the eary 1990s, so while certainly not
  * all email file formats will be fully supported, there should be quite a
  * decent coverage. Should you encounter an email that cannot be imported,
  * please send it to me as a file attachment to improve this class.
  *
  * Be aware that while there are methods available to parse literally any
  * header element I have encountered so far, the actual parsing of the
  * information in these elements exists only for the ones that I needed
  * access to so far.
  *
  * \todo Perpetual to do item: Keep up with email headers and content types.
  * \todo Implement parsing of remaining header items
  */

// Just include once
#ifndef EMAIL_H
#define EMAIL_H

// Qt includes
#include <QByteArray>
#include <QDomElement>
#include <QHash>
#include <QObject>
#include <QString>

// Forward declarations
class NavigatedTextFile;

// Class definition
class Email :
    public QObject
{
    Q_OBJECT
    
    
    
    // ============================================================== Lifecycle
public:
    /** \brief Constructor from file
      * \details
      * Note that if an error occurs (and the email cannot be successfully
      * implemented), the object will provide you with information about the
      * details of what happened.
      * \param mcFilename Filename of the single email to be imported.
      */
    Email(const QString mcFilename);

private:
    /** \brief Constructor from open file
      * \details
      * This methods makes parsing the email file efficient in the way that
      * it first reads the whole email, then processes it while in memory
      * rather than reading it line by line.
      * \param mcEmailFile Content of the email text file
      * \param mcType type of the file. Can be either "mbox" for traditional
      * mbox files, or "emlx" for Apple's own idea of organizing multiple
      * emails in a single file.
      */
    Email(NavigatedTextFile & mrEmailFile, const QString mcType = QString());

public:
    /** \brief Import multiple emails from an mbox file
      * \details
      * mbox files may contain multiple emails that can be imported in a
      * single pass.
      * \param mcFilename Filename of the mbox file
      */
	static QList < Email * > ImportFromMBox(const QString mcFilename);
	
    /** \brief Import multiple emails from an Apple Mail emlx file
      * \details
      * emlx files may contain multiple emails that can be imported in a
      * single pass.
      * \param mcFilename Filename of the emlx file
      */
    static QList < Email * > ImportFromEMLXFile(const QString mcFilename);
	
    /** \brief Destructor
      */
	~Email();
	
private:
    /** \brief Remembering if file being imported is an mbox file
      */
    bool m_IsMBox;
    /** \brief Remembering if file being imported is an Apple Mail emlx file
      */
    bool m_IsEMLX;
    
    
	
    // ================================================================ Import
private:
    // == Header

    /** \brief Read full header information of an email
      * \param mrEmailFile file with one or multiple emails. This one will
      * read the header of the next email only.
      */
    void ReadHeader(NavigatedTextFile & mrEmailFile);
    
    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_AcceptLanguage(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_AMQDeliveryMessageID(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ApparentlyFrom(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ApparentlyTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ARCAuthenticationResults(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ARCMessageSignature(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ARCSeal(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_AuthenticationResults(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_AuthenticationResultsOriginal(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_AutoSubmitted(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Bcc(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_BouncesTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_CampaignID(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_CampaignToken(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_Cc(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_Comments(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ContentClass(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ContentDescription(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ContentDisposition(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ContentId(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ContentLanguage(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ContentLength(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ContentMD5(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ContentTransferEncoding(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ContentType(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ConversationId(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_Date(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_DeferredDelivery(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_DeliveredTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_DispositionNotificationTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_DKIMFilter(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_DKIMSignature(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_DomainKeySignature(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Encoding(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_EnvelopeTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ErrorsTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_FeedbackID(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_FollowupTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_From(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_IllegalObject(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Importance(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_InReplyTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_Keywords(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_Lines(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ListArchive(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ListHelp(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ListId(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ListOwner(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ListPost(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ListSubscribe(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ListUnsubscribe(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ListUnsubscribePost(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_MailFollowupTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_MailingList(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_MessageId(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_MimeVersion(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_MSIPLabels(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Newsgroups(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_NNTPPostingHost(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_NonStandardTagHeader(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_OldContentType(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_OldSubject(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Organization(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Originator(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_OrigTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_PostedDate(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_PPCorrelationID(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_PPToMDOMigrated(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Precedence(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Priority(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_RCPTDomain(const QString mcBody);

    /** \brief Parse corresponing header item
      * Note that there are usually multiple "Received" header items; all of
      * them are stored.
      * \param mcBody Content of the header item
      */
    void ReadHeader_Received(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ReceivedDate(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ReceivedSPF(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_RecipientID(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_RequireRecipientValidSince(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_References(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ReplyTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ResentCc(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ResentDate(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ResentFrom(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ResentMessageId(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ResentReplyTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ResentSender(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_ResentTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ReturnPath(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ReturnReceiptTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_SavedFromEmail(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_Sender(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Sensitivity(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_SentOn(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_SiteID(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_SpamDiagnosticMetadata(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_SpamDiagnosticOutput(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_Status(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_Subject(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_SuggestedAttachmentSessionID(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ThreadIndex(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_ThreadTopic(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      */
    void ReadHeader_To(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_UIOutboundReport(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_UserAgent(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_WarningsTo(const QString mcBody);

    /** \brief Parse corresponing header item
      * \param mcBody Content of the header item
      * \todo Currently, this header item is not parsed but its raw data are
      * made available.
      */
    void ReadHeader_XMailer(const QString mcBody);
    
    /** \brief Parse a variety of email address formats
      * \details
      * Recognized email formats are:
      * - foo@bar.com
      * - &lt;foo@bar.com&gt;
      * - snafu &lt;foo@bar.com&gt;
      * - "Bar, Foo" &lt;foo@bar.com&gt;
      * - "Foo Bar" &lt;foo@bar.com&gt;
      * - Foo Bar foo@bar.com
      * - foo.bar@foo.com (foo.bar)
      * - foo (Foo Bar) [emails on the local system]
      * - "Bar, Foo" [yup, with no email address]
      * \param mAddress Email address (may include full name)
      * \returns QHash containing one or more of the following, depending on
      * the information contained in the email address provided:\n
      * "email" - just the email address\n
      * "first name" - first name of the person\n
      * "full name" - full name of the person\n
      * "last name" - last name of the person
      */
    QHash < QString, QString > ParseEmailAddress(QString mAddress);

    /** \brief Parse multiple email addresses
      * \param mcAddressList List of email addresses
      * \returns List of email addresses, see \link ParseEmailAddress()\endlink
      * for details.
      */
    QList < QHash < QString, QString > > ParseEmailAddressList(
        const QString mcAddressList);

    /** \brief Parse a date and time
      * \param mcDate textual representation of the date.
      * Note that back in the days, years were sometimes two digits only or
      * ignored altogether. The method does the best it can.
      * \returns QHash containing the following:\n
      * "date" - date as provided in \c mcDate\n
      * "date UTC" - date converted to UTC\n
      * "time" - time as provided in \c mcDate\n
      * "time UTC" - time converted to UTC\n
      * "timezone name" - name of the timezone
      */
    QHash < QString, QString > ParseDate(const QString mcDate);

    /** \brief Decode text from escaped text, base64 etc.
      * \param mcText Encoded text
      * \returns Decoded human-readable text
      * \todo Handling of character encodings is probably no fully correct.
      */
    QString DecodeIfNecessary(const QString mcText) const;
    
    // == Body
    /** \brief Read the entire body of an email.
      * \param mrEmailFile file with one or multiple emails. This one will
      * read the body of the current email only.
      */
    void ReadBody(NavigatedTextFile & mrEmailFile);

    /** \brief Parse the next part of an email
      * Emails are divided up in parts, e.g. plain text email, HTML
      * representation, and one or more attachments. Each of the parts may
      * have a header and may have child parts.
      * \param mrEmailFile file with one or multiple emails.
      * \param mcParentPartHeader Header of the parent part
      * \param mcPartHeader Header of this part
      * \param mcParentId ID of the parent part
      */
    void ReadBody_Part(NavigatedTextFile & mrEmailFile,
        const QHash < QString, QString > mcParentPartHeader,
        const QHash < QString, QString > mcPartHeader,
        const int mcParentId);

    /** \brief Parse the header of the next part
      * \param mrEmailFile file with one or multiple emails.
      * \returns A QHash with the following information:\n
      * "charset" - Character set of the part\n
      * "content-disposition"\n
      * "content-type" - Type of the content of this part\n
      * "creation-date" - Creation date of the attachment\n
      * "filename" - Filename of an attachment\n
      * "modification-date" - Modification date of the attachment\n
      * "name" - Name of the part, often the filename\n
      * "transfer-encoding" - Encoding of the part for transfer by email\n
      * "size" - Size of the attachment\n
      * Any additional urecognized tags are included in the QHash
      */
    QHash < QString, QString > ReadBody_PartHeader(
        NavigatedTextFile & mrEmailFile);

    /** \brief Save content of a part, e.g. an attached file
      * \param mrEmailFile file with one or multiple emails.
      * \param mcParentPartHeader Header of the parent part
      * \param mcPartHeader Header of this part
      * \param mcParentId ID of the parent part
      */
    void ReadBody_SavePart(NavigatedTextFile & mrEmailFile,
        const QHash < QString, QString > mcParentPartHeader,
        const QHash < QString, QString > mcPartHeader, const int mcParentId);

    /** \brief Deal with hierarchical multipart messages
      * \param mrEmailFile file with one or multiple emails.
      * \param mcParentPartHeader Header of the parent part
      * \param mcParentId ID of the parent part
      */
    void ReadBody_Multipart(NavigatedTextFile & mrEmailFile,
        const QHash < QString, QString > mcParentHeader, const int mcParentId);
    
    
    
    // ================================================================= Access
public:
    /** \brief Get filename for this email object
      * \returns The filename
      */
    QString GetFilename() const;
private:
    /** \brief Filename
      */
    QString m_Filename;

public:
    /** \brief Return start line number in this file
      * \returns Start line number
      */
    int GetStartLineNumber() const;
private:
    /** \brief Start line number
      */
    int m_StartLineNumber;

public:
    /** \brief Check if an error occurred during import
      * \returns \c true if there was an error \c false otherwise
      */
    bool HasError() const;

    /** \brief Get the error description that occurred during import
      * \returns Error description
      */
    QString GetError() const;

    /** \brief Get the line in the file where the error occurred
      * \returns Line in the file
      */
    int GetErrorLine() const;
private:
    /** \brief Error text
      */
    QString m_Error;

    /** \brief Error line
      */
    int m_ErrorLine;

public:
    /** \brief Check if email has a certaint header item
      * \param mcHeaderItem Header item bein considered
      * \returns \c true if email mentioned said header item, \c false
      * otherwise
      */
    bool HasHeaderItem(const QString mcHeaderItem) const;

    /** \brief Get a list of all available header items
      * \returns List of available header items
      */
    QList < QString > GetAvailableHeaderItems() const;

    /** \brief Get information to a particular header item.
      * \param mcHeaderItem Name of the header item. Many choices.
      * \returns QHash with header item information. The hash items really
      * depend on the header item and on the question whether or not the
      * information could be parsed into pieces.
      */
    QHash < QString, QString > GetHeaderItem(const QString mcHeaderItem) const;

    /** \brief Check if the email object has a particular item/subitem
      * combination.
      * \param mcHeaderItem Name of the header item. Many choices.
      * \param mcSubItem A hash item for the particular header item.
      * \returns \c true if the header item/sub item combination exists,
      * \c false otherwise.
      */
    bool HasHeaderItem(const QString mcHeaderItem,
        const QString mcSubItem) const;

    /** \brief Get value of a header item/sub item combination
      * \param mcHeaderItem Name of the header item. Many choices.
      * \param mcSubItem A hash item for the particular header item.
      * \returns Value of said header item/sub item combination
      */
    QString GetHeaderItem(const QString mcHeaderItem,
        const QString mcSubItem) const;
private:
    /** \brief All header item data.
      * \details Maps a particular header item to its components, the latter
      * of which are stored in a QHash as well.
      */
    QHash < QString, QHash < QString, QString > > m_HeaderData;
    
public:
    /** \brief Obtain the number of addresses in the "to" header item
      * \returns Number of addresses.
      */
    int GetNumberOfToAddresses() const;

    /** \brief Get information of a particular "to" header item
      * \param mcIndex Index of the header item
      * \returns QHash with email address information; depending on what
      * was available, any of the following items:\n
      * "email" - just the email address\n
      * "first name" - first name of the person\n
      * "full name" - full name of the person\n
      * "last name" - last name of the person
      */
    QHash < QString, QString > GetToAddress(const int mcIndex) const;
private:
    /** \brief Detail information about email addresses in "to" header item
      */
    QList < QHash < QString, QString > > m_HeaderData_To;

public:
    /** \brief Obtain the number of addresses in the "cc" header item
      * \returns Number of addresses.
      */
    int GetNumberOfCcAddresses() const;
    QHash < QString, QString > GetCcAddress(const int mcIndex) const;
private:
    QList < QHash < QString, QString > > m_HeaderData_Cc;

public:
    /** \brief Obtain the number of addresses in the "bcc" header item
      * \returns Number of addressed.
      */
    int GetNumberOfBccAddresses() const;
    QHash < QString, QString > GetBccAddress(const int mcIndex) const;
private:
    QList < QHash < QString, QString > > m_HeaderData_Bcc;

public:
    /** \brief Obtain the number of references
      * \returns Number of references.
      */
    int GetNumberOfReferences() const;
    QHash < QString, QString > GetReference(const int mcIndex) const;
private:
    QList < QHash < QString, QString > > m_HeaderData_References;

public:
    int GetNumberOfReceived() const;
    QHash < QString, QString > GetReceived(const int mcIndex) const;
private:
    QList < QHash < QString, QString > > m_HeaderData_Received;

public:
    int GetNumberOfParts() const;
    QByteArray GetPart(const int mcIndex) const;
    QHash < QString, QString > GetPartInfo(const int mcIndex) const;
    QString GetPartType(const int mcIndex) const;
    int GetPartParentId(const int mcIndex) const;
    QList < int > GetPartChildIds(const int mcIndex) const;
private:
    QList < QByteArray > m_BodyData_Part;
    QList < QHash < QString, QString > > m_BodyData_PartInfo;
    QList < QString > m_BodyData_Type;
    QList < int > m_BodyData_ParentId;
    QHash < int, QList < int > > m_BodyData_ChildIds;
    
    
    
    // ==================================================================== XML
public:
    // Convert to XML
    QString ToXML();
private:
    void ToXML_Header_Bcc(QDomElement & mrDomItem);
    void ToXML_Header_Cc(QDomElement & mrDomItem);
    void ToXML_Header_ContentType(QDomElement & mrDomItem);
    void ToXML_Header_InReplyTo(QDomElement & mrDomItem);
    void ToXML_Header_Lines(QDomElement & mrDomItem);
    void ToXML_Header_MessageId(QDomElement & mrDomItem);
    void ToXML_Header_Received(QDomElement & mrDomItem);
    void ToXML_Header_References(QDomElement & mrDomItem);
    void ToXML_Header_ResentMessageId(QDomElement & mrDomItem);
    void ToXML_Header_Subject(QDomElement & mrDomItem);
    void ToXML_Header_To(QDomElement & mrDomItem);
    void ToXML_Header_Date(QDomElement & mrDomItem,
        const QHash < QString, QString > mcDate);
    void ToXML_Header_Individual(QDomElement & mrDomItem,
        const QHash < QString, QString > mcIndividual);
    
    void ToXML_BodyPart(QDomElement & mrDomBody, const int mcId);


    

    // =================================================================== Dump
public:
    // Show everything
    void Dump();
};

#endif
