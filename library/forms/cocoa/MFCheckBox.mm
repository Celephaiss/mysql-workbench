/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#import "MFCheckBox.h"

#import "MFView.h"
#import "MFMForms.h"

@implementation MFCheckBoxImpl


- (instancetype)initWithObject:(::mforms::CheckBox*)aCheckBox square:(BOOL)square
{
  self= [super initWithObject:aCheckBox buttonType: ::mforms::PushButton];
  if (self)
  {
    [self setButtonType: square ? NSPushOnPushOffButton : NSSwitchButton];
    if (square)
      self.bezelStyle = NSShadowlessSquareBezelStyle;
    else
      self.bezelStyle = NSRegularSquareBezelStyle;

    mTopLeftOffset= NSMakePoint(0, 0);
    mBottomRightOffset= NSMakePoint(0, 0);
    mAddPadding= NO;
    
    self.target = self;
    self.action = @selector(performCallback:);
  }
  return self;
}


- (void)performCallback:(id)sender
{
  mOwner->callback();
}

- (NSSize)minimumSize
{
  // We have to explicitly add space for the check box. No idea why this isn't done by cocoa implicitly.
  NSSize result = super.minimumSize;
  result.width += 4; // Seems only the spacing is missing.
  return result;
}

static bool checkbox_create(::mforms::CheckBox *self, bool square)
{
  return [[MFCheckBoxImpl alloc] initWithObject: self square: square] != nil;
}

static void checkbox_set_active(::mforms::CheckBox *self, bool flag)
{
  if ( self )
  {
    MFCheckBoxImpl* checkbox = self->get_data();
    
    if ( checkbox )
    {
      checkbox.state = flag ? NSOnState : NSOffState;
    }
  }
}

static bool checkbox_get_active(::mforms::CheckBox *self)
{
  if ( self )
  {
    MFCheckBoxImpl* checkbox = self->get_data();
    
    if ( checkbox )
    {
      return checkbox.state == NSOnState;
    }
  }
  return false;
}


void cf_checkbox_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_checkbox_impl.create= &checkbox_create;
  f->_checkbox_impl.set_active= &checkbox_set_active;
  f->_checkbox_impl.get_active= &checkbox_get_active;
}


@end

