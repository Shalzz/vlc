/*****************************************************************************
 * playlist.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2002-2004 VideoLAN
 * $Id: playlist.m,v 1.58 2004/02/26 14:40:29 hartman Exp $
 *
 * Authors: Jon Lech Johansen <jon-vl@nanocrew.net>
 *          Derk-Jan Hartman <hartman at videolan dot org>
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
#include <sys/param.h>                                    /* for MAXPATHLEN */
#include <string.h>
#include <math.h>
#include <sys/mount.h>
#include <vlc_keys.h>

#include "intf.h"
#include "playlist.h"
#include "controls.h"

/*****************************************************************************
 * VLCPlaylistView implementation 
 *****************************************************************************/
@implementation VLCPlaylistView

- (NSMenu *)menuForEvent:(NSEvent *)o_event
{
    return( [[self delegate] menuForEvent: o_event] );
}

- (void)keyDown:(NSEvent *)o_event
{
    unichar key = 0;
    int i, c, i_row;
    NSMutableArray *o_to_delete;
    NSNumber *o_number;
    
    playlist_t * p_playlist;
    intf_thread_t * p_intf = [NSApp getIntf];

    if( [[o_event characters] length] )
    {
        key = [[o_event characters] characterAtIndex: 0];
    }

    p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                          FIND_ANYWHERE );
    
    if ( p_playlist == NULL )
    {
        return;
    }
    
    switch( key )
    {
        case NSDeleteCharacter:
        case NSDeleteFunctionKey:
        case NSDeleteCharFunctionKey:
        case NSBackspaceCharacter:
            o_to_delete = [NSMutableArray arrayWithArray:[[self selectedRowEnumerator] allObjects]];
            c = [o_to_delete count];
            
            for( i = 0; i < c; i++ ) {
                o_number = [o_to_delete lastObject];
                i_row = [o_number intValue];
                
                if( p_playlist->i_index == i_row && p_playlist->i_status )
                {
                    playlist_Stop( p_playlist );
                }
                [o_to_delete removeObject: o_number];
                [self deselectRow: i_row];
                playlist_Delete( p_playlist, i_row );
            }
            [self reloadData];
            break;
            
        default:
            [super keyDown: o_event];
            break;
    }

    if( p_playlist != NULL )
    {
        vlc_object_release( p_playlist );
    }
}


@end

/*****************************************************************************
 * VLCPlaylist implementation 
 *****************************************************************************/
@implementation VLCPlaylist

- (id)init
{
    self = [super init];
    if ( self !=nil )
    {
        i_moveRow = -1;
    }
    return self;
}

- (void)awakeFromNib
{
    [o_table_view setTarget: self];
    [o_table_view setDelegate: self];
    [o_table_view setDataSource: self];

    [o_table_view setDoubleAction: @selector(playItem:)];

    [o_table_view registerForDraggedTypes: 
        [NSArray arrayWithObjects: NSFilenamesPboardType, nil]];

    [o_window setExcludedFromWindowsMenu: TRUE];

/* We need to check whether _defaultTableHeaderSortImage exists, since it 
belongs to an Apple hidden private API, and then can "disapear" at any time*/

    if( [[NSTableView class] respondsToSelector:@selector(_defaultTableHeaderSortImage)] )
    {
        o_ascendingSortingImage = [[NSTableView class] _defaultTableHeaderSortImage];
    }
    else
    {
        o_ascendingSortingImage = nil;
    }

    if( [[NSTableView class] respondsToSelector:@selector(_defaultTableHeaderReverseSortImage)] )
    {
        o_descendingSortingImage = [[NSTableView class] _defaultTableHeaderReverseSortImage];
    }
    else
    {
        o_descendingSortingImage = nil;
    }

    [self initStrings];
}

- (void)initStrings
{
    [o_window setTitle: _NS("Playlist")];
    [o_mi_save_playlist setTitle: _NS("Save Playlist...")];
    [o_mi_play setTitle: _NS("Play")];
    [o_mi_delete setTitle: _NS("Delete")];
    [o_mi_selectall setTitle: _NS("Select All")];
    [[o_tc_name headerCell] setStringValue:_NS("Name")];
    [[o_tc_author headerCell] setStringValue:_NS("Author")];
    [[o_tc_duration headerCell] setStringValue:_NS("Duration")];
    [o_random_ckb setTitle: _NS("Random")];
    [o_loop_ckb setTitle: _NS("Repeat All")];
    [o_repeat_ckb setTitle: _NS("Repeat One")];
    [o_search_button setTitle: _NS("Search")];
    [o_btn_playlist setToolTip: _NS("Playlist")];
}

