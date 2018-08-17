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

#ifndef DLNA_H
#define DLNA_H

#include "../../services_discovery/upnp-wrapper.hpp"
#include "dlna_common.hpp"
#include "profile_names.hpp"

struct protocol_info_t {
    std::string protocol_str;
    std::string transport;
    dlna_profile profile;
};

typedef std::unique_ptr<protocol_info_t> ProtocolPtr;
#define make_protocol(ptr) std::make_unique<protocol_info_t>(ptr)

const protocol_info_t default_audio_protocol = {
    "http-get:*:audio/mpeg:DLNA.ORG_PN=MP3"
    ,"http-get"
    ,default_audio_profile
};

const protocol_info_t default_video_protocol = {
    "http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_SD;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000"
    ,"http-get"
    ,default_video_profile
};

namespace Sout
{

class MediaRenderer
{
public:
    MediaRenderer(sout_stream_t *p_stream, UpnpInstanceWrapper *upnp,
            std::string base_url, std::string device_url)
        : parent(p_stream)
        , base_url(base_url)
        , device_url(device_url)
        , handle(upnp->handle())
    {
    }

    sout_stream_t *parent;
    std::string base_url;
    std::string device_url;
    UpnpClient_Handle handle;

    char *getServiceURL(const char* type, const char* service);
    IXML_Document *SendAction(const char* action_name, const char *service_type,
                    std::list<std::pair<const char*, const char*>> arguments);

    int Play(const char *speed);
    int Stop();
    std::vector<protocol_info_t> GetProtocolInfo();
    int SetAVTransportURI(const char* uri, const protocol_info_t proto);
};

}
#endif /* DLNA_H */
