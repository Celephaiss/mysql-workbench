/* 
 * Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#import <Cocoa/Cocoa.h>

#import "WBPluginEditorBase.h"
#include "mysql_routinegroup_editor.h"

@class MVerticalLayoutView;

@interface DbMysqlRoutineGroupEditor : WBPluginEditorBase {
  IBOutlet NSTabView *tabView;
  
  IBOutlet NSTextField *nameText;
  IBOutlet NSTableView *routineTable;
  IBOutlet NSTextView *commentText;
  
  IBOutlet MVerticalLayoutView *editorHost;
  
  NSMutableArray *mRoutineArray;
 
  MySQLRoutineGroupEditorBE *mBackEnd;  
}

- (IBAction) removeItem: (id) sender;

@end