- (void) tableView:(NSTableView*)o_tv
                  didClickTableColumn:(NSTableColumn *)o_tc
{
    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t *p_playlist =
        (playlist_t *)vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                       FIND_ANYWHERE );

    int max = [[o_table_view tableColumns] count];
    int i;

    if( p_playlist == NULL )
    {
        return;
    }

    if( o_tc_sortColumn == o_tc )
    { 
        b_isSortDescending = !b_isSortDescending;
    }
    else if( o_tc == o_tc_name || o_tc == o_tc_author || 
        o_tc == o_tc_id )
    {
        b_isSortDescending = VLC_FALSE;
        [o_table_view setHighlightedTableColumn:o_tc];
        o_tc_sortColumn = o_tc;
        for( i=0 ; i<max ; i++ )
        {
            [o_table_view setIndicatorImage:nil inTableColumn:[[o_table_view tableColumns] objectAtIndex:i]];
        }
    }

    if( o_tc_id == o_tc && !b_isSortDescending )
    {    
        playlist_SortID( p_playlist , ORDER_NORMAL );
        [o_table_view setIndicatorImage:o_ascendingSortingImage inTableColumn:o_tc];    
    }
    else if( o_tc_name == o_tc && !b_isSortDescending )
    {    
        playlist_SortTitle( p_playlist , ORDER_NORMAL );
        [o_table_view setIndicatorImage:o_ascendingSortingImage inTableColumn:o_tc];    
    }
    else if( o_tc_author == o_tc && !b_isSortDescending )
    {
        playlist_SortAuthor( p_playlist , ORDER_NORMAL );
        [o_table_view setIndicatorImage:o_ascendingSortingImage inTableColumn:o_tc];
    }
    else if( o_tc_id == o_tc && b_isSortDescending )
    {    
        playlist_SortID( p_playlist , ORDER_REVERSE );
        [o_table_view setIndicatorImage:o_ascendingSortingImage inTableColumn:o_tc];    
    }
    else if( o_tc_name == o_tc && b_isSortDescending )
    {    
        playlist_SortTitle( p_playlist , ORDER_REVERSE );
        [o_table_view setIndicatorImage:o_descendingSortingImage inTableColumn:o_tc];
    }
    else if( o_tc_author == o_tc && b_isSortDescending )
    {
        playlist_SortAuthor( p_playlist , ORDER_REVERSE );
        [o_table_view setIndicatorImage:o_descendingSortingImage inTableColumn:o_tc];
    } 
    vlc_object_release( p_playlist );
    [self playlistUpdated];
}


- (BOOL)tableView:(NSTableView *)o_tv 
                  shouldEditTableColumn:(NSTableColumn *)o_tc
                  row:(int)i_row
{
    return( NO );
}

- (NSMenu *)menuForEvent:(NSEvent *)o_event
{
    NSPoint pt;
    vlc_bool_t b_rows;
    vlc_bool_t b_item_sel;

    pt = [o_table_view convertPoint: [o_event locationInWindow] 
                                                 fromView: nil];
    b_item_sel = ( [o_table_view rowAtPoint: pt] != -1 &&
                   [o_table_view selectedRow] != -1 );
    b_rows = [o_table_view numberOfRows] != 0;

    [o_mi_play setEnabled: b_item_sel];
    [o_mi_delete setEnabled: b_item_sel];
    [o_mi_selectall setEnabled: b_rows];

    return( o_ctx_menu );
}

- (IBAction)toggleWindow:(id)sender
{
    if( [o_window isVisible] )
    {
        [o_window orderOut:sender];
        [o_btn_playlist setState:NSOffState];
    }
    else
    {
        [o_window makeKeyAndOrderFront:sender];
        [o_btn_playlist setState:NSOnState];
    }
}

- (IBAction)savePlaylist:(id)sender
{
    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                                       FIND_ANYWHERE );
    
    NSSavePanel *o_save_panel = [NSSavePanel savePanel];
    NSString * o_name = [NSString stringWithFormat: @"%@.m3u", _NS("Untitled")];
    [o_save_panel setTitle: _NS("Save Playlist")];
    [o_save_panel setPrompt: _NS("Save")];

    if( [o_save_panel runModalForDirectory: nil
            file: o_name] == NSOKButton )
    {
        playlist_Export( p_playlist, [[o_save_panel filename] fileSystemRepresentation], "export-m3u" );
    }

}

