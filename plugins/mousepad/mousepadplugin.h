/**
 * Copyright 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 * Copyright 2015 Martin Gräßlin <mgraesslin@kde.org>
 * Copyright 2014 Ahmed I. Khalil <ahmedibrahimkhali@gmail.com>
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

#ifndef MOUSEPADPLUGIN_H
#define MOUSEPADPLUGIN_H

#include <core/kdeconnectplugin.h>
#include <config-mousepad.h>

#include "abstractremoteinput.h"

#define PACKET_TYPE_MOUSEPAD_KEYBOARDSTATE QLatin1String("kdeconnect.mousepad.keyboardstate")

class MousepadPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit MousepadPlugin(QObject* parent, const QVariantList& args);
    ~MousepadPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

private:
    AbstractRemoteInput* m_impl;

};

#endif
