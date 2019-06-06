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

#include "conversationlistmodel.h"

#include <QString>
#include <QLoggingCategory>
#include "interfaces/conversationmessage.h"
#include "interfaces/dbusinterfaces.h"
#include "smshelper.h"

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL, "kdeconnect.sms.conversations_list")

OurSortFilterProxyModel::OurSortFilterProxyModel(){}
OurSortFilterProxyModel::~OurSortFilterProxyModel(){}

ConversationListModel::ConversationListModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    //qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Constructing" << this;
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    roles.insert(AddressRole, "address");
    roles.insert(PersonUriRole, "personUri");
    roles.insert(ConversationIdRole, "conversationId");
    roles.insert(DateRole, "date");
    setItemRoleNames(roles);

    ConversationMessage::registerDbusType();
}

ConversationListModel::~ConversationListModel()
{
}

void ConversationListModel::setDeviceId(const QString& deviceId)
{
    if (deviceId == m_deviceId) {
        return;
    }

    if (deviceId.isEmpty()) {
        return;
    }

    qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "setDeviceId" << deviceId << "of" << this;

    if (m_conversationsInterface) {
        disconnect(m_conversationsInterface, SIGNAL(conversationCreated(QVariantMap)), this, SLOT(handleCreatedConversation(QVariantMap)));
        disconnect(m_conversationsInterface, SIGNAL(conversationUpdated(QVariantMap)), this, SLOT(handleConversationUpdated(QVariantMap)));
        delete m_conversationsInterface;
        m_conversationsInterface = nullptr;
    }

    // This method still gets called *with a valid deviceID* when the device is not connected while the component is setting up
    // Detect that case and don't do anything.
    DeviceDbusInterface device(deviceId);
    if (!(device.isValid() && device.isReachable())) {
        return;
    }

    m_deviceId = deviceId;
    Q_EMIT deviceIdChanged();

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, SIGNAL(conversationCreated(QVariantMap)), this, SLOT(handleCreatedConversation(QVariantMap)));
    connect(m_conversationsInterface, SIGNAL(conversationUpdated(QVariantMap)), this, SLOT(handleConversationUpdated(QVariantMap)));
    prepareConversationsList();

    m_conversationsInterface->requestAllConversationThreads();
}

void ConversationListModel::prepareConversationsList()
{
    if (!m_conversationsInterface->isValid()) {
        qCWarning(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Tried to prepareConversationsList with an invalid interface!";
        return;
    }
    QDBusPendingReply<QVariantList> validThreadIDsReply = m_conversationsInterface->activeConversations();

    setWhenAvailable(validThreadIDsReply, [this](const QVariantList& convs) {
        clear(); // If we clear before we receive the reply, there might be a (several second) visual gap!
        for (const QVariant& headMessage : convs) {
            QDBusArgument data = headMessage.value<QDBusArgument>();
            QVariantMap message;
            data >> message;
            handleCreatedConversation(message);
        }
    }, this);
}

void ConversationListModel::handleCreatedConversation(const QVariantMap& msg)
{
    createRowFromMessage(msg);
}

void ConversationListModel::handleConversationUpdated(const QVariantMap& msg)
{
    createRowFromMessage(msg);
}

void ConversationListModel::printDBusError(const QDBusError& error)
{
    qCWarning(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << error;
}

QStandardItem * ConversationListModel::conversationForThreadId(qint32 threadId)
{
    for(int i=0, c=rowCount(); i<c; ++i) {
        auto it = item(i, 0);
        if (it->data(ConversationIdRole) == threadId)
            return it;
    }
    return nullptr;
}

void ConversationListModel::createRowFromMessage(const QVariantMap& msg)
{
    const ConversationMessage message(msg);
    if (message.type() == -1) {
        // The Android side currently hacks in -1 if something weird comes up
        // TODO: Remove this hack when MMS support is implemented
        return;
    }

    bool toadd = false;
    QStandardItem* item = conversationForThreadId(message.threadID());
    if (!item) {
        toadd = true;
        item = new QStandardItem();
        QScopedPointer<KPeople::PersonData> personData(lookupPersonByAddress(message.address()));
        if (personData) {
            item->setText(personData->name());
            item->setIcon(QIcon(personData->photo()));
            item->setData(personData->personUri(), PersonUriRole);
        } else {
            item->setData(QString(), PersonUriRole);
            item->setText(message.address());
        }
        item->setData(message.threadID(), ConversationIdRole);
    }

    // Update the message if the data is newer
    // This will be true if a conversation receives a new message, but false when the user
    // does something to trigger past conversation history loading
    bool oldDateExists;
    qint64 oldDate = item->data(DateRole).toLongLong(&oldDateExists);
    if (!oldDateExists || message.date() >= oldDate) {
        // If there was no old data or incoming data is newer, update the record
        item->setData(message.address(), AddressRole);
        item->setData(message.type() == ConversationMessage::MessageTypeSent, FromMeRole);
        item->setData(message.body(), Qt::ToolTipRole);
        item->setData(message.date(), DateRole);
    }

    if (toadd)
        appendRow(item);
}

KPeople::PersonData* ConversationListModel::lookupPersonByAddress(const QString& address)
{
    const QString& canonicalAddress = SmsHelper::canonicalizePhoneNumber(address);
    int rowIndex = 0;
    for (rowIndex = 0; rowIndex < m_people.rowCount(); rowIndex++) {
        const QString& uri = m_people.get(rowIndex, KPeople::PersonsModel::PersonUriRole).toString();
        KPeople::PersonData* person = new KPeople::PersonData(uri);

        const QStringList& allEmails = person->allEmails();
        for (const QString& email : allEmails) {
            // Although we are nominally an SMS messaging app, it is possible to send messages to phone numbers using email -> sms bridges
            if (address == email) {
                return person;
            }
        }

        // TODO: Either upgrade KPeople with an allPhoneNumbers method
        const QVariantList allPhoneNumbers = person->contactCustomProperty(QStringLiteral("all-phoneNumber")).toList();
        for (const QVariant& rawPhoneNumber : allPhoneNumbers) {
            const QString& phoneNumber = SmsHelper::canonicalizePhoneNumber(rawPhoneNumber.toString());
            bool matchingPhoneNumber = SmsHelper::isPhoneNumberMatchCanonicalized(canonicalAddress, phoneNumber);

            if (matchingPhoneNumber) {
                //qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Matched" << address << "to" << person->name();
                return person;
            }
        }

        delete person;
    }

    return nullptr;
}
