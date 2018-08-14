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

#include "../services_discovery/upnp-wrapper.hpp"

#include <vlc_fourcc.h>

#define SOUT_CFG_PREFIX "sout-upnp-"

#define UPNP_CONTROL_PORT 8079
#define HTTP_PORT         8080

#define HTTP_PORT_TEXT N_("HTTP port")
#define HTTP_PORT_LONGTEXT N_("This sets the HTTP port of the local server used to stream the media to the UPnP Renderer.")
#define HAS_VIDEO_TEXT N_("Video")
#define HAS_VIDEO_LONGTEXT N_("The UPnP Renderer can receive video.")
#define MUX_TEXT N_("Muxer")
#define MUX_LONGTEXT N_("This sets the muxer used to stream to the UPnP Renderer.")
#define MIME_TEXT N_("MIME content type")
#define MIME_LONGTEXT N_("This sets the media MIME content type sent to the UPnP Renderer.")

#define IP_ADDR_TEXT N_("IP Address")
#define IP_ADDR_LONGTEXT N_("IP Address of the UPnP Renderer.")
#define PORT_TEXT N_("UPnP Renderer port")
#define PORT_LONGTEXT N_("The port used to talk to the UPnP Renderer.")
#define URL_TEXT N_("description URL")
#define URL_LONGTEXT N_("The Url used to get the xml descriptor of the UPnP Renderer")

static const vlc_fourcc_t DEFAULT_TRANSCODE_AUDIO = VLC_CODEC_MP3;
static const vlc_fourcc_t DEFAULT_TRANSCODE_VIDEO = VLC_CODEC_H264;
static const char DEFAULT_MUXER[] = "avformat{mux=matroska,options={live=1}}}";

namespace Sout
{

/* module callbacks */
int OpenSout(vlc_object_t *);
void CloseSout(vlc_object_t *);

class MediaRenderer : public UpnpInstanceWrapper::Listener
{
public:
    MediaRenderer(sout_stream_t *p_stream, UpnpInstanceWrapper *upnp,
            int device_port, std::string device_ip, std::string device_url)
    {
        parent = p_stream;
        handle = upnp->handle();
        this->device_ip = device_ip;
        this->device_port = device_port;
        this->device_url = device_url;
    }

    ~MediaRenderer()
    {
         parent = NULL;
    }

    sout_stream_t *parent;
    std::string device_ip;
    std::string device_url;
    int device_port;
    UpnpClient_Handle handle;
    int onEvent( Upnp_EventType event_type,
                 UpnpEventPtr Event,
                 void *p_user_data ) override;

    char *getServiceURL(const char* type, const char* service);
    int parseAVTransportState(IXML_Document* event);

    int Subscribe();
    int Play(const char *speed);
    int Pause();
    int Stop();
    int Next();
    int Previous();
    int Seek();
    int PrepareForConnection();
    int GetProtocolInfo();
    int SetAVTransportURI(const char* uri);
};

}
