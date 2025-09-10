// TaskList - a software organizing everyday tasks
// Copyright (C) 2023 Chris von Toerne
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Contact the author by email: christian.vontoerne@gmail.com

// Email.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "Email.h"
#include "MessageLogger.h"
#include "NavigatedTextFile.h"
#include "StringHelper.h"

// Qt includes
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>

// Debug mode
#define DEBUG false



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor from file
Email::Email(const QString mcFilename)
{
    CALL_IN(QString("mcFilename=%1")
        .arg(CALL_SHOW_FULL(mcFilename)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // No error line
    m_ErrorLine = -1;
    
    // See if file exists
    if (!QFile::exists(mcFilename))
    {
        m_Error = tr("Could not open file \"%1\".").arg(mcFilename);
        CALL_OUT(m_Error);
        return;
    }
    
    // Open file
    NavigatedTextFile file(mcFilename);
    
    // Not an MBox or EMLX file
    m_IsMBox = false;
    m_IsEMLX = false;
    
    // Initialize start line
    m_StartLineNumber = 0;

    // Read header
    ReadHeader(file);
    
    // Keep a potential error because it may get overwritten by another error
    // in the email body
    QString keep_error;
    int keep_error_line = -1;
    if (HasError())
    {
        keep_error = m_Error;
        keep_error_line = m_ErrorLine;
    }
    
    // Read body
    ReadBody(file);
    if (!keep_error.isEmpty())
    {
        m_Error = keep_error;
        m_ErrorLine = keep_error_line;
    }
    
    // Done
    CALL_OUT(m_Error);
}



///////////////////////////////////////////////////////////////////////////////
// Constructor from open file
Email::Email(NavigatedTextFile & mrEmailFile, const QString mcType)
{
    CALL_IN(QString("mrEmailFile=..., mcType=%1")
        .arg(CALL_SHOW(mcType)));

    // No checks - private constructor
    
    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Remember source filename and line number
    m_Filename = mrEmailFile.GetFilename();
    m_StartLineNumber = mrEmailFile.GetCurrentLineNumber();
    
    // Remember if this is an MBox file
    m_IsMBox = (mcType == "mbox");
    m_IsEMLX = (mcType == "emlx");

    // Read header
    ReadHeader(mrEmailFile);

    // Keep a potential error because it may get overwritten by another error
    // in the email body
    QString keep_error;
    int keep_error_line = -1;
    if (HasError())
    {
        keep_error = m_Error;
        keep_error_line = m_ErrorLine;
    }
    
    // Read body
    ReadBody(mrEmailFile);
    if (!keep_error.isEmpty())
    {
        m_Error = keep_error;
        m_ErrorLine = keep_error_line;
    }
    
    // Done
    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// MBoxes may contain multiple emails
QList < Email * > Email::ImportFromMBox(const QString mcFilename)
{
    CALL_IN(QString("mcFilename=%1")
        .arg(CALL_SHOW_FULL(mcFilename)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // See if file exists
    if (!QFile::exists(mcFilename))
    {
        const QString reason =
            tr("Could not open mbox file \"%1\".").arg(mcFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QList < Email * >();
    }
    
    // Open file
    NavigatedTextFile file(mcFilename);
    
    // Debug info
    if (DEBUG)
    {
        qDebug().noquote() << tr("================================== "
            "Importing from %1").arg(mcFilename);
    }
    
    // Read mbox
    QList < Email * > ret;
    while (!file.AtEnd())
    {
        ret << new Email(file, "mbox");
    }
    
    // Done
    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// AppleMail .emlx file
QList < Email * > Email::ImportFromEMLXFile(const QString mcFilename)
{
    CALL_IN(QString("mcFilename=%1")
        .arg(CALL_SHOW_FULL(mcFilename)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // See if file exists
    if (!QFile::exists(mcFilename))
    {
        const QString reason =
            tr("Could not open AppleMail EMLX file \"%1\".").arg(mcFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QList < Email * >();
    }
    
    // Open file
    NavigatedTextFile file(mcFilename);
    
    // Debug info
    if (DEBUG)
    {
        qDebug().noquote() << tr("================================== "
            "Importing from %1").arg(mcFilename);
    }
    
    // Read EMLX file
    QList < Email * > ret;
    while (!file.AtEnd())
    {
        ret << new Email(file, "emlx");
    }
    
    // Done
    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
Email::~Email()
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Nothing to do.

    CALL_OUT("");
}


	
// ===================================================================== Import



///////////////////////////////////////////////////////////////////////////////
// Read email header
void Email::ReadHeader(NavigatedTextFile & mrEmailFile)
{
    CALL_IN("mrEmailFile=...");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (DEBUG)
    {
        qDebug().noquote() << tr("Line %1: Email header start")
            .arg(mrEmailFile.GetCurrentLineNumber());
    }

    static const QRegularExpression format(
        "^([^\\(\\s][^: ]*):(\\s*)?(\\s+(\\S.*))?$");
    QString line = mrEmailFile.ReadLine();
    
    // Skip the first line for EMLX files
    if (m_IsEMLX)
    {
        // Check format, just to be sure
        static const QRegularExpression format_number("^[0-9]+$");
        QRegularExpressionMatch match_number = format_number.match(line);
        if (!match_number.hasMatch())
        {
            m_ErrorLine = -1;
            m_Error = tr("First line should contain a number but is \"%1\".")
                .arg(line);
            CALL_OUT(m_Error);
            return;
        }
        line = mrEmailFile.ReadLine();
    }
    
    // Skip the "From ..." line (can only be the first line)
    if (line.startsWith("From "))
    {
        line = mrEmailFile.ReadLine();
    }
    
    while (!mrEmailFile.AtEnd() &&
        !line.isEmpty())
    {
        // Read header item
        QString item = line;
        const int item_start_line = mrEmailFile.GetCurrentLineNumber();
        line = mrEmailFile.ReadLine();
        while (!mrEmailFile.AtEnd() &&
               !line.isEmpty())
        {
            QRegularExpressionMatch match = format.match(line);
            if (match.hasMatch())
            {
                break;
            }
            // !!! item += (item.isEmpty() ? "" : "\n") + line;
            item += (item.isEmpty() ? "" : " ") + line;
            line = mrEmailFile.ReadLine();
        }

        // Separate tag and body
        QRegularExpressionMatch match = format.match(item);
        if (!match.hasMatch())
        {
            m_ErrorLine = item_start_line;
            m_Error = tr("Invalid header item structure: \"%1\"").arg(item);
            CALL_OUT(m_Error);
            return;
        }
        const QString item_tag = match.captured(1).toLower();
        const QString item_body = match.captured(4).trimmed();
        
        // Check header item tag (alphabetical order)
        if (item_tag == "accept-language" ||
            item_tag == "acceptlanguage")
        {
            ReadHeader_AcceptLanguage(item_body);
        } else if (item_tag == "amq-delivery-message-id")
        {
            ReadHeader_AMQDeliveryMessageID(item_body);
        } else if (item_tag == "apparently-from")
        {
            ReadHeader_ApparentlyFrom(item_body);
        } else if (item_tag == "apparently-to")
        {
            ReadHeader_ApparentlyTo(item_body);
        } else if (item_tag == "arc-authentication-results")
        {
            ReadHeader_ARCAuthenticationResults(item_body);
        } else if (item_tag == "arc-message-signature")
        {
            ReadHeader_ARCMessageSignature(item_body);
        } else if (item_tag == "arc-seal")
        {
            ReadHeader_ARCSeal(item_body);
        } else if (item_tag == "authentication-results")
        {
            ReadHeader_AuthenticationResults(item_body);
        } else if (item_tag == "authentication-results-original")
        {
            ReadHeader_AuthenticationResultsOriginal(item_body);
        } else if (item_tag == "auto-submitted")
        {
            ReadHeader_AutoSubmitted(item_body);
        } else if (item_tag == "bcc")
        {
            ReadHeader_Bcc(item_body);
        } else if (item_tag == "bounces-to")
        {
            ReadHeader_BouncesTo(item_body);
        } else if (item_tag == "campaign_id")
        {
            ReadHeader_CampaignID(item_body);
        } else if (item_tag == "campaign_token")
        {
            ReadHeader_CampaignToken(item_body);
        } else if (item_tag == "cc")
        {
            ReadHeader_Cc(item_body);
        } else if (item_tag == "comment" ||
            item_tag == "comments")
        {
            ReadHeader_Comments(item_body);
        } else if (item_tag == "content-class")
        {
            ReadHeader_ContentClass(item_body);
        } else if (item_tag == "content-description")
        {
            ReadHeader_ContentDescription(item_body);
        } else if (item_tag == "content-disposition")
        {
            ReadHeader_ContentDisposition(item_body);
        } else if (item_tag == "content-id")
        {
            ReadHeader_ContentId(item_body);
        } else if (item_tag == "content-language")
        {
            ReadHeader_ContentLanguage(item_body);
        } else if (item_tag == "content-length")
        {
            ReadHeader_ContentLength(item_body);
        } else if (item_tag == "content-md5")
        {
            ReadHeader_ContentMD5(item_body);
        } else if (item_tag == "content-transfer-encoding")
        {
            ReadHeader_ContentTransferEncoding(item_body);
        } else if (item_tag == "content-type")
        {
            ReadHeader_ContentType(item_body);
        } else if (item_tag == "conversation-id")
        {
            ReadHeader_ConversationId(item_body);
        } else if (item_tag == "date")
        {
            ReadHeader_Date(item_body);
        } else if (item_tag == "deferred-delivery")
        {
            ReadHeader_DeferredDelivery(item_body);
        } else if (item_tag == "delivered-to")
        {
            ReadHeader_DeliveredTo(item_body);
        } else if (item_tag == "disposition-notification-to")
        {
            ReadHeader_DispositionNotificationTo(item_body);
        } else if (item_tag == "dkim-filter")
        {
            ReadHeader_DKIMFilter(item_body);
        } else if (item_tag == "dkim-signature")
        {
            ReadHeader_DKIMSignature(item_body);
        } else if (item_tag == "domainkey-signature")
        {
            ReadHeader_DomainKeySignature(item_body);
        } else if (item_tag == "encoding")
        {
            ReadHeader_Encoding(item_body);
        } else if (item_tag == "envelope-to")
        {
            ReadHeader_EnvelopeTo(item_body);
        } else if (item_tag == "errors-to" ||
            item_tag == "error-to")
        {
            ReadHeader_ErrorsTo(item_body);
        } else if (item_tag == "feedback-id")
        {
            ReadHeader_FeedbackID(item_body);
        } else if (item_tag == "followup-to")
        {
            ReadHeader_FollowupTo(item_body);
        } else if (item_tag == "from")
        {
            ReadHeader_From(item_body);
        } else if (item_tag == "illegal-object")
        {
            ReadHeader_IllegalObject(item_body);
        } else if (item_tag == "importance")
        {
            ReadHeader_Importance(item_body);
        } else if (item_tag == "in-reply-to")
        {
            ReadHeader_InReplyTo(item_body);
        } else if (item_tag == "ironport-data" ||
            item_tag == "ironport-hdrordr" ||
            item_tag == "ironport-phdr" ||
            item_tag == "ironport-sdr")
        {
            // Ignore Cisco IronPort encryption
            continue;
        } else if (item_tag == "keywords")
        {
            ReadHeader_Keywords(item_body);
        } else if (item_tag == "lines")
        {
            ReadHeader_Lines(item_body);
        } else if (item_tag == "list-archive")
        {
            ReadHeader_ListArchive(item_body);
        } else if (item_tag == "list-help")
        {
            ReadHeader_ListHelp(item_body);
        } else if (item_tag == "list-id")
        {
            ReadHeader_ListId(item_body);
        } else if (item_tag == "list-owner")
        {
            ReadHeader_ListOwner(item_body);
        } else if (item_tag == "list-post")
        {
            ReadHeader_ListPost(item_body);
        } else if (item_tag == "list-subscribe")
        {
            ReadHeader_ListSubscribe(item_body);
        } else if (item_tag == "list-unsubscribe")
        {
            ReadHeader_ListUnsubscribe(item_body);
        } else if (item_tag == "list-unsubscribe-post")
        {
            ReadHeader_ListUnsubscribePost(item_body);
        } else if (item_tag == "mail-followup-to")
        {
            ReadHeader_MailFollowupTo(item_body);
        } else if (item_tag == "mailing-list")
        {
            ReadHeader_MailingList(item_body);
        } else if (item_tag == "message-id")
        {
            ReadHeader_MessageId(item_body);
        } else if (item_tag == "mime-version")
        {
            ReadHeader_MimeVersion(item_body);
        } else if (item_tag == "msip_labels")
        {
            ReadHeader_MSIPLabels(item_body);
        } else if (item_tag == "newsgroups")
        {
            ReadHeader_Newsgroups(item_body);
        } else if (item_tag == "nntp-posting-host")
        {
            ReadHeader_NNTPPostingHost(item_body);
        } else if (item_tag == "non_standard_tag_header")
        {
            ReadHeader_NonStandardTagHeader(item_body);
        } else if (item_tag == "old-content-type")
        {
            ReadHeader_OldContentType(item_body);
        } else if (item_tag == "old-subject")
        {
            ReadHeader_OldSubject(item_body);
        } else if (item_tag == "organization" ||
            item_tag == "organisation")
        {
            ReadHeader_Organization(item_body);
        } else if (item_tag == "originator")
        {
            ReadHeader_Originator(item_body);
        } else if (item_tag == "orig-to")
        {
            ReadHeader_OrigTo(item_body);
        } else if (item_tag == "posted-date")
        {
            ReadHeader_PostedDate(item_body);
        } else if (item_tag == "pp-correlation-id")
        {
            ReadHeader_PPCorrelationID(item_body);
        } else if (item_tag == "pp-to-mdo-migrated")
        {
            ReadHeader_PPToMDOMigrated(item_body);
        } else if (item_tag == "precedence")
        {
            ReadHeader_Precedence(item_body);
        } else if (item_tag == "priority")
        {
            ReadHeader_Priority(item_body);
        } else if (item_tag == "rcpt_domain")
        {
            ReadHeader_RCPTDomain(item_body);
        } else if (item_tag == "received" ||
            item_tag == ">received")
        {
            ReadHeader_Received(item_body);
        } else if (item_tag == "received-date")
        {
            ReadHeader_ReceivedDate(item_body);
        } else if (item_tag == "received-spf")
        {
            ReadHeader_ReceivedSPF(item_body);
        } else if (item_tag == "recipient-id")
        {
            ReadHeader_RecipientID(item_body);
        } else if (item_tag == "references" ||
            item_tag == "reference")
        {
            ReadHeader_References(item_body);
        } else if (item_tag == "reply-to")
        {
            ReadHeader_ReplyTo(item_body);
        } else if (item_tag == "require-recipient-valid-since")
        {
            ReadHeader_RequireRecipientValidSince(item_body);
        } else if (item_tag == "resent-cc")
        {
            ReadHeader_ResentCc(item_body);
        } else if (item_tag == "resent-date")
        {
            ReadHeader_ResentDate(item_body);
        } else if (item_tag == "resent-from")
        {
            ReadHeader_ResentFrom(item_body);
        } else if (item_tag == "resent-message-id")
        {
            ReadHeader_ResentMessageId(item_body);
        } else if (item_tag == "resent-reply-to")
        {
            ReadHeader_ResentReplyTo(item_body);
        } else if (item_tag == "resent-sender")
        {
            ReadHeader_ResentSender(item_body);
        } else if (item_tag == "resent-to")
        {
            ReadHeader_ResentTo(item_body);
        } else if (item_tag == "return-path")
        {
            ReadHeader_ReturnPath(item_body);
        } else if (item_tag == "return-receipt-to" ||
            item_tag == "return-receipt")
        {
            ReadHeader_ReturnReceiptTo(item_body);
        } else if (item_tag == "savedfromemail")
        {
            ReadHeader_SavedFromEmail(item_body);
        } else if (item_tag == "sender")
        {
            ReadHeader_Sender(item_body);
        } else if (item_tag == "sensitivity")
        {
            ReadHeader_Sensitivity(item_body);
        } else if (item_tag == "sent-on")
        {
            ReadHeader_SentOn(item_body);
        } else if (item_tag == "site-id")
        {
            ReadHeader_SiteID(item_body);
        } else if (item_tag == "spamdiagnosticmetadata")
        {
            ReadHeader_SpamDiagnosticMetadata(item_body);
        } else if (item_tag == "spamdiagnosticoutput")
        {
            ReadHeader_SpamDiagnosticOutput(item_body);
        } else if (item_tag == "status")
        {
            ReadHeader_Status(item_body);
        } else if (item_tag == "subject")
        {
            ReadHeader_Subject(item_body);
        } else if (item_tag == "suggested_attachment_session_id")
        {
            ReadHeader_SuggestedAttachmentSessionID(item_body);
        } else if (item_tag == "thread-index")
        {
            ReadHeader_ThreadIndex(item_body);
        } else if (item_tag == "thread-topic")
        {
            ReadHeader_ThreadTopic(item_body);
        } else if (item_tag == "to")
        {
            ReadHeader_To(item_body);
        } else if (item_tag == "ui-outboundreport")
        {
            ReadHeader_UIOutboundReport(item_body);
        } else if (item_tag == "user-agent")
        {
            ReadHeader_UserAgent(item_body);
        } else if (item_tag == "warnings-to")
        {
            ReadHeader_WarningsTo(item_body);
        } else if (item_tag == "x-mailer" ||
            item_tag == "mailer")
        {
            ReadHeader_XMailer(item_body);
        } else if (item_tag.startsWith("x-") ||
            item_tag.startsWith("X-"))
        {
            // Ignore extended headers
            continue;
        } else if (item_tag == "-ms-exchange-organization-bypassclutter")
        {
            // Ignore malformed Microsoft header element
            // (should be "x-ms-exchange...")
            continue;
        } else
        {
            // Unknown header (non-fatal)
            const QString reason = tr("Unknown header item: %1: %2")
                .arg(item_tag,
                     item_body);
            MessageLogger::Error(CALL_METHOD, reason);
        }
        
        // Check if there was an error
        if (!m_Error.isEmpty())
        {
            CALL_OUT("");
            return;
        }
    }
    
    // Check if the essential header items are present
    if (!m_HeaderData.contains("From"))
    {
        m_Error = tr("Error in email header: no sender (\"from\") specified.");
    }
    if (!m_HeaderData.contains("To"))
    {
        m_HeaderData["To"]["full name"] = tr("Undisclosed recipients");
    }
    if (!m_HeaderData.contains("Date"))
    {
        m_Error = tr("Error in email header: no date specified.");
    }
    if (!m_HeaderData.contains("Subject"))
    {
        m_HeaderData["Subject"]["subject"] = tr("(no subject)");
    }
    
    // Done with header
    if (DEBUG)
    {
        qDebug().noquote() << tr("Line %1: Email header end")
            .arg(mrEmailFile.GetCurrentLineNumber());
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Accept-Language
void Email::ReadHeader_AcceptLanguage(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Accept-Language: en-US
    m_HeaderData["Accept-Language"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: AMQ-Delivery-Message-ID
void Email::ReadHeader_AMQDeliveryMessageID(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // amq-delivery-message-id: nullval
    m_HeaderData["AMQ-Delivery-Message-ID"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Apparently-From
void Email::ReadHeader_ApparentlyFrom(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Apparently-To: foo@bar.com
    m_HeaderData["Apparently-From"] = ParseEmailAddress(mcBody);
    m_HeaderData["Apparently-From"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Apparently-To
void Email::ReadHeader_ApparentlyTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Apparently-To: foo@bar.com
    m_HeaderData["Apparently-To"] = ParseEmailAddress(mcBody);
    m_HeaderData["Apparently-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: ARC-Authentication-Results
void Email::ReadHeader_ARCAuthenticationResults(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // ARC-Authentication-Results: i=1; mx.google.com;
    //   dkim=pass header.i=@yahoo.com header.s=s2048 header.b=c6qCmh8T;
    //   dkim=pass header.i=@gmail.com header.s=20161025 header.b=k9crSEf0;
    //   spf=softfail (google.com: domain of transitioning 
    //     bernard.landgraf@gmail.com does not designate 77.238.178.168 as 
    //     permitted sender) smtp.mailfrom=bernard.landgraf@gmail.com;
    //   dmarc=pass (p=NONE sp=NONE dis=NONE) header.from=gmail.com
    m_HeaderData["ARC-Authentication-Results"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: ARC-Message-Signature
void Email::ReadHeader_ARCMessageSignature(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // ARC-Message-Signature: i=1; a=rsa-sha256; c=relaxed/relaxed;
    //   d=google.com; s=arc-20160816;
    //   h=to:subject:message-id:date:from:references:in-reply-to:mime-version
    //   :dkim-signature:dkim-signature:arc-authentication-results;
    //   bh=r4PFT+o9R141tkZrxiuZ1+/pNBLr2jxJnlrIu1yKS6w=;
    //   b=aBLMhY6RFJb8fCz4kHzjPi7HDUAoPpiT9r05qlpwIuvDPYB3Z2JPFee0P9L8oDeGsY
    //     Ht6bReSNLIj0QZGHwga1DYAnt0qfwuARUlw7pIiX0Y2GKReIoNv0TkhF0tTvuHQTqK8S
    //     VCkIfoS2285Re5cWyyjTPLepCzaHl9oN1xAikeOXeBvMAlLBccH69WuvRxi0wx1FO3mH
    //     G3gZFWnWcpsfBqSVzNZNf4rtHBYyjYC0XXt+UL9V4CQ0FBGIVwbgxGB9K+Z7L3zNSPR3
    //     4Dvjg89xzpXkqzqoXWKq74v48B6U/zmPOFuyt+zifbG4TwQwQsUp+eOHf6uZTihQiV+Q
    //     Y9lw==
    m_HeaderData["ARC-Message-Signature"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: ARC-Seal
void Email::ReadHeader_ARCSeal(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // ARC-Seal: i=1; a=rsa-sha256; t=1516693234; cv=none;
    //   d=google.com; s=arc-20160816;
    //   b=p3g/NRMS/euP14gdSLgYHC9pOrGQwxy71/li7JGtQmX3yyhVS/lwdKLVzfb1DH67OY
    //     wNxJCKww+9sxuyDfMwd8NdEfHvQkLb+7EvkYPbQE02jInk/1Vo6mGxYmqqk+spdiV0U
    //     THGJGQoFtdFXu6dixcvh3KYxevU1nJVqYNFqfdfH96MAJNZ5XnkKqwMotTjDLedqJset
    //     XHrjgu/GojPMdP2uS4v3zfMtim4ZkHr25zPUZdMd/+IO1LH6GhEHS/4UziP/4zkHeh7t
    //     fhSgt0zRnd8cbBCxb7XBN8cmobVIcamPt/sxqkRlsE1Am+xb9/KgS81D8JmobXwzV/en
    //     zogg==
    m_HeaderData["ARC-Seal"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Authentication-Results
void Email::ReadHeader_AuthenticationResults(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Authentication-Results: mta1156.mail.ir2.yahoo.com  from=hotmail.com;
    //    domainkeys=neutral (no sig);  from=hotmail.com; dkim=pass (ok)
    m_HeaderData["Authentication-Results"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Authentication-Results-Original
void Email::ReadHeader_AuthenticationResultsOriginal(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // authentication-results-original: dkim=none (message not signed)
    //   header.d=none;dmarc=none action=none header.from=eurogentec.com;
    m_HeaderData["Authentication-Results-Original"]["raw"] = mcBody;

    CALL_OUT("");
}


///////////////////////////////////////////////////////////////////////////////
// Email header: Auto-Submitted
void Email::ReadHeader_AutoSubmitted(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Auto-Submitted: auto-generated
    m_HeaderData["Auto-Submitted"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Bcc
void Email::ReadHeader_Bcc(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Bcc: John Doe <john.doe@oo.com>
    m_HeaderData_Bcc = ParseEmailAddressList(mcBody);
    m_HeaderData["Bcc"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Bounces-To
void Email::ReadHeader_BouncesTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // bounces-to: ems+ZH76WQVWJXCKNJ@bounces.amazon.com
    m_HeaderData["Bounces-To"] = ParseEmailAddress(mcBody);
    m_HeaderData["Bounces-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Campaign_ID
void Email::ReadHeader_CampaignID(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // campaign_id: 524450
    m_HeaderData["Campaign-ID"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Campaign_Token
void Email::ReadHeader_CampaignToken(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // campaign_token: citizens-main-prod-277f9dc54215db4604309ce08c01fdc6-1
    m_HeaderData["Campaign-Token"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Cc
void Email::ReadHeader_Cc(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Cc: Doe John <john.doe@foo.com>
    m_HeaderData_Cc = ParseEmailAddressList(mcBody);
    m_HeaderData["Cc"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Comments
void Email::ReadHeader_Comments(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Comments: Sender has elected to use 8-bit data in this message.
    //      If problems arise, refer to postmaster at sender's site.
    QString body(mcBody);
    body.replace("\n", " ");
    body.replace("\t", " ");
    while (body.contains("  "))
    {
        body.replace("  ", " ");
    }
    m_HeaderData["Comments"]["raw"] = body;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-Class
void Email::ReadHeader_ContentClass(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // content-class: urn:content-classes:message
    m_HeaderData["Content-Class"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-Description
void Email::ReadHeader_ContentDescription(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Content-Description: Mail message body
    m_HeaderData["Content-Description"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-Disposition
void Email::ReadHeader_ContentDisposition(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Content-Disposition: inline
    m_HeaderData["Content-Disposition"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-Id
void Email::ReadHeader_ContentId(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Content-Id: <Pine.SUN.3.95.990621111127.19454H@foo.bar.com>
    m_HeaderData["Content-Id"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-Language
void Email::ReadHeader_ContentLanguage(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Content-Language: en-us
    m_HeaderData["Content-Language"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-Length
void Email::ReadHeader_ContentLength(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Content-Length: 7589695
    m_HeaderData["Content-Length"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-MD5
void Email::ReadHeader_ContentMD5(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // content-md5: uJvqFgw5iOaZ421bZMR5Xw==
    m_HeaderData["Content-MD5"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-Transfer-Encoding
void Email::ReadHeader_ContentTransferEncoding(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Content-Transfer-Encoding: 7bit
    m_HeaderData["Content-Transfer-Encoding"]["raw"] = mcBody;
    m_HeaderData["Content-Transfer-Encoding"]["encoding"] = mcBody.toLower();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Content-Type
void Email::ReadHeader_ContentType(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    m_HeaderData["Content-Type"]["raw"] = mcBody;
    QString rest = mcBody.trimmed();
    
    // Type
    const QString known_formats(
        "application/pkcs7-mime|"
        "application/pgp|"
        "application/x-macbinary|"
        "image/heif|"
        "image/jpe?g|"
        "text|"
        "([Tt]ext|TEXT)/([Ee]nriched|[Hh]tml|HTML|[Pp]lain|PLAIN)|"
        "([Mm]essage|MESSAGE)/(RFC|rfc)822|"
        "([Mm]ultipart|MULTIPART)/"
            "(alternate|"
            "[Aa]lternative|ALTERNATIVE|"
            "encrypted|"
            "[Mm]ixed|MIXED|"
            "[Rr]elated|"
            "[Rr]eport|REPORT|"
            "[Ss]igned)");
    static const QRegularExpression format_known_format(
        QString("^(%1)(;|;\\s*(\\S.*))?$")
            .arg(known_formats));
    int last_index = 1 + known_formats.count("(") + 2;
    QRegularExpressionMatch match_known_format =
        format_known_format.match(rest);
    if (match_known_format.hasMatch())
    {
        m_HeaderData["Content-Type"]["type"] =
            match_known_format.captured(1).toLower();
        if (m_HeaderData["Content-Type"]["type"] == "multipart/alternate")
        {
            // multipart/alternate is an invalid type that is used sometimes;
            // support it anyway as the correct type "multipart/alternative"
            m_HeaderData["Content-Type"]["type"] = "multipart/alternative";
        }
        rest = match_known_format.captured(last_index);
    } else
    {
        // Unknown type
        m_Error = tr("Unknown content type \"%1\" in email header.")
            .arg(mcBody); 
        MessageLogger::Error(CALL_METHOD, m_Error);
        CALL_OUT(m_Error);
        return;
    }
    rest = rest.trimmed();

    // Boundary
    if (m_HeaderData["Content-Type"]["type"].startsWith("multipart"))
    {
        // boundary="----=_NextPart_000_008B_01D25C60.8194E3C0"
        // boundary="---- =_NextPart_000_01BE8A7C.F983E120"
        static const QRegularExpression format_boundary1(
            "^(.*)([Bb]oundary|BOUNDARY)=\"([^\";]+)\"(;|\\s*;\\s+(\\S.*))?$");
        QRegularExpressionMatch match_boundary1 = format_boundary1.match(rest);
        if (match_boundary1.hasMatch())
        {
            m_HeaderData["Content-Type"]["boundary"] =
                match_boundary1.captured(3);
            rest = match_boundary1.captured(1) + match_boundary1.captured(5);
        } else
        {
            // boundary=Apple-Mail-5907DAE7-C9AE-4276-82A5-53A8B887052D
            static const QRegularExpression format_boundary2(
                "^(.*)([Bb]oundary|BOUNDARY)=([^\";]+)(;|\\s*;\\s+(\\S.*))?$");
            QRegularExpressionMatch match_boundary2 =
                format_boundary2.match(rest);
            if (match_boundary2.hasMatch())
            {
                m_HeaderData["Content-Type"]["boundary"] =
                    match_boundary2.captured(3);
                rest =
                    match_boundary2.captured(1) + match_boundary2.captured(5);
            } else
            {
                // Unknown boundary
                m_Error = tr("Unknown boundary \"%1\" in email header "
                    "content type.").arg(mcBody);
                MessageLogger::Error(CALL_METHOD, m_Error);
                CALL_OUT(m_Error);
                return;
            }
        }
        rest = rest.trimmed();
    }

    // Report type
    if (m_HeaderData["Content-Type"]["type"] == "multipart/report")
    {
        static const QRegularExpression format_report_type1(
            "^(.*)(report-type|REPORT-TYPE)="
            "\"([^\"; ]+)\"(\\s*;?)(\\s+(\\S.*))?$");
        QRegularExpressionMatch match_report_type1 =
            format_report_type1.match(rest);
        if (match_report_type1.hasMatch())
        {
            m_HeaderData["Content-Type"]["report-type"] =
                match_report_type1.captured(3);
            rest = match_report_type1.captured(1) +
                match_report_type1.captured(6);
        }
        static const QRegularExpression format_report_type2(
            "^(.*)(report-type|REPORT-TYPE)="
            "([^\"; ]+)(\\s*;)?(\\s+(\\S.*))?$");
        QRegularExpressionMatch match_report_type2 =
            format_report_type2.match(rest);
        if (match_report_type2.hasMatch())
        {
            m_HeaderData["Content-Type"]["report-type"] =
                match_report_type2.captured(3);
            rest = match_report_type2.captured(1) +
                match_report_type2.captured(6);
        }
        rest = rest.trimmed();
    }
    
    // Reply type
    static const QRegularExpression format_reply_type1(
        "^(.*)reply-type=([^\"; ]+)(\\s*;?)(\\s+(\\S.*))?$");
    QRegularExpressionMatch match_reply_type1 = format_reply_type1.match(rest);
    if (match_reply_type1.hasMatch())
    {
        m_HeaderData["Content-Type"]["reply-type"] =
            match_reply_type1.captured(2);
        rest = match_reply_type1.captured(1) + match_reply_type1.captured(5);
    }
    static const QRegularExpression format_reply_type2(
        "^(.*)reply-type=([^\"; ]+)(\\s*;)?(\\s+\"(\\S[^\"]*)\")?$");
    QRegularExpressionMatch match_reply_type2 = format_reply_type2.match(rest);
    if (match_reply_type2.hasMatch())
    {
        m_HeaderData["Content-Type"]["reply-type"] =
            match_reply_type2.captured(2);
        rest = match_reply_type2.captured(1) +
            match_reply_type2.captured(5);
    }
    rest = rest.trimmed();
    
    // Protocol + Micalg
    if (m_HeaderData["Content-Type"]["type"] == "multipart/signed" ||
        m_HeaderData["Content-Type"]["type"] == "multipart/encrypted")
    {
        static const QRegularExpression format_protocol(
            "^(.*)protocol=\"([^\"; ]+)\"(\\s*;)?(\\s+(\\S.*))?$");
        QRegularExpressionMatch match_protocol = format_protocol.match(rest);
        if (match_protocol.hasMatch())
        {
            m_HeaderData["Content-Type"]["protocol"] =
                match_protocol.captured(2);
            rest = match_protocol.captured(1) + match_protocol.captured(5);
        }
        static const QRegularExpression format_micalg(
            "^(.*)micalg=([^\"; ]+)(\\s*;)?(\\s+(\\S.*)?)?$");
        QRegularExpressionMatch match_micalg = format_micalg.match(rest);
        if (match_micalg.hasMatch())
        {
            m_HeaderData["Content-Type"]["micalg"] = match_micalg.captured(2);
            rest = match_micalg.captured(1) + match_micalg.captured(5);
        }
        rest = rest.trimmed();
    }
    
    // method
    if (m_HeaderData["Content-Type"]["type"] == "text/calendar")
    {
        static const QRegularExpression format_method1(
            "^(.*)method=\"(CANCEL)\"(\\s*;\\s+(\\S.*))?$");
        QRegularExpressionMatch match_method1 = format_method1.match(rest);
        QString method;
        if (match_method1.hasMatch())
        {
            method = match_method1.captured(2).toLower();
            rest = match_method1.captured(1) + match_method1.captured(4);
        } else {
            static const QRegularExpression format_method2(
                "^(.*)method=([^\"; ]+)(\\s*;\\s+(\\S.*))?$");
            QRegularExpressionMatch match_method2 = format_method2.match(rest);
            if (match_method2.hasMatch())
            {
                method = match_method2.captured(2).toLower();
                rest = match_method2.captured(1) + match_method2.captured(4);
            }
        }
        rest = rest.trimmed();

        // Check value
        if (!method.isEmpty())
        {
            if (method != "CANCEL")
            {
                const QString reason =
                    tr("Method parameter for text/calendar has an invalid "
                        "value \"%1\"")
                        .arg(method);
                MessageLogger::Error(CALL_METHOD, reason);
            } else
            {
                m_HeaderData["Content-Type"]["method"] = method.toLower();
            }
        }
    }

    // delsp
    if (m_HeaderData["Content-Type"]["type"] == "text/plain")
    {
        static const QRegularExpression format_delsp1(
            "^(.*)delsp=\"([^\"; ]+)\"(\\s*;\\s+(\\S.*))?$");
        QRegularExpressionMatch match_delsp1 = format_delsp1.match(rest);
        QString delsp;
        if (match_delsp1.hasMatch())
        {
            delsp = match_delsp1.captured(2).toLower();
            rest = match_delsp1.captured(1) + match_delsp1.captured(4);
        } else {
            static const QRegularExpression format_delsp2(
                "^(.*)delsp=([^\"; ]+)(\\s*;\\s+(\\S.*))?$");
            QRegularExpressionMatch match_delsp2 = format_delsp2.match(rest);
            if (match_delsp2.hasMatch())
            {
                delsp = match_delsp2.captured(2).toLower();
                rest = match_delsp2.captured(1) + match_delsp2.captured(4);
            }
        }
        rest = rest.trimmed();
        
        // Check value
        if (!delsp.isEmpty())
        {
            if (delsp != "yes" && delsp != "no")
            {
                const QString reason =
                    tr("delsp parameter has an invalid value \"%1\"")
                        .arg(delsp);
                MessageLogger::Error(CALL_METHOD, reason);
            } else
            {
                m_HeaderData["Content-Type"]["delsp"] = delsp;
            }
        }
    }

    // X-Mac-Type and X-Mac-Creator
    static const QRegularExpression format_mac_type1(
        "^(.*)x-mac-type=\"([^\"; ]+)\"(\\s*;\\s+(\\S.*))?$");
    QRegularExpressionMatch match_mac_type1 = format_mac_type1.match(rest);
    if (match_mac_type1.hasMatch())
    {
        m_HeaderData["Content-Type"]["x-mac-type"] =
            match_mac_type1.captured(2);
        rest = match_mac_type1.captured(1) + match_mac_type1.captured(4);
    } else
    {
        static const QRegularExpression format_mac_type2(
            "^(.*)x-mac-type=([^\"; ]+)(\\s*;\\s+(\\S.*))?$");
        QRegularExpressionMatch match_mac_type2 = format_mac_type2.match(rest);
        if (match_mac_type2.hasMatch())
        {
            m_HeaderData["Content-Type"]["x-mac-type"] =
                match_mac_type2.captured(2);
            rest = match_mac_type2.captured(1) + match_mac_type2.captured(4);
        }
    }
    rest = rest.trimmed();
    static const QRegularExpression format_mac_creator1(
        "^(.*)x-mac-creator=\"([^\"; ]+)\"(\\s*;\\s+(\\S.*))?$");
    QRegularExpressionMatch match_mac_creator1 =
        format_mac_creator1.match(rest);
    if (match_mac_creator1.hasMatch())
    {
        m_HeaderData["Content-Type"]["x-mac-creator"] =
            match_mac_creator1.captured(2);
        rest = match_mac_creator1.captured(1) + match_mac_creator1.captured(4);
    } else
    {
        static const QRegularExpression format_mac_creator2(
            "^(.*)x-mac-creator=([^\"; ]+)(\\s*;\\s+(\\S.*))?$");
        QRegularExpressionMatch match_mac_creator2 =
            format_mac_creator2.match(rest);
        if (match_mac_creator2.hasMatch())
        {
            m_HeaderData["Content-Type"]["x-mac-creator"] =
                match_mac_creator2.captured(2);
            rest = match_mac_creator2.captured(1) +
                match_mac_creator2.captured(4);
        }
    }
    rest = rest.trimmed();
    
    // X-Action
    static const QRegularExpression format_action1(
        "^(.*)x-action=\"([^\"; ]+)\"(\\s*;\\s+(\\S.*))?$");
    QRegularExpressionMatch match_action1 = format_action1.match(rest);
    if (match_action1.hasMatch())
    {
        m_HeaderData["Content-Type"]["x-action"] = match_action1.captured(2);
        rest = match_action1.captured(1) + match_action1.captured(4);
    } else
    {
        static const QRegularExpression format_action2(
            "^(.*)x-action=([^\"; ]+)(\\s*;\\s+(\\S.*))?$");
        QRegularExpressionMatch match_action2 = format_action2.match(rest);
        if (match_action2.hasMatch())
        {
            m_HeaderData["Content-Type"]["x-action"] =
                match_action2.captured(2);
            rest = match_action2.captured(1) + match_action2.captured(4);
        }
    }
    rest = rest.trimmed();

    // X-Unix-Mode
    static const QRegularExpression format_unix_mode(
        "^(.*)x-unix-mode=([0-7]+)(\\s*;\\s+(\\S.*))?$");
    QRegularExpressionMatch match_unix_mode = format_unix_mode.match(rest);
    if (match_unix_mode.hasMatch())
    {
        m_HeaderData["Content-Type"]["x-unix-mode"] =
            match_unix_mode.captured(2);
        rest = match_unix_mode.captured(1) + match_unix_mode.captured(4);
    }
    rest = rest.trimmed();
    
    // Charset
    // (Can't make "rest" all lower case because boundary is case sensitive)
    const QString known_charsets(
        "ascii|"
        "koi8-r|"
        "(ISO|iso)-2022-(JP|jp)|"
        "(ISO|iso)-2022-(KR|kr)|"
        "(ISO|iso)-8859-1|"
        "(ISO|iso)-8859-2|"
        "(ISO|iso)-8859-7|"
        "(ISO|iso)-8859-13|"
        "(ISO|iso)-8859-15|"
        "(unknown-8bit|UNKNOWN-8BIT)|"
        "(US|us)-(ASCII|ascii)|"
        "(UTF|utf)-8|"
        "([Ww]indows|WINDOWS)-1250|"
        "([Ww]indows|WINDOWS)-1251|"
        "([Ww]indows|WINDOWS)-1252|"
        "([Ww]indows|WINDOWS)-1254|"
        "[Xx]-[Rr]oman8|"
        "(X-UNKNOWN|x-unknown)");
    static const QRegularExpression format_charset1(
        QString("^(.*)([Cc]harset|CHARSET)=(%1)(;|\\s*;\\s*(\\S.*))?$")
            .arg(known_charsets));
    last_index = 3 + known_charsets.count("(") + 2;
    QRegularExpressionMatch match_charset1 = format_charset1.match(rest);
    if (match_charset1.hasMatch())
    {
        m_HeaderData["Content-Type"]["charset"] =
            match_charset1.captured(3).toLower();
        rest = match_charset1.captured(1) + match_charset1.captured(last_index);
    } else
    {
        static const QRegularExpression format_charset2(
            QString("^(.*)([Cc]harset|CHARSET)\\s*="
                "\\s*\"(%1)\"(;|\\s*;\\s*(\\S.*))?$")
            .arg(known_charsets));
        QRegularExpressionMatch match_charset2 = format_charset2.match(rest);
        if (match_charset2.hasMatch())
        {
            m_HeaderData["Content-Type"]["charset"] =
                match_charset2.captured(3).toLower();
            rest = match_charset2.captured(1) +
                match_charset2.captured(last_index);
        }
    }
    rest = rest.trimmed();
    
    // Format
    static const QRegularExpression format_format1(
        "^(.*)format=([^\"; ]+)(;|\\s*;\\s*(\\S.*))?$");
    QRegularExpressionMatch match_format1 = format_format1.match(rest);
    if (match_format1.hasMatch())
    {
        m_HeaderData["Content-Type"]["format"] =
            match_format1.captured(2).toLower();
        rest = match_format1.captured(1) + match_format1.captured(4);
    } else
    {
        static const QRegularExpression format_format2(
            "^(.*)format=\"([^\"; ]+)\"(\\s*;\\s*(\\S.*))?$");
        QRegularExpressionMatch match_format2 = format_format2.match(rest);
        if (match_format2.hasMatch())
        {
            m_HeaderData["Content-Type"]["format"] =
                match_format2.captured(2).toLower();
            rest = match_format2.captured(1) + match_format2.captured(4);
        }
    }
    rest = rest.trimmed();
    
    // Name
    static const QRegularExpression format_name1(
        "^(.*)[Nn]ame=([^\";]+)(;|\\s*;\\s*(\\S.*))?$");
    QRegularExpressionMatch match_name1 = format_name1.match(rest);
    if (match_name1.hasMatch())
    {
        m_HeaderData["Content-Type"]["name"] =
            match_name1.captured(2).toLower();
        rest = match_name1.captured(1) + match_name1.captured(4);
    } else
    {
        static const QRegularExpression format_name2(
            "^(.*)[Nn]ame=\"([^\"]+)\"(\\s*;\\s*(\\S.*))?$");
        QRegularExpressionMatch match_name2 = format_name2.match(rest);
        if (match_name2.hasMatch())
        {
            m_HeaderData["Content-Type"]["name"] =
                match_name2.captured(2).toLower();
            rest = match_name2.captured(1) + match_name2.captured(4);
        }
    }
    rest = rest.trimmed();
    
    // Type (should be redundant with content-type)
    static const QRegularExpression format_type1(
        "^(.*)[Tt]ype=([^\";]+)(;|\\s*;\\s*(\\S.*))?$");
    QRegularExpressionMatch match_type1 = format_type1.match(rest);
    if (match_type1.hasMatch())
    {
        // Ignore type, but keep rest
        rest = match_type1.captured(1) + match_type1.captured(4);
    } else
    {
        static const QRegularExpression format_type2(
            "^(.*)[Tt]ype=\"([^\"]+)\"(\\s*;\\s*(\\S.*))?$");
        QRegularExpressionMatch match_type2 = format_type2.match(rest);
        if (match_type2.hasMatch())
        {
            // Ignore type, but keep rest
            rest = match_type2.captured(1) + match_type2.captured(4);
        }
    }
    rest = rest.trimmed();
    
    // Any rest?
    if (!rest.isEmpty())
    {
        const QString reason = tr("Residual information \"%1\"")
            .arg(rest);
        MessageLogger::Error(CALL_METHOD, reason);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Conversation-Id
void Email::ReadHeader_ConversationId(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Conversation-Id: <199701260409.XAA25501@boris.interlynx.net>
    m_HeaderData["Conversation-Id"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Date
void Email::ReadHeader_Date(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Date - several formats
    m_HeaderData["Date"] = ParseDate(mcBody);
    m_HeaderData["Date"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: deferred-delivery
void Email::ReadHeader_DeferredDelivery(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // deferred-delivery: Sun, 14 Mar 2019 00:08:05 +0000
    m_HeaderData["Deferred-Delivery"] = ParseDate(mcBody);
    m_HeaderData["Deferred-Delivery"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Delivered-To
void Email::ReadHeader_DeliveredTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Delivered-To: john.doe@foo.com
    m_HeaderData["Delivered-To"] = ParseEmailAddress(mcBody);
    m_HeaderData["Delivered-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Disposition-Notification-To
void Email::ReadHeader_DispositionNotificationTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Disposition-Notification-To:
    //    John Doe <john.doe@foo.bar.org>
    m_HeaderData["Disposition-Notification-To"] = ParseEmailAddress(mcBody);
    m_HeaderData["Disposition-Notification-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: DKIM-Filter
void Email::ReadHeader_DKIMFilter(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // dkim-filter: OpenDKIM Filter v2.11.0 mailwsout15.transactional-mail-d
    //  .com 4Q2nJk2QHDz10Vb

    m_HeaderData["DKIM-Filter"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: DKIM-Signature
void Email::ReadHeader_DKIMSignature(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // DKIM-Signature: v=1; a=rsa-sha256; c=simple/simple; d=foo.com; s=dkim;
    //    t=1484072938; bh=uK72fMJiJdNa3bzE1sRMqP1hxyFsf/4znU84lf2mUnQ=;
    //    h=From:Date:Subject:To:From;
    //    b=Rdy0xTavQbUnUEaJzERu2IUwy/bxfAMQLKmvn6h/xNES2ee88wlvEz+/Fr5b2IKn
    //    gHezecBhiTz26UfpRaHbJtUncwdkXPhPDpRb/NitSPB1Db+ZQAK1kasvZ46H8PlrpZ
    //    ZRjd51d1ixVuCi6Xrvv2LIzb6RGS5oLTf0zUz6yw=
    m_HeaderData["DKIM-Signature"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: DomainKey-Signature
void Email::ReadHeader_DomainKeySignature(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // DomainKey-Signature: a=rsa-sha1; c=nofws; q=dns; s=k1;
    //    d=mail79.atl111.rsgsv.net;
    //    b=BqgJtGcU9Ezy67ZKhF7n8ZfkQ3oHBVssyLaqtOQ3qPbsXzCg1+mWh1CBu1sR4
    //    BqgHQufssmkyRf2   R3gFjvavVUmFSoBEkczOyBqKneWMYtj+2ojEh7F+lAMgs
    //    r8C89xpF9W7cJ1VW2Ns89+s8fWt9xDD   jjndwgAu3bNi5a/wSPQ=;
    m_HeaderData["DomainKey-Signature"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Encoding
void Email::ReadHeader_Encoding(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Encoding: 91 TEXT
    m_HeaderData["Encoding"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Envelope-To
void Email::ReadHeader_EnvelopeTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Envelope-to: john.doe@foo.bar.org
    m_HeaderData["Envelope-To"] = ParseEmailAddress(mcBody);
    m_HeaderData["Envelope-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Errors-To
void Email::ReadHeader_ErrorsTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Errors-To: john.doe@foo.bar.org
    m_HeaderData["Errors-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Feedback-ID
void Email::ReadHeader_FeedbackID(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // feedback-id: 67517977:67517977.1015499:us14:mc
    m_HeaderData["Feedback-ID"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Followup-To
void Email::ReadHeader_FollowupTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Followup-To: de.rec.fotografie
    m_HeaderData["Followup-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: From
void Email::ReadHeader_From(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // From: John Doe <john.doe@foo.com>
    QHash < QString, QString > from = ParseEmailAddress(mcBody);
    for (auto key_iterator = from.keyBegin();
         key_iterator != from.keyEnd();
         key_iterator++)
    {
        const QString key = *key_iterator;
        from[key].replace("\n", " ");
        from[key].replace("\r", " ");
        from[key].replace("\t", " ");
        while (from[key].contains("  "))
        {
            from[key].replace("  ", " ");
        }
    }
    m_HeaderData["From"] = from;
    m_HeaderData["From"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Illegal-Object
void Email::ReadHeader_IllegalObject(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Illegal-Object: Syntax error in To: address found on foo.net:
    //    To:	Something.Stupid <john.doe@foo.com>
	//		    ^-missing end of address
    m_HeaderData["Illegal-Object"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Importance
void Email::ReadHeader_Importance(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Importance: Normal
    m_HeaderData["Importance"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: In-Reply-To
void Email::ReadHeader_InReplyTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Can be empty!
    if (mcBody.isEmpty())
    {
        // Nothing to do.
        CALL_OUT("");
        return;
    }
    
    m_HeaderData["In-Reply-To"]["raw"] = mcBody;
    
    // Decode
    const QString decoded = DecodeIfNecessary(mcBody);
    
    // In-Reply-To: <4B8E5BC42A7976056069818E4A308B13F@NCEXMBX04.some.com>
    // In-Reply-To: Your message of "Wed, 24 Feb 1999 18:31:49 +0100"
    //      <003b01be601b$8f2b0fa0$fc9fdc83@bar.com>
    // In-Reply-To: <20040411193721.A3882@doo.com>;
    //    from john.doe@foo.com on Sun, Apr 11, 2004 at "
    //    07:37:21PM +0200
    // I have seen <bla@foo bar> (with a space)
    static const QRegularExpression format_1("^[^<>]*<([^<>]+)>[^<>]*$");
    QRegularExpressionMatch match = format_1.match(decoded);
    if (match.hasMatch())
    {
        m_HeaderData["In-Reply-To"]["id"] = match.captured(1);
        CALL_OUT("");
        return;
    }

    // In-Reply-To: 79717d5cafac81936666e49659ba0358@mail.gmail.com
    static const QRegularExpression format_2("^([^<> ]+)$");
    match = format_2.match(decoded);
    if (match.hasMatch())
    {
        m_HeaderData["In-Reply-To"]["id"] = match.captured(1);
        CALL_OUT("");
        return;
    }

    // Unknown format
    const QString reason = tr("Unknown format \"%1\" (\"%2\")")
        .arg(mcBody,
             decoded);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Keywords
void Email::ReadHeader_Keywords(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    m_HeaderData["Keywords"]["raw"] = mcBody;

    // keywords: a,b,c
    m_HeaderData["Keywords"]["keywords"] = mcBody;
    CALL_OUT("");
    return;
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Lines
void Email::ReadHeader_Lines(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    m_HeaderData["Lines"]["raw"] = mcBody;

    // Lines: 12
    static const QRegularExpression format("^([0-9]+)$");
    QRegularExpressionMatch match = format.match(mcBody);
    if (match.hasMatch())
    {
        m_HeaderData["Lines"]["lines"] = match.captured(1);
        CALL_OUT("");
        return;
    }

    // Unknown format
    const QString reason = tr("Unknown format \"%1\"").arg(mcBody);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
}



///////////////////////////////////////////////////////////////////////////////
// Email header: List-Archive
void Email::ReadHeader_ListArchive(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // list-archive: <http://lists.foo.com/archives/public/announce/>
    m_HeaderData["List-Archive"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: List-Help
void Email::ReadHeader_ListHelp(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // List-Help: <https://www.xing.com/app/settings?op=notifications>,
    //    <mailto:fbl@xing.com>
    m_HeaderData["List-Help"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: List-Id
void Email::ReadHeader_ListId(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // List-Id: <c0b777f196e0076385a0a1f558985e1a.foo.com>
    m_HeaderData["List-Id"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: List-Owner
void Email::ReadHeader_ListOwner(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // List-Owner: <mailto:listmaster@list.foo.com>
    m_HeaderData["List-Owner"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: List-Post
void Email::ReadHeader_ListPost(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // List-Post: <mailto:foo@bar.com>
    m_HeaderData["List-Post"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: List-Subscribe
void Email::ReadHeader_ListSubscribe(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // List-Subscribe:
    //  <http://www.lists.foo.com/mailman/listinfo/weekly-kbase-changes>,
    //  <mailto:weekly-kbase-changes-request@lists.foo.com?subject=subscribe>
    m_HeaderData["List-Subscribe"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: List-Unsubscribe
void Email::ReadHeader_ListUnsubscribe(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // List-Unsubscribe: <mailto:unsubscribe-mc.us8_c000eccb7b70061a7e76aef5
    //    6.853e25b752-3828270a59@mailin1.us2.mcsv.net?subject=unsubscribe>,
    //    <http://onemedical.us8.list-manage.com/unsubscribe?u=c000eccb7b70
    //    061a7e76aef56&id=27da5a82f1&e=3828270a59&c=853e25b752>
    m_HeaderData["List-Unsubscribe"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: List-Unsubscribe-Post
void Email::ReadHeader_ListUnsubscribePost(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // list-unsubscribe-post: List-Unsubscribe=One-Click
    m_HeaderData["List-Unsubscribe-Post"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Mail-Followup-To
void Email::ReadHeader_MailFollowupTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Mail-Followup-To: Foo Group <foo@bar.de>
    m_HeaderData["Mail-Followup-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Mailing-List
void Email::ReadHeader_MailingList(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Mailing-List: list foo@bar.de;
    //    contact foo@bar.de
    m_HeaderData["Mailing-List"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Message-Id
void Email::ReadHeader_MessageId(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    m_HeaderData["Message-Id"]["raw"] = mcBody;

    // Message-Id: <0EA1829A-500F-43FA-9DFD-8320F0B3BE11@foo.com> CRAP
    static const QRegularExpression format("^<([^<> ]+)>(\\s+.*)?$");
    QRegularExpressionMatch match = format.match(mcBody);
    if (match.hasMatch())
    {
        m_HeaderData["Message-Id"]["id"] = match.captured(1);
        CALL_OUT("");
        return;
    }
    
    // Unknown format
    const QString reason = tr("Unknown format \"%1\"").arg(mcBody);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Mime-Version
void Email::ReadHeader_MimeVersion(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Mime-Version: 1.0 (1.0)
    m_HeaderData["Mime-Version"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: MSIP Labels
void Email::ReadHeader_MSIPLabels(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // msip_labels:
    //  MSIP_Label_d3d538fd-7cd2-4b8b-bd42-f6ee8cc1e568_Enabled=true;
    //  MSIP_Label_d3d538fd-7cd2-4b8b-bd42-f6ee8cc1e568_SetDate=
    //    2023-01-26T17:45:03Z;
    //  MSIP_Label_d3d538fd-7cd2-4b8b-bd42-f6ee8cc1e568_Method=Standard;
    //  MSIP_Label_d3d538fd-7cd2-4b8b-bd42-f6ee8cc1e568_Name=
    //    d3d538fd-7cd2-4b8b-bd42-f6ee8cc1e568;
    //  MSIP_Label_d3d538fd-7cd2-4b8b-bd42-f6ee8cc1e568_SiteId=
    //    255bd3b3-8412-4e31-a3ec-56916c7ae8c0;
    //  MSIP_Label_d3d538fd-7cd2-4b8b-bd42-f6ee8cc1e568_ActionId=
    //    f4423ae0-372d-4a03-b253-3f513ccf1fc9;
    //  MSIP_Label_d3d538fd-7cd2-4b8b-bd42-f6ee8cc1e568_ContentBits=0
    m_HeaderData["MSIPLabels"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Newgroups
void Email::ReadHeader_Newsgroups(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Newsgroups: rec.photo.technique.nature
    m_HeaderData["Newsgroups"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: NNTP-Posting-Host
void Email::ReadHeader_NNTPPostingHost(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // NNTP-Posting-Host: 207.11.223.33
    m_HeaderData["NNTP-Posting-Host"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Non-Standard Tag Header
void Email::ReadHeader_NonStandardTagHeader(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // non_standard_tag_header:
    m_HeaderData["Non-Standard Tag Header"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Old-Content-Type
void Email::ReadHeader_OldContentType(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Old-Content-Type: text/plain; charset=us-ascii
    m_HeaderData["Old-Content-Type"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Old-Subject
void Email::ReadHeader_OldSubject(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Old-Subject: Silverberg.Net
    m_HeaderData["Old-Subject"]["raw"] = mcBody;
    m_HeaderData["Old-Subject"]["subject"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Organization
void Email::ReadHeader_Organization(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Organization: foo.bar
    m_HeaderData["Organization"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Originator
void Email::ReadHeader_Originator(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Originator: announce@list.foo.com
    m_HeaderData["Originator"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Orig-To
void Email::ReadHeader_OrigTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Originator: Orig-To: foo@bar.de
    m_HeaderData["Orig-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Posted-Date
void Email::ReadHeader_PostedDate(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Posted-Date: Sat, 7 Dec 1996 10:15:23 +0100 (MET)
    m_HeaderData["Posted-Date"] = ParseDate(mcBody);
    m_HeaderData["Posted-Date"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: PP-Correlation-ID
void Email::ReadHeader_PPCorrelationID(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // pp-correlation-id: f57384940b898
    m_HeaderData["PP-Correlation-ID"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: PP-To-MDO-Migrated
void Email::ReadHeader_PPToMDOMigrated(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // pp-to-mdo-migrated: FirstRulePass
    m_HeaderData["PP-To-MDO-Migrated"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Precedence
void Email::ReadHeader_Precedence(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Precedence: bulk
    m_HeaderData["Precedence"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Priority
void Email::ReadHeader_Priority(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Priority: normal
    m_HeaderData["Priority"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: RCPT_Domain
void Email::ReadHeader_RCPTDomain(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // rcpt_domain: gmail.com
    m_HeaderData["RCPT-Domain"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Received
void Email::ReadHeader_Received(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Usually, there's more than one of these.
    
    // Received: from [10.235.171.182] (unknown [205.197.242.183])
	//    (using TLSv1.2 with cipher ECDHE-RSA-AES256-GCM-SHA384 (256/256 bits))
	//    (No client certificate requested)
	//    by tigress.com (Postfix) with ESMTPSA id 79023C009B;
	//    Tue, 10 Jan 2017 19:28:58 +0100 (CET)
	QHash < QString, QString > data;
	data["raw"] = mcBody;
    m_HeaderData_Received << data;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Received-Date
void Email::ReadHeader_ReceivedDate(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Received-Date: Fri, 12 Apr 1996 09:51:43 +0200
    m_HeaderData["Received-Date"] = ParseDate(mcBody);
    m_HeaderData["Received-Date"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Received-SPF
void Email::ReadHeader_ReceivedSPF(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Received-SPF: pass (domain of hotmail.com designates 65.55.90.228 as
    //    permitted sender)
    m_HeaderData["Received-SPF"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Recipient-ID
void Email::ReadHeader_RecipientID(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // recipient-id: 6a4025a3-75d1-4784-a1a3-b4c2066d2bf7
    m_HeaderData["Recipient-ID"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Require-Recipient-Valid-Since
void Email::ReadHeader_RequireRecipientValidSince(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // require-recipient-valid-since:
    //   christian.vontoerne@gmail.com; Thu, 5 Jul 2018 12:03:11 +0000

    m_HeaderData["Require-Recipient-Valid-Since"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: References
void Email::ReadHeader_References(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    m_HeaderData["References"]["raw"] = mcBody;
    
    // May be encoded
    // References: =?iso-8859-1?Q?=3C3EECD190.1040207@foo.com=3E_=3C3EED9E87
    //    .000008.03807?=
    //    =?iso-8859-1?Q?@John_Doe=DF=3E?=
	QString rest = DecodeIfNecessary(mcBody);

	// May have several lines
	rest.replace("\n\t", " ");
    
    // References: <4B8963481CE5BC42A7976056069818E4A308B13F@NCEXMBX04.foo
    //    bar.com>
    static const QRegularExpression format("^[^<>]*(<[^<>]+>)\\s*(\\S.*)?$");
    // Part: I've seen something line <bla@foo bar> (with a space)
    static const QRegularExpression format_part("^<([^<>]+)>$");
    while (true)
    {
        QRegularExpressionMatch match = format.match(rest);
        if (!match.hasMatch())
        {
            break;
        }
        const QString part = match.captured(1);
        QRegularExpressionMatch match_part = format_part.match(part);
        if (match_part.hasMatch())
        {
            QHash < QString, QString > reference;
            reference["raw"] = part;
            reference["id"] = match_part.captured(1);
            m_HeaderData_References << reference;
            rest = match.captured(2);
        } else
        {
            // Unknown format
            const QString reason = tr("Unknown part format \"%1\"").arg(part);
            MessageLogger::Error(CALL_METHOD, reason);
            rest = QString();
        }
    }
    
    // Unknown format
    if (!rest.isEmpty())
    {
        const QString reason = tr("Unknown format \"%1\"").arg(mcBody);
        MessageLogger::Error(CALL_METHOD, reason);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Reply-To
void Email::ReadHeader_ReplyTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Reply-To: no.reply@xing.com
    m_HeaderData["Reply-To"] = ParseEmailAddress(mcBody);
    m_HeaderData["Reply-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Resent-Cc
void Email::ReadHeader_ResentCc(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Resent-Date: Foo bar <foo@bar.com>
    m_HeaderData["Resent-Cc"] = ParseEmailAddress(mcBody);
    m_HeaderData["Resent-Cc"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Resent-Date
void Email::ReadHeader_ResentDate(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Resent-Date: various formats
    m_HeaderData["Resent-Date"] = ParseDate(mcBody);
    m_HeaderData["Resent-Date"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Resent-From
void Email::ReadHeader_ResentFrom(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Resent-From: Foo bar <foo@bar.com>
    m_HeaderData["Resent-From"] = ParseEmailAddress(mcBody);
    m_HeaderData["Resent-From"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Resent-Message-Id
void Email::ReadHeader_ResentMessageId(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    m_HeaderData["Resent-Message-Id"]["raw"] = mcBody;

    // Resent-Message-Id: <0EA1829A-500F-43FA-9DFD-8320F0B3BE11@foo.com>
    static const QRegularExpression format("^<([^<> ]+)>$");
    QRegularExpressionMatch match = format.match(mcBody);
    if (match.hasMatch())
    {
        m_HeaderData["Resent-Message-Id"]["id"] = match.captured(1);
        CALL_OUT("");
        return;
    }

    // Unknown format
    const QString reason = tr("Unknown format \"%1\"").arg(mcBody);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Resent-Reply-To
void Email::ReadHeader_ResentReplyTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Resent-Reply-To: foo@bar.com
    m_HeaderData["Resent-Reply-To"] = ParseEmailAddress(mcBody);
    m_HeaderData["Resent-Reply-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Resent-Sender
void Email::ReadHeader_ResentSender(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Resent-Sender: Ircle-Announce-request@ircle.com
    m_HeaderData["Resent-Sender"] = ParseEmailAddress(mcBody);
    m_HeaderData["Resent-Sender"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Resent-To
void Email::ReadHeader_ResentTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Resent-To: cocoa-dev@lists.apple.com
    m_HeaderData["Resent-To"] = ParseEmailAddress(mcBody);
    m_HeaderData["Resent-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Return-Path
void Email::ReadHeader_ReturnPath(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Return-Path: <john.doe@foo.com>
    m_HeaderData["Return-Path"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Return-Receipt-To
void Email::ReadHeader_ReturnReceiptTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Return-Receipt-To:
    m_HeaderData["Return-Receipt-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Saved-From-Email
void Email::ReadHeader_SavedFromEmail(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // savedfromemail: foo@bar.de
    m_HeaderData["Saved-From-Email"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Sender
void Email::ReadHeader_Sender(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Sender: Google Calendar <calendar-notification@google.com>
    m_HeaderData["Sender"] = ParseEmailAddress(mcBody);
    m_HeaderData["Sender"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Sensitivity
void Email::ReadHeader_Sensitivity(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Sensitivity:
    m_HeaderData["Sensitivity"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Sent on
void Email::ReadHeader_SentOn(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // sent-on: (date) (time)
    m_HeaderData["Sent-On"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Site-ID
void Email::ReadHeader_SiteID(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // site-id: 4
    m_HeaderData["Site-ID"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: spamdiagnosticmetadata
void Email::ReadHeader_SpamDiagnosticMetadata(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // spamdiagnosticmetadata: NSPM
    m_HeaderData["spamdiagnosticmetadata"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: spamdiagnosticoutput
void Email::ReadHeader_SpamDiagnosticOutput(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // spamdiagnosticoutput: 1:99
    m_HeaderData["spamdiagnosticoutput"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Status
void Email::ReadHeader_Status(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Status: RO
    m_HeaderData["Status"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Subject
void Email::ReadHeader_Subject(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Subject: Receipe
    m_HeaderData["Subject"]["raw"] = mcBody;
    QString subject = DecodeIfNecessary(mcBody);
    subject.replace("\n", " ");
    subject.replace("\r", " ");
    subject.replace("\t", " ");
    subject = subject.simplified();
    m_HeaderData["Subject"]["subject"] = subject;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: suggested_attachment_session_id
void Email::ReadHeader_SuggestedAttachmentSessionID(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // suggested_attachment_session_id: 1149cd0e-9b5a-7202-9bb3-80f033cf5c31
    m_HeaderData["suggested_attachment_session_id"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Thread-Index
void Email::ReadHeader_ThreadIndex(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Thread-Index: AdJcoOjyZqkgDFbdQPWpv87m1q/21QAAOnwQ
    m_HeaderData["Thread-Index"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Thread-Topic
void Email::ReadHeader_ThreadTopic(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Thread-Topic: Rain
    m_HeaderData["Thread-Topic"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: To
void Email::ReadHeader_To(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // To: unlisted-recipients: ;(no To-header on input)
    if (mcBody.startsWith("unlisted-receipients"))
    {
        m_HeaderData["To"]["raw"] = mcBody;
        CALL_OUT("");
        return;
    }
    
    // To: Doe John <john.doe@foo.com>
    m_HeaderData_To = ParseEmailAddressList(mcBody);
    m_HeaderData["To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: UI Outbound Report
void Email::ReadHeader_UIOutboundReport(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // i-outboundreport:
    //   notjunk:1;M01:P0:Zi93XTTA0vc=;HNE9osKS5q6tSlYi/MMlm4eFib8
    //   N5r1/nsCYTdgxdP5nd5r7Og4dRihWNSBZSNK29HAtAyloK3cCjpC9l5nA
    //   RAOzwPvPTBbsB+bS
    //   ...
    //   /6iFaZBPk2ReXEhhQ8j/NJ7ZnlM=
    m_HeaderData["UI-UIOutboundReport"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: User-Agent
void Email::ReadHeader_UserAgent(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // User-Agent: Mutt/1.2.5.1i
    m_HeaderData["User-Agent"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: Warnings-To
void Email::ReadHeader_WarningsTo(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Warnings-To: tlk-l-error@nwoca.ohio.gov
    m_HeaderData["Warnings-To"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Email header: X-Mailer
void Email::ReadHeader_XMailer(const QString mcBody)
{
    CALL_IN(QString("mcBody=%1")
        .arg(CALL_SHOW(mcBody)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // X-Mailer: iPhone Mail (14C92)
    m_HeaderData["X-Mailer"]["raw"] = mcBody;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Helper functions
QHash < QString, QString > Email::ParseEmailAddress(QString mAddress)
{
    CALL_IN(QString("mAddress=%1")
        .arg(CALL_SHOW(mAddress)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Return value
    QHash < QString, QString > ret;

    // Decode encoded text if necessary    
    mAddress = DecodeIfNecessary(mAddress);

    // foo@bar.com
    static const QRegularExpression format_1("^([^<>, ]*)$");
    QRegularExpressionMatch match_1 = format_1.match(mAddress);
    if (match_1.hasMatch())
    {
        ret["email"] = match_1.captured(1).trimmed();
        CALL_OUT("");
        return ret;
    }
    
    // <foo@bar.com>
    static const QRegularExpression format_2("^<(.*)>$");
    QRegularExpressionMatch match_2 = format_2.match(mAddress);
    if (match_2.hasMatch())
    {
        ret["email"] = match_2.captured(1).trimmed();
        CALL_OUT("");
        return ret;
    }
    
    // snafu <foo@bar.com>
    // snafu<foo@bar.com>
    static const QRegularExpression format_3("^([^,\"]*[^,\" ])\\s*<(.*)>$");
    QRegularExpressionMatch match_3 = format_3.match(mAddress);
    if (match_3.hasMatch())
    {
        ret["full name"] = match_3.captured(1).trimmed();
        ret["email"] = match_3.captured(2).trimmed();
        CALL_OUT("");
        return ret;
    }
    
    // "last, first" <foo@bar.com>
    static const QRegularExpression format_4(
        "^\"([^,]+),\\s+(\\S[^,]*)\"\\s+<(.*)>$");
    QRegularExpressionMatch match_4 = format_4.match(mAddress);
    if (match_4.hasMatch())
    {
        ret["last name"] = match_4.captured(1).trimmed();
        ret["first name"] = match_4.captured(2).trimmed();
        ret["full name"] = QString("%1 %2")
            .arg(match_4.captured(2),
                 match_4.captured(1));
        ret["email"] = match_4.captured(3);
        CALL_OUT("");
        return ret;
    }
    
    // "foo bar"
	//     <foo@bar.com>
    static const QRegularExpression format_5("^\"([^\",]+)\"\\s+<(.*)>$");
    QRegularExpressionMatch match_5 = format_5.match(mAddress);
    if (match_5.hasMatch())
    {
        ret["full name"] = match_5.captured(1).trimmed();
        ret["email"] = match_5.captured(2).trimmed();
        CALL_OUT("");
        return ret;
    }
    
    // foo bar foo@bar.com
    static const QRegularExpression format_6("^(.+)\\s+(\\S+@\\S+)$");
    QRegularExpressionMatch match_6 = format_6.match(mAddress);
    if (match_6.hasMatch())
    {
        ret["full name"] = match_6.captured(1).trimmed();
        ret["email"] = match_6.captured(2).trimmed();
        CALL_OUT("");
        return ret;
    }
    
    // john.doe@foo.com (john.doe)
    static const QRegularExpression format_7(
        "^([^\\s]+@[^\\s]+)\\s+\\((.+)\\)$");
    QRegularExpressionMatch match_7 = format_7.match(mAddress);
    if (match_7.hasMatch())
    {
        ret["full name"] = match_7.captured(2).trimmed();
        ret["email"] = match_7.captured(1).trimmed();
        CALL_OUT("");
        return ret;
    }
    
    // Emails on the local system (e.g sent per Unix shell command)
    // foo (Foo Bar)
    static const QRegularExpression format_8("^([^\\(]+\\S)\\s+\\((.+)\\)$");
    QRegularExpressionMatch match_8 = format_8.match(mAddress);
    if (match_8.hasMatch())
    {
        ret["full name"] = match_8.captured(2).trimmed();
        ret["email"] = QString("%1@localhost")
            .arg(match_8.captured(1).trimmed());
        CALL_OUT("");
        return ret;
    }
    
    // Missing email address with name provided
    // "last, first"
    static const QRegularExpression format_9("^\"([^,]+),\\s+(\\S[^,]*)\"$");
    QRegularExpressionMatch match_9 = format_9.match(mAddress);
    if (match_9.hasMatch())
    {
        ret["last name"] = match_9.captured(1).trimmed();
        ret["first name"] = match_9.captured(2).trimmed();
        ret["full name"] = QString("%1 %2")
            .arg(match_9.captured(2),
                 match_9.captured(1));
        ret["email"] = QString();
        CALL_OUT("");
        return ret;
    }

    // Suppressed email addresses
    const QString address_lower = mAddress.toLower();
    if (address_lower.contains("recipient list suppressed"))
    {
        ret["full name"] = tr("Suppressed recipients");
        CALL_OUT("");
        return ret;
    }
    if (address_lower.contains("unlisted-recipients"))
    {
        ret["full name"] = tr("Unlisted recipients");
        CALL_OUT("");
        return ret;
    }
    if (address_lower.contains("recipient list not shown"))
    {
        ret["full name"] = tr("Recipient list not shown");
        CALL_OUT("");
        return ret;
    }
    if (address_lower.contains("undisclosed recipients") ||
        address_lower.contains("undisclosed-recipients"))
    {
        ret["full name"] = tr("Undisclosed recipients");
        CALL_OUT("");
        return ret;
    }
    if (address_lower.contains("whom it may concern"))
    {
        ret["full name"] = tr("Whom it may concern");
        CALL_OUT("");
        return ret;
    }
    
    // Unknown format
    const QString reason =
        tr("Unknown email address format \"%1\".").arg(mAddress);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Helper function: Parse a list of email addresses
QList < QHash < QString, QString > > Email::ParseEmailAddressList(
    const QString mcAddressList)
{
    CALL_IN(QString("mcAddressList=%1")
        .arg(CALL_SHOW(mcAddressList)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Return value
    QList < QHash < QString, QString > > ret;

    // There may be escaped quotation marks (\")
    QString rest = mcAddressList;
    rest.replace("\\\"", "&quot;");
    
    // Decode encoded text if necessary    
    rest = DecodeIfNecessary(rest);
    
    // "last, first" <foo@bar.com>, ...
    while (!rest.isEmpty())
    {
        static const QRegularExpression format_list1(
            "^(\"[^\"]+\"\\s*<[^>]+>)(,\\s*(\\S.*))?$");
        QRegularExpressionMatch match_list1 = format_list1.match(rest);
        if (match_list1.hasMatch())
        {
            ret << ParseEmailAddress(match_list1.captured(1));
            rest = match_list1.captured(3);
        } else
        {
            // name <foo@bar.com>, ...
            // foo@bar.com
            static const QRegularExpression format_list2(
                "^([^\",]+)(,\\s*(\\S.*))?$");
            QRegularExpressionMatch match_list2 = format_list2.match(rest);
            if (match_list2.hasMatch())
            {
                ret << ParseEmailAddress(match_list2.captured(1));
                rest = match_list2.captured(3);
            } else
            {
                // Some invalid email address (name with no email address)
                static const QRegularExpression format_list3(
                    "^(\"[^<\"]+\")(,\\s*(\\S.*))?$");
                QRegularExpressionMatch match_list3 = format_list3.match(rest);
                if (match_list3.hasMatch())
                {
                    ret << ParseEmailAddress(match_list3.captured(1));
                    rest = match_list3.captured(3);
                } else
                {
                    // Not sure.
                    const QString reason =
                        tr("Unknown email address list format \"%1\".")
                            .arg(rest);
                    MessageLogger::Error(CALL_METHOD, reason);
                    CALL_OUT(reason);
                    return ret;
                }
            }
        }
    }
    
    // "Unescape" quotation marks
    for (int idx = 0; idx < ret.size(); idx++)
    {
        for (auto key_iterator = ret[idx].keyBegin();
             key_iterator != ret[idx].keyEnd();
             key_iterator++)
        {
            const QString key = *key_iterator;
            ret[idx][key].replace("&quot;", "\"");
        }
    }
    
    // Done
    CALL_OUT("");
    return ret;
}


///////////////////////////////////////////////////////////////////////////////
// Parse date
QHash < QString, QString > Email::ParseDate(const QString mcDate)
{
    CALL_IN(QString("mcDate=%1")
        .arg(CALL_SHOW(mcDate)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Translate month name
    static QHash < QString, QString > month_en;
    if (month_en.isEmpty())
    {
        month_en["jan"] = "01";
        month_en["feb"] = "02";
        month_en["mar"] = "03";
        month_en["apr"] = "04";
        month_en["may"] = "05";
        month_en["jun"] = "06";
        month_en["jul"] = "07";
        month_en["aug"] = "08";
        month_en["sep"] = "09";
        month_en["oct"] = "10";
        month_en["nov"] = "11";
        month_en["dec"] = "12";
    }
    
    // Translate time zones to UTC
    static QHash < QString, int > timezone_to_utc;
    if (timezone_to_utc.isEmpty())
    {
        timezone_to_utc["CDT"] = -(5 * 60 + 0);
        timezone_to_utc["CEST"] = +(2 * 60 + 0);
        timezone_to_utc["CET"] = +(1 * 60 + 0);
        timezone_to_utc["CST"] = -(6 * 60 + 0);
        timezone_to_utc["EDT"] = -(4 * 60 + 0);
        timezone_to_utc["EET"] = +(2 * 60 + 0);
        timezone_to_utc["EET DST"] = +(3 * 60 + 0);
        timezone_to_utc["EST"] = -(5 * 60 + 0);
        timezone_to_utc["GMT"] = -(0 * 60 + 0);
        timezone_to_utc["MESZ"] = +(2 * 60 + 0);
        timezone_to_utc["MET DST"] = +(2 * 60 + 0);
        timezone_to_utc["MET"] = +(1 * 60 + 0);
        timezone_to_utc["MEZ"] = +(1 * 60 + 0);
        timezone_to_utc["PDT"] = -(7 * 60 + 0);
        timezone_to_utc["PST"] = -(8 * 60 + 0);
    }
    
    // Trim date
    const QString date = mcDate.trimmed();
    
    // Return value
    QHash < QString, QString > ret;

    // The date/time details
    QString day;
    QString month;
    int year = 0;
    QString time;
    QString timezone_shift;
    QString timezone_name;
    
    while (true)
    {
        // Try normal date formats
        // Date: Tue, 10 Jan 2017 10:28:56 -0800
        // Date: 13 Jun 2003 16:49:20 -0400
        // Date: Wed, 20 Aug 2003 9:44:19 +0200
        static const QRegularExpression format_date1("^(...,?\\s+)?"
            "(0?[1-9]|[12][0-9]|3[01])"
            "\\s+(Jan|JAN|Feb|FEB|Mar|MAR|Apr|APR|May|MAY|Jun|JUN|"
                "Jul|JUL|Aug|AUG|Sep|SEP|Oct|OCT|Nov|NOV|Dec|DEC)"
            "\\s+([0-9]{2,4}),?"
            "\\s+(([0-9]|[01][0-9]|2[0-3]):[0-5][0-9](:[0-5][0-9])?)"
            "(\\s+((\\+|-)[0-9]{4}))?"
            "(\\s+\\(?([A-Z ]+|\\?\\?\\?)\\)?)?"
            "(\\s+((\\+|-)?[0-9]{4}))?"
            ".*$");

        QRegularExpressionMatch match_date1 = format_date1.match(date);
        if (match_date1.hasMatch())
        {
            day = ("0" + match_date1.captured(2)).right(2);
            month = month_en[match_date1.captured(3).toLower()];
            year = match_date1.captured(4).toInt();
            if (year >=0 && year <= 9)
            {
                // Short form of the 2000's
                year += 2000;
            } else if (year >=80 && year <= 99)
            {
                // Short form of the 1970-1990's
                year += 1900;
            }
            time = match_date1.captured(5);
            if (time.count(":") == 1)
            {
                // Missing seconds
                time += ":00";
            }
            // Make sure we have two digits for hours
            time = ("0" + time).right(8);
            timezone_shift = match_date1.captured(9).isEmpty() ?
                match_date1.captured(14) : match_date1.captured(9);
            timezone_name = match_date1.captured(11);
            if (timezone_name == "???")
            {
                // Unknown timezone name
                timezone_name = "";
            }
            break;
        }
        
        // Dat: Mon, Aug 26 1996 14:16:01 MDT
        static const QRegularExpression format_date2("^(...,?\\s+)?"
            "(Jan|JAN|Feb|FEB|Mar|MAR|Apr|APR|May|MAY|Jun|JUN|"
                "Jul|JUL|Aug|AUG|Sep|SEP|Oct|OCT|Nov|NOV|Dec|DEC)"
            "\\s+(0?[1-9]|[12][0-9]|3[01])"
            "\\s+([0-9]{2,4})"
            "\\s+(([01][0-9]|2[0-3]|[0-9]):[0-5][0-9](:[0-5][0-9])?)"
            "(\\s+((\\+|-)?[0-9]{4}))?"
            "(\\s+\\(?([A-Z ]+|\\?\\?\\?)\\)?)?"
            "(\\s+((\\+|-)?[0-9]{4}))?"
            ".*$");
        QRegularExpressionMatch match_date2 = format_date2.match(date);
        if (match_date2.hasMatch())
        {
            day = ("0" + match_date2.captured(3)).right(2);
            month = month_en[match_date2.captured(2).toLower()];
            year = match_date2.captured(4).toInt();
            if (year >=0 && year <= 9)
            {
                // Short form of the 2000's
                year += 2000;
            } else if (year >=80 && year <= 99)
            {
                // Short form of the 1970-1990's
                year += 1900;
            }
            time = match_date2.captured(5);
            if (time.count(":") == 1)
            {
                // Missing seconds
                time += ":00";
            }
            // Make sure we have two digits for hours
            time = ("0" + time).right(8);
            timezone_shift = match_date2.captured(8);
            timezone_shift = match_date2.captured(8).isEmpty() ?
                match_date2.captured(14) : match_date2.captured(8);
            if (timezone_name == "???")
            {
                // Unknown timezone name
                timezone_name = "";
            }
            break;
        }

        // Date: 25.11.2003
        static const QRegularExpression format_date3(
            "^(0[1-9]|[12][0-9]|3[01])\\."
            "(0[0-9]|1[0-2])\\."
            "([0-9]{4})$");
        QRegularExpressionMatch match_date3 = format_date3.match(date);
        if (match_date3.hasMatch())
        {
            day = match_date3.captured(1);
            month = match_date3.captured(2);
            year = match_date3.captured(3).toInt();
            time = "12:00:00";
            break;
        }
        
        // Date: 03 Dec 96
        static const QRegularExpression format_date4(
            "^(0[1-9]|[12][0-9]|3[01])\\s+"
            "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\\s+"
            "([0-9]{2,4})$");
        QRegularExpressionMatch match_date4 = format_date4.match(date);
        if (match_date4.hasMatch())
        {
            day = match_date4.captured(1);
            month = month_en[match_date4.captured(2)];
            year = match_date4.captured(3).toInt();
            if (year >=0 && year <= 9)
            {
                // Short form of the 2000's
                year += 2000;
            } else if (year >=80 && year <= 99)
            {
                // Short form of the 1970-1990's
                year += 1900;
            }
            time = "12:00:00";
            break;
        }

        // Error - unknown format
        const QString reason = tr("Unknown date format \"%1\"").arg(mcDate);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }
    
    // Post process UTC date and time
    QDateTime date_time = QDateTime::fromString(QString("%1-%2-%3 %4")
        .arg(QString::number(year),
             month,
             day,
             time),
        "yyyy-MM-dd hh:mm:ss");
    ret["date"] = date_time.toString("yyyy-MM-dd");
    ret["time"] = date_time.toString("hh:mm:ss");
    if (timezone_shift == "+0000")
    {
        timezone_shift = "0000";
    }
    ret["timezone"] = timezone_shift;
    if (!timezone_name.isEmpty())
    {
        ret["timezone name"] = timezone_name;
    }

    // Convert to UTC
    const QString offset = timezone_shift.right(4);
    int delta_min = offset.left(2).toInt() * 60 +
        offset.right(2).toInt();
    if (timezone_shift.left(1) == "-")
    {
        date_time = date_time.addSecs(delta_min * 60);
    } else
    {
        date_time = date_time.addSecs(-delta_min * 60);
    }
    ret["date UTC"] = date_time.toString("yyyy-MM-dd");
    ret["time UTC"] = date_time.toString("hh:mm:ss");

    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Decode string (if necessary)
QString Email::DecodeIfNecessary(const QString mcText) const
{
    CALL_IN(QString("mcText=%1")
        .arg(CALL_SHOW(mcText)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // !!!
    // ISO-8859-1''Hilferuf%20Hunderettung112DTierheim%20Fulda112DH%FCnfeld.pdf
    

    // !!!
    // =?utf-8?B?QVc6IExlYmVuc2xhdWYgQ2hyaXN0aWFuIHZvbiBUw7ZybmU=?=

    // Could be multiple items to decode
    QString text = mcText;
    while (true)
    {
        // === ISO-8859-1
        // Binary
        // =?ISO-8859-1?B?R2VzdWNodDogSFNILVG9ydG32Z2xpY2hrZWl0IExhbmRzaHV0?=
        // andersart|Gaby Gro=?ISO-8859-1?B?3w==?= <gaby@anders-art.de>
        static const QRegularExpression format_split1(
            "^(.*)=\\?(ISO|iso)-8859-1\\?[Bb]\\?([A-Za-z0-9]+)\\?=(.*)$");
        QRegularExpressionMatch match_split1 = format_split1.match(text);
        if (match_split1.hasMatch())
        {
            const QString iso_text = StringHelper::DecodeText(
                QByteArray(match_split1.captured(3).toUtf8()),
                "iso-8859-1",
                "base64");
            text = match_split1.captured(1)
                + iso_text
                + match_split1.captured(4).trimmed();
            continue;
        }

        // Quoted
        // =?ISO-8859-1?Q?Jack_=DC_and_Underworld_-_Presale_Info?=
        static const QRegularExpression format_split2(
            "^(.*)=\\?(ISO-8859-1|iso-8859-1)\\?[Qq]\\?(.*)\\?=(.*)$");
        QRegularExpressionMatch match_split2 = format_split2.match(text);
        if (match_split2.hasMatch())
        {
            QString encoded_text = match_split2.captured(3).replace("_", " ");
            const QString iso_text =
                StringHelper::DecodeText(QByteArray(encoded_text.toLatin1()),
                    "iso-8859-1",
                    "quoted-printable");
            text = match_split2.captured(1)
                + iso_text
                + match_split2.captured(4).trimmed();
            continue;
        }
        
        // === ISO-8859-2
        // Quoted
        // =?iso-8859-2?Q?Kapuv=E1ry_Edina?=">=?iso-8859-2?Q?Kapuv=E1ry_Edina?=
        static const QRegularExpression format_split3(
            "^(.*)=\\?(ISO-8859-2|iso-8859-2)\\?[Qq]\\?(.*)\\?=(.*)$");
        QRegularExpressionMatch match_split3 = format_split3.match(text);
        if (match_split3.hasMatch())
        {
            QString encoded_text = match_split3.captured(3).replace("_", " ");
            const QString iso_text = StringHelper::DecodeText(
                QByteArray(encoded_text.toLatin1()),
                "iso-8859-2",
                "quoted-printable");
            text = match_split3.captured(1)
                + iso_text
                + match_split3.captured(4).trimmed();
            continue;
        }

        // === ISO-8859-15
        // Binary
        // =?ISO-8859-15?B?VGhvbWFzIEf2dHo=?=        
        static const QRegularExpression format_split4(
            "^(.*)=\\?(ISO|iso)-8859-15\\?[Bb]\\?([A-Za-z0-9]+)\\?=(.*)$");
        QRegularExpressionMatch match_split4 = format_split4.match(text);
        if (match_split4.hasMatch())
        {
            const QString iso_text = StringHelper::DecodeText(
                QByteArray(match_split4.captured(3).toUtf8()),
                "iso-8859-15",
                "base64");
            text = match_split4.captured(1)
                + iso_text
                + match_split4.captured(4).trimmed();
            continue;
        }

        // === UTF-8
        // Binary
        // =?UTF-8?B?RHIuIENocmlzdGlhbiB2b24gVMO2cm5l?=
        static const QRegularExpression format_split5(
            "^(.*)=\\?(UTF-8|utf-8)\\?[Bb]\\?([A-Za-z0-9]+)\\?=(.*)$");
        QRegularExpressionMatch match_split5 = format_split5.match(text);
        if (match_split5.hasMatch())
        {
            const QString iso_text = StringHelper::DecodeText(
                QByteArray(match_split5.captured(3).toUtf8()),
                "utf-8",
                "base64");
            text = match_split5.captured(1)
                + iso_text
                + match_split5.captured(4).trimmed();
            continue;
        }
        
        // Quoted
        // =?utf-8?Q?Foo=20Bar=20Something?=
        static const QRegularExpression format_split6(
            "^(.*)=\\?(UTF-8|utf-8)\\?[Qq]\\?(.*)\\?=(.*)$");
        QRegularExpressionMatch match_split6 = format_split6.match(text);
        if (match_split6.hasMatch())
        {
            QString encoded_text = match_split6.captured(3).replace("_", " ");
            const QString iso_text = StringHelper::DecodeText(
                QByteArray(encoded_text.toLatin1()),
                "utf-8",
                "quoted-printable");
            text = match_split6.captured(1)
                + iso_text
                + match_split6.captured(4).trimmed();
            continue;
        }
        
        // === Windows-1252
        // Quoted
        // =?windows-1252?Q?Bla_=DF?= <twe@whitenet.de>
        static const QRegularExpression format_split7(
            "^(.*)=\\?([wW]indows-1252)\\?[Qq]\\?(.*)\\?=(.*)$");
        QRegularExpressionMatch match_split7 = format_split7.match(text);
        if (match_split7.hasMatch())
        {
            QString encoded_text = match_split7.captured(3).replace("_", " ");
            const QString iso_text = StringHelper::DecodeText(
                QByteArray(encoded_text.toLatin1()),
                "windows-1252",
                "quoted-printable");
            text = match_split7.captured(1)
                + iso_text
                + match_split7.captured(4).trimmed();
            continue;
        }

        // Base64 encoding with no indication (Outlook for Mac does that)
        // UmU6IEV4Y2VsIFNwcmVhZHNoZWV0IE1hbmFnZW1lbnQ
        static const QRegularExpression format_split8("^([a-zA-Z0-9]+)$");
        QRegularExpressionMatch match_split8 = format_split8.match(text);
        if (match_split8.hasMatch())
        {
            const QByteArray decoded_text =
                QByteArray::fromBase64(text.toLatin1());
            if (decoded_text.isValidUtf8())
            {
                text = decoded_text;
                // !!! Never nested
                break;
            }
        }

        // If nothing matches, we're done.
        break;
    }

    // Done
    CALL_OUT("");
    return text;
}



///////////////////////////////////////////////////////////////////////////////
// Body
void Email::ReadBody(NavigatedTextFile & mrEmailFile)
{
    CALL_IN("mrEmailFile=...");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (DEBUG)
    {
        qDebug().noquote() << tr("Line %1: Email body start")
            .arg(mrEmailFile.GetCurrentLineNumber());
    }
    
    // Header data for part
    QHash < QString, QString > header_data;
    header_data["content-type"] = m_HeaderData["Content-Type"]["type"];
    if (m_HeaderData["Content-Type"].contains("boundary"))
    {
        header_data["boundary"] = m_HeaderData["Content-Type"]["boundary"];
    }
    if (m_HeaderData["Content-Type"].contains("charset"))
    {
        header_data["charset"] = m_HeaderData["Content-Type"]["charset"];
    }
    if (m_HeaderData.contains("Content-Transfer-Encoding"))
    {
        header_data["transfer-encoding"] =
            m_HeaderData["Content-Transfer-Encoding"]["encoding"];
    }
    
    if (DEBUG)
    {
        QStringList data;
        for (auto tag_iterator = header_data.keyBegin();
             tag_iterator != header_data.keyEnd();
             tag_iterator++)
        {
            const QString tag = *tag_iterator;
            data << QString("%1: \"%2\"")
                .arg(tag,
                     header_data[tag]);
        }
        std::sort(data.begin(), data.end());
        qDebug().noquote() << tr("Line %1: Header data: %2")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 data.join(", "));
    }

    // For multipart, we may need to skip some junk lines before the fist part
    if (header_data.contains("content-type") &&
        header_data["content-type"].startsWith("multipart"))
    {
        ReadBody_Multipart(mrEmailFile, header_data, -1);
    } else
    {
        // Read part
        QHash < QString, QString > empty_parent_header;
        ReadBody_Part(mrEmailFile, empty_parent_header, header_data, -1);
    }
    
    // Check for next email if that's necessary
    if (!m_IsMBox ||
        mrEmailFile.AtEnd())
    {
        // If it's not an mbox, we don't expect a next email
        // If we're at the end of the file, there simply is no next email
        CALL_OUT("");
        return;
    }
    
    // Search for the start of the next email
    QString line;
    while (!mrEmailFile.AtEnd() &&
        !line.startsWith("From "))
    {
        line = mrEmailFile.ReadLine();
    }
    if (!mrEmailFile.AtEnd())
    {
        mrEmailFile.Rewind(1);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Body
void Email::ReadBody_Part(NavigatedTextFile & mrEmailFile,
    const QHash < QString, QString > mcParentPartHeader,
    const QHash < QString, QString > mcPartHeader, const int mcParentId)
{
    CALL_IN(QString("mrEmailFile=..., mcParentPartHeader=%1, "
        "mcPartHeader=%2, mcParentId=%3")
        .arg(CALL_SHOW(mcParentPartHeader),
             CALL_SHOW(mcPartHeader),
             CALL_SHOW(mcParentId)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (DEBUG)
    {
        QStringList data;
        for (auto tag_iterator = mcPartHeader.keyBegin();
             tag_iterator != mcPartHeader.keyEnd();
             tag_iterator++)
        {
            const QString tag = *tag_iterator;
            if (tag != "content-type")
            {
                data << QString("%1: \"%2\"")
                    .arg(tag,
                         mcPartHeader[tag]);
            }
        }
        std::sort(data.begin(), data.end());
        qDebug().noquote() << tr("Line %1: Reading %2 part (%3); parent ID %4")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 mcPartHeader["content-type"],
                 data.join(", "),
                 QString::number(mcParentId));
        data.clear();
        for (auto tag_iterator = mcParentPartHeader.keyBegin();
             tag_iterator != mcParentPartHeader.keyEnd();
             tag_iterator++)
        {
            data << QString("%1: \"%2\"")
                .arg(*tag_iterator,
                     mcParentPartHeader[*tag_iterator]);
        }
        std::sort(data.begin(), data.end());
        qDebug().noquote() << tr("Line %1: (Parent: %2)")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 data.join(", "));
    }

    // We'll need this quite a bit
    QString line;
    
    // Assuming we're on the part start
    
    // Check for unexpected end of file
    if (mrEmailFile.AtEnd())
    {
        m_Error = tr("Unexpected end of file reading a %1 part in line %2.")
            .arg(mcPartHeader["content-type"],
                 QString::number(mrEmailFile.GetCurrentLineNumber()));
        m_ErrorLine = mrEmailFile.GetCurrentLineNumber();
        MessageLogger::Error(CALL_METHOD, m_Error);
        CALL_OUT(m_Error);
        return;
    }
    
    // Check if we're on a new email instead
    // (fallback if parts are not correctly terminated)
    line = mrEmailFile.ReadLine();
    mrEmailFile.Rewind(1);
    if (m_IsMBox && line.startsWith("From "))
    {
        // Next email in mbox
        m_Error = tr("%1 part unexpectedly ended by new email in line %2.")
            .arg(mcPartHeader["content-type"],
                 QString::number(mrEmailFile.GetCurrentLineNumber()));
        m_ErrorLine = mrEmailFile.GetCurrentLineNumber();
        MessageLogger::Error(CALL_METHOD, m_Error);
        CALL_OUT(m_Error);
        return;
    }
    
    // Read part
    static QSet < QString > simple_types;
    if (simple_types.isEmpty())
    {
        simple_types
           << "application/applefile"
           << "application/ics"
           << "application/mac-binhex40"
           << "application/ms-tnef"
           << "application/msexcel"
           << "application/msword"
           << "application/octet-stream"
           << "application/pkcs7-mime"
           << "application/pkcs7-signature"
           << "application/pdf"
           << "application/pgp"
           << "application/pgp-encrypted"
           << "application/pgp-signature"
           << "application/postscript"
           << "application/rtf"
           << "application/vnd.ms-excel"
           << "application/vnd.ms-excel.sheet.binary.macroenabled.12"
           << "application/vnd.ms-excel.sheet.macroenabled.12"
           << "application/vnd.ms-powerpoint"
           << "application/vnd.openxmlformats-officedocument.presentationml.presentation"
           << "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
           << "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
           << "application/vnd.openxmlformats-officedocument.wordprocessingml.template"
           << "application/x-dvi"
           << "application/x-gzip"
           << "application/x-macbinary"
           << "application/x-msdownload"
           << "application/x-pdf"
           << "application/x-pkcs7-signature"
           << "application/x-rpm"
           << "application/x-shar"
           << "application/x-stuffit"
           << "application/x-tar"
           << "application/x-tar-gz"
           << "application/x-tex"
           << "application/x-zip-compressed"
           << "application/zip"
           << "audio/mid"
           << "audio/mp3"
           << "audio/mpeg"
           << "audio/x-midi"
           << "audio/x-wav"
           << "image/bmp"
           << "image/gif"
           << "image/heif"
           << "image/jpeg"
           << "image/jpg"
           << "image/pjpeg"
           << "image/png"
           << "image/svg+xml"
           << "image/tiff"
           << "image/vnd.microsoft.icon"
           << "image/x-portable-pixmap"
           << "message/delivery-status"
           << "message/news"
           << "message/rfc822"
           << "text"
           << "text/calendar"
           << "text/csv"
           << "text/english"
           << "text/enriched"
           << "text/html"
           << "text/plain"
           << "text/rtf"
           << "text/rfc822-headers"
           << "text/x-aol"
           << "text/x-csrc"
           << "text/x-gunzip"
           << "text/x-tex"
           << "text/x-vcard"
           << "video/mp4"
           << "video/mpeg"
           << "video/quicktime";
    }
    if (!mcPartHeader.contains("content-type") ||
        mcPartHeader["content-type"].isEmpty())
    {
        QHash < QString, QString > default_header = mcParentPartHeader;
        default_header["content-type"] = "text/plain";
        ReadBody_SavePart(mrEmailFile, mcParentPartHeader, default_header,
            mcParentId);
        CALL_OUT("");
        return;
    } else if (simple_types.contains(mcPartHeader["content-type"]))
    {
        ReadBody_SavePart(mrEmailFile, mcParentPartHeader, mcPartHeader,
            mcParentId);
        CALL_OUT("");
        return;
    } else if (mcPartHeader["content-type"].startsWith("multipart"))
    {
        ReadBody_Multipart(mrEmailFile, mcPartHeader, mcParentId);
        CALL_OUT("");
        return;
    } else
    {
        m_Error = tr("Unknown content type \"%1\"")
            .arg(mcPartHeader["content-type"]);
        m_ErrorLine = mrEmailFile.GetCurrentLineNumber();
        const QString reason = tr("%1 in line %2")
            .arg(m_Error,
                 QString::number(m_ErrorLine));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }

    // We never get here
}



///////////////////////////////////////////////////////////////////////////////
// Body: Read part header
QHash < QString, QString > Email::ReadBody_PartHeader(
    NavigatedTextFile & mrEmailFile)
{
    CALL_IN("mrEmailFile=...");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (DEBUG)
    {
        qDebug().noquote() << tr("Line %1: Start of part header")
            .arg(mrEmailFile.GetCurrentLineNumber());
    }
    
    // Return value
    QHash < QString, QString > ret;

    // Skip separator lines
    QString line;
    while (line.isEmpty())
    {
        line = mrEmailFile.ReadLine();
    }

    // Read header
    static const QRegularExpression format("^([^: ]+):\\s*(\\S.*)?$");
    while (!line.isEmpty())
    {
        // Check for unexpected end of file
        if (mrEmailFile.AtEnd())
        {
            m_Error = tr("Unexpected end of file reading part header");
            m_ErrorLine = mrEmailFile.GetCurrentLineNumber();
            const QString reason = tr("%1 in line %2")
                .arg(m_Error,
                     QString::number(m_ErrorLine));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QHash < QString, QString >();
        }

        // Check if we're on a new email instead
        // (fallback if parts are not correctly terminated)
        line = mrEmailFile.ReadLine();
        mrEmailFile.Rewind(1);
        if (m_IsMBox && line.startsWith("From "))
        {
            // Next email in mbox
            m_Error = tr("Part header unexpectedly ended by new email.");
            m_ErrorLine = mrEmailFile.GetCurrentLineNumber();
            const QString reason = tr("%1 in line %2")
                .arg(m_Error,
                     QString::number(m_ErrorLine));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QHash < QString, QString >();
        }

        // Read header item
        const int item_start_line = mrEmailFile.GetCurrentLineNumber();
        QString item;
        bool first_line = true;
        while (true)
        {
            line = mrEmailFile.ReadLine();
            QRegularExpressionMatch match = format.match(line);
            if (line.isEmpty() ||
                (!first_line && match.hasMatch()))
            {
                break;
            }
            item += line;
            first_line = false;
        } 
        mrEmailFile.Rewind(1);

        // Check for no headers; e.g.
        // --FAA01308.921905482/foo.bar.com
        // **********************************************
        // **      THIS IS A WARNING MESSAGE ONLY      **
        // **  YOU DO NOT NEED TO RESEND YOUR MESSAGE  **
        // **********************************************
        QRegularExpressionMatch match = format.match(item);
        if (!match.hasMatch())
        {
            break;
        }

        // Separate tag and body
        const QString item_tag = match.captured(1).toLower();
        const QString item_body = match.captured(2);
        
        // Interpret item
        if (item_tag == "content-type")
        {
            static const QRegularExpression format_split1(
                "^([^\\s;]+)(;(\\s*(\\S.*))?)?$");
            QRegularExpressionMatch match_split1 =
                format_split1.match(item_body);
            if (!match_split1.hasMatch())
            {
                m_ErrorLine = item_start_line;
                m_Error = tr("Invalid item structure for Content-Type in "
                    "multipart header: \"%1\"").arg(item_body);
                const QString reason = tr("%1 in line %2")
                        .arg(m_Error,
                             QString::number(m_ErrorLine));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return QHash < QString, QString >();
            }
            ret["content-type"] = match_split1.captured(1).toLower();
            if (ret["content-type"] == "unknown/unknown")
            {
                ret["content-type"] = QString();
            }
            QString rest = match_split1.captured(4);

            static const QRegularExpression format_split2(
                "^([^\\s=]+)=(\"[^\"]+\"|[^\\s;\"]+);?\\s*([^\\s;].*)?$");
            QRegularExpressionMatch match_split2 = format_split2.match(rest);
            while (match_split2.hasMatch())
            {
                const QString tag = match_split2.captured(1).toLower();
                const QString value =
                    match_split2.captured(2).replace("\"", "");
                if (tag == "charset")
                {
                    ret["charset"] = value.toLower();
                } else if (tag == "name")
                {
                    ret["name"] = DecodeIfNecessary(value);
                } else if (tag == "boundary" ||
                    tag == "delsp" ||
                    tag == "format" ||
                    tag == "method" ||
                    tag == "x-apple-mail-type" ||
                    tag == "x-apple-part-url" ||
                    tag == "x-mac-creator" ||
                    tag == "x-mac-hide-extension" ||
                    tag == "x-mac-type" ||
                    tag == "x-unix-mode")
                {
                    ret[tag] = value;
                } else if (tag == "type")
                {
                    // Ignore - should be redundant with content-type 
                } else
                {
                    const QString reason =
                        tr("Unknown tag \"%1\" (\"%2\") in Content-Type "
                            "(line %3)")
                            .arg(tag,
                                 value,
                                 QString::number(
                                    mrEmailFile.GetCurrentLineNumber()));
                    MessageLogger::Error(CALL_METHOD, reason);
                }
                rest = match_split2.captured(3);
                match_split2 = format_split2.match(rest);
            }
        } else if (item_tag == "content-transfer-encoding")
        {
            ret["transfer-encoding"] = item_body.toLower();
        } else if (item_tag == "content-disposition")
        {
            static const QRegularExpression format_split1(
                "^([^\\s;]+)(;(\\s*(\\S.*))?)?$");
            QRegularExpressionMatch match_split1 =
                format_split1.match(item_body);
            if (!match_split1.hasMatch())
            {
                m_ErrorLine = item_start_line;
                m_Error = tr("Invalid item structure for Content-Disposition "
                    "in multipart header: \"%1\"").arg(item_body);
                const QString reason = tr("%1 in line %2")
                    .arg(m_Error,
                         QString::number(m_ErrorLine));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return QHash < QString, QString >();
            }
            ret["content-disposition"] = match_split1.captured(1).toLower();
            QString rest = match_split1.captured(4);

            static const QRegularExpression format_split2(
                "^([^\\s=]+)=(\"[^\"]*\"|[^\\s;\"]+);?\\s*([^\\s;].*)?$");
            QRegularExpressionMatch match_split2 = format_split2.match(rest);
            while (match_split2.hasMatch())
            {
                const QString tag = match_split2.captured(1).toLower();
                const QString value =
                    match_split2.captured(2).replace("\"", "");
                if (tag == "filename" ||
                    tag == "filename*")
                {
                    ret["filename"] = DecodeIfNecessary(value);
                } else if (tag == "creation-date" ||
                    tag == "modification-date" ||
                    tag == "size")
                {
                    // Just copy
                    ret[tag] = value;
                } else
                {
                    const QString reason =
                        tr("Unknown tag \"%1\" (\"%2\") in "
                            "Content-Disposition (line %3)")
                            .arg(tag,
                                 value,
                                 QString::number(
                                    mrEmailFile.GetCurrentLineNumber()));
                    MessageLogger::Error(CALL_METHOD, reason);
                }
                rest = match_split2.captured(3);
                match_split2 = format_split2.match(rest);
            }
        } else
        {
            // Just keep it
            ret[item_tag] = item_body;
        }
    }

    if (DEBUG)
    {
        qDebug().noquote() << tr("Line %1: End of part header")
            .arg(mrEmailFile.GetCurrentLineNumber());
    }
    
    // Done
    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Body: Read and save part
void Email::ReadBody_SavePart(NavigatedTextFile & mrEmailFile,
    const QHash < QString, QString > mcParentPartHeader,
    const QHash < QString, QString > mcPartHeader, const int mcParentId)
{
    CALL_IN(QString("mrEmailFile=..., mcParentPartHeader=%1, "
        "mcPartHeader=%2, mcParentId=%3")
        .arg(CALL_SHOW(mcParentPartHeader),
             CALL_SHOW(mcPartHeader),
             CALL_SHOW(mcParentId)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (DEBUG)
    {
        QStringList data;
        for (auto tag_iterator = mcPartHeader.keyBegin();
             tag_iterator != mcPartHeader.keyEnd();
             tag_iterator++)
        {
            QString tag = *tag_iterator;
            if (tag != "content-type")
            {
                data << QString("%1: \"%2\"")
                    .arg(tag,
                         mcPartHeader[tag]);
            }
        }
        std::sort(data.begin(), data.end());
        qDebug().noquote() << tr("Line %1: Start of %2 part (%3); "
            "parent ID %4")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 mcPartHeader["content-type"],
                 data.join(", "),
                 QString::number(mcParentId));
        data.clear();
        for (auto tag_iterator = mcParentPartHeader.keyBegin();
             tag_iterator != mcParentPartHeader.keyEnd();
             tag_iterator++)
        {
            const QString tag = *tag_iterator;
            data << QString("%1: \"%2\"")
                .arg(tag,
                     mcParentPartHeader[tag]);
        }
        std::sort(data.begin(), data.end());
        qDebug().noquote() << tr("Line %1: (Parent: %2)")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 data.join(", "));
    }
    
    QByteArray body;
    while (true)
    {
        // Catch some error modes
        if (mrEmailFile.AtEnd())
        {
            // End of file
            break;
        }
        
        const char * line_raw = mrEmailFile.ReadLine();
        const QString line(line_raw);
        
        // Check for boundary (new start or end)
        if (mcParentPartHeader.contains("boundary"))
        {
            // Part of a multipart
            if (line == QString("--%1").arg(mcParentPartHeader["boundary"]) ||
                line == QString("--%1--").arg(mcParentPartHeader["boundary"]))
            {
                // End of part
                mrEmailFile.Rewind(1);
                break;
            }
        }
        
        // Check for new email (if we're importing an mbox file)
        if (m_IsMBox)
        {
            // MBox file
            if (line.startsWith("From "))
            {
                mrEmailFile.Rewind(1);
                break;
            }
        }
        
        // Check for end of .emlx email
        if (m_IsEMLX)
        {
            // AppleMail .emlx file
            int rewind_to = mrEmailFile.GetCurrentLineNumber();
            if (line == "<?XML version=\"1.0\" encoding=\"UTF-8\"?>")
            {
                // End of email body, start of trailing plist
                QString next_line = mrEmailFile.ReadLine();
                if (next_line.startsWith("<!DOCTYPE plist PUBLIC"))
                {
                    next_line = mrEmailFile.ReadLine();
                    if (next_line == "<plist version=\"1.0\">")
                    {
                        // Skip trailing plist
                        if (DEBUG)
                        {
                            qDebug().noquote() << 
                                tr("Line %1: Skipping trailing plist in EMLX "
                                    "file").arg(rewind_to);
                        }
                        while (true)
                        {
                            // Check for end of plist
                            if (next_line == "</plist>")
                            {
                                // End of .emlx email reached
                                break;
                            }
                            
                            // Check for end of file
                            // (which would be before <plist> is closed)
                            if (mrEmailFile.AtEnd())
                            {
                                // That was unexpected!
                                m_ErrorLine =
                                    mrEmailFile.GetCurrentLineNumber();
                                m_Error = tr("Unexpected end of EMLX file.");
                                break;
                            }
                            
                            // Skip to next line
                            next_line = mrEmailFile.ReadLine();
                        }
                        break;
                    }
                }
            }
            mrEmailFile.MoveTo(rewind_to);
        }
        
        // Other than that - normal data line
        body += line_raw;
        body += '\n';
    }
    
    // Undo text encoding
    const QByteArray decoded = StringHelper::DecodeText(body,
        mcPartHeader["charset"], mcPartHeader["transfer-encoding"]);

    // Store it
    const int this_id = m_BodyData_Part.size();

    // Hook up to parent
    m_BodyData_ChildIds[mcParentId] << this_id;

    // Store data
    m_BodyData_Part << decoded;
    m_BodyData_Type << mcPartHeader["content-type"];
    m_BodyData_ParentId << mcParentId;
    m_BodyData_PartInfo << mcPartHeader;

    // This part has no children for now
    m_BodyData_ChildIds[this_id] = QList < int >();
    if (DEBUG)
    {
        qDebug().noquote() << tr("Line %1: Stored %2 part as ID %3")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 mcPartHeader["content-type"],
                 QString::number(this_id));
        qDebug().noquote() << tr("Line %1: End of %2 part")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 mcPartHeader["content-type"]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Body: Read part - multipart
void Email::ReadBody_Multipart(NavigatedTextFile & mrEmailFile,
    const QHash < QString, QString > mcParentPartHeader, const int mcParentId)
{
    CALL_IN(QString("mrEmailFile=..., mcParentPartHeader=%1, mcParentId=%2")
        .arg(CALL_SHOW(mcParentPartHeader),
             CALL_SHOW(mcParentId)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (DEBUG)
    {
        QStringList data;
        for (auto tag_iterator = mcParentPartHeader.keyBegin();
             tag_iterator != mcParentPartHeader.keyEnd();
             tag_iterator++)
        {
            const QString tag = *tag_iterator;
            if (tag != "content-type")
            {
                data << QString("%1: \"%2\"")
                    .arg(tag,
                         mcParentPartHeader[tag]);
            }
        }
        std::sort(data.begin(), data.end());
        qDebug().noquote() << tr("Line %1: Start of %2 part (%3); "
            "parent ID %4")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 mcParentPartHeader["content-type"],
                 data.join(", "),
                 QString::number(mcParentId));
    }
    
    // No body content - just sub parts
    const int part_id = m_BodyData_Part.size();
    m_BodyData_ChildIds[mcParentId] << part_id;
    m_BodyData_Part << QByteArray();
    m_BodyData_PartInfo << QHash < QString, QString >();
    m_BodyData_Type << mcParentPartHeader["content-type"];
    m_BodyData_ParentId << mcParentId;
    if (DEBUG)
    {
        qDebug().noquote() << tr("Line %1: Stored multipart %2 part as ID %3")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 mcParentPartHeader["content-type"],
                 QString::number(part_id));
    }
    
    // Search for start boundary
    while (true)
    {
        // Catch some error modes
        if (mrEmailFile.AtEnd())
        {
            // End of file - incorrect, there should be sub-parts.
            break;
        }
        
        QString line = mrEmailFile.ReadLine();

        // Skip any heading garbage
        bool end_multipart = false;
        bool end_file = false;
        bool next_email = false;
        while (true)
        {
            // Start of new subpart
            if (line == QString("--%1").arg(mcParentPartHeader["boundary"]))
            {
                mrEmailFile.Rewind(1);
                break;
            }
            
            // Check for end of multipart
            if (line == QString("--%1--").arg(mcParentPartHeader["boundary"]))
            {
                end_multipart = true;
                break;
            }
            
            // Check if at end
            if (mrEmailFile.AtEnd())
            {
                m_Error = tr("End of file reached while reading multipart.");
                m_ErrorLine = mrEmailFile.GetCurrentLineNumber();
                const QString reason =
                    tr("Unexpected end of file in line %1.")
                        .arg(mrEmailFile.GetCurrentLineNumber());
                MessageLogger::Error(CALL_METHOD, reason);
                end_file = true;
                break;
            }

            // Check for start of a new email
            if (m_IsMBox)
            {
                // MBox file
                if (line.startsWith("From "))
                {
                    next_email = true;
                    m_Error = tr("New email starts while reading multipart.");
                    m_ErrorLine = mrEmailFile.GetCurrentLineNumber();
                    const QString reason =
                        tr("Unexpected end of email in line %1.")
                            .arg(mrEmailFile.GetCurrentLineNumber());
                    MessageLogger::Error(CALL_METHOD, reason);
                    mrEmailFile.Rewind(1);
                    break;
                }
            }
            
            // Next line
            line = mrEmailFile.ReadLine();
        }
        
        // Deal with errors
        if (end_file || next_email || end_multipart)
        {
            break;
        }
        
        // Read next part
        QHash < QString, QString > part_info =
            ReadBody_PartHeader(mrEmailFile);
        ReadBody_Part(mrEmailFile, mcParentPartHeader, part_info, part_id);
    }
    
    // Check for trailing plist in .emlx email
    if (m_IsEMLX)
    {
        // AppleMail .emlx file
        int rewind_to = mrEmailFile.GetCurrentLineNumber();
        QString line = mrEmailFile.ReadLine();
        if (line == "<?XML version=\"1.0\" encoding=\"UTF-8\"?>")
        {
            // End of email body, start of trailing plist
            line = mrEmailFile.ReadLine();
            if (line.startsWith("<!DOCTYPE plist PUBLIC"))
            {
                line = mrEmailFile.ReadLine();
                if (line == "<plist version=\"1.0\">")
                {
                    // Skip trailing plist
                    if (DEBUG)
                    {
                        qDebug().noquote() << 
                            tr("Line %1: Skipping trailing plist in EMLX file")
                                .arg(rewind_to);
                    }
                    while (true)
                    {
                        // Check for end of plist
                        if (line == "</plist>")
                        {
                            // End of .emlx email reached
                            break;
                        }
                        
                        // Check for end of file
                        // (which would be before <plist> is closed)
                        if (mrEmailFile.AtEnd())
                        {
                            // That was unexpected!
                            m_ErrorLine = mrEmailFile.GetCurrentLineNumber();
                            m_Error = tr("Unexpected end of EMLX file.");
                            break;
                        }
                            
                        // Skip to next line
                        line = mrEmailFile.ReadLine();
                    }
                    rewind_to = -1;
                }
            }
        }
        
        // Check if this wasn't the plist we were looking for
        if (rewind_to != -1)
        {
            mrEmailFile.MoveTo(rewind_to);
        }
    }
        
    if (DEBUG)
    {
        qDebug().noquote() << tr("Line %1: End of %2 part")
            .arg(QString::number(mrEmailFile.GetCurrentLineNumber()),
                 mcParentPartHeader["content-type"]);
    }

    CALL_OUT("");
}



// ===================================================================== Access



///////////////////////////////////////////////////////////////////////////////
// Filename
QString Email::GetFilename() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_Filename;
}



///////////////////////////////////////////////////////////////////////////////
// Start line number in the source file
int Email::GetStartLineNumber() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_StartLineNumber;
}



///////////////////////////////////////////////////////////////////////////////
// Check if there has been an error
bool Email::HasError() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return !m_Error.isEmpty();
}



///////////////////////////////////////////////////////////////////////////////
// Error (during import)
QString Email::GetError() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_Error;
}



///////////////////////////////////////////////////////////////////////////////
// Error line (during import)
int Email::GetErrorLine() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_ErrorLine;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a certain header item is available
bool Email::HasHeaderItem(const QString mcHeaderItem) const
{
    CALL_IN(QString("mcHeaderItem=%1")
        .arg(CALL_SHOW(mcHeaderItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_HeaderData.contains(mcHeaderItem);
}



///////////////////////////////////////////////////////////////////////////////
// Get list of available header items
QList < QString > Email::GetAvailableHeaderItems() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_HeaderData.keys();
}



///////////////////////////////////////////////////////////////////////////////
// Get a particular header item
QHash < QString, QString > Email::GetHeaderItem(
    const QString mcHeaderItem) const
{
    CALL_IN(QString("mcHeaderItem=%1")
        .arg(CALL_SHOW(mcHeaderItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if header item exists
    if (!m_HeaderData.contains(mcHeaderItem))
    {
        const QString reason =
            tr("Email does not have header item \"%1\".").arg(mcHeaderItem);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }
    
    // Okay
    CALL_OUT("");
    return m_HeaderData[mcHeaderItem];
}



///////////////////////////////////////////////////////////////////////////////
// Check if a certain header item/subitem combination is available
bool Email::HasHeaderItem(const QString mcHeaderItem, 
    const QString mcSubItem) const
{
    CALL_IN(QString("mcHeaderItem=%1, mcSubItem=%2")
        .arg(CALL_SHOW(mcHeaderItem),
             CALL_SHOW(mcSubItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");

    // Check if header item exists
    return m_HeaderData.contains(mcHeaderItem) &&
        m_HeaderData[mcHeaderItem].contains(mcSubItem);
}



///////////////////////////////////////////////////////////////////////////////
// Get a header item/subitem combination
QString Email::GetHeaderItem(const QString mcHeaderItem, 
    const QString mcSubItem) const
{
    CALL_IN(QString("mcHeaderItem=%1, mcSubItem=%2")
        .arg(CALL_SHOW(mcHeaderItem),
             CALL_SHOW(mcSubItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if header item exists
    if (!m_HeaderData.contains(mcHeaderItem))
    {
        qDebug().noquote() << m_HeaderData;
        const QString reason =
            tr("Email does not have header item \"%1\".").arg(mcHeaderItem);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    if (!m_HeaderData[mcHeaderItem].contains(mcSubItem))
    {
        const QString reason =
            tr("Email header item \"%1\" does not have subitem \"%2\".")
                .arg(mcHeaderItem,
                     mcSubItem);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // Okay
    CALL_OUT("");
    return m_HeaderData[mcHeaderItem][mcSubItem];
}



///////////////////////////////////////////////////////////////////////////////
// Get count of "to" addresses
int Email::GetNumberOfToAddresses() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_HeaderData_To.size();
}



///////////////////////////////////////////////////////////////////////////////
// Get a "to" address
QHash < QString, QString > Email::GetToAddress(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 || mcIndex >= m_HeaderData_To.size())
    {
        const QString reason =
            tr("Email does not have a \"to\" item %1 (has %2 only)")
                .arg(QString::number(mcIndex),
                     QString::number(m_HeaderData_To.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }
    
    // Okay
    CALL_OUT("");
    return m_HeaderData_To[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get count of "cc" addresses
int Email::GetNumberOfCcAddresses() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_HeaderData_Cc.size();
}



///////////////////////////////////////////////////////////////////////////////
// Get a "cc" address
QHash < QString, QString > Email::GetCcAddress(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 || mcIndex >= m_HeaderData_Cc.size())
    {
        const QString reason =
            tr("Email does not have a \"to\" item %1 (has %2 only)")
                .arg(QString::number(mcIndex),
                     QString::number(m_HeaderData_Cc.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }
    
    // Okay
    CALL_OUT("");
    return m_HeaderData_Cc[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get count of "bcc" addresses
int Email::GetNumberOfBccAddresses() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_HeaderData_Bcc.size();
}



///////////////////////////////////////////////////////////////////////////////
// Get a "bcc" address
QHash < QString, QString > Email::GetBccAddress(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 || mcIndex >= m_HeaderData_Bcc.size())
    {
        const QString reason =
            tr("Email does not have a \"to\" item %1 (has %2 only)")
                .arg(QString::number(mcIndex),
                     QString::number(m_HeaderData_Bcc.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }
    
    // Okay
    CALL_OUT("");
    return m_HeaderData_Bcc[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get count of references
int Email::GetNumberOfReferences() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_HeaderData_References.size();
}



///////////////////////////////////////////////////////////////////////////////
// Get a reference
QHash < QString, QString > Email::GetReference(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 || mcIndex >= m_HeaderData_References.size())
    {
        const QString reason =
            tr("Email does not have a reference item %1 (has %2 only)")
                .arg(QString::number(mcIndex),
                     QString::number(m_HeaderData.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }
    
    // Okay
    CALL_OUT("");
    return m_HeaderData_References[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get count of "received" items
int Email::GetNumberOfReceived() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_HeaderData_Received.size();
}



///////////////////////////////////////////////////////////////////////////////
// Get a "received" item
QHash < QString, QString > Email::GetReceived(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 || mcIndex >= m_HeaderData_Received.size())
    {
        const QString reason =
            tr("Email does not have a received item %1 (has %2 only)")
                .arg(QString::number(mcIndex),
                     QString::number(m_HeaderData_Received.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }
    
    // Okay
    CALL_OUT("");
    return m_HeaderData_Received[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get number of parts in the body of the email
int Email::GetNumberOfParts() const
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    CALL_OUT("");
    return m_BodyData_Part.size();
}



///////////////////////////////////////////////////////////////////////////////
// Get part of the email
QByteArray Email::GetPart(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 || mcIndex >= m_BodyData_Part.size())
    {
        const QString reason =
            tr("Email does not have a body part %1 (has %2 only)")
                .arg(QString::number(mcIndex),
                     QString::number(m_BodyData_Part.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QByteArray();
    }
    
    // Okay
    CALL_OUT("");
    return m_BodyData_Part[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get part information
QHash < QString, QString > Email::GetPartInfo(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 ||
        mcIndex >= m_BodyData_PartInfo.size())
    {
        const QString reason =
            tr("No information available for part %1")
                .arg(QString::number(mcIndex));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }

    // Okay
    CALL_OUT("");
    return m_BodyData_PartInfo[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get type of the part of the email
QString Email::GetPartType(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 || mcIndex >= m_BodyData_Type.size())
    {
        const QString reason =
            tr("Email does not have a body part %1 (has %2 only)")
                .arg(QString::number(mcIndex),
                     QString::number(m_BodyData_Type.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }
    
    // Okay
    CALL_OUT("");
    return m_BodyData_Type[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get parent index of a part of the email
int Email::GetPartParentId(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (mcIndex < 0 || mcIndex >= m_BodyData_ParentId.size())
    {
        const QString reason =
            tr("Email does not have a body part %1 (has %2 only)")
                .arg(QString::number(mcIndex),
                     QString::number(m_BodyData_ParentId.size()));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }
    
    // Okay
    CALL_OUT("");
    return m_BodyData_ParentId[mcIndex];
}



///////////////////////////////////////////////////////////////////////////////
// Get child IDs of a part of the email
QList < int > Email::GetPartChildIds(const int mcIndex) const
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // Check if index is within bounds
    if (!m_BodyData_ChildIds.contains(mcIndex))
    {
        const QString reason =
            tr("Email does not have a body part %1.").arg(mcIndex);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT("");
        return QList < int >();
    }
    
    // Okay
    CALL_OUT("");
    return m_BodyData_ChildIds[mcIndex];
}



// ======================================================================== XML



///////////////////////////////////////////////////////////////////////////////
// Convert to XML
QString Email::ToXML()
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    // <email>
    //   <header>
    //     ...
    //   </header>
    //   <body>
    //   <part type="...">
    //     ...
    //   </part>
    // </email>
    
    QDomDocument xml("stuff");
    QDomElement dom_email = xml.createElement("email");
    xml.appendChild(dom_email);
    
    // === Header
    QDomElement dom_header = xml.createElement("header");
    dom_email.appendChild(dom_header);
    for (auto type_iterator = m_HeaderData.keyBegin();
         type_iterator != m_HeaderData.keyEnd();
         type_iterator++)
    {
        const QString type = *type_iterator;
        // Ignore some headers that were applied to the content already
        if (type =="Content-Transfer-Encoding")
        {
            continue;
        }
        
        QString tag(type.toLower());
        tag.replace("-", "_");
        QDomElement dom_item = xml.createElement(tag);
        dom_header.appendChild(dom_item);
        if (m_HeaderData[type].contains("raw"))
        {
            QDomElement dom_raw = xml.createElement("raw");
            dom_item.appendChild(dom_raw);
            QDomText dom_raw_text =
                xml.createTextNode(m_HeaderData[type]["raw"]);
            dom_raw.appendChild(dom_raw_text);
        }
        
        // Deal with special cases
        if (type == "Bcc")
        {
            ToXML_Header_Bcc(dom_item);
        } else if (type =="Cc")
        {
            ToXML_Header_Cc(dom_item);
        } else if (type =="Content-Type")
        {
            ToXML_Header_ContentType(dom_item);
        } else if (type =="Date")
        {
            ToXML_Header_Date(dom_item, m_HeaderData["Date"]);
        } else if (type =="Delivered-To")
        {
            ToXML_Header_Individual(dom_item, m_HeaderData["Delivered-To"]);
        } else if (type =="Envelope-To")
        {
            ToXML_Header_Individual(dom_item, m_HeaderData["Envelope-To"]);
        } else if (type =="From")
        {
            ToXML_Header_Individual(dom_item, m_HeaderData["From"]);
        } else if (type =="In-Reply-To")
        {
            ToXML_Header_InReplyTo(dom_item);
        } else if (type =="Lines")
        {
            ToXML_Header_Lines(dom_item);
        } else if (type =="Message-Id")
        {
            ToXML_Header_MessageId(dom_item);
        } else if (type == "Received")
        {
            ToXML_Header_Received(dom_item);
        } else if (type == "References")
        {
            ToXML_Header_References(dom_item);
        } else if (type == "Reply-To")
        {
            ToXML_Header_Individual(dom_item, m_HeaderData["Reply-To"]);
        } else if (type == "Resent-Date")
        {
            ToXML_Header_Date(dom_item, m_HeaderData["Resent-Date"]);
        } else if (type == "Resent-From")
        {
            ToXML_Header_Individual(dom_item, m_HeaderData["Resent-From"]);
        } else if (type == "Resent-Message-Id")
        {
            ToXML_Header_ResentMessageId(dom_item);
        } else if (type == "Resent-Sender")
        {
            ToXML_Header_Individual(dom_item, m_HeaderData["Resent-Sender"]);
        } else if (type == "Sender")
        {
            ToXML_Header_Individual(dom_item, m_HeaderData["Sender"]);
        } else if (type == "Subject")
        {
            ToXML_Header_Subject(dom_item);
        } else if (type == "To")
        {
            ToXML_Header_To(dom_item);
        } else
        {
            // Ignore interpreted stuff - but check if there is any
            const QList < QString > names_list = m_HeaderData[type].keys();
            QSet < QString > names(names_list.begin(), names_list.end());
            names -= "raw";
            if (!names.isEmpty())
            {
                const QString reason =
                    tr("Email header item \"%1\" has interpreted elements "
                        "that are not adequately exported to XML: %2")
                        .arg(type,
                             QStringList(names_list).join(", "));
                MessageLogger::Error(CALL_METHOD, reason);
            }
        }
    }
    
    // === Body
    QDomElement dom_body = xml.createElement("body");
    dom_email.appendChild(dom_body);
    ToXML_BodyPart(dom_body, -1);
    
    // Done
    QString ret = xml.toString();
    static const QRegularExpression format("<!DOCTYPE [^>]+>\\s*(<.*>\\s*)");
    QRegularExpressionMatch match = format.match(ret);
    if (match.hasMatch())
    {
        CALL_OUT("");
        return match.captured(1);
    } else
    {
        CALL_OUT("");
        return ret;
    }

    // We never get here
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_Bcc(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    for (int idx = 0; idx < m_HeaderData_Bcc.size(); idx++)
    {
        ToXML_Header_Individual(mrDomItem, m_HeaderData_Bcc[idx]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_Cc(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    for (int idx = 0; idx < m_HeaderData_Bcc.size(); idx++)
    {
        ToXML_Header_Individual(mrDomItem, m_HeaderData_Cc[idx]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_ContentType(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    QDomDocument xml = mrDomItem.ownerDocument();
    const QList < QString > attributes_list =
        m_HeaderData["content-type"].keys();
    QSet < QString > attributes(attributes_list.begin(),
        attributes_list.end());
    attributes -= "raw";
    attributes -= "boundary";
    if (m_HeaderData["Content-Type"].contains("type"))
    {
        mrDomItem.setAttribute("type", m_HeaderData["Content-Type"]["type"]);
        attributes -= "type";
    }
    if (!attributes.isEmpty())
    {
        // Non-fatal: There ware left over attributes
        const QString reason = tr("Remaining attributes: %1")
            .arg(QStringList(attributes_list).join(", "));
        MessageLogger::Error(CALL_METHOD, reason);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_InReplyTo(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (m_HeaderData["In-Reply-To"].contains("id"))
    {
        mrDomItem.setAttribute("id", m_HeaderData["In-Reply-To"]["id"]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_Lines(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (m_HeaderData["Lines"].contains("lines"))
    {
        mrDomItem.setAttribute("lines", m_HeaderData["Lines"]["lines"]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_MessageId(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (m_HeaderData["Message-Id"].contains("id"))
    {
        mrDomItem.setAttribute("id", m_HeaderData["Message-Id"]["id"]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_Received(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    QDomDocument xml = mrDomItem.ownerDocument();
    for (int idx = 0; idx < m_HeaderData_Received.size(); idx++)
    {
        QDomElement dom_reference = xml.createElement("reference");
        mrDomItem.appendChild(dom_reference);

        QDomElement dom_raw = xml.createElement("raw");
        dom_reference.appendChild(dom_raw);
        QDomText dom_raw_text =
            xml.createTextNode(m_HeaderData_Received[idx]["raw"]);
        dom_raw.appendChild(dom_raw_text); 
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_References(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    QDomDocument xml = mrDomItem.ownerDocument();
    for (int idx = 0; idx < m_HeaderData_References.size(); idx++)
    {
        QDomElement dom_reference = xml.createElement("reference");
        mrDomItem.appendChild(dom_reference);

        QDomElement dom_raw = xml.createElement("raw");
        dom_reference.appendChild(dom_raw);
        QDomText dom_raw_text =
            xml.createTextNode(m_HeaderData_References[idx]["raw"]);
        dom_raw.appendChild(dom_raw_text); 

        QDomElement dom_id = xml.createElement("id");
        dom_reference.appendChild(dom_id);
        QDomText dom_id_text =
            xml.createTextNode(m_HeaderData_References[idx]["id"]);
        dom_id.appendChild(dom_id_text); 
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_ResentMessageId(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    QDomDocument xml = mrDomItem.ownerDocument();
    if (m_HeaderData["Resent-Message-Id"].contains("id"))
    {
        QDomElement dom_id = xml.createElement("id");
        mrDomItem.appendChild(dom_id);
        QDomText dom_id_text =
            xml.createTextNode(m_HeaderData["Resent-Message-Id"]["id"]);
        dom_id.appendChild(dom_id_text);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_Subject(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    QDomDocument xml = mrDomItem.ownerDocument();
    if (m_HeaderData["Subject"].contains("subject"))
    {
        QDomElement dom_subject = xml.createElement("subject");
        mrDomItem.appendChild(dom_subject);
        QDomText dom_subject_text =
            xml.createTextNode(m_HeaderData["Subject"]["subject"]);
        dom_subject.appendChild(dom_subject_text);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_To(QDomElement & mrDomItem)
{
    CALL_IN(QString("mrDomItem=%1")
        .arg(CALL_SHOW(mrDomItem)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    for (int idx = 0; idx < m_HeaderData_To.size(); idx++)
    {
        ToXML_Header_Individual(mrDomItem, m_HeaderData_To[idx]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_Date(QDomElement & mrDomItem,
    const QHash < QString, QString > mcDate)
{
    CALL_IN(QString("mrDomItem=%1, mcDate=%2")
        .arg(CALL_SHOW(mrDomItem),
             CALL_SHOW(mcDate)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    if (mcDate.contains("date"))
    {
        mrDomItem.setAttribute("date", mcDate["date"]);
    }
    if (mcDate.contains("time"))
    {
        mrDomItem.setAttribute("time", mcDate["time"]);
    }
    if (mcDate.contains("timezone"))
    {
        mrDomItem.setAttribute("timezone", mcDate["timezone"]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_Header_Individual(QDomElement & mrDomItem,
    const QHash < QString, QString > mcIndividual)
{
    CALL_IN(QString("mrDomItem=%1, mcIndividual=%2")
        .arg(CALL_SHOW(mrDomItem),
             CALL_SHOW(mcIndividual)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    QDomDocument xml = mrDomItem.ownerDocument();
    QDomElement dom_individual = xml.createElement("individual");
    mrDomItem.appendChild(dom_individual);
    if (mcIndividual.contains("first name"))
    {
        QDomElement dom_first_name = xml.createElement("first_name");
        dom_individual.appendChild(dom_first_name);
        QDomText dom_first_name_text = 
            xml.createTextNode(mcIndividual["first name"]);
        dom_first_name.appendChild(dom_first_name_text);
    }
    if (mcIndividual.contains("last name"))
    {
        QDomElement dom_last_name = xml.createElement("last_name");
        dom_individual.appendChild(dom_last_name);
        QDomText dom_last_name_text = 
            xml.createTextNode(mcIndividual["last name"]);
        dom_last_name.appendChild(dom_last_name_text);
    }
    if (mcIndividual.contains("full name"))
    {
        QDomElement dom_full_name = xml.createElement("full_name");
        dom_individual.appendChild(dom_full_name);
        QDomText dom_full_name_text = 
            xml.createTextNode(mcIndividual["full name"]);
        dom_full_name.appendChild(dom_full_name_text);
    }
    if (mcIndividual.contains("email"))
    {
        QDomElement dom_email = xml.createElement("email");
        dom_individual.appendChild(dom_email);
        QDomText dom_email_text = xml.createTextNode(mcIndividual["email"]);
        dom_email.appendChild(dom_email_text);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void Email::ToXML_BodyPart(QDomElement & mrDomParent, const int mcId)
{
    CALL_IN(QString("mrDomParent=%1, mcId=%2")
        .arg(CALL_SHOW(mrDomParent),
             CALL_SHOW(mcId)));

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    QDomDocument xml = mrDomParent.ownerDocument();

    // Create this part
    QDomElement dom_part = mrDomParent;
    if (mcId != -1)
    {
        dom_part = xml.createElement("part");
        mrDomParent.appendChild(dom_part);
        dom_part.setAttribute("type", m_BodyData_Type[mcId]);
        if (m_BodyData_Type[mcId].isEmpty() ||
            m_BodyData_Type[mcId].startsWith("text"))
        {
            // Text
            QDomText dom_part_text = xml.createTextNode(m_BodyData_Part[mcId]);
            dom_part.appendChild(dom_part_text);
        } else
        {
            // Binary
            const QString base64 = m_BodyData_Part[mcId].toBase64();
            QDomText dom_part_text = xml.createTextNode(base64);
            dom_part.appendChild(dom_part_text);
        }
    }

    // Loop all children
    for (int child_id : m_BodyData_ChildIds[mcId])
    {
        ToXML_BodyPart(dom_part, child_id);
    }

    CALL_OUT("");
}



// ======================================================================= Dump



///////////////////////////////////////////////////////////////////////////////
// Show everything
void Email::Dump()
{
    CALL_IN("");

    // Debugging
    if (DEBUG)
    {
        qDebug().noquote() << CALL_METHOD;
    }

    qDebug().noquote() << "==================================================";

    // == Any error
    qDebug().noquote() << "Error line: " << m_ErrorLine;
    qDebug().noquote() << "Error text: " << m_Error;

    // == All header data
    qDebug().noquote() << "Header Data";
    for (auto tag_iterator = m_HeaderData.keyBegin();
         tag_iterator != m_HeaderData.keyEnd();
         tag_iterator++)
    {
        const QString tag  = *tag_iterator;
        qDebug().noquote() << tag;
        for (auto subtag_iterator = m_HeaderData[tag].keyBegin();
             subtag_iterator != m_HeaderData[tag].keyEnd();
             subtag_iterator++)
        {
            const QString subtag = *subtag_iterator;
            qDebug().noquote() << QString("\t%1\t%2")
                .arg(subtag,
                     m_HeaderData[tag][subtag]);
        }
    }
    qDebug().noquote() << "";
    
    // "Received" items
    for (int idx = 0; idx < m_HeaderData_Received.size(); idx++)
    {
        qDebug().noquote() << tr("Received %1").arg(idx);
        for (auto subtag_iterator = m_HeaderData_Received[idx].keyBegin();
             subtag_iterator != m_HeaderData_Received[idx].keyEnd();
             subtag_iterator++)
        {
            const QString subtag = *subtag_iterator;
            qDebug().noquote() << QString("\t%1\t%2")
                .arg(subtag,
                     m_HeaderData_Received[idx][subtag]);
        }
    }
    
    // "References" items
    for (int idx = 0; idx < m_HeaderData_References.size(); idx++)
    {
        qDebug().noquote() << tr("Reference %1").arg(idx);
        for (auto subtag_iterator = m_HeaderData_References[idx].keyBegin();
             subtag_iterator != m_HeaderData_References[idx].keyEnd();
             subtag_iterator++)
        {
            const QString subtag = *subtag_iterator;
            qDebug().noquote() << QString("\t%1\t%2")
                .arg(subtag,
                     m_HeaderData_References[idx][subtag]);
        }
    }
    
    // "To" email addresses
    for (int idx = 0; idx < m_HeaderData_To.size(); idx++)
    {
        qDebug().noquote() << tr("To %1").arg(idx);
        for (auto subtag_iterator = m_HeaderData_To[idx].keyBegin();
             subtag_iterator != m_HeaderData_To[idx].keyEnd();
             subtag_iterator++)
        {
            const QString subtag = *subtag_iterator;
            qDebug().noquote() << QString("\t%1\t%2")
                .arg(subtag,
                     m_HeaderData_To[idx][subtag]);
        }
    }
    
    // "Cc" email addresses
    for (int idx = 0; idx < m_HeaderData_Cc.size(); idx++)
    {
        qDebug().noquote() << tr("Cc %1").arg(idx);
        for (auto subtag_iterator = m_HeaderData_Cc[idx].keyBegin();
             subtag_iterator != m_HeaderData_Cc[idx].keyEnd();
             subtag_iterator++)
        {
            const QString subtag = *subtag_iterator;
            qDebug().noquote() << QString("\t%1\t%2")
                .arg(subtag,
                     m_HeaderData_Cc[idx][subtag]);
        }
    }
    
    // == Body
    qDebug().noquote() << "Body Data";
    for (int idx = 0; idx < m_BodyData_Part.size(); idx++)
    {
        qDebug().noquote() << tr("======= Part %1 (%2, parent %3)").arg(idx)
            .arg(m_BodyData_Type[idx],
                 m_BodyData_ParentId[idx]);
        qDebug().noquote() << m_BodyData_Part[idx];
        qDebug().noquote() << tr("======= End Part %1").arg(idx);
    }

    CALL_OUT("");
}
