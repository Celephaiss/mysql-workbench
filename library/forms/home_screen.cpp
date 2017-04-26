/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/threading.h"
#include "base/log.h"

#include "mforms/popup.h"
#include "mforms/menu.h"
#include "mforms/menubar.h"
#include "mforms/utilities.h"
#include "mforms/drawbox.h"
#include "mforms/textentry.h"
#include "mforms/imagebox.h"
#include "mforms/scrollpanel.h"

#include "mforms/home_screen.h"
#include "mforms/home_screen_connections.h"

#include "base/any.h"

using namespace mforms;

//----------------- ShortcutSection ----------------------------------------------------------------

SidebarEntry::SidebarEntry()
    : owner(nullptr), canSelect(false), icon(nullptr) {

}

// ------ Accesibility Methods -----
std::string SidebarEntry::get_acc_name() {
  return title;
}

Accessible::Role SidebarEntry::get_acc_role() {
  return Accessible::ListItem;
}

base::Rect SidebarEntry::get_acc_bounds() {
  return acc_bounds;
}

std::string SidebarEntry::get_acc_default_action() {
  return "Open Item";
}

void SidebarEntry::do_default_action() {
  if (owner != nullptr) {
    owner->mouse_move(MouseButtonLeft, (int)acc_bounds.center().x, (int)acc_bounds.center().y);
    owner->mouse_click(MouseButtonLeft, (int)acc_bounds.center().x, (int)acc_bounds.center().y);
  }
}
;

SidebarSection::SidebarSection(HomeScreen *owner) {
  _owner = owner;
  _hotEntry = nullptr;
  _activeEntry = nullptr;

  _accessible_click_handler = std::bind(&SidebarSection::mouse_click, this, mforms::MouseButtonLeft,
                                        std::placeholders::_1, std::placeholders::_2);
}

//--------------------------------------------------------------------------------------------------

