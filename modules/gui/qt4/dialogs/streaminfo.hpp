/*****************************************************************************
 * streaminfo.hpp : Information about a stream
 ****************************************************************************
 * Copyright (C) 2006 the VideoLAN team
 * $Id$
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA. *****************************************************************************/

#ifndef _STREAMINFO_DIALOG_H_
#define _STREAMINFO_DIALOG_H_

#include "util/qvlcframe.hpp"
#include <QTabWidget>
#include <QBoxLayout>


class InputStatsPanel;
class MetaPanel;
class InfoPanel;

class InfoTab: public QTabWidget
{
    Q_OBJECT;
public:
    InfoTab( QWidget *, intf_thread_t * );
    virtual ~InfoTab();
private:
    intf_thread_t *p_intf;
    input_thread_t *p_input;
    InputStatsPanel *ISP;
    MetaPanel *MP;
    InfoPanel *IP;

public slots:
    void update();
};

class StreamInfoDialog : public QVLCFrame
{
    Q_OBJECT;
public:
    static StreamInfoDialog * getInstance( intf_thread_t *p_intf, bool a )
    {
        if( !instance)
            instance = new StreamInfoDialog( p_intf, a );
        return instance;
    }
    virtual ~StreamInfoDialog();
private:
    StreamInfoDialog( intf_thread_t *,  bool );
    input_thread_t *p_input;
    InfoTab *IT;
    bool main_input;
    static StreamInfoDialog *instance;
public slots:
    void update();
    void close();
};

#endif
