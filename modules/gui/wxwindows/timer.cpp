/*****************************************************************************
 * timer.cpp : wxWindows plugin for vlc
 *****************************************************************************
 * Copyright (C) 2000-2003 VideoLAN
 * $Id: timer.cpp,v 1.36 2003/12/03 13:27:51 rocky Exp $
 *
 * Authors: Gildas Bazin <gbazin@netcourrier.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <stdlib.h>                                      /* malloc(), free() */
#include <errno.h>                                                 /* ENOMEM */
#include <string.h>                                            /* strerror() */
#include <stdio.h>

#include <vlc/vlc.h>
#include <vlc/aout.h>
#include <vlc/intf.h>

#include "wxwindows.h"
#include <wx/timer.h>

//void DisplayStreamDate( wxControl *, intf_thread_t *, int );

/* Callback prototype */
static int PopupMenuCB( vlc_object_t *p_this, const char *psz_variable,
                        vlc_value_t old_val, vlc_value_t new_val, void *param );

/*****************************************************************************
 * Constructor.
 *****************************************************************************/
Timer::Timer( intf_thread_t *_p_intf, Interface *_p_main_interface )
{
    p_intf = _p_intf;
    p_main_interface = _p_main_interface;
    i_old_playing_status = PAUSE_S;
    i_old_rate = DEFAULT_RATE;

    /* Register callback for the intf-popupmenu variable */
    playlist_t *p_playlist =
        (playlist_t *)vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                       FIND_ANYWHERE );
    if( p_playlist != NULL )
    {
        var_AddCallback( p_playlist, "intf-popupmenu", PopupMenuCB, p_intf );
        vlc_object_release( p_playlist );
    }

    Start( 100 /*milliseconds*/, wxTIMER_CONTINUOUS );
}

Timer::~Timer()
{
    /* Unregister callback */
    playlist_t *p_playlist =
        (playlist_t *)vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                       FIND_ANYWHERE );
    if( p_playlist != NULL )
    {
        var_DelCallback( p_playlist, "intf-popupmenu", PopupMenuCB, p_intf );
        vlc_object_release( p_playlist );
    }
}

/*****************************************************************************
 * Private methods.
 *****************************************************************************/

/*****************************************************************************
 * Manage: manage main thread messages
 *****************************************************************************
 * In this function, called approx. 10 times a second, we check what the
 * main program wanted to tell us.
 *****************************************************************************/