SidebarSection::~SidebarSection() {
  for (auto &it : _entries) {
    deleteSurface(it.first->icon);
    delete it.first;
  }
  _entries.clear();
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::drawTriangle(cairo_t *cr, int x1, int y1, int x2, int y2, base::Color &color, float alpha) {
  cairo_set_source_rgba(cr, color.red, color.green, color.blue, alpha);
  cairo_move_to(cr, x2, y1 + abs(y2 - y1) / 3);
  cairo_line_to(cr, x1 + abs(x2 - x1) * 0.6, y1 + abs(y2 - y1) / 2);
  cairo_line_to(cr, x2, y2 - abs(y2 - y1) / 3);
  cairo_fill(cr);
}

//------------------------------------------------------------------------------------------------

void SidebarSection::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {

  int height = get_height();

  // Shortcuts block.
  int yoffset = SIDEBAR_TOP_PADDING;
  if (_entries.size() > 0 && yoffset < height) {
    for (auto &iterator : _entries) {
      float alpha = iterator.first == _activeEntry ? 1 : 0.5f;
      if ((yoffset + SIDEBAR_ROW_HEIGHT) > height)
        alpha = 0.25f;

      iterator.first->acc_bounds.pos.x = SIDEBAR_LEFT_PADDING;
      iterator.first->acc_bounds.pos.y = yoffset;
      iterator.first->acc_bounds.size.width = get_width() - (SIDEBAR_LEFT_PADDING + SIDEBAR_RIGHT_PADDING);
      iterator.first->acc_bounds.size.height = SIDEBAR_ROW_HEIGHT;

      mforms::Utilities::paint_icon(cr, iterator.first->icon, SIDEBAR_LEFT_PADDING, yoffset, alpha);

      if (iterator.first == _activeEntry)  //we need to draw an indicator
        drawTriangle(cr, get_width() - SIDEBAR_RIGHT_PADDING, yoffset, get_width(), yoffset + SIDEBAR_ROW_HEIGHT,
                     iterator.first->indicatorColor, alpha);

      yoffset += SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING;
      if (yoffset >= height)
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

int SidebarSection::shortcutFromPoint(int x, int y) {
  if (x < SIDEBAR_LEFT_PADDING || y < SIDEBAR_TOP_PADDING || x > get_width() - SIDEBAR_RIGHT_PADDING)
    return -1;

  y -= SIDEBAR_TOP_PADDING;
  int point_in_row = y % (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING);
  if (point_in_row >= SIDEBAR_ROW_HEIGHT)
    return -1;  // In the spacing between entries.

  size_t row = y / (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING);
  size_t height = get_height() - SIDEBAR_TOP_PADDING;
  size_t row_bottom = row * (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING) + SIDEBAR_ROW_HEIGHT;
  if (row_bottom > height)
    return -1;  // The last shortcut is dimmed if it goes over the bottom border.
                // Take it out from the hit test too.

  if (row < _entries.size())
    return (int)row;

  return -1;
}

//  //--------------------------------------------------------------------------------------------------
//
/**
 * Adds a new sidebar entry to the internal list. The function performs some sanity checks.
 */
void SidebarSection::addEntry(const std::string &title, const std::string &icon_name, HomeScreenSection *section,
                              std::function<void()> callback, bool canSelect) {
  SidebarEntry *entry = new SidebarEntry;

  entry->callback = callback;
  entry->canSelect = canSelect;
  entry->owner = this;
  entry->title = title;

  if (section)
    entry->indicatorColor = section->getIndicatorColor();
  else
    entry->indicatorColor = base::Color("#ffffff");  // Use white as default indicator color

  // See if we can load the icon. If not use the placeholder.
  entry->icon = mforms::Utilities::load_icon(icon_name, true);
  if (entry->icon == NULL)
    throw std::runtime_error("Icon not found: " + icon_name);

  _entries.push_back( { entry, section });
  if (_activeEntry == nullptr && entry->canSelect && section != nullptr) {
    _activeEntry = entry;
    // If this is first entry, we need to show it.
    section->get_parent()->show(true);
  }

  set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

HomeScreenSection *SidebarSection::getActive() {
  if (_activeEntry != nullptr) {
    for (auto &it : _entries) {
      if (it.first == _activeEntry)
        return it.second;
    }
  }

  return nullptr;
}

//--------------------------------------------------------------------------------------------------

void SidebarSection::setActive(HomeScreenSection *section) {
  SidebarEntry *entryForSection = nullptr;
  for (auto &it : _entries) {
    if (it.second == section) {
      if (it.first == _activeEntry)
        return;  // Section already active. Nothing to do.

      entryForSection = it.first;
    }
  }

  if (_activeEntry != nullptr) {
    for (auto &it : _entries) {
      if (it.first == _activeEntry)
        it.second->get_parent()->show(false);
    }
  }

  _activeEntry = entryForSection;
  section->get_parent()->show(true);
  set_needs_repaint();
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::mouse_click(mforms::MouseButton button, int x, int y) {
  switch (button) {
    case mforms::MouseButtonLeft: {
      if (_hotEntry != nullptr && _hotEntry->canSelect) {
        _activeEntry = _hotEntry;
        set_needs_repaint();
      }

      if (_hotEntry != nullptr && _hotEntry->callback)
        _hotEntry->callback();
    }
      break;

    default:
      break;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::mouse_leave() {
  if (_hotEntry != nullptr) {
    _hotEntry = nullptr;
    set_needs_repaint();
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool SidebarSection::mouse_move(mforms::MouseButton button, int x, int y) {
  SidebarEntry *shortcut = nullptr;
  int row = shortcutFromPoint(x, y);
  if (row > -1)
    shortcut = _entries[row].first;
  if (shortcut != _hotEntry) {
    _hotEntry = shortcut;
    set_needs_repaint();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------------------------

int SidebarSection::get_acc_child_count() {
  return (int)_entries.size();
}

//------------------------------------------------------------------------------------------------

Accessible *SidebarSection::get_acc_child(int index) {
  mforms::Accessible *accessible = NULL;

  if (index < (int)_entries.size())
    accessible = _entries[index].first;

  return accessible;
}

//------------------------------------------------------------------------------------------------

Accessible::Role SidebarSection::get_acc_role() {
  return Accessible::List;
}

mforms::Accessible *SidebarSection::hit_test(int x, int y) {
  mforms::Accessible *accessible = NULL;

  int row = shortcutFromPoint(x, y);
  if (row != -1)
    accessible = _entries[row].first;

  return accessible;
}

//----------------- HomeScreen ---------------------------------------------------------------------

HomeScreen::HomeScreen(bool singleSection)
    : AppView(true, "home", true), _singleSection(singleSection) {
  if (!_singleSection) {
    _sidebarSection = new SidebarSection(this);
    _sidebarSection->set_name("Home Shortcuts Section");
    _sidebarSection->set_size(85, -1);
    add(_sidebarSection, false, true);
  } else
    _sidebarSection = nullptr;

  update_colors();

  Box::scoped_connect(signal_resized(), std::bind(&HomeScreen::on_resize, this));
  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");
}

//--------------------------------------------------------------------------------------------------

HomeScreen::~HomeScreen() {
  base::NotificationCenter::get()->remove_observer(this);
  clear_subviews();  // Remove our sections or the View d-tor will try to release them.

  delete _sidebarSection;
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::update_colors() {
  set_back_color("#ffffff");
  if (_sidebarSection != nullptr)
#ifdef __APPLE__
    _sidebarSection->set_back_color("#323232");
#else
    _sidebarSection->set_back_color("#464646");
#endif

}

//--------------------------------------------------------------------------------------------------

void HomeScreen::addSection(HomeScreenSection *section) {
  if (section == nullptr)
    throw std::runtime_error("Empty HomeScreenSection given");

  if (_singleSection && !_sections.empty())
    throw std::runtime_error("HomeScreen is in singleSection mode. Only one section allowed.");

  _sections.push_back(section);

  if (_sidebarSection != nullptr) {
    mforms::ScrollPanel *scroll = mforms::manage(new mforms::ScrollPanel(mforms::ScrollPanelNoFlags));
    scroll->add(section->getContainer());
    add(scroll, true, true);
    scroll->show(false);

    bool isCallbackOnly = section->callback ? true : false;
    _sidebarSection->addEntry(section->getTitle(), section->getIcon(), section, [this, isCallbackOnly, section]() {
      if (isCallbackOnly)
       section->callback();
      else {
        for (auto &it : _sections) {
          if (it != section)
           it->getContainer()->get_parent()->show(false);
          else
           it->getContainer()->get_parent()->show(true);
        }
      }
    }, !isCallbackOnly);
  } else {
    add(section->getContainer(), true, true);
    section->getContainer()->show(true);
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::addSectionEntry(const std::string &title, const std::string &icon_name, std::function<void()> callback,
                                 bool canSelect) {
  if (_sidebarSection != nullptr)
    _sidebarSection->addEntry(title, icon_name, nullptr, callback, canSelect);
  else
    throw std::runtime_error("HomeScreen is in single section mode");
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::trigger_callback(HomeScreenAction action, const base::any &object) {
  onHomeScreenAction(action, object);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::cancelOperation() {
  for (auto &it : _sections)
    it->cancelOperation();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::set_menu(mforms::Menu *menu, HomeScreenMenuType type) {
  switch (type) {
    case HomeMenuConnection:
    case HomeMenuConnectionGroup:
    case HomeMenuConnectionGeneric: {
      for (auto &it : _sections)
        it->setContextMenu(menu, type);
      break;
    }
    case HomeMenuDocumentModelAction: {
      for (auto &it : _sections)
        it->setContextMenuAction(menu, type);
      break;
    }

    case HomeMenuDocumentModel:
      for (auto &it : _sections)
        it->setContextMenu(menu, type);
      break;

    case HomeMenuDocumentSQLAction:
      for (auto &it : _sections)
        it->setContextMenuAction(menu, type);
      break;

    case HomeMenuDocumentSQL:
      for (auto &it : _sections)
        it->setContextMenu(menu, type);
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::on_resize() {
  // Resize changes the layout so if there is pending script loading the popup is likely misplaced.
  cancelOperation();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::setup_done() {
  if (_sidebarSection != nullptr) {
    if (_sidebarSection->getActive()) {
      _sidebarSection->getActive()->setFocus();
    }
  } else {
    if (!_sections.empty())
      _sections.back()->setFocus();
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::showSection(size_t index) {
  if (index < _sections.size() && _sidebarSection != nullptr) {
    _sidebarSection->setActive(_sections[index]);
    _sidebarSection->getActive()->setFocus();
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNColorsChanged") {
    update_colors();
  }
}

//--------------------------------------------------------------------------------------------------
