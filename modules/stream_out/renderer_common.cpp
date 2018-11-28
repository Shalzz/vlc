/*****************************************************************************
 * renderer_common.cpp : renderer helper functions
 *****************************************************************************
 * Copyright © 2014-2018 VideoLAN
 *
 * Authors: Adrien Maglo <magsoft@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Steve Lhomme <robux4@videolabs.io>
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

#include <assert.h>
#include <sstream>

#include "renderer_common.hpp"

std::string
vlc_sout_renderer_GetVencOption( sout_stream_t *p_stream, int venc_opt_idx,
        venc_options venc_opt_list[], size_t opt_list_length,
        vlc_fourcc_t *p_codec_video, const video_format_t *p_vid,
        int i_quality )
{
    for( size_t i = (venc_opt_idx == -1 ? 0 : venc_opt_idx);
         i < opt_list_length; ++i )
    {
        std::stringstream ssout, ssvenc;
        char fourcc[5];
        ssvenc << "vcodec=";
        vlc_fourcc_to_char( venc_opt_list[i].fcc, fourcc );
        fourcc[4] = '\0';
        ssvenc << fourcc << ',';

        if( venc_opt_list[i].get_opt != NULL )
            ssvenc << venc_opt_list[i].get_opt( p_stream, p_vid, i_quality ) << ',';

        if( venc_opt_list[i].get_opt == NULL
         || ( venc_opt_idx != -1 && (unsigned) venc_opt_idx == i) )
        {
            venc_opt_idx = i;
            *p_codec_video = venc_opt_list[i].fcc;
            return ssvenc.str();
        }

        /* Test if a module can encode with the specified options / fmt_video. */
        ssout << "transcode{" << ssvenc.str() << "}:dummy";

        sout_stream_t *p_sout_test =
            sout_StreamChainNew( p_stream->p_sout, ssout.str().c_str(), NULL, NULL );

        if( p_sout_test != NULL )
        {
            p_sout_test->obj.flags |= OBJECT_FLAGS_QUIET|OBJECT_FLAGS_NOINTERACT;

            es_format_t fmt;
            es_format_InitFromVideo( &fmt, p_vid );
            fmt.i_codec = fmt.video.i_chroma = VLC_CODEC_I420;

            /* Test the maximum size/fps we will encode */
            fmt.video.i_visible_width = fmt.video.i_width = 1920;
            fmt.video.i_visible_height = fmt.video.i_height = 1080;
            fmt.video.i_frame_rate = 30;
            fmt.video.i_frame_rate_base = 1;

            void *id = sout_StreamIdAdd( p_sout_test, &fmt );

            es_format_Clean( &fmt );
            const bool success = id != NULL;

            if( id )
                sout_StreamIdDel( p_sout_test, id );
            sout_StreamChainDelete( p_sout_test, NULL );

            if( success )
            {
                venc_opt_idx = i;
                *p_codec_video = venc_opt_list[i].fcc;
                return ssvenc.str();
            }
        }
    }
    vlc_assert_unreachable();
}

std::string vlc_sout_renderer_GetVencVPXOption( sout_stream_t * /* p_stream */,
                                      const video_format_t * /* p_vid */,
                                      int /* i_quality */ )
{
    return "venc=vpx{quality-mode=1}";
}

std::string vlc_sout_renderer_GetVencQSVH264Option( sout_stream_t * /* p_stream */,
                                         const video_format_t * /* p_vid */,
                                         int i_quality )
{
    std::stringstream ssout;
    static const char video_target_usage_quality[]  = "quality";
    static const char video_target_usage_balanced[] = "balanced";
    static const char video_target_usage_speed[]    = "speed";
    static const char video_bitrate_high[] = "vb=8000000";
    static const char video_bitrate_low[]  = "vb=3000000";
    const char *psz_video_target_usage;
    const char *psz_video_bitrate;

    switch ( i_quality )
    {
        case CONVERSION_QUALITY_HIGH:
            psz_video_target_usage = video_target_usage_quality;
            psz_video_bitrate = video_bitrate_high;
            break;
        case CONVERSION_QUALITY_MEDIUM:
            psz_video_target_usage = video_target_usage_balanced;
            psz_video_bitrate = video_bitrate_high;
            break;
        case CONVERSION_QUALITY_LOW:
            psz_video_target_usage = video_target_usage_balanced;
            psz_video_bitrate = video_bitrate_low;
            break;
        default:
        case CONVERSION_QUALITY_LOWCPU:
            psz_video_target_usage = video_target_usage_speed;
            psz_video_bitrate = video_bitrate_low;
            break;
    }

    ssout << "venc=qsv{target-usage=" << psz_video_target_usage <<
             "}," << psz_video_bitrate;
    return ssout.str();
}

std::string vlc_sout_renderer_GetVencX264Option( sout_stream_t * /* p_stream */,
                                      const video_format_t *p_vid,
                                      int i_quality )
{
    std::stringstream ssout;
    static const char video_x264_preset_veryfast[] = "veryfast";
    static const char video_x264_preset_ultrafast[] = "ultrafast";
    const char *psz_video_x264_preset;
    unsigned i_video_x264_crf_hd, i_video_x264_crf_720p;

    switch ( i_quality )
    {
        case CONVERSION_QUALITY_HIGH:
            i_video_x264_crf_hd = i_video_x264_crf_720p = 21;
            psz_video_x264_preset = video_x264_preset_veryfast;
            break;
        case CONVERSION_QUALITY_MEDIUM:
            i_video_x264_crf_hd = 23;
            i_video_x264_crf_720p = 21;
            psz_video_x264_preset = video_x264_preset_veryfast;
            break;
        case CONVERSION_QUALITY_LOW:
            i_video_x264_crf_hd = i_video_x264_crf_720p = 23;
            psz_video_x264_preset = video_x264_preset_veryfast;
            break;
        default:
        case CONVERSION_QUALITY_LOWCPU:
            i_video_x264_crf_hd = i_video_x264_crf_720p = 23;
            psz_video_x264_preset = video_x264_preset_ultrafast;
            break;
    }

    const bool b_hdres = p_vid->i_height == 0 || p_vid->i_height >= 800;
    unsigned i_video_x264_crf = b_hdres ? i_video_x264_crf_hd : i_video_x264_crf_720p;

    ssout << "venc=x264{preset=" << psz_video_x264_preset
          << ",crf=" << i_video_x264_crf << "}";
    return ssout.str();
}

#ifdef __APPLE__
std::string vlc_sout_renderer_GetVencAvcodecVTOption( sout_stream_t * /* p_stream */,
                                           const video_format_t * p_vid,
                                           int i_quality )
{
    std::stringstream ssout;
    ssout << "venc=avcodec{codec=h264_videotoolbox,options{realtime=1}}";
    switch( i_quality )
    {
        /* Here, performances issues won't come from videotoolbox but from
         * some old chromecast devices */

        case CONVERSION_QUALITY_HIGH:
            break;
        case CONVERSION_QUALITY_MEDIUM:
            ssout << ",vb=8000000";
            break;
        case CONVERSION_QUALITY_LOW:
        case CONVERSION_QUALITY_LOWCPU:
            ssout << ",vb=3000000";
            break;
    }

    return ssout.str();
}
#endif
