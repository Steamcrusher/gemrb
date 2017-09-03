/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 */

#include "GUI/MapControl.h"

#include "win32def.h"
#include "ie_cursors.h"

#include "Game.h"
#include "GlobalTimer.h"
#include "Map.h"
#include "Sprite2D.h"
#include "GUI/GameControl.h"
#include "Scriptable/Actor.h"

namespace GemRB {

#define MAP_NO_NOTES   0
#define MAP_VIEW_NOTES 1
#define MAP_SET_NOTE   2
#define MAP_REVEAL     3

typedef enum {black=0, gray, violet, green, orange, red, blue, darkblue, darkgreen} colorcode;

static const Color colors[]={
 ColorBlack,
 ColorGray,
 ColorViolet,
 ColorGreen,
 ColorOrange,
 ColorRed,
 ColorBlue,
 ColorBlueDark,
 ColorGreenDark
};

MapControl::MapControl(const Region& frame)
	: Control(frame)
{
	ControlType = IE_GUI_MAP;

	LinkedLabel = NULL;
	NotePosX = 0;
	NotePosY = 0;
	MapWidth = MapHeight = 0;

	memset(Flag,0,sizeof(Flag) );

	MyMap = core->GetGame()->GetCurrentArea();
	if (MyMap && MyMap->SmallMap) {
		MapMOS = MyMap->SmallMap;
		MapMOS->acquire();
	} else
		MapMOS = NULL;
}

MapControl::~MapControl(void)
{
	if (MapMOS) {
		Sprite2D::FreeSprite(MapMOS);
	}
	for(int i=0;i<8;i++) {
		if (Flag[i]) {
			Sprite2D::FreeSprite(Flag[i]);
		}
	}
}

// Draw fog on the small bitmap
void MapControl::DrawFog(const Region& /*rgn*/)
{
/*
	Video *video = core->GetVideoDriver();
	Point p = rgn.Origin();

	// FIXME: this is ugly, the knowledge of Map and ExploredMask
	//   sizes should be in Map.cpp
	int w = MyMap->GetWidth() / 2;
	int h = MyMap->GetHeight() / 2;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			p.x = (MAP_MULT * x), p.y = (MAP_MULT * y);
			bool visible = MyMap->IsVisible( p, true );
			if (!visible) {
				p.x = (MAP_DIV * x), p.y = (MAP_DIV * y);
				Region rgn = Region ( ConvertPointToScreen(p), Size(MAP_DIV, MAP_DIV) );
				video->DrawRect( rgn, colors[black] );
			}
		}
	}
*/
}

void MapControl::UpdateState(unsigned int Sum)
{
	SetValue(Sum);
}

/** Draws the Control on the Output Display */
void MapControl::DrawSelf(Region rgn, const Region& /*clip*/)
{
	Video* video = core->GetVideoDriver();
	if (MapMOS == NULL) return; // FIXME: I'm not sure if/why/when a NULL MOS is valid
	
	const Size mosSize(MapMOS->Width, MapMOS->Height);
	const Point center(rgn.w/2 - mosSize.w/2, rgn.h/2 - mosSize.h/2);
	const Point origin = rgn.Origin() + center;
	
	video->BlitSprite( MapMOS, origin.x, origin.y, &rgn );

	if (core->FogOfWar&FOG_DRAWFOG)
		DrawFog(rgn);

	GameControl* gc = core->GetGameControl();
	Map* map = core->GetGame()->GetCurrentArea();
	Size mapsize = map->GetSize();

	Region vp = gc->Viewport();
	vp.x *= double(mosSize.w) / mapsize.w;
	vp.y *= double(mosSize.h) / mapsize.h;
	vp.w *= double(mosSize.w) / mapsize.w;
	vp.h *= double(mosSize.h) / mapsize.h;
	
	vp.x += origin.x;
	vp.y += origin.y;
	video->DrawRect(vp, colors[green], false );
	
	// Draw PCs' ellipses
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	while (i--) {
		Actor* actor = game->GetPC( i, true );
		if (MyMap->HasActor(actor) ) {
			Point pos = actor->Pos;
			pos.x *= double(mosSize.w) / mapsize.w;
			pos.y *= double(mosSize.h) / mapsize.h;
			pos.x += origin.x;
			pos.y += origin.y;
			
			video->DrawEllipse( pos.x, pos.y, 3, 2, actor->Selected ? colors[green] : colors[darkgreen] );
		}
	}
	// Draw Map notes, could be turned off in bg2
	// we use the common control value to handle it, because then we
	// don't need another interface
	if (GetValue()!=MAP_NO_NOTES) {
		i = MyMap -> GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap -> GetMapNote(i);
			Sprite2D *anim = Flag[mn.color&7];
			
			Point pos = mn.Pos;
			pos.x *= double(mosSize.w) / mapsize.w;
			pos.y *= double(mosSize.h) / mapsize.h;
			pos.x += origin.x;
			pos.y += origin.y;
			
			//Skip unexplored map notes
			bool visible = MyMap->IsVisible( pos, true );
			if (!visible)
				continue;

			if (anim) {
				video->BlitSprite( anim, pos.x - anim->Width/2, pos.y - anim->Height/2, &rgn );
			} else {
				video->DrawEllipse( pos.x, pos.y, 6, 5, colors[mn.color&7] );
			}
		}
	}
}