- (IBAction)playItem:(id)sender
{
    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                                       FIND_ANYWHERE );

    if( p_playlist != NULL )
    {
        playlist_Goto( p_playlist, [o_table_view selectedRow] );
        vlc_object_release( p_playlist );
    }
}

- (IBAction)deleteItems:(id)sender
{
    int i, c, i_row;
    NSMutableArray *o_to_delete;
    NSNumber *o_number;

    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                                       FIND_ANYWHERE );

    if( p_playlist == NULL )
    {
        return;
    }
    
    o_to_delete = [NSMutableArray arrayWithArray:[[o_table_view selectedRowEnumerator] allObjects]];
    c = (int)[o_to_delete count];
    
    for( i = 0; i < c; i++ ) {
        o_number = [o_to_delete lastObject];
        i_row = [o_number intValue];
        
        if( p_playlist->i_index == i_row && p_playlist->i_status )
        {
            playlist_Stop( p_playlist );
        }
        [o_to_delete removeObject: o_number];
        [o_table_view deselectRow: i_row];
        playlist_Delete( p_playlist, i_row );
    }

    vlc_object_release( p_playlist );

    /* this is actually duplicity, because the intf.m manage also updates the view
     * when the playlist changes. we do this on purpose, because else there is a 
     * delay of .5 sec or so when we delete an item */
    [self playlistUpdated];
    [self updateRowSelection];
}

- (IBAction)selectAll:(id)sender
{
    [o_table_view selectAll: nil];
}


- (IBAction)searchItem:(id)sender
{
    int i_current = -1;
    NSString *o_current_name;
    NSString *o_current_author;

    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                               FIND_ANYWHERE );
    
    if( p_playlist == NULL )
    {
        return;
    }
    if( [o_table_view numberOfRows] < 1 )
    {
        return;
    }

    if( [o_table_view selectedRow] == [o_table_view numberOfRows]-1 )
    {
        i_current = -1;
    }
    else
    {
        i_current = [o_table_view selectedRow]; 
    }

    do
    {
        i_current++;

        vlc_mutex_lock( &p_playlist->object_lock );
        o_current_name = [NSString stringWithUTF8String: 
            p_playlist->pp_items[i_current]->psz_name];
        o_current_author = [NSString stringWithUTF8String: 
            playlist_GetInfo(p_playlist, i_current ,_("General"),_("Author") )];
        vlc_mutex_unlock( &p_playlist->object_lock );


        if( [o_current_name rangeOfString:[o_search_keyword stringValue] options:NSCaseInsensitiveSearch ].length ||
             [o_current_author rangeOfString:[o_search_keyword stringValue] options:NSCaseInsensitiveSearch ].length )
        {
             [o_table_view selectRow: i_current byExtendingSelection: NO];
             [o_table_view scrollRowToVisible: i_current];
             break;
        }
        if( i_current == [o_table_view numberOfRows] - 1 )
        {
             i_current = -1;
        }
    }
    while (i_current != [o_table_view selectedRow]);
    vlc_object_release( p_playlist );
}


