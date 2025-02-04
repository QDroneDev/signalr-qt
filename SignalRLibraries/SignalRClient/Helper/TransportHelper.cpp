/*
 *  Copyright (c) 2013-2014, p3root - Patrik Pfaffenbauer (patrik.pfaffenbauer@p3.co.at)
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *
 *    Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright notice, this
 *    list of conditions and the following disclaimer in the documentation and/or
 *    other materials provided with the distribution.
 *
 *    Neither the name of the {organization} nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QJsonDocument>
#include <QJsonObject>

#include "TransportHelper.h"
#include <QtGlobal>
#include "Helper.h"
#include "Connection_p.h"
#include "Transports/NegotiateResponse.h"

const QByteArray EMPTY, RECORD_SEPERATOR( "\u001E" );

const QString
      TYPE_KEY          ( "type"           )
    , TARGET_KEY        ( "target"         )
    , ARGUMENTS_KEY     ( "arguments"      )
    , RESULT_KEY        ( "result"         )
    , ERROR_KEY         ( "error"          )
    , ALLOW_RECONN_KEY  ( "allowReconnect" )
    , INVOCATION_ID_KEY ( "invocationId"   )
;

enum MessageType
{
    UNDEFINED,
    invocation,
    stream_item,
    completion,
    stream_invocation,
    cancel_invocation,
    ping,
    close,
};

namespace P3 { namespace SignalR { namespace Client {

TransportHelper::TransportHelper(void)
{
}


TransportHelper::~TransportHelper(void)
{
}

QString TransportHelper::getReceiveQueryString(ConnectionPrivate* connection, QString transport)
{
    QString connectionTokenKey = "connectionToken";
    QString conData = "&" + connectionTokenKey + "=" + QString(connection->getConnectionToken());
    QString qs = "?transport=" + transport + conData;

    QString messageId = connection->getMessageId();
    QString groupsToken =  connection->getGroupsToken();

    if(!messageId.isEmpty())
    {
        qs += "&messageId=" + messageId;
    }

    if(!groupsToken.isEmpty())
    {
        qs += "&groupsToken=" + groupsToken;
    }

    QString onSending = connection->onSending();
    if(!onSending.isEmpty())
    {
        qs += "&connectionData=" + onSending;
    }

    if(!connection->getTid().isEmpty())
        qs += "&tid=" + connection->getTid();

    return qs;
}

QSharedPointer<SignalException> TransportHelper::processMessages(ConnectionPrivate* connection, QString raw, bool* timedOut, bool* disconnected, quint64 *messageId)
{
    QSharedPointer<SignalException> e;
    QVariant var = QextJson::parse(raw);
    if(var.convert(QVariant::Map))
    {
        QVariantMap map = var.value<QVariantMap>();

        if(map.contains("T"))
        {
            if(map["T"].value<int>() == 1)
            {
                *timedOut = true;
            }
        }

        if(map.contains("D"))
        {
            if(map["D"].value<int>() == 1)
            {
                connection->emitLogMessage("Received message to disconnect from SignalR Server", SignalR::Warning);
                *disconnected = true;
            }
        }

        if(map.contains("G"))
        {
            connection->setGroupsToken(map["G"].toString());
        }

        if(map.contains("C"))
        {
            connection->setMessageId(map["C"].toString());
        }

        if(map.contains("S"))
        {
            if(messageId)
            {
                *messageId = map["S"].toULongLong();
            }
        }

        if(map.contains("I") && !map.contains("M"))
        {
            if(messageId)
            {
                *messageId = map["I"].toULongLong();
            }
            connection->onReceived(var);
        }

        if(map.contains("E"))
        {
            //error occured
            QString errorMessage(map["E"].toString());
            if (errorMessage.size() > 250)
                errorMessage = errorMessage.left(250) + "...";

            connection->emitLogMessage(errorMessage, SignalR::Error);
            connection->emitLogMessage(raw, SignalR::Debug);

            e = QSharedPointer<SignalException>(new SignalException(errorMessage, SignalException::SignalRServerException));
        }

        if(*disconnected)
            return e;

        if(map.contains("M"))
        {
            if(map["M"].convert(QVariant::List))
            {
                QVariantList lst = map["M"].value<QVariantList>();

                if(messageId != 0 && map.contains("I"))
                {
                    *messageId = map["I"].toULongLong();
                }

                if(lst.count() != 0)
                {
                    foreach(QVariant cur, lst)
                    {
                        connection->onReceived(cur);
                    }
                }
            }
        }
    }

    return e;
}

QSharedPointer<SignalException> TransportHelper::processMessages(
    ConnectionPrivate *connection, const QString &raw )
{
    const QByteArray bytes( raw.toLocal8Bit() );
    return TransportHelper::processMessages( connection, bytes );
}

QSharedPointer<SignalException> TransportHelper::processMessages(
    ConnectionPrivate *connection, const QByteArray &raw )
{
    connection->emitLogMessage(
        "received message: " + QString(raw), SignalR::Debug );

    // Convert the raw message to a QVariantMap
    QByteArray scrubbed( raw );
    scrubbed.replace( RECORD_SEPERATOR, EMPTY );
    QJsonParseError fromJsonError;
    const QJsonObject jsonObj(
        QJsonDocument::fromJson( scrubbed, &fromJsonError ).object() );
    if( fromJsonError.error != QJsonParseError::NoError )
    {
        const QString errMsg( fromJsonError.errorString() +
           "\nJSON parsing error occurred at offset position: " +
               QString::number( fromJsonError.offset ) +
           "\nRaw data:\n\n" + QString( raw ) );
        return QSharedPointer<SignalException>( new SignalException(
            errMsg, SignalException::UnkownContentError ) );
    }
    const QVariantMap msgMap( jsonObj.toVariantMap() );

    // Get the message type, and react to it accordingly
    const MessageType msgType( (MessageType)msgMap.value( TYPE_KEY,
        (int)MessageType::UNDEFINED ).toInt() );
    connection->emitLogMessage( "msgType: " + QString::number(msgType), SignalR::Debug );
    switch( msgType )
    {
    case invocation:
        connection->onInvocationReceived(
            msgMap.value( TARGET_KEY        ).toString(),
            msgMap.value( ARGUMENTS_KEY     ).toList(),
            msgMap.value( INVOCATION_ID_KEY ).toString() );
        break;
    case stream_item:
        // TODO!
        break;
    case completion:
        connection->onCompletionReceived(
            msgMap.value( RESULT_KEY        ), // variant
            msgMap.value( ERROR_KEY         ).toString(),
            msgMap.value( INVOCATION_ID_KEY ).toString()
        );
        break;
    case stream_invocation:
        // TODO!
        break;
    case cancel_invocation:
        connection->onCancelInvocationReceived(
            msgMap.value( INVOCATION_ID_KEY ).toString()
        );
        break;
    case ping:
        connection->onPingReceived();
        break;
    case close:
        connection->onCloseReceived(
            msgMap.value( ERROR_KEY ).toString(),
            msgMap.value( ALLOW_RECONN_KEY ).toBool()
        );
        return QSharedPointer<SignalException>( new SignalException(
            msgMap.value( ERROR_KEY ).toString(),
            SignalException::RemoteHostClosedConnection ) );
    default:
        if( msgMap.isEmpty() )
            connection->emitLogMessage(
                "Empty message received", SignalR::Info );
        else
        {
            const QString errMsg( "Unknown or missing message type"
               "\nRaw data:\n\n" + QString( raw ) );
            return QSharedPointer<SignalException>( new SignalException(
                errMsg, SignalException::UnkownContentError ) );
        }
    }

    // Indicate "no error encountered" by returning a null smart pointer
    return QSharedPointer<SignalException>();
}

QString TransportHelper::getInvokeRequest(
    const QString &target, const QVariantList &arguments,
    const QString &invocationId )
{
    QVariantMap msgMap({
          { TYPE_KEY,          (int)MessageType::invocation }
        , { TARGET_KEY,        target                       }
        , { ARGUMENTS_KEY,     arguments                    }
    });
    if( !invocationId.isEmpty() ) msgMap[ INVOCATION_ID_KEY ] = invocationId;
    const QByteArray msgBytes( QJsonDocument::fromVariant( msgMap )
                               .toJson( QJsonDocument::Compact ) );
    return QString( msgBytes + RECORD_SEPERATOR );
}

const NegotiateResponse* TransportHelper::parseNegotiateHttpResponse(const QString &httpResponse)
{
    NegotiateResponse* response = 0;
    QVariant var = QextJson::parse(httpResponse);
    if(var.convert(QVariant::Map))
    {
        QVariantMap map = var.value<QVariantMap>();

        response = new NegotiateResponse();
        response->connectionId = map.value("ConnectionId").toString();
        response->connectionToken = map.value("ConnectionToken").toString();
        response->protocolVersion = map.value("ProtocolVersion").toString();

        if(map.contains("KeepAliveTimeout"))
            response->keepAliveTimeout = map.value("KeepAliveTimeout").toDouble();
        else
            response->keepAliveTimeout = -1;

        if(map.contains("DisconnectTimeout"))
            response->disconnectTimeout = map.value("DisconnectTimeout").toDouble();
        else
            response->disconnectTimeout = -1;

        if(map.contains("TryWebSockets") && map.value("TryWebSockets").toBool())
        {
            response->tryWebSockets = true;
            response->transportConnectTimeout = map.value("TransportConnectTimeout").toInt();
            if(map.contains("WebSocketServerUrl"))
            {
                response->webSocketsUrl = map.value("WebSocketServerUrl").toString();
            }
        }
        else
        {
            response->tryWebSockets = false;
            response->webSocketsUrl ="";
        }
    }
    return response;
}

}}}