/** Mouse Over Event */
void MapControl::OnMouseOver(const MouseEvent& me)
{
	Point p = ConvertPointFromScreen(me.Pos());

	ieDword val = GetValue();
	// FIXME: implement cursor changing
	switch (val) {
		case MAP_REVEAL: //for farsee effect
			//Owner->Cursor = IE_CURSOR_CAST;
			break;
		case MAP_SET_NOTE:
			//Owner->Cursor = IE_CURSOR_GRAB;
			break;
		default:
			//Owner->Cursor = IE_CURSOR_NORMAL;
			break;
	}

	if (val) {
		/*
		unsigned int dist;
		if (convertToGame) {
			mp.x = (short) SCREEN_TO_GAMEX(p.x);
			mp.y = (short) SCREEN_TO_GAMEY(p.y);
			dist = 100;
		} else {
			mp.x = (short) SCREEN_TO_MAPX(p.x);
			mp.y = (short) SCREEN_TO_MAPY(p.y);
			dist = 16;
		}
		int i = MyMap -> GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap -> GetMapNote(i);
			if (Distance(mp, mn.Pos)<dist) {
				if (LinkedLabel) {
					LinkedLabel->SetText( mn.text );
				}
				NotePosX = mn.Pos.x;
				NotePosY = mn.Pos.y;
				return;
			}
		}
		*/
		NotePosX = p.x;
		NotePosY = p.y;
	}
	if (LinkedLabel) {
		LinkedLabel->SetText( L"" );
	}
}

void MapControl::ClickHandle()
{
	core->GetDictionary()->SetAt( "MapControlX", NotePosX );
	core->GetDictionary()->SetAt( "MapControlY", NotePosY );
}

void MapControl::ViewHandle(unsigned short x, unsigned short y)
{
	// clear any previously scheduled moves and then do it asap, so it works while paused
	Point p(x,y);//(xp * MAP_MULT / MAP_DIV, yp * MAP_MULT / MAP_DIV);
	core->timer->SetMoveViewPort( p, 0, false );
}

/** Mouse Button Down */
void MapControl::OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/)
{
	/*
	Region vp = core->GetGameControl()->Viewport();
	vp.w = vp.x+ViewWidth*MAP_MULT/MAP_DIV;
	vp.h = vp.y+ViewHeight*MAP_MULT/MAP_DIV;
	ViewHandle(p.x, p.y);
	lastMouseX = p.x;
	lastMouseY = p.y;
	*/
}

/** Mouse Button Up */
void MapControl::OnMouseUp(const MouseEvent& me, unsigned short /*Mod*/)
{
	if (me.button == GEM_MB_ACTION && me.repeats == 2) {
		window->Close();
	}

	/*
	switch(Value) {
		case MAP_REVEAL:
			ViewHandle(p.x, p.y);
			NotePosX = (short) SCREEN_TO_MAPX(p.x) * MAP_MULT / MAP_DIV;
			NotePosY = (short) SCREEN_TO_MAPY(p.y) * MAP_MULT / MAP_DIV;
			ClickHandle(Button);
			return;
		case MAP_NO_NOTES:
			ViewHandle(p.x, p.y);
			return;
		case MAP_VIEW_NOTES:
			//left click allows setting only when in MAP_SET_NOTE mode
			if (Button == GEM_MB_ACTION) {
				ViewHandle(p.x, p.y);
			}
			ClickHandle(Button);
			return;
		default:
			return Control::OnMouseUp(me, Mod);
	}
	*/
}

bool MapControl::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	switch (key.keycode) {
		case GEM_LEFT:
		case GEM_RIGHT:
		case GEM_UP:
		case GEM_DOWN:
			GameControl* gc = core->GetGameControl();
			return gc->OnKeyPress(key, mod);
	}
	return false;
}

}
