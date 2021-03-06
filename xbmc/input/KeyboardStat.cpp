/*
 *      Copyright (C) 2007-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// C++ Implementation: CKeyboard

// Comment OUT, if not really debugging!!!:
//#define DEBUG_KEYBOARD_GETCHAR

#include "KeyboardStat.h"
#include "KeyboardLayoutConfiguration.h"
#include "windowing/XBMC_events.h"
#include "utils/TimeUtils.h"
#include "input/XBMC_keytable.h"

#if defined(_LINUX) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#endif

CKeyboardStat g_Keyboard;

CKeyboardStat::CKeyboardStat()
{
  memset(&m_lastKeysym, 0, sizeof(m_lastKeysym));
  m_lastKeyTime = 0;
}

CKeyboardStat::~CKeyboardStat()
{
}

void CKeyboardStat::Initialize()
{
}

const CKey CKeyboardStat::ProcessKeyDown(XBMC_keysym& keysym)
{ uint8_t vkey;
  wchar_t unicode;
  char ascii;
  uint32_t modifiers;
  unsigned int held;
  XBMCKEYTABLE keytable;

  modifiers = 0;
  if (keysym.mod & XBMCKMOD_CTRL)
    modifiers |= CKey::MODIFIER_CTRL;
  if (keysym.mod & XBMCKMOD_SHIFT)
    modifiers |= CKey::MODIFIER_SHIFT;
  if (keysym.mod & XBMCKMOD_ALT)
    modifiers |= CKey::MODIFIER_ALT;
  if (keysym.mod & XBMCKMOD_SUPER)
    modifiers |= CKey::MODIFIER_SUPER;

  CLog::Log(LOGDEBUG, "SDLKeyboard: scancode: %02x, sym: %04x, unicode: %04x, modifier: %x", keysym.scancode, keysym.sym, keysym.unicode, keysym.mod);

  // The keysym.unicode is usually valid, even if it is zero. A zero
  // unicode just means this is a non-printing keypress. The ascii and
  // vkey will be set below.
  unicode = keysym.unicode;
  ascii = 0;
  vkey = 0;
  held = 0;

  // Start by trying to match both the sym and unicode. This will identify
  // the majority of keypresses
  if (KeyTableLookupSymAndUnicode(keysym.sym, keysym.unicode, &keytable))
  {
    vkey = keytable.vkey;
    ascii = keytable.ascii;
  }

  // If we failed to match the sym and unicode try just the unicode. This
  // will match keys like \ that are on different keys on regional keyboards.
  else if (KeyTableLookupUnicode(keysym.unicode, &keytable))
  {
    vkey = keytable.vkey;
    ascii = keytable.ascii;
  }

  // If there is still no match try the sym
  else if (KeyTableLookupSym(keysym.sym, &keytable))
  {
    vkey = keytable.vkey;

    // Occasionally we get non-printing keys that have a non-zero value in
    // the keysym.unicode. Check for this here and replace any rogue unicode
    // values.
    if (keytable.unicode == 0 && unicode != 0)
      unicode = 0;
    else if (keysym.unicode > 32 && keysym.unicode < 128)
      ascii = unicode & 0x7f;
  }

  // The keysym.sym is unknown ...
  else
  {
    if (!vkey && !ascii)
    {
      if (keysym.mod & XBMCKMOD_LSHIFT) vkey = 0xa0;
      else if (keysym.mod & XBMCKMOD_RSHIFT) vkey = 0xa1;
      else if (keysym.mod & XBMCKMOD_LALT) vkey = 0xa4;
      else if (keysym.mod & XBMCKMOD_RALT) vkey = 0xa5;
      else if (keysym.mod & XBMCKMOD_LCTRL) vkey = 0xa2;
      else if (keysym.mod & XBMCKMOD_RCTRL) vkey = 0xa3;
      else if (keysym.unicode > 32 && keysym.unicode < 128)
        // only TRUE ASCII! (Otherwise XBMC crashes! No unicode not even latin 1!)
        ascii = (char)(keysym.unicode & 0xff);
    }
  }

  // At this point update the key hold time
  if (keysym.mod == m_lastKeysym.mod && keysym.scancode == m_lastKeysym.scancode && keysym.sym == m_lastKeysym.sym && keysym.unicode == m_lastKeysym.unicode)
  {
    held = CTimeUtils::GetFrameTime() - m_lastKeyTime;
  }
  else
  {
    m_lastKeysym = keysym;
    m_lastKeyTime = CTimeUtils::GetFrameTime();
    held = 0;
  }

  // Create and return a CKey

  CKey key(vkey, unicode, ascii, modifiers, held);

  return key;
}

void CKeyboardStat::ProcessKeyUp(void)
{
  memset(&m_lastKeysym, 0, sizeof(m_lastKeysym));
  m_lastKeyTime = 0;
}

// Return the key name given a key ID
// Used to make the debug log more intelligable
// The KeyID includes the flags for ctrl, alt etc

CStdString CKeyboardStat::GetKeyName(int KeyID)
{ int keyid;
  CStdString keyname;
  XBMCKEYTABLE keytable;

  keyname.clear();

// Get modifiers

  if (KeyID & CKey::MODIFIER_CTRL)
    keyname.append("ctrl-");
  if (KeyID & CKey::MODIFIER_SHIFT)
    keyname.append("shift-");
  if (KeyID & CKey::MODIFIER_ALT)
    keyname.append("alt-");
  if (KeyID & CKey::MODIFIER_SUPER)
    keyname.append("win-");

// Now get the key name

  keyid = KeyID & 0xFF;
  if (KeyTableLookupVKeyName(keyid, &keytable))
    keyname.append(keytable.keyname);
  else
    keyname.AppendFormat("%i", keyid);
  keyname.AppendFormat(" (%02x)", KeyID);

  return keyname;
}


