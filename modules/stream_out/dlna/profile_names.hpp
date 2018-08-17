/*****************************************************************************
 * profile_names.hpp : DLNA media profile names
 *****************************************************************************
 * Copyright (C) 2004-2018 VLC authors and VideoLAN
 *
 * Authors: Shaleen Jain <shaleen@jain.sh>
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

#ifndef DLNA_PROFILES_H
#define DLNA_PROFILES_H

#include<string>
#include<vlc_common.h>
#include<vlc_fourcc.h>
#include<vector>

#define VLC_CODEC_NONE 0
#define CODEC_PROFILE_NONE 0

class dlna_profile {
public:
    std::string name;
    std::string mux;
    std::string mime;
    vlc_fourcc_t video_codec;
    vlc_fourcc_t audio_codec;

    dlna_profile()
        : name{}
        , mux{}
        , video_codec{VLC_CODEC_UNKNOWN}
        , audio_codec{VLC_CODEC_UNKNOWN}
        {};

    dlna_profile(std::string profile, std::string mux, std::string mime,
            vlc_fourcc_t video, vlc_fourcc_t audio)
        : name{profile}
        , mux{mux}
        , mime{mime}
        , video_codec{video}
        , audio_codec{audio}
        {};

};

const dlna_profile default_audio_profile = {
    "MP3",
    "ts",
    "audio/mpeg",
    VLC_CODEC_NONE,
    VLC_CODEC_MPGA,
};

/**
 * AVC Main Profile SD video with MPEG-4 AAC audio, encapsulated in MP4.
 */
const dlna_profile default_video_profile = {
    "AVC_MP4_MP_SD",
    "mp4stream",
    "video/mp4",
    VLC_CODEC_H264,
    VLC_CODEC_MP4A,
};

