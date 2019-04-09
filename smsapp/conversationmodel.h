/**
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * Copyright (C) 2018 Simon Redman <simon@ergotech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONVERSATIONMODEL_H
#define CONVERSATIONMODEL_H

#include <QStandardItemModel>
#include <QLoggingCategory>
#include <QSet>

#include "interfaces/dbusinterfaces.h"

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATION_MODEL)

#define INVALID_THREAD_ID -1

class ConversationModel
    : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(qint64 threadId READ threadId WRITE setThreadId)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId)

public:
    ConversationModel(QObject* parent = nullptr);
    ~ConversationModel();

    enum Roles {
        FromMeRole = Qt::UserRole,
        DateRole,
    };

    Q_ENUM(Roles)

    qint64 threadId() const;
    void setThreadId(const qint64& threadId);

    QString deviceId() const { return m_deviceId; }
    void setDeviceId(const QString &/*deviceId*/);

    Q_INVOKABLE void sendReplyToConversation(const QString& message);
    Q_INVOKABLE void requestMoreMessages(const quint32& howMany = 10);

private Q_SLOTS:
    void createRowFromMessage(const QVariantMap &msg, int pos);
    void handleConversationUpdate(const QVariantMap &msg);

private:
    DeviceConversationsDbusInterface* m_conversationsInterface;
    QString m_deviceId;
    qint64 m_threadId = INVALID_THREAD_ID;
    QSet<qint32> knownMessageIDs; // Set of known Message uIDs
};

#endif // CONVERSATIONMODEL_H
