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

#include <vlc_rand.h>
#include <vlc_sout.h>

static const char* AV_TRANSPORT_SERVICE_TYPE = "urn:schemas-upnp-org:service:AVTransport:1";
static const char* CONNECTION_MANAGER_SERVICE_TYPE = "urn:schemas-upnp-org:service:ConnectionManager:1";

static const char *const ppsz_sout_options[] = {
    "ip", "port", "http-port", "mux", "mime", "video", "base_url", "url", NULL
};

struct sout_stream_id_sys_t
{
    es_format_t           fmt;
    sout_stream_id_sys_t  *p_sub_id;
};

struct sout_stream_sys_t
{
    sout_stream_sys_t(int http_port, bool supports_video,
            std::string default_mux, std::string default_mime)
        : p_out(NULL)
        , default_muxer(default_mux)
        , default_mime(default_mime)
        , b_supports_video(supports_video)
        , es_changed(true)
        , http_port(http_port)
    {
    }

    std::shared_ptr<Sout::MediaRenderer> renderer;
    UpnpInstanceWrapper *p_upnp;

    bool canDecodeAudio( vlc_fourcc_t i_codec ) const;
    bool canDecodeVideo( vlc_fourcc_t i_codec ) const;
    bool startSoutChain( sout_stream_t* p_stream,
                         const std::vector<sout_stream_id_sys_t*> &new_streams,
                         const std::string &sout );
    void stopSoutChain( sout_stream_t* p_stream );
    sout_stream_id_sys_t *GetSubId( sout_stream_t *p_stream,
                                    sout_stream_id_sys_t *id,
                                    bool update = true );

    sout_stream_t                       *p_out;
    std::string                         default_muxer;
    std::string                         default_mime;
    bool                                b_supports_video;
    bool                                es_changed;
    int                                 http_port;
    std::vector<sout_stream_id_sys_t*>  streams;
    std::vector<sout_stream_id_sys_t*>  out_streams;

    int UpdateOutput( sout_stream_t *p_stream );

};

bool sout_stream_sys_t::canDecodeAudio(vlc_fourcc_t i_codec) const
{
    return i_codec == VLC_CODEC_MP4A;
}

bool sout_stream_sys_t::canDecodeVideo(vlc_fourcc_t i_codec) const
{
    return i_codec == VLC_CODEC_H264;
}

bool sout_stream_sys_t::startSoutChain(sout_stream_t *p_stream,
                                       const std::vector<sout_stream_id_sys_t*> &new_streams,
                                       const std::string &sout)
{
    msg_Dbg( p_stream, "Creating chain %s", sout.c_str() );
    out_streams = new_streams;

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
            ++it;
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

    for (std::vector<sout_stream_id_sys_t*>::iterator it = streams.begin();
            it != streams.end(); ++it)
    {
        const es_format_t *p_es = &(*it)->fmt;
        if (p_es->i_cat == AUDIO_ES)
        {
            if (!canDecodeAudio( p_es->i_codec ))
            {
                msg_Dbg( p_stream, "can't remux audio track %d codec %4.4s",
                        p_es->i_id, (const char*)&p_es->i_codec );
                canRemux = false;
            }
            else if (i_codec_audio == 0)
            {
                i_codec_audio = p_es->i_codec;
            }
            new_streams.push_back(*it);
        }
        else if (b_supports_video && p_es->i_cat == VIDEO_ES)
        {
            if (!canDecodeVideo( p_es->i_codec ))
            {
                msg_Dbg( p_stream, "can't remux video track %d codec %4.4s",
                        p_es->i_id, (const char*)&p_es->i_codec );
                canRemux = false;
            }
            else if (i_codec_video == 0)
            {
                i_codec_video = p_es->i_codec;
            }
            new_streams.push_back(*it);
        }
    }

    if (new_streams.empty())
    {
        return VLC_SUCCESS;
    }

    std::ostringstream ssout;
    if ( !canRemux )
    {
        /* TODO: provide audio samplerate and channels */
        ssout << "transcode{";
        char s_fourcc[5];
        if ( i_codec_audio == 0 )
        {
            i_codec_audio = DEFAULT_TRANSCODE_AUDIO;
            msg_Dbg( p_stream, "Converting audio to %.4s",
                    (const char*)&i_codec_audio );
            ssout << "acodec=";
            vlc_fourcc_to_char( i_codec_audio, s_fourcc );
            s_fourcc[4] = '\0';
            ssout << s_fourcc << ',';
        }
        if ( b_supports_video && i_codec_video == 0 )
        {
            i_codec_video = DEFAULT_TRANSCODE_VIDEO;
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

    std::string mime;
    std::string mux;

    mime = default_mime;
    mux  = default_muxer;

    std::ostringstream ss;
    ss << "/dlna"
       << "/" << vlc_tick_now()
       << "/" << static_cast<uint64_t>( vlc_mrand48() )
       << "/stream";
    std::string root_url = ss.str();

    ssout << "http{dst=:" << http_port << root_url
          << ",mux=" << mux
          << ",access=http{mime=" << mime << "}}";

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
    renderer->SetAVTransportURI(uri);
    renderer->Play("1");

    return VLC_SUCCESS;
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
        ixmlDocument_free(p_response);
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
        ixmlDocument_free(p_response);
        return VLC_EGENERIC;
    }
    ixmlDocument_free(p_response);
    return VLC_SUCCESS;
}

int MediaRenderer::SetAVTransportURI(const char* uri)
{
    std::list<std::pair<const char*, const char*>> arg_list;
    arg_list.push_back(std::make_pair("InstanceID", "0"));
    arg_list.push_back(std::make_pair("CurrentURI", uri));
    arg_list.push_back(std::make_pair("CurrentURIMetaData", "")); // NOT_IMPLEMENTED

    IXML_Document *p_response = SendAction("SetAVTransportURI",
                                    AV_TRANSPORT_SERVICE_TYPE, arg_list);
    if(!p_response)
    {
        ixmlDocument_free(p_response);
        return VLC_EGENERIC;
    }
    ixmlDocument_free(p_response);
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
    char *default_muxer = var_GetNonEmptyString(p_stream, SOUT_CFG_PREFIX "mux");
    char *default_mime = var_GetNonEmptyString(p_stream,  SOUT_CFG_PREFIX "mime");
    char *base_url = var_GetNonEmptyString(p_stream, SOUT_CFG_PREFIX "base_url");
    char *device_url = var_GetNonEmptyString(p_stream, SOUT_CFG_PREFIX "url");
    if ( device_url == NULL)
    {
        msg_Err( p_stream, "missing Url" );
        goto error;
    }

    try {
        p_sys = new sout_stream_sys_t(http_port, b_supports_video,
                                        default_muxer, default_mime);
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

    p_stream->pf_add     = Add;
    p_stream->pf_del     = Del;
    p_stream->pf_send    = Send;
    p_stream->pf_flush   = Flush;
    p_stream->pf_control = Control;

    p_stream->p_sys = p_sys;

    free(default_mime);
    free(default_muxer);
    free(base_url);
    free(device_url);

    return VLC_SUCCESS;

error:
    free(default_mime);
    free(default_muxer);
    free(base_url);
    free(device_url);
    delete p_sys;
    return VLC_EGENERIC;
}

void CloseSout( vlc_object_t *p_this)
{
    sout_stream_t *p_stream = reinterpret_cast<sout_stream_t*>( p_this );
    sout_stream_sys_t *p_sys = static_cast<sout_stream_sys_t *>( p_stream->p_sys );

    p_sys->p_upnp->release();
    delete p_sys;
}

}
