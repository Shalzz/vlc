/*****************************************************************************
 * dlna.cpp : DLNA/UPNP (renderer) sout module
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "dlna.hpp"

#include <vector>
#include <string>
#include <sstream>

#include <vlc_block.h>
#include <vlc_rand.h>
#include <vlc_sout.h>

static const char* AV_TRANSPORT_SERVICE_TYPE = "urn:schemas-upnp-org:service:AVTransport:1";
static const char* CONNECTION_MANAGER_SERVICE_TYPE = "urn:schemas-upnp-org:service:ConnectionManager:1";
static const char* RENDERING_CONTROL_SERVICE_TYPE = "urn:schemas-upnp-org:service:RenderingControl:1";

static const char *const ppsz_sout_options[] = {
    "ip", "port", "http-port", "video", "base_url", "url", NULL
};

struct sout_stream_id_sys_t
{
    es_format_t           fmt;
    sout_stream_id_sys_t  *p_sub_id;
};

struct sout_stream_sys_t
{
    sout_stream_sys_t(int http_port, bool supports_video)
        : p_out(NULL)
        , b_supports_video(supports_video)
        , http_port(http_port)
        , es_changed(true)
        , cc_has_input( false )
    {
    }

    std::shared_ptr<Sout::MediaRenderer> renderer;
    UpnpInstanceWrapper *p_upnp;

    ProtocolPtr canDecodeAudio( vlc_fourcc_t i_codec ) const;
    ProtocolPtr canDecodeVideo( vlc_fourcc_t audio_codec,
                         vlc_fourcc_t video_codec ) const;
    bool startSoutChain( sout_stream_t* p_stream,
                         const std::vector<sout_stream_id_sys_t*> &new_streams,
                         const std::string &sout );
    void stopSoutChain( sout_stream_t* p_stream );
    sout_stream_id_sys_t *GetSubId( sout_stream_t *p_stream,
                                    sout_stream_id_sys_t *id,
                                    bool update = true );

    sout_stream_t                       *p_out;
    bool                                b_supports_video;
    int                                 http_port;

    sout_stream_id_sys_t *              video_proxy_id;
    vlc_tick_t                          first_video_keyframe_pts;
    std::string                         transport_uri;
    bool                                has_video;
    bool                                es_changed;
    bool                                cc_has_input;
    std::vector<sout_stream_id_sys_t*>  streams;
    std::vector<sout_stream_id_sys_t*>  out_streams;
    unsigned int                        out_streams_added;
    unsigned int                        spu_streams_count;
    std::vector<protocol_info_t>        device_protocols;
    ProtocolPtr                         protocol;

    int UpdateOutput( sout_stream_t *p_stream );

};

static void *ProxyAdd(sout_stream_t *p_stream, const es_format_t *p_fmt)
{
    sout_stream_sys_t *p_sys = reinterpret_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id = reinterpret_cast<sout_stream_id_sys_t *>( sout_StreamIdAdd(p_stream->p_next, p_fmt) );
    if (id)
    {
        if (p_fmt->i_cat == VIDEO_ES)
            p_sys->video_proxy_id = id;
        p_sys->out_streams_added++;
    }
    return id;
}

static void ProxyDel(sout_stream_t *p_stream, void *_id)
{
    sout_stream_sys_t *p_sys = reinterpret_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id = reinterpret_cast<sout_stream_id_sys_t *>( _id );
    p_sys->out_streams_added--;
    if (id == p_sys->video_proxy_id)
        p_sys->video_proxy_id = NULL;
    return sout_StreamIdDel(p_stream->p_next, id);
}

static int ProxySend(sout_stream_t *p_stream, void *_id, block_t *p_buffer)
{
    sout_stream_sys_t *p_sys = reinterpret_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id = reinterpret_cast<sout_stream_id_sys_t *>( _id );
    if (p_sys->cc_has_input
     || p_sys->out_streams_added >= p_sys->out_streams.size() - p_sys->spu_streams_count)
    {
        if (p_sys->has_video)
        {
            // In case of video, the first block must be a keyframe
            if (id == p_sys->video_proxy_id)
            {
                if (p_sys->first_video_keyframe_pts == -1
                 && p_buffer->i_flags & BLOCK_FLAG_TYPE_I)
                    p_sys->first_video_keyframe_pts = p_buffer->i_pts;
            }
            else // no keyframe for audio
                p_buffer->i_flags &= ~BLOCK_FLAG_TYPE_I;

            if (p_buffer->i_pts < p_sys->first_video_keyframe_pts
             || p_sys->first_video_keyframe_pts == -1)
            {
                block_ChainRelease(p_buffer);
                return VLC_SUCCESS;
            }
        }

        int ret = sout_StreamIdSend(p_stream->p_next, id, p_buffer);
        if (ret == VLC_SUCCESS && !p_sys->cc_has_input)
        {
            /* Start the cast only when all streams are added into the
             * last sout (the http one) */
            p_sys->renderer->SetAVTransportURI(p_sys->transport_uri.c_str(), *p_sys->protocol);
            p_sys->renderer->Play("1");
            p_sys->cc_has_input = true;
        }
        return ret;
    }
    else
    {
        block_ChainRelease(p_buffer);
        return VLC_SUCCESS;
    }
}