- (void)appendArray:(NSArray*)o_array atPos:(int)i_position enqueue:(BOOL)b_enqueue
{
    int i_item;
    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                                       FIND_ANYWHERE );

    if( p_playlist == NULL )
    {
        return;
    }

    for ( i_item = 0; i_item < (int)[o_array count]; i_item++ )
    {
        /* One item */
        NSDictionary *o_one_item;
        int j, i_total_options = 0, i_new_id = -1;
        int i_mode = PLAYLIST_INSERT;
        BOOL b_rem = FALSE, b_dir = FALSE;
        NSString *o_uri, *o_name;
        NSArray *o_options;
        NSURL *o_true_file;
        char **ppsz_options = NULL;
    
        /* Get the item */
        o_one_item = [o_array objectAtIndex: i_item];
        o_uri = (NSString *)[o_one_item objectForKey: @"ITEM_URL"];
        o_name = (NSString *)[o_one_item objectForKey: @"ITEM_NAME"];
        o_options = (NSArray *)[o_one_item objectForKey: @"ITEM_OPTIONS"];
        
        /* If no name, then make a guess */
        if( !o_name) o_name = [[NSFileManager defaultManager] displayNameAtPath: o_uri];
    
        if( [[NSFileManager defaultManager] fileExistsAtPath:o_uri isDirectory:&b_dir] && b_dir &&
            [[NSWorkspace sharedWorkspace] getFileSystemInfoForPath: o_uri isRemovable: &b_rem
                    isWritable:NULL isUnmountable:NULL description:NULL type:NULL] && b_rem   )
        {
            /* All of this is to make sure CD's play when you D&D them on VLC */
            /* Converts mountpoint to a /dev file */
            struct statfs *buf;
            char *psz_dev;
            buf = (struct statfs *) malloc (sizeof(struct statfs));
            statfs( [o_uri fileSystemRepresentation], buf );
            psz_dev = strdup(buf->f_mntfromname);
            o_uri = [NSString stringWithCString: psz_dev ];
        }

        if( o_options && [o_options count] > 0 )
        {
            /* Count the input options */
            i_total_options = [o_options count];
    
            /* Allocate ppsz_options */
            for( j = 0; j < i_total_options; j++ )
            {
                if( !ppsz_options )
                    ppsz_options = (char **)malloc( sizeof(char *) * i_total_options );
    
                ppsz_options[j] = strdup([[o_options objectAtIndex:j] UTF8String]);
            }
        }

        /* Add the item */
        i_new_id = playlist_AddExt( p_playlist, [o_uri fileSystemRepresentation], 
                      [o_name UTF8String], i_mode, 
                      i_position == -1 ? PLAYLIST_END : i_position + i_item,
                      0, (ppsz_options != NULL ) ? (const char **)ppsz_options : 0, i_total_options );

        /* clean up 
        for( j = 0; j < i_total_options; j++ )
            free( ppsz_options[j] );
        if( ppsz_options ) free( ppsz_options ); */

        /* Recent documents menu */
        o_true_file = [NSURL fileURLWithPath: o_uri];
        if( o_true_file != nil )
        { 
            [[NSDocumentController sharedDocumentController]
                noteNewRecentDocumentURL: o_true_file]; 
        }
        
        if( i_item == 0 && !b_enqueue )
        {
            playlist_Goto( p_playlist, playlist_GetPositionById( p_playlist, i_new_id ) );
            playlist_Play( p_playlist );
        }
    }

    vlc_object_release( p_playlist );
}

- (void)playlistUpdated
{
    vlc_value_t val;
    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                                       FIND_ANYWHERE );
    if( p_playlist != NULL )
    {
        var_Get( p_playlist, "random", &val );
        [o_random_ckb setState: val.b_bool];

        var_Get( p_playlist, "loop", &val );
        [o_loop_ckb setState: val.b_bool];

        var_Get( p_playlist, "repeat", &val );
        [o_repeat_ckb setState: val.b_bool];

        vlc_object_release( p_playlist );
    }
    [o_table_view reloadData];
}

- (void)updateRowSelection
{
    int i_row;

    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                                       FIND_ANYWHERE );

    if( p_playlist == NULL )
    {
        return;
    }

    i_row = p_playlist->i_index;
    vlc_object_release( p_playlist );

    [o_table_view selectRow: i_row byExtendingSelection: NO];
    [o_table_view scrollRowToVisible: i_row];
}


@end

@implementation VLCPlaylist (NSTableDataSource)

- (int)numberOfRowsInTableView:(NSTableView *)o_tv
{
    int i_count = 0;
    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                                       FIND_ANYWHERE );

    if( p_playlist != NULL )
    {
        vlc_mutex_lock( &p_playlist->object_lock );
        i_count = p_playlist->i_size;
        vlc_mutex_unlock( &p_playlist->object_lock );
        vlc_object_release( p_playlist );
    }
    [o_status_field setStringValue: [NSString stringWithFormat:_NS("%i items in playlist"), i_count]];
    return( i_count );
}

