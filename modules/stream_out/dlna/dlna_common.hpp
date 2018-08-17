/*****************************************************************************
 * dlna.hpp : DLNA/UPNP (renderer) sout module header
 *****************************************************************************
 * Copyright (C) 2004-2018 VLC authors and VideoLAN
 *
 * Authors: William Ung <william1.ung@epitech.eu>
 *          Shaleen Jain <shaleen@jain.sh>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef DLNA_COMMON_H
#define DLNA_COMMON_H

#include <list>

#include<vlc_common.h>
#include <vlc_fourcc.h>

#define SOUT_CFG_PREFIX "sout-upnp-"

#define HTTP_PORT         7070

#define HTTP_PORT_TEXT N_("HTTP port")
#define HTTP_PORT_LONGTEXT N_("This sets the HTTP port of the local server used to stream the media to the UPnP Renderer.")
#define HAS_VIDEO_TEXT N_("Video")
#define HAS_VIDEO_LONGTEXT N_("The UPnP Renderer can receive video.")

#define IP_ADDR_TEXT N_("IP Address")
#define IP_ADDR_LONGTEXT N_("IP Address of the UPnP Renderer.")
#define PORT_TEXT N_("UPnP Renderer port")
#define PORT_LONGTEXT N_("The port used to talk to the UPnP Renderer.")
#define BASE_URL_TEXT N_("base URL")
#define BASE_URL_LONGTEXT N_("The base Url relative to which all other UPnP operations must be called")
#define URL_TEXT N_("description URL")
#define URL_LONGTEXT N_("The Url used to get the xml descriptor of the UPnP Renderer")

namespace Sout
{

/* module callbacks */
int OpenSout(vlc_object_t *);
void CloseSout(vlc_object_t *);

}
#endif /* DLNA_COMMON_H */
