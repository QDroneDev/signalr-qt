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

#ifndef TRANSPORTHELPER_H
#define TRANSPORTHELPER_H

#include <QString>
#include "Transports/HttpClient.h"
#include "ClientTransport.h"
#include "SignalException.h"
#include <QextJson.h>


namespace P3 { namespace SignalR { namespace Client {

struct NegotiateResponse;

class TransportHelper : public QObject
{
    Q_OBJECT
public:
    TransportHelper(void);
    ~TransportHelper(void);

    static QString getReceiveQueryString(ConnectionPrivate *connection, QString transport);
    static const NegotiateResponse* parseNegotiateHttpResponse(const QString& httpResponse);

    static QSharedPointer<SignalException> processMessages(ConnectionPrivate *connection, QString raw, bool* timedOut, bool* disconnected, quint64 *messageId=0);
    // replacements for the original...
    static QSharedPointer<SignalException> processMessages(ConnectionPrivate *connection, const QString &raw);
    static QSharedPointer<SignalException> processMessages(ConnectionPrivate *connection, const QByteArray &raw);

    static QString getInvokeRequest(const QString &target, const QVariantList &arguments, const QString &invocationId);
};

}}}

#endif