static void ProxyFlush(sout_stream_t *p_stream, void *id)
{
    sout_StreamFlush(p_stream->p_next, id);
}

int ProxyOpen(vlc_object_t *p_this)
{
    sout_stream_t *p_stream = reinterpret_cast<sout_stream_t*>(p_this);
    sout_stream_sys_t *p_sys = (sout_stream_sys_t *) var_InheritAddress(p_this, SOUT_CFG_PREFIX "sys");
    if (p_sys == NULL || p_stream->p_next == NULL)
        return VLC_EGENERIC;

    p_stream->p_sys = (sout_stream_sys_t *) p_sys;
    p_sys->out_streams_added = 0;

    p_stream->pf_add     = ProxyAdd;
    p_stream->pf_del     = ProxyDel;
    p_stream->pf_send    = ProxySend;
    p_stream->pf_flush   = ProxyFlush;
    return VLC_SUCCESS;
}

ProtocolPtr sout_stream_sys_t::canDecodeAudio(vlc_fourcc_t audio_codec) const
{
    for (protocol_info_t protocol : device_protocols) {
        if ((protocol.profile.mime.substr(0, 6) == "audio/"
                || protocol.profile.mime.substr(0, 6) == "application/")
                && protocol.profile.audio_codec == audio_codec)
        {
            return std::make_unique<protocol_info_t>(protocol);
        }
    }
    return nullptr;
}

ProtocolPtr sout_stream_sys_t::canDecodeVideo(vlc_fourcc_t audio_codec,
                vlc_fourcc_t video_codec) const
{
    for (protocol_info_t protocol : device_protocols) {
        if (protocol.profile.mime.substr(0, 6) == "video/"
                && protocol.profile.audio_codec == audio_codec
                && protocol.profile.video_codec == video_codec)
        {
            return std::make_unique<protocol_info_t>(protocol);
        }
    }
    return nullptr;
}

bool sout_stream_sys_t::startSoutChain(sout_stream_t *p_stream,
                                       const std::vector<sout_stream_id_sys_t*> &new_streams,
                                       const std::string &sout)
{
    msg_Dbg( p_stream, "Creating chain %s", sout.c_str() );
    cc_has_input = false;
    first_video_keyframe_pts = -1;
    video_proxy_id = NULL;
    has_video = false;
    out_streams = new_streams;
    spu_streams_count = 0;

    p_out = sout_StreamChainNew( p_stream->p_sout, sout.c_str(), NULL, NULL);
    if (p_out == NULL) {
        msg_Err(p_stream, "could not create sout chain:%s", sout.c_str());
        out_streams.clear();
        return false;
    }

    /* check the streams we can actually add */
    for (std::vector<sout_stream_id_sys_t*>::iterator it = out_streams.begin();
            it != out_streams.end(); )
    {
        sout_stream_id_sys_t *p_sys_id = *it;
        p_sys_id->p_sub_id = static_cast<sout_stream_id_sys_t *>(
                sout_StreamIdAdd( p_out, &p_sys_id->fmt ) );
        if ( p_sys_id->p_sub_id == NULL )
        {
            msg_Err( p_stream, "can't handle %4.4s stream",
                    (char *)&p_sys_id->fmt.i_codec );
            es_format_Clean( &p_sys_id->fmt );
            it = out_streams.erase( it );
        }
        else
        {
            if( p_sys_id->fmt.i_cat == VIDEO_ES )
                has_video = true;
            else if( p_sys_id->fmt.i_cat == SPU_ES )
                spu_streams_count++;
            ++it;
        }
    }

    if (out_streams.empty())
    {
        stopSoutChain( p_stream );
        return false;
    }

    return true;
}

