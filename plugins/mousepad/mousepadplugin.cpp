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

#include "mousepadplugin.h"
#include <KPluginFactory>
#include <KLocalizedString>
#include <QGuiApplication>

#if HAVE_WINDOWS
    #include "windowsremoteinput.h"
#else
    #if HAVE_X11
        #include "x11remoteinput.h"
    #endif
    #if HAVE_WAYLAND
        #include "waylandremoteinput.h"
    #endif
#endif


K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_mousepad.json", registerPlugin< MousepadPlugin >(); )

MousepadPlugin::MousepadPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_impl(nullptr)
{
#if HAVE_WINDOWS
    m_impl = new WindowsRemoteInput(this);
#else
    #if HAVE_X11
    if (QGuiApplication::platformName() == QLatin1String("xcb")) {
        m_impl = new X11RemoteInput(this);
    }
    #endif

    #if HAVE_WAYLAND
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        m_impl = new WaylandRemoteInput(this);
    }
    #endif
#endif

    if (!m_impl) {
        qDebug() << "KDE Connect was built without" << QGuiApplication::platformName() << "support";
    }

}

MousepadPlugin::~MousepadPlugin()
{
    delete m_impl;
}

bool MousepadPlugin::receivePacket(const NetworkPacket& np)
{
    if (m_impl) {
        return m_impl->handlePacket(np);
    } else {
        return false;
    }
}


void MousepadPlugin::connected() {
    NetworkPacket np(PACKET_TYPE_MOUSEPAD_KEYBOARDSTATE);
    if (m_impl) {
        np.set<bool>(QStringLiteral("state"), m_impl->hasKeyboardSupport());
    }
    sendPacket(np);
}

#include "mousepadplugin.moc"
