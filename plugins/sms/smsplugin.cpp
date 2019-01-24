/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2018 Simon Redman <simon@ergotech.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "smsplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>

#include <core/device.h>
#include <core/daemon.h>

#include "sendreplydialog.h"

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_sms.json", registerPlugin< SmsPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SMS, "kdeconnect.plugin.sms")

QMap<QString, ConversationsDbusInterface*> SmsPlugin::conversationInterfaces;

SmsPlugin::SmsPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_telepathyInterface(QStringLiteral("org.freedesktop.Telepathy.ConnectionManager.kdeconnect"), QStringLiteral("/kdeconnect"))
    , m_conversationInterface(new ConversationsDbusInterface(this))
{
    // Once the new ConversationsDbusInterface has been fully set up, the old one can safely be
    // deleted
    const auto& oldInterfaceItr = SmsPlugin::conversationInterfaces.find(device()->id());
    if (oldInterfaceItr != SmsPlugin::conversationInterfaces.end()) {
        ConversationsDbusInterface* oldInterface = oldInterfaceItr.value();
        oldInterface->deleteLater();
        SmsPlugin::conversationInterfaces.erase(oldInterfaceItr);
    }
}

SmsPlugin::~SmsPlugin()
{
    // Because of how Qt's Dbus is designed, we are unable to immediately delete m_conversationInterface,
    // See the comment in ~NotificationsPlugin() for more information
    // We save the old pointer for now; the internal data can be used as a cache.
    // The next time we construct an SmsPlugin, we access the old pointer and delete it
    SmsPlugin::conversationInterfaces[device()->id()] = m_conversationInterface;
}

bool SmsPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.type() == PACKET_TYPE_SMS_MESSAGES) {
        return handleBatchMessages(np);
    }

    return true;
}

void SmsPlugin::sendSms(const QString& phoneNumber, const QString& messageBody)
{
    NetworkPacket np(PACKET_TYPE_SMS_REQUEST, {
        {"sendSms", true},
        {"phoneNumber", phoneNumber},
        {"messageBody", messageBody}
    });
    qCDebug(KDECONNECT_PLUGIN_SMS) << "Dispatching SMS send request to remote";
    sendPacket(np);
}

void SmsPlugin::requestAllConversations()
{
    NetworkPacket np(PACKET_TYPE_SMS_REQUEST_CONVERSATIONS);

    sendPacket(np);
}

void SmsPlugin::requestConversation (const qint64& conversationID) const
{
    NetworkPacket np(PACKET_TYPE_SMS_REQUEST_CONVERSATION);
    np.set("threadID", conversationID);

    sendPacket(np);
}

void SmsPlugin::forwardToTelepathy(const ConversationMessage& message)
{
    // If we don't have a valid Telepathy interface, bail out
    if (!(m_telepathyInterface.isValid())) return;

    qCDebug(KDECONNECT_PLUGIN_SMS) << "Passing a text message to the telepathy interface";
    connect(&m_telepathyInterface, SIGNAL(messageReceived(QString,QString)), SLOT(sendSms(QString,QString)), Qt::UniqueConnection);
    const QString messageBody = message.body();
    const QString contactName; // TODO: When telepathy support is improved, look up the contact with KPeople
    const QString phoneNumber = message.address();
    m_telepathyInterface.call(QDBus::NoBlock, QStringLiteral("sendMessage"), phoneNumber, contactName, messageBody);
}

bool SmsPlugin::handleBatchMessages(const NetworkPacket& np)
{
    const auto messages = np.get<QVariantList>("messages");
    QList<ConversationMessage> messagesList;
    messagesList.reserve(messages.count());

    for (const QVariant& body : messages) {
        ConversationMessage message(body.toMap());
        if (message.containsTextBody()) {
            forwardToTelepathy(message);
        messagesList.append(message);
        }
    }

    m_conversationInterface->addMessages(messagesList);

    return true;
}


QString SmsPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/sms";
}

#include "smsplugin.moc"