void sout_stream_sys_t::stopSoutChain(sout_stream_t *p_stream)
{
    (void) p_stream;

    for ( size_t i = 0; i < out_streams.size(); i++ )
    {
        sout_StreamIdDel( p_out, out_streams[i]->p_sub_id );
        out_streams[i]->p_sub_id = NULL;
    }
    out_streams.clear();
    sout_StreamChainDelete( p_out, NULL );
    p_out = NULL;
}

sout_stream_id_sys_t *sout_stream_sys_t::GetSubId( sout_stream_t *p_stream,
                                                   sout_stream_id_sys_t *id,
                                                   bool update)
{
    assert( p_stream->p_sys == this );

    if ( update && UpdateOutput( p_stream ) != VLC_SUCCESS )
        return NULL;

    for (size_t i = 0; i < out_streams.size(); ++i)
    {
        if ( id == (sout_stream_id_sys_t*) out_streams[i] )
            return out_streams[i]->p_sub_id;
    }

    msg_Err( p_stream, "unknown stream ID" );
    return NULL;
}

int sout_stream_sys_t::UpdateOutput( sout_stream_t *p_stream )
{
    assert( p_stream->p_sys == this );

    if ( !es_changed )
        return VLC_SUCCESS;

    es_changed = false;

    bool canRemux = true;
    vlc_fourcc_t i_codec_video = 0, i_codec_audio = 0;
    std::vector<sout_stream_id_sys_t*> new_streams;

    for (sout_stream_id_sys_t *stream : streams)
    {
        const es_format_t *p_es = &stream->fmt;
        if (p_es->i_cat == AUDIO_ES)
        {
            i_codec_audio = p_es->i_codec;
            new_streams.push_back(stream);
        }
        else if (b_supports_video && p_es->i_cat == VIDEO_ES)
        {
            i_codec_video = p_es->i_codec;
            new_streams.push_back(stream);
        }
    }

    if (new_streams.empty())
        return VLC_SUCCESS;

    ProtocolPtr stream_protocol;
    // check if we have an audio only stream
    if (!i_codec_video && i_codec_audio)
    {
        if( !(stream_protocol = canDecodeAudio(i_codec_audio)) )
        {
            stream_protocol = make_protocol(default_audio_protocol);
            canRemux = false;
        }
    }
    else
    {
        if( !(stream_protocol = canDecodeVideo(i_codec_audio, i_codec_video)) )
        {
            stream_protocol = make_protocol(default_video_protocol);
            canRemux = false;
        }
    }

    msg_Dbg( p_stream, "using DLNA profile %s:%s",
                stream_protocol->profile.mime.c_str(),
                stream_protocol->profile.name.c_str() );

    std::ostringstream ssout;
    if ( !canRemux )
    {
        /* TODO: provide audio samplerate and channels */
        ssout << "transcode{";
        char s_fourcc[5];
        if ( i_codec_audio )
        {
            i_codec_audio = stream_protocol->profile.audio_codec;
            msg_Dbg( p_stream, "Converting audio to %.4s",
                    (const char*)&i_codec_audio );
            ssout << "acodec=";
            vlc_fourcc_to_char( i_codec_audio, s_fourcc );
            s_fourcc[4] = '\0';
            ssout << s_fourcc << ',';
        }
        if ( i_codec_video )
        {
            i_codec_video = stream_protocol->profile.video_codec;
            msg_Dbg( p_stream, "Converting video to %.4s",
                    (const char*)&i_codec_video );
            /* TODO: provide maxwidth,maxheight */
            ssout << "vcodec=";
            vlc_fourcc_to_char( i_codec_video, s_fourcc );
            s_fourcc[4] = '\0';
            ssout << s_fourcc;
        }
        ssout << "}:";
    }

    std::ostringstream ss;
    ss << "/dlna"
       << "/" << vlc_tick_now()
       << "/" << static_cast<uint64_t>( vlc_mrand48() )
       << "/stream"
       << ".mp4";
    std::string root_url = ss.str();

    ssout << "cast-proxy:"
          << "http{dst=:" << http_port << root_url
          << ",mux=" << stream_protocol->profile.mux
          << ",access=http{mime=" << stream_protocol->profile.mime << "}}";

    if ( !startSoutChain( p_stream, new_streams, ssout.str() ) )
        return VLC_EGENERIC;

    char *ip = getIpv4ForMulticast();
    if (ip == NULL)
    {
        ip = UpnpGetServerIpAddress();
    }
    if (ip == NULL)
    {
        msg_Err(p_stream, "could not get the local ip address");
        return VLC_EGENERIC;
    }

    char *uri;
    if (asprintf(&uri, "http://%s:%d%s", ip, http_port, root_url.c_str()) < 0) {
        return VLC_ENOMEM;
    }

    msg_Dbg(p_stream, "AVTransportURI: %s", uri);
    transport_uri = uri;
    protocol = make_protocol(*stream_protocol);

    free(uri);
    return VLC_SUCCESS;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

namespace Sout
{

char *MediaRenderer::getServiceURL(const char* type, const char *service)
{
    IXML_Document *p_description_doc = NULL;
    if (UpnpDownloadXmlDoc(device_url.c_str(), &p_description_doc) != UPNP_E_SUCCESS)
        return NULL;

    IXML_NodeList* p_device_list = ixmlDocument_getElementsByTagName( p_description_doc, "device");
    free(p_description_doc);
    if ( !p_device_list )
        return NULL;

    for (unsigned int i = 0; i < ixmlNodeList_length(p_device_list); ++i)
    {
        IXML_Element* p_device_element = ( IXML_Element* ) ixmlNodeList_item( p_device_list, i );
        if( !p_device_element )
            continue;

        IXML_NodeList* p_service_list = ixmlElement_getElementsByTagName( p_device_element, "service" );
        if ( !p_service_list )
            continue;
        for ( unsigned int j = 0; j < ixmlNodeList_length( p_service_list ); j++ )
        {
            IXML_Element* p_service_element = (IXML_Element*)ixmlNodeList_item( p_service_list, j );

            const char* psz_service_type = xml_getChildElementValue( p_service_element, "serviceType" );
            if ( !psz_service_type || !strstr(psz_service_type, type))
                continue;
            const char* psz_control_url = xml_getChildElementValue( p_service_element,
                                                                    service );
            if ( !psz_control_url )
                continue;

            char* psz_url = ( char* ) malloc( base_url.length() + strlen( psz_control_url ) + 1 );
            if ( psz_url && UpnpResolveURL( base_url.c_str(), psz_control_url, psz_url ) == UPNP_E_SUCCESS )
                return psz_url;
            return NULL;
        }
    }
    return NULL;
}

/**
 * Send an action to the control url of the service specified.
 *
 * \return the response as a IXML document or NULL for failure
 **/
IXML_Document *MediaRenderer::SendAction(const char* action_name,const char *service_type,
                    std::list<std::pair<const char*, const char*>> arguments)
{
    /* Create action */
    IXML_Document *action = UpnpMakeAction(action_name, service_type, 0, NULL);

    /* Add argument to action */
    for (std::pair<const char*, const char*> arg : arguments) {
      const char *arg_name, *arg_val;
      arg_name = arg.first;
      arg_val  = arg.second;
      UpnpAddToAction(&action, action_name, service_type, arg_name, arg_val);
    }

    /* Get the controlURL of the service */
    char *control_url = getServiceURL(service_type, "controlURL");

    /* Send action */
    IXML_Document *response = NULL;
    int ret = UpnpSendAction(handle, control_url, service_type,
                                    NULL, action, &response);

    /* Free action */
    if (action) ixmlDocument_free(action);
    if (control_url) free(control_url);

    if (ret != UPNP_E_SUCCESS) {
        msg_Err(parent, "Unable to send action: %s (%d: %s) response: %s",
                action_name, ret, UpnpGetErrorMessage(ret), ixmlPrintDocument(response));
        if (response) ixmlDocument_free(response);
        return NULL;
    }

    return response;
}

int MediaRenderer::Play(const char *speed)
{
    std::list<std::pair<const char*, const char*>> arg_list;
    arg_list.push_back(std::make_pair("InstanceID", "0"));
    arg_list.push_back(std::make_pair("Speed", speed));

    IXML_Document *p_response = SendAction("Play", AV_TRANSPORT_SERVICE_TYPE, arg_list);
    if(!p_response)
    {
        return VLC_EGENERIC;
    }
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

int MediaRenderer::Stop()
{
    std::list<std::pair<const char*, const char*>> arg_list;
    arg_list.push_back(std::make_pair("InstanceID", "0"));

    IXML_Document *p_response = SendAction("Stop", AV_TRANSPORT_SERVICE_TYPE, arg_list);
    if(!p_response)
    {
        return VLC_EGENERIC;
    }
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

std::vector<protocol_info_t> MediaRenderer::GetProtocolInfo()
{
    std::string protocol_csv;
    std::vector<protocol_info_t> supported_protocols;
    std::list<std::pair<const char*, const char*>> arg_list;

    IXML_Document *response = SendAction("GetProtocolInfo",
                                CONNECTION_MANAGER_SERVICE_TYPE, arg_list);
    if(!response)
    {
        return supported_protocols;
    }

    // Get the CSV list of protocols/profiles supported by the device
    if( IXML_NodeList *protocol_list = ixmlDocument_getElementsByTagName( response , "Sink" ) )
    {
        if ( IXML_Node* protocol_node = ixmlNodeList_item( protocol_list, 0 ) )
        {
            IXML_Node* p_text_node = ixmlNode_getFirstChild( protocol_node );
            if ( p_text_node )
            {
                protocol_csv.assign(ixmlNode_getNodeValue( p_text_node ));
            }
        }
        ixmlNodeList_free( protocol_list);
    }
    ixmlDocument_free(response);

    // parse the CSV list
    // format: <transportProtocol>:<network>:<mime>:<additionalInfo>
    std::vector<std::string> protocols = split(protocol_csv, ',');
    for (std::string protocol : protocols ) {
        std::vector<std::string> protocol_info = split(protocol, ':');

        msg_Dbg(parent, "Device supports protocols: %s", protocol.c_str());
        // We only support http transport for now.
        if (protocol_info.size() == 4 && protocol_info.at(0) == "http-get")
        {
            protocol_info_t proto;
            proto.protocol_str = protocol;
            proto.transport = protocol_info.at(0);

            // Get the DLNA profile name
            std::string profile_name;
            std::string tag = "DLNA.ORG_PN=";

            if (protocol_info.at(3) == "*")
            {
               profile_name = "*";
            }
            else if (std::size_t index = protocol_info.at(3).find(tag) != std::string::npos)
            {
                std::size_t end = protocol_info.at(3).find(";", index);
                profile_name = protocol_info.at(3).substr(index + tag.length() - 1, end);
            }

            // Match our supported profiles to device profiles
            for (dlna_profile profile : dlna_profile_list) {
                if (protocol_info.at(2) == profile.mime
                        && profile_name == profile.name)
                {
                    proto.profile = std::move(profile);
                    supported_protocols.push_back(proto);
                    //we do not break here to account for wildcard profile names
                }
            }
        }
    }

    msg_Dbg( parent , "Got %lu supported profiles", supported_protocols.size() );
    return supported_protocols;
}

int MediaRenderer::SetAVTransportURI(const char* uri, const protocol_info_t proto)
{
    static const char didl[] =
        "<DIDL-Lite "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
        "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" "
        "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
        "<item id=\"f-0\" parentID=\"0\" restricted=\"0\">"
        "<dc:title>%s</dc:title>"
        "<upnp:class>%s</upnp:class>"
        "<res protocolInfo=\"%s\">%s</res>"
        "</item>"
        "</DIDL-Lite>";

    bool audio = proto.profile.mime.substr(0, 6) == "audio/";
    char *meta_data;
    if (asprintf(&meta_data, didl,
                audio ? "Audio" : "Video",
                audio ? "object.item.audioItem" : "object.item.videoItem",
                proto.protocol_str.c_str(),
                uri) < 0) {
        return VLC_ENOMEM;
    }

    msg_Dbg(parent, "didl: %s", meta_data);
    std::list<std::pair<const char*, const char*>> arg_list;
    arg_list.push_back(std::make_pair("InstanceID", "0"));
    arg_list.push_back(std::make_pair("CurrentURI", uri));
    arg_list.push_back(std::make_pair("CurrentURIMetaData", meta_data));

    IXML_Document *p_response = SendAction("SetAVTransportURI",
                                    AV_TRANSPORT_SERVICE_TYPE, arg_list);

    free(meta_data);
    if(!p_response)
    {
        return VLC_EGENERIC;
    }
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

int MediaRenderer::Subscribe()
{
    int timeout = 300;
    Upnp_SID sid1;

    char *url = getServiceURL(RENDERING_CONTROL_SERVICE_TYPE, "eventSubURL");
    int ret = UpnpSubscribe(handle, url, &timeout, sid1);
    if (ret != UPNP_E_SUCCESS) {
        msg_Err(parent, "Unable to Subscribe to %s: %d",
                url, ret);
        return VLC_EGENERIC;
    }
    Upnp_sid = sid1;
    msg_Dbg(parent, "Subscribed successfully to %s with timeout: %d",
            url, timeout);
    return VLC_SUCCESS;
}

int MediaRenderer::UnSubscribe()
{
    int ret = UpnpUnSubscribe(handle, Upnp_sid.c_str());
    if (ret != UPNP_E_SUCCESS) {
        msg_Err(parent, "Unable to unsubscribe to %s: %d",
                Upnp_sid.c_str(), ret);
        return VLC_EGENERIC;
    }
    msg_Dbg(parent, "Unsubscribed successfully to %s", Upnp_sid.c_str());
    return VLC_SUCCESS;
}

static void *Add(sout_stream_t *p_stream, const es_format_t *p_fmt)
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );

    if (!p_sys->b_supports_video)
    {
        if (p_fmt->i_cat != AUDIO_ES)
            return NULL;
    }

    sout_stream_id_sys_t *p_sys_id = (sout_stream_id_sys_t *)malloc(sizeof(sout_stream_id_sys_t));
    if(p_sys_id != NULL)
    {
        es_format_Copy(&p_sys_id->fmt, p_fmt);
        p_sys_id->p_sub_id = NULL;
        p_sys->streams.push_back(p_sys_id);
        p_sys->es_changed = true;
    }
    return p_sys_id;
}

static int Send(sout_stream_t *p_stream, void *id,
                block_t *p_buffer)
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id_sys = static_cast<sout_stream_id_sys_t*>( id );

    id_sys = p_sys->GetSubId( p_stream, id_sys );
    if ( id_sys == NULL )
        return VLC_EGENERIC;

    return sout_StreamIdSend(p_sys->p_out, id_sys, p_buffer);
}

static void Flush( sout_stream_t *p_stream, void *id )
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id_sys = static_cast<sout_stream_id_sys_t*>( id );

    id_sys = p_sys->GetSubId( p_stream, id_sys, false );
    if ( id_sys == NULL )
        return;

    sout_StreamFlush( p_sys->p_out, id_sys );
}

static int Control(sout_stream_t *p_stream, int i_query, va_list args)
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );

    if (i_query == SOUT_STREAM_EMPTY)
        return VLC_SUCCESS;
    if (!p_sys->p_out->pf_control)
        return VLC_EGENERIC;
    return p_sys->p_out->pf_control(p_sys->p_out, i_query, args);
}