- (id)tableView:(NSTableView *)o_tv 
                objectValueForTableColumn:(NSTableColumn *)o_tc 
                row:(int)i_row
{
    id o_value = nil;
    intf_thread_t * p_intf = [NSApp getIntf];
    playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                               FIND_ANYWHERE );

    if( p_playlist == NULL )
    {
        return( nil );
    }

    if( [[o_tc identifier] isEqualToString:@"0"] )
    {
        o_value = [NSString stringWithFormat:@"%i", i_row + 1];
    }
    else if( [[o_tc identifier] isEqualToString:@"1"] )
    {
        vlc_mutex_lock( &p_playlist->object_lock );
        o_value = [NSString stringWithUTF8String: 
            p_playlist->pp_items[i_row]->psz_name];
        vlc_mutex_unlock( &p_playlist->object_lock );
    }
    else if( [[o_tc identifier] isEqualToString:@"2"] )
    {
        vlc_mutex_lock( &p_playlist->object_lock );
        o_value = [NSString stringWithUTF8String: 
            playlist_GetInfo(p_playlist, i_row ,_("General"),_("Author") )];
        vlc_mutex_unlock( &p_playlist->object_lock );
    }
    else if( [[o_tc identifier] isEqualToString:@"3"] )
    {
        char psz_duration[MSTRTIME_MAX_SIZE];
        mtime_t dur = p_playlist->pp_items[i_row]->i_duration;
        if( dur != -1 )
        {
            secstotimestr( psz_duration, dur/1000000 );
            o_value = [NSString stringWithUTF8String: psz_duration];
        }
        else
        {
            o_value = @"-:--:--";
        }
    }

    vlc_object_release( p_playlist );

    return( o_value );
}

- (BOOL)tableView:(NSTableView *)o_tv
                    writeRows:(NSArray*)o_rows
                    toPasteboard:(NSPasteboard*)o_pasteboard 
{
    int i_rows = [o_rows count];
    NSArray *o_filenames = [NSArray array];
    
    [o_pasteboard declareTypes:[NSArray arrayWithObject:NSFilenamesPboardType] owner:self];
    [o_pasteboard setPropertyList:o_filenames forType:NSFilenamesPboardType];
    if ( i_rows == 1 )
    {
        i_moveRow = [[o_rows objectAtIndex:0]intValue];
        return YES;
    }
    return NO;
}

- (NSDragOperation)tableView:(NSTableView*)o_tv
                    validateDrop:(id <NSDraggingInfo>)o_info
                    proposedRow:(int)i_row
                    proposedDropOperation:(NSTableViewDropOperation)o_operation 
{
    if ( o_operation == NSTableViewDropAbove )
    {
        if ( i_moveRow >= 0 )
        {
            if ( i_row != i_moveRow )
            {
                return NSDragOperationMove;
            }
            /* what if in the previous run, the row wasn't actually moved? 
               then we can't drop new files on this location */
            return NSDragOperationNone;
        }
        return NSDragOperationGeneric;
    }
    return NSDragOperationNone;
}

- (BOOL)tableView:(NSTableView*)o_tv
                    acceptDrop:(id <NSDraggingInfo>)o_info
                    row:(int)i_proposed_row
                    dropOperation:(NSTableViewDropOperation)o_operation 
{
    if (  i_moveRow >= 0 )
    {
        if (i_moveRow != -1 && i_proposed_row != -1)
        {
            intf_thread_t * p_intf = [NSApp getIntf];
            playlist_t * p_playlist = vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                                            FIND_ANYWHERE );
        
            if( p_playlist == NULL )
            {
                i_moveRow = -1;
                return NO;
            }
    
            playlist_Move( p_playlist, i_moveRow, i_proposed_row ); 
        
            vlc_object_release( p_playlist );
        }
        [self playlistUpdated];
        i_moveRow = -1;
        return YES;
    }
    else
    {
        NSPasteboard * o_pasteboard;
        o_pasteboard = [o_info draggingPasteboard];
        
        if( [[o_pasteboard types] containsObject: NSFilenamesPboardType] )
        {
            int i;
            NSArray *o_array = [NSArray array];
            NSArray *o_values = [[o_pasteboard propertyListForType: NSFilenamesPboardType]
                        sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];

            for( i = 0; i < (int)[o_values count]; i++)
            {
                NSDictionary *o_dic;
                o_dic = [NSDictionary dictionaryWithObject:[o_values objectAtIndex:i] forKey:@"ITEM_URL"];
                o_array = [o_array arrayByAddingObject: o_dic];
            }
            [self appendArray: o_array atPos: i_proposed_row enqueue:YES];
            return YES;
        }
        return NO;
    }
    [self updateRowSelection];
}

/* Delegate method of NSWindow */
- (void)windowWillClose:(NSNotification *)aNotification
{
    [o_btn_playlist setState: NSOffState];
}

@end

