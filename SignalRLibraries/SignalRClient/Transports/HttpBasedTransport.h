/*
 *  Copyright (c) 2013, p3root - Patrik Pfaffenbauer (patrik.pfaffenbauer@p3.co.at)
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

#ifndef HTTPBASEDTRANSPORT_H
#define HTTPBASEDTRANSPORT_H

#include "ClientTransport.h"
#include "Connection.h"
#include "Transports/HttpClient.h"
#include "Helper/TransportHelper.h"
#include "SignalException.h"
#include <QQueue>
#include <QTimer>

namespace P3 { namespace SignalR { namespace Client {

class HttpBasedTransport : public ClientTransport
{
    Q_OBJECT
protected:
    HttpClient* _httpClient;

public:
    HttpBasedTransport(HttpClient* httpClient, Connection* con);
    virtual ~HttpBasedTransport(void);

    void negotiateCompleted(QString data, SignalException* ex);
    virtual void onNegotiatenCompleted(const NegotiateResponse& res);
    void negotiate();
    void send(QString data);
    bool abort(int timeoutMs = 0);

    virtual void retry();

    void tryDequeueNextWorkItem();
private:    
    
    struct SendQueueItem
    {
        Connection* connection;
        QString url;
        QMap<QString, QString> postData;
    };
    
    QQueue<SendQueueItem*> _sendQueue;
    bool _sending;
    QTimer _retryTimerTimeout;

private Q_SLOTS:
    void onSendHttpResponse(const QString& httpResponse, SignalException* error);
    void retryNegotiation();
};

}}}

#endif