static void Del(sout_stream_t *p_stream, void *_id)
{
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );
    sout_stream_id_sys_t *id = static_cast<sout_stream_id_sys_t *>( _id );

    for (std::vector<sout_stream_id_sys_t*>::iterator it = p_sys->streams.begin();
         it != p_sys->streams.end(); )
    {
        sout_stream_id_sys_t *p_sys_id = *it;
        if ( p_sys_id == id )
        {
            if ( p_sys_id->p_sub_id != NULL )
            {
                sout_StreamIdDel( p_sys->p_out, p_sys_id->p_sub_id );
                for (std::vector<sout_stream_id_sys_t*>::iterator out_it = p_sys->out_streams.begin();
                     out_it != p_sys->out_streams.end(); )
                {
                    if (*out_it == id)
                    {
                        p_sys->out_streams.erase(out_it);
                        if( p_sys_id->fmt.i_cat == VIDEO_ES )
                            p_sys->has_video = false;
                        else if( p_sys_id->fmt.i_cat == SPU_ES )
                            p_sys->spu_streams_count--;
                        break;
                    }
                    out_it++;
                }
            }

            es_format_Clean( &p_sys_id->fmt );
            free( p_sys_id );
            p_sys->streams.erase( it );
            break;
        }
        it++;
    }

    if (p_sys->out_streams.empty())
    {
        p_sys->stopSoutChain(p_stream);
        p_sys->renderer->Stop();
    }
}

