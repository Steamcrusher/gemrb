/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef GemRB_GUIScriptInterface_h
#define GemRB_GUIScriptInterface_h

#include "Control.h"
#include "ScriptEngine.h"
#include "Window.h"

namespace GemRB {

View* GetView(ScriptingRefBase* base);
Window* GetWindow(ScriptingId id);
Control* GetControl(ScriptingId id, Window* win);

ControlScriptingRef* GetControlRef(ScriptingId id, Window* win);

template <class T>
T* GetControl(ScriptingId id, Window* win) {
	return dynamic_cast<T*>(GetControl(id, win));
}

}

#endif