void Timer::Notify()
{
    vlc_bool_t b_pace_control;

    vlc_mutex_lock( &p_intf->change_lock );

    /* Update the input */
    if( p_intf->p_sys->p_input == NULL )
    {
        p_intf->p_sys->p_input =
            (input_thread_t *)vlc_object_find( p_intf, VLC_OBJECT_INPUT,
                                               FIND_ANYWHERE );

        /* Show slider */
        if( p_intf->p_sys->p_input )
        {
            //if( p_intf->p_sys->p_input->stream.b_seekable )
            {
                p_main_interface->slider_frame->Show();
                p_main_interface->frame_sizer->Show(
                    p_main_interface->slider_frame );
                p_main_interface->frame_sizer->Layout();
                p_main_interface->frame_sizer->Fit( p_main_interface );
            }

            p_main_interface->statusbar->SetStatusText(
                wxU(p_intf->p_sys->p_input->psz_source), 2 );

            p_main_interface->TogglePlayButton( PLAYING_S );
            i_old_playing_status = PLAYING_S;

            /* Take care of the volume */
            audio_volume_t i_volume;
            aout_VolumeGet( p_intf, &i_volume );
            p_main_interface->volctrl->SetValue( i_volume * 200 * 2 /
                                                 AOUT_VOLUME_MAX );
            p_main_interface->volctrl->SetToolTip(
                wxString::Format((wxString)wxU(_("Volume")) + wxT(" %d"),
                i_volume * 200 / AOUT_VOLUME_MAX ) );

            /* control buttons for free pace streams */
            b_pace_control = p_intf->p_sys->p_input->stream.b_pace_control;
        }
    }
    else if( p_intf->p_sys->p_input->b_dead )
    {
        /* Hide slider */
        //if( p_intf->p_sys->p_input->stream.b_seekable )
        {
            p_main_interface->slider_frame->Hide();
            p_main_interface->frame_sizer->Hide(
                p_main_interface->slider_frame );
            p_main_interface->frame_sizer->Layout();
            p_main_interface->frame_sizer->Fit( p_main_interface );

            p_main_interface->TogglePlayButton( PAUSE_S );
            i_old_playing_status = PAUSE_S;
        }

        p_main_interface->statusbar->SetStatusText( wxT(""), 2 );

        vlc_object_release( p_intf->p_sys->p_input );
        p_intf->p_sys->p_input = NULL;
    }



    if( p_intf->p_sys->p_input )
    {
        input_thread_t *p_input = p_intf->p_sys->p_input;

        vlc_mutex_lock( &p_input->stream.stream_lock );

        if( !p_input->b_die )
        {
            /* New input or stream map change */
            p_intf->p_sys->b_playing = 1;
#if 0
            if( p_input->stream.b_changed )
            {
                wxModeManage( p_intf );
                wxSetupMenus( p_intf );
                p_intf->p_sys->b_playing = 1;

                p_main_interface->TogglePlayButton( PLAYING_S );
                i_old_playing_status = PLAYING_S;
            }
#endif

            /* Manage the slider */
            if( p_input->stream.b_seekable && p_intf->p_sys->b_playing )
            {
                /* Update the slider if the user isn't dragging it. */
                if( p_intf->p_sys->b_slider_free )
                {
                    vlc_value_t pos;
                    char psz_time[ MSTRTIME_MAX_SIZE ];
                    vlc_value_t time;
                    mtime_t i_seconds;

                    /* Update the value */
                    var_Get( p_input, "position", &pos );
                    if( pos.f_float >= 0.0 )
                    {
                        p_intf->p_sys->i_slider_pos =
                            (int)(SLIDER_MAX_POS * pos.f_float);

                        p_main_interface->slider->SetValue(
                            p_intf->p_sys->i_slider_pos );

                        var_Get( p_intf->p_sys->p_input, "time", &time );
                        i_seconds = time.i_time / 1000000;

                        secstotimestr ( psz_time, i_seconds );

                        p_main_interface->slider_box->SetLabel( wxU(psz_time) );
                    }
                }
            }

            /* Manage Playing status */
            if( i_old_playing_status != p_input->stream.control.i_status )
            {
                if( p_input->stream.control.i_status == PAUSE_S )
                {
                    p_main_interface->TogglePlayButton( PAUSE_S );
                }
                else
                {
                    p_main_interface->TogglePlayButton( PLAYING_S );
                }
                i_old_playing_status = p_input->stream.control.i_status;
            }

            /* Manage Speed status */
            if( i_old_rate != p_input->stream.control.i_rate )
            {
                p_main_interface->statusbar->SetStatusText(
                    wxString::Format(wxT("x%.2f"),
                    1000.0 / p_input->stream.control.i_rate), 1 );
                i_old_rate = p_input->stream.control.i_rate;
            }
        }

        vlc_mutex_unlock( &p_input->stream.stream_lock );
    }
    else if( p_intf->p_sys->b_playing && !p_intf->b_die )
    {
        p_intf->p_sys->b_playing = 0;
        p_main_interface->TogglePlayButton( PAUSE_S );
        i_old_playing_status = PAUSE_S;
    }

    if( p_intf->b_die )
    {
        vlc_mutex_unlock( &p_intf->change_lock );

        /* Prepare to die, young Skywalker */
        p_main_interface->Close(TRUE);
        return;
    }

    vlc_mutex_unlock( &p_intf->change_lock );
}

/*****************************************************************************
 * PopupMenuCB: callback triggered by the intf-popupmenu playlist variable.
 *  We don't show the menu directly here because we don't want the
 *  caller to block for a too long time.
 *****************************************************************************/
static int PopupMenuCB( vlc_object_t *p_this, const char *psz_variable,
                        vlc_value_t old_val, vlc_value_t new_val, void *param )
{
    intf_thread_t *p_intf = (intf_thread_t *)param;

    if( p_intf->p_sys->pf_show_dialog )
    {
        p_intf->p_sys->pf_show_dialog( p_intf, INTF_DIALOG_POPUPMENU,
                                       new_val.b_bool, 0 );
    }

    return VLC_SUCCESS;
}