int OpenSout( vlc_object_t *p_this )
{
    sout_stream_t *p_stream = reinterpret_cast<sout_stream_t*>(p_this);
    sout_stream_sys_t *p_sys = NULL;

    config_ChainParse(p_stream, SOUT_CFG_PREFIX, ppsz_sout_options, p_stream->p_cfg);

    int http_port = var_InheritInteger(p_stream, SOUT_CFG_PREFIX "http-port");
    bool b_supports_video = var_GetBool(p_stream, SOUT_CFG_PREFIX "video");
    char *base_url = var_GetNonEmptyString(p_stream, SOUT_CFG_PREFIX "base_url");
    char *device_url = var_GetNonEmptyString(p_stream, SOUT_CFG_PREFIX "url");
    if ( device_url == NULL)
    {
        msg_Err( p_stream, "missing Url" );
        goto error;
    }

    try {
        p_sys = new sout_stream_sys_t(http_port, b_supports_video);
    }
    catch ( const std::exception& ex ) {
        msg_Err( p_stream, "Failed to instantiate sout_stream_sys_t: %s", ex.what() );
        return VLC_EGENERIC;
    }

    p_sys->p_upnp = UpnpInstanceWrapper::get( p_this );
    if ( !p_sys->p_upnp )
        goto error;
    try {
        p_sys->renderer = std::make_shared<Sout::MediaRenderer>(p_stream,
                            p_sys->p_upnp, base_url, device_url);
    }
    catch ( const std::bad_alloc& ) {
        msg_Err( p_stream, "Failed to create a MediaRenderer");
        p_sys->p_upnp->release();
        goto error;
    }

    var_Create( p_stream->p_sout, SOUT_CFG_PREFIX "sys", VLC_VAR_ADDRESS );
    var_SetAddress( p_stream->p_sout, SOUT_CFG_PREFIX "sys", p_sys );

    p_sys->renderer->Subscribe();
    p_sys->device_protocols = p_sys->renderer->GetProtocolInfo();

    p_stream->pf_add     = Add;
    p_stream->pf_del     = Del;
    p_stream->pf_send    = Send;
    p_stream->pf_flush   = Flush;
    p_stream->pf_control = Control;

    p_stream->p_sys = p_sys;

    free(base_url);
    free(device_url);

    return VLC_SUCCESS;

error:
    free(base_url);
    free(device_url);
    delete p_sys;
    return VLC_EGENERIC;
}

void CloseSout( vlc_object_t *p_this)
{
    sout_stream_t *p_stream = reinterpret_cast<sout_stream_t*>( p_this );
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );
    var_Destroy( p_stream->p_sout, SOUT_CFG_PREFIX "sys" );

    p_sys->renderer->UnSubscribe();
    p_sys->p_upnp->release();
    delete p_sys;
}

}