std::vector<dlna_profile> dlna_profile_list = {

    default_audio_profile,
    default_video_profile,

    //------ Audio Profiles ------//

    {
        "*",
        "ts",
        "audio/mpeg",
        VLC_CODEC_NONE,
        VLC_CODEC_MP3,
    },
    {
        "*",
        "ogg",
        "application/ogg",
        VLC_CODEC_NONE,
        VLC_CODEC_VORBIS,
    },
    {
        "*",
        "ogg",
        "application/ogg",
        VLC_CODEC_NONE,
        VLC_CODEC_OPUS,
    },
    {
        "*",
        "ogg",
        "audio/x-vorbis",
        VLC_CODEC_NONE,
        VLC_CODEC_VORBIS,
    },
    {
        "*",
        "ogg",
        "audio/x-vorbis+ogg",
        VLC_CODEC_NONE,
        VLC_CODEC_VORBIS,
    },
    {
        "*",
        "ogg",
        "audio/ogg",
        VLC_CODEC_NONE,
        VLC_CODEC_OPUS,
    },
    {
        "*",
        "ogg",
        "audio/ogg",
        VLC_CODEC_NONE,
        VLC_CODEC_VORBIS,
    },
    {
        "MP3X",
        "ts",
        "audio/mpeg",
        VLC_CODEC_NONE,
        VLC_CODEC_MP3,
    },

    //------ Video Profiles ------//

    {
        "*",
        "ts",
        "video/mpeg",
        VLC_CODEC_MP2V,
        VLC_CODEC_MP3,
    },
    {
        "*",
        "ts",
        "video/mpeg",
        VLC_CODEC_MP2V,
        VLC_CODEC_MPGA,
    },
    {
        "*",
        "ts",
        "video/mpeg",
        VLC_CODEC_MP2V,
        VLC_CODEC_MP4A,
    },
    {
        "*",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_MP3,
    },
    {
        "*",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_MPGA,
    },
    {
        "*",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    {
        "*",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP3,
    },
    {
        "*",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MPGA,
    },
    {
        "*",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    {
        "*",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-mkv",
        VLC_CODEC_H264,
        VLC_CODEC_MP3,
    },
    {
        "*",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-mkv",
        VLC_CODEC_H264,
        VLC_CODEC_MPGA,
    },
    {
        "*",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-mkv",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * MPEG-2 HD/SD video wrapped in MPEG-2 transport stream as constrained by
     * SCTE-43 standards, with AC-3 audio, without a timestamp field
     */
    {
        "MPEG_TS_NA_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_MP2V,
        VLC_CODEC_A52,
    },
    /**
     * North America region profile for MPEG-2 HD
     * 3Dframe-compatible video with AC-3 audio,
     * utilizing a DLNA Transport Packet without a Timestamp field
     */
    {
        "MPEG_TS_NA_3DFC_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_MP2V,
        VLC_CODEC_A52,
    },
    /**
     * MPEG-2 Video, wrapped in MPEG-2 transport stream, Main Profile,
     * Standard Definition, with AC-3 audio, without a timestamp field.
     */
    {
        "MPEG_TS_SD_EU_AC3_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_MP2V,
        VLC_CODEC_A52,
    },
    /**
     * MPEG-2 Video, wrapped in MPEG-2 transport stream, Main Profile
     * Standard Definition, with AC-3 audio, with a valid non-zero timestamp
     * field.
     */
    {
        "MPEG_TS_SD_EU_AC3_T",
        "ts",
        "video/mpeg",
        VLC_CODEC_MP2V,
        VLC_CODEC_A52,
    },
    /**
     * MPEG-2 Main Profile at Main, High-1 440 and High Level with MPEG-2 AAC
     * encapsulated in MPEG-2 TS with valid timestamp
     */
    {
        "MPEG_TS_JP_T",
        "ts",
        "video/vnd.dlna.mpeg-tts",
        VLC_CODEC_MP2V,
        VLC_CODEC_MP4A,
    },
    /**
     * MPEG-2 Video, encapsulated in MPEG-2 transport stream, Main Profile
     * at Main Level, with MPEG-1 L2 audio, with a valid non-zero timestamp
     * field.
     */
    {
        "MPEG_TS_SD_JP_MPEG1_L2_T",
        "ts",
        "video/mpeg",
        VLC_CODEC_MP2V,
        VLC_CODEC_MP2,
    },
    /**
     * AVC video wrapped in MPEG-2 transport stream, as
     * constrained by SCTE standards,
     * with AC-3, Enhanced AC-3, MPEG-4 HE-AAC
     * v2 or MPEG-1 Layer II audio, with a valid
     * non-zero timestamp field.
     */
    {
        "AVC_TS_NA_T",
        "ts",
        "video/vnd.dlna.mpeg-tts",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * AVC video wrapped in MPEG-2 transport stream, as
     * constrained by SCTE standards,
     * with AC-3, Enhanced AC-3, MPEG-4 HE-AAC
     * v2 or MPEG-1 Layer II audio, with a valid
     * non-zero timestamp field.
     */
    {
        "AVC_TS_NA_T",
        "ts",
        "video/vnd.dlna.mpeg-tts",
        VLC_CODEC_H264,
        VLC_CODEC_EAC3,
    },
    /**
     * AVC video wrapped in MPEG-2 transport stream, as
     * constrained by SCTE standards,
     * with AC-3, Enhanced AC-3, MPEG-4 HE-AAC
     * v2 or MPEG-1 Layer II audio, with a valid
     * non-zero timestamp field.
     */
    {
        "AVC_TS_NA_T",
        "ts",
        "video/vnd.dlna.mpeg-tts",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC video wrapped in MPEG-2 transport stream, as
     * constrained by SCTE standards,
     * with AC-3, Enhanced AC-3, MPEG-4 HE-AAC
     * v2 or MPEG-1 Layer II audio, with a valid
     * non-zero timestamp field.
     */
    {
        "AVC_TS_NA_T",
        "ts",
        "video/vnd.dlna.mpeg-tts",
        VLC_CODEC_H264,
        VLC_CODEC_MP2,
    },
    /**
     * AVC video wrapped in MPEG-2 transport stream, as
     * constrained by SCTE standards,
     * with AC-3, Enhanced AC-3, MPEG-4 HE-AAC
     * v2 or MPEG-1 Layer II audio,
     * without a timestamp field.
     */
    {
        "AVC_TS_NA_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * AVC video wrapped in MPEG-2 transport stream, as
     * constrained by SCTE standards,
     * with AC-3, Enhanced AC-3, MPEG-4 HE-AAC
     * v2 or MPEG-1 Layer II audio,
     * without a timestamp field.
     */
    {
        "AVC_TS_NA_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_EAC3,
    },
    /**
     * AVC video wrapped in MPEG-2 transport stream, as
     * constrained by SCTE standards,
     * with AC-3, Enhanced AC-3, MPEG-4 HE-AAC
     * v2 or MPEG-1 Layer II audio,
     * without a timestamp field.
     */
    {
        "AVC_TS_NA_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC video wrapped in MPEG-2 transport stream, as
     * constrained by SCTE standards,
     * with AC-3, Enhanced AC-3, MPEG-4 HE-AAC
     * v2 or MPEG-1 Layer II audio,
     * without a timestamp field.
     */
    {
        "AVC_TS_NA_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_MP2,
    },
    /**
     * AVC high profile, HD 3D frame-compatible video
     * wrapped in MPEG-2 transport stream with AC-3,
     * Enhanced AC-3 or HE AAC audio, without a Timestamp field.
     */
    {
        "AVC_TS_NA_3DFC_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * AVC high profile, HD 3D frame-compatible video
     * wrapped in MPEG-2 transport stream with AC-3,
     * Enhanced AC-3 or HE AAC audio, without a Timestamp field.
     */
    {
        "AVC_TS_NA_3DFC_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_EAC3,
    },
    /**
     * AVC high profile, HD 3D frame-compatible video
     * wrapped in MPEG-2 transport stream with AC-3,
     * Enhanced AC-3 or HE AAC audio, without a Timestamp field.
     */
    {
        "AVC_TS_NA_3DFC_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC video wrapped in MPEG-2 TS transport stream
     * as constrained by DVB standards, with AC-3,
     * Enhanced AC-3 and MPEG-4 HE-AAC v2 audio.
     */
    {
        "AVC_TS_EU_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * AVC video wrapped in MPEG-2 TS transport stream
     * as constrained by DVB standards, with AC-3,
     * Enhanced AC-3 and MPEG-4 HE-AAC v2 audio.
     */
    {
        "AVC_TS_EU_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_EAC3,
    },
    /**
     * AVC video wrapped in MPEG-2 TS transport stream
     * as constrained by DVB standards, with AC-3,
     * Enhanced AC-3 and MPEG-4 HE-AAC v2 audio.
     */
    {
        "AVC_TS_EU_ISO",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_EAC3,
    },
    /**
     * European region profile for HD/SD resolution.
     * AVC video using the Scalable High Profile (SVC)
     * wrapped in MPEG-2 Transport Stream with AC-3 audio,
     * with a valid non-zero timestamp field.
     */
    {
        "AVC_TS_SHP_HD_EU_AC3_T",
        "ts",
        "video/vnd.dlna.mpeg-tts",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * European region profile for HD/SD resolution. AVC video using the
     * Scalable High Profile (SVC) wrapped in MPEG-2 Transport Stream
     * with MPEG-4 HE-AAC v2 Level 4 audio, with a valid non-zero timestamp field.
     */
    {
        "AVC_TS_SHP_HD_EU_HEAACv2_L4_T",
        "ts",
        "video/vnd.dlna.mpeg-tts",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC HD/SD video with AC-3 audio including dual-mono channel mode, wrapped
     * in MPEG-2 TS with valid timestamp for 24 Hz system.
     */
    {
        "AVC_TS_HD_24_AC3_X_T",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * AVC HD/SD video with AC-3 audio including dual-mono channel mode, wrapped
     * in MPEG-2 TS with valid timestamp for 50 Hz system.
     */
    {
        "AVC_TS_HD_50_AC3_X_T",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * AVC HD/SD video with AC-3 audio including dual-mono channel mode,
     * wrapped in MPEG-2 TS with valid timestamp for 60 Hz system.
     */
    {
        "AVC_TS_HD_60_AC3_X_T",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * AVC wrapped in MPEG-2 transport stream, Main/High profile, with
     * MPEG-2 AAC audio, with a valid non-zero timestamp field.
     */
    {
        "AVC_TS_JP_AAC_T",
        "ts",
        "video/mpeg",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC video with MPEG-4 HE-AAC v2
     * and Enhanced AC-3 audio, encapsulated in MP4.
     */
    {
        "AVC_MP4_EU",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC video with MPEG-4 HE-AAC v2
     * and Enhanced AC-3 audio, encapsulated in MP4.
     */
    {
        "AVC_MP4_EU",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_A52,
    },
    /**
     * AVC wrapped in MP4 baseline profile CIF15 with AAC LC audio.
     */
    {
        "AVC_MP4_BL_CIF15_AAC_520",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC wrapped in MP4 baseline profile CIF30 with AAC LC audio.
     */
    {
        "AVC_MP4_BL_CIF30_AAC_940",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC Main Profile video with Enhanced AC-3 audio, encapsulated in MP4.
     */
    {
        "AVC_MP4_MP_SD_EAC3",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_EAC3,
    },
    /**
     * AVC High Profile HD video with MPEG-4 AAC audio, encapsulated in MP4.
     */
    {
        "AVC_MP4_MP_SD",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC High Profile video with Enhanced AC-3 audio, encapsulated in MP4.
     */
    {
        "AVC_MP4_HP_HD_EAC3",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_EAC3,
    },
    /**
     * AVC high profile video with HE AAC v2 stereo or HE AAC 7.1-channel audio,
     * encapsulated in an MP4 file.
     */
    {
        "AVC_MP4_HD_HEAACv2_L6",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC Main Profile video with MPEG-4 AAC audio, encapsulated in MKV.
     */
    {
        "AVC_MKV_MP_HD_AAC_MULT5",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-matroska",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC Main Profile video with MPEG-4 HE-AAC audio, encapsulated in MKV.
     */
    {
        "AVC_MKV_MP_HD_HEAAC_L4",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-matroska",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC Main Profile video with MP3 audio, encapsulated in MKV
     */
    {
        "AVC_MKV_MP_HD_MPEG1_L3",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-matroska",
        VLC_CODEC_H264,
        VLC_CODEC_MPGA,
    },
    /**
     * AVC Main Profile video with MP3 audio, encapsulated in MKV
     */
    {
        "AVC_MKV_MP_HD_MPEG1_L3",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-matroska",
        VLC_CODEC_H264,
        VLC_CODEC_MP3,
    },
    /**
     * AVC High Profile video with MP3 audio, encapsulated in MKV
     */
    {
        "AVC_MKV_HP_HD_MPEG1_L3",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-matroska",
        VLC_CODEC_H264,
        VLC_CODEC_MPGA,
    },
    /**
     * AVC High Profile video with MP3 audio, encapsulated in MKV
     */
    {
        "AVC_MKV_HP_HD_MPEG1_L3",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-matroska",
        VLC_CODEC_H264,
        VLC_CODEC_MP3,
    },
    /**
     * AVC High Profile video with MPEG-4 AAC audio, encapsulated in MKV.
     */
    {
        "AVC_MKV_HP_HD_AAC_MULT5",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-matroska",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC High Profile video with MPEG-4 HE-AAC audio, encapsulated in MKV.
     */
    {
        "AVC_MKV_HP_HD_HEAAC_L4",
        "avformat{mux=matroska,options={live=1}}",
        "video/x-matroska",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC SD video wrapped in MPEG-2 transport stream, as constrained by
     * SCTE standards, with AAC audio and optional enhanced audio,
     * without a timestamp field and constrained to an SD video profile
     */
    {
        "DASH_AVC_TS_SD_ISO",
        "ts",
        "video/mp2t",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC HD video wrapped in MPEG-2 transport stream, as constrained by
     * SCTE standards, with AAC audio and optional enhanced audio,
     * without a timestamp field and constrained to an HD video profile
     */
    {
        "DASH_AVC_TS_HD_ISO",
        "ts",
        "video/mp2t",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC Main Profile SD video with AAC audio and optional enhanced audio,
     * encapsulated in MP4 conforming to the additional DECE CFF requirements
     * including encryption and constrained to the DECE SD profile requirements.
     */
    {
        "DASH_AVC_MP4_SD",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC Main Profile HD video with AAC audio and optional enhanced audio,
     * encapsulated in MP4 conforming to the additional DECE CFF requirements
     * including encryption and constrained to the DECE HD profile requirements.
     */
    {
        "DASH_AVC_MP4_HD",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC Main Profile video with HE AACv2 L4 audio, encapsulated in MP4.
     */
    {
        "DASH_AVC_MP4_SD_HEAACv2_L4",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC High Profile video with HE AACv2 L4 audio, encapsulated in MP4.
     */
    {
        "DASH_AVC_MP4_HD_HEAACv2_L4",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * AVC high profile video with HE AAC v2 stereo or HE AAC 7.1-channel audio,
     * encapsulated in an MP4 file suitable for MPEG DASH conformant adaptive delivery
     */
    {
        "DASH_AVC_MP4_HD_HEAACv2_L6",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_H264,
        VLC_CODEC_MP4A,
    },
    /**
     * HEVC High Profile HD and UHD video with AC-3,
     * Enhanced AC-3, HE-AACv2 or MPEG-1 LII audio,
     * encapsulated in MP4.
     */
    {
        "DASH_HEVC_MP4_UHD_NA",
        "mp4stream",
        "video/mp4",
        VLC_CODEC_HEVC,
        VLC_CODEC_MP2,
    },
};

#endif /* DLNA_PROFILES_H */
