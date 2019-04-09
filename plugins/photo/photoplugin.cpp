/**
 * Copyright 2019 Nicolas Fella <nicolas.fella@gmx.de>
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

#include "photoplugin.h"

#include <KPluginFactory>

#include <QDebug>
#include <QLoggingCategory>

#include <core/filetransferjob.h>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_photo.json", registerPlugin< PhotoPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_PHOTO, "kdeconnect.plugin.photo")

PhotoPlugin::PhotoPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

PhotoPlugin::~PhotoPlugin()
{
}

bool PhotoPlugin::receivePacket(const NetworkPacket& np)
{

    if (requestedFiles.isEmpty() || !np.hasPayload()) {
        return true;
    }

    const QString& fileName = requestedFiles.takeFirst();
    FileTransferJob* job = np.createPayloadTransferJob(QUrl::fromLocalFile(fileName));
    connect(job, &FileTransferJob::result, this, [this, fileName] {
        Q_EMIT photoReceived(fileName);
    });
    job->start();
    return true;
}

void PhotoPlugin::requestPhoto(const QString& fileName)
{
    requestedFiles.append(fileName);
    NetworkPacket np(PACKET_TYPE_PHOTO_REQUEST);
    sendPacket(np);
}

QString PhotoPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/photo";
}

#include "photoplugin.moc"

