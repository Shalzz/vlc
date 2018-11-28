#ifndef RENDERER_COMMON_H
#define RENDERER_COMMON_H

#include <string>

#include <vlc_common.h>
#include <vlc_sout.h>

#define PERF_TEXT N_( "Performance warning" )
#define PERF_LONGTEXT N_( "Display a performance warning when transcoding" )
#define CONVERSION_QUALITY_TEXT N_( "Conversion quality" )
#define CONVERSION_QUALITY_LONGTEXT N_( "Change this option to increase conversion speed or quality." )

#if defined (__ANDROID__) || defined (__arm__) || (defined (TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
# define CONVERSION_QUALITY_DEFAULT CONVERSION_QUALITY_LOW
#else
# define CONVERSION_QUALITY_DEFAULT CONVERSION_QUALITY_MEDIUM
#endif

#define RENDERER_CFG_PREFIX "sout-renderer-"

static const char *const conversion_quality_list_text[] = {
    N_( "High (high quality and high bandwidth)" ),
    N_( "Medium (medium quality and medium bandwidth)" ),
    N_( "Low (low quality and low bandwidth)" ),
    N_( "Low CPU (low quality but high bandwidth)" ),
};

enum {
    CONVERSION_QUALITY_HIGH = 0,
    CONVERSION_QUALITY_MEDIUM = 1,
    CONVERSION_QUALITY_LOW = 2,
    CONVERSION_QUALITY_LOWCPU = 3,
};

static const int conversion_quality_list[] = {
    CONVERSION_QUALITY_HIGH,
    CONVERSION_QUALITY_MEDIUM,
    CONVERSION_QUALITY_LOW,
    CONVERSION_QUALITY_LOWCPU,
};

struct venc_options
{
    vlc_fourcc_t fcc;
    std::string (*get_opt)( sout_stream_t *, const video_format_t *, int);
};

std::string
GetVencOption( sout_stream_t *p_stream, int venc_opt_idx,
        venc_options venc_opt_list[], size_t opt_list_length,
        vlc_fourcc_t *p_codec_video, const video_format_t *p_vid,
        int i_quality );

std::string GetVencVPXOption( sout_stream_t * /* p_stream */,
                                      const video_format_t * /* p_vid */,
                                      int /* i_quality */ );

std::string GetVencQSVH264Option( sout_stream_t * /* p_stream */,
                                         const video_format_t * /* p_vid */,
                                         int i_quality );

std::string GetVencX264Option( sout_stream_t * /* p_stream */,
                                      const video_format_t *p_vid,
                                      int i_quality );
#ifdef __APPLE__
std::string GetVencAvcodecVTOption( sout_stream_t * /* p_stream */,
                                           const video_format_t * p_vid,
                                           int i_quality );
#endif

#endif /* RENDERER_COMMON_H */
