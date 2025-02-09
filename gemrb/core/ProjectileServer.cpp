/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
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
 *
 */

#include "ProjectileServer.h"

#include "GameData.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "ProjectileMgr.h"
#include "SymbolMgr.h"

namespace GemRB {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_PROJ_IDX  0x1fff

ProjectileServer::ProjectileServer()
{
	projectilecount = -1;
	projectiles = NULL;
	explosioncount = -1;
	explosions = NULL;
}

ProjectileServer::~ProjectileServer()
{
	if (projectiles) {
		delete[] projectiles;
	}
	if (explosions) {
		delete[] explosions;
	}
}

Projectile *ProjectileServer::CreateDefaultProjectile(unsigned int idx)
{
	Projectile *pro = new Projectile();

	//take care, this projectile is not freed up by the server
	if(idx==(unsigned int) ~0 ) {
		return pro;
	}

	projectiles[idx].projectile = pro;
	pro->SetIdentifiers(projectiles[idx].resname, idx);
	return ReturnCopy(idx);
}

//this function can return only projectiles listed in projectl.ids
Projectile *ProjectileServer::GetProjectileByName(const ResRef &resname)
{
	if (!core->IsAvailable(IE_PRO_CLASS_ID)) {
		return NULL;
	}
	unsigned int idx=GetHighestProjectileNumber();
	while(idx--) {
		if (resname == projectiles[idx].resname) {
			return GetProjectile(idx);
		}
	}
	return NULL;
}

Projectile *ProjectileServer::GetProjectileByIndex(unsigned int idx)
{
	if (!core->IsAvailable(IE_PRO_CLASS_ID)) {
		return NULL;
	}
	if (idx>=GetHighestProjectileNumber()) {
		return GetProjectile(0);
	}

	return GetProjectile(idx);
}

Projectile *ProjectileServer::ReturnCopy(unsigned int idx)
{
	const ProjectileEntry& old = projectiles[idx];
	Projectile *pro = new Projectile(*old.projectile);
	pro->SetIdentifiers(old.resname, idx);
	return pro;
}

Projectile *ProjectileServer::GetProjectile(unsigned int idx)
{
	if (projectiles[idx].projectile) {
		return ReturnCopy(idx);
	}
	DataStream* str = gamedata->GetResource( projectiles[idx].resname, IE_PRO_CLASS_ID );
	PluginHolder<ProjectileMgr> sm = MakePluginHolder<ProjectileMgr>(IE_PRO_CLASS_ID);
	if (!sm) {
		delete ( str );
		return CreateDefaultProjectile(idx);
	}
	if (!sm->Open(str)) {
		return CreateDefaultProjectile(idx);
	}
	Projectile *pro = new Projectile();
	projectiles[idx].projectile = pro;
	pro->SetIdentifiers(projectiles[idx].resname, idx);

	sm->GetProjectile( pro );
	int Type = 0xff;

	if(pro->Extension) {
		Type = pro->Extension->ExplType;
	}
	if(Type<0xff) {
		ResRef res;

		//fill the spread field according to the hardcoded explosion type
		res = GetExplosion(Type,0);
		if (res) {
			pro->Extension->Spread = res;
		}
	
		//if the hardcoded explosion type has a center animation
		//override the VVC animation field with it
		res = GetExplosion(Type,1);
		if(res) {
			pro->Extension->AFlags|=PAF_VVC;
			pro->Extension->VVCRes = res;
		}

		//fill the secondary spread field out
		res = GetExplosion(Type,2);
		if(res) {
			pro->Extension->Secondary = res;
		}

		//the explosion sound, used for the first explosion
		//(overrides an original field)
		res = GetExplosion(Type,3);
		if(res) {
			pro->Extension->SoundRes = res;
		}

		//the area sound (used for subsequent explosions)
		res = GetExplosion(Type,4);
		if(res) {
			pro->Extension->AreaSound = res;
		}

		//fill the explosion/spread animation flags
		pro->Extension->APFlags = GetExplosionFlags(Type);
	}

	return ReturnCopy(idx);
}

int ProjectileServer::InitExplosion()
{
	if (explosioncount>=0) {
		return explosioncount;
	}

	AutoTable explist("areapro");
	if (explist) {
		explosioncount = 0;

		unsigned int rows = explist->GetRowCount();
		//cannot handle 0xff and it is easier to set up the fields
		//without areapro.2da anyway. So this isn't a restriction
		if(rows>254) {
			rows=254;
		}
		explosioncount = rows;
		explosions = new ExplosionEntry[rows];

		while(rows--) {
			int i;

			for(i=0;i<AP_RESCNT;i++) {
				explosions[rows].resources[i] = ResRef::MakeUpperCase(explist->QueryField(rows, i));
			}
			//using i so the flags field will always be after the resources
			explosions[rows].flags = atoi(explist->QueryField(rows,i));
		}
	}
	return explosioncount;
}

unsigned int ProjectileServer::PrepareSymbols(const Holder<SymbolMgr>& projlist) {
	unsigned int count = 0;

	unsigned int rows = (unsigned int) projlist->GetSize();
	while(rows--) {
		unsigned int value = projlist->GetValueIndex(rows);
		if (value>MAX_PROJ_IDX) {
			//value = MAX_PROJ_IDX;
			Log(WARNING, "ProjectileServer", "Too high projectilenumber");
			continue; // ignore
		}
		if (value > count) {
			count = value;
		}
	}

	return count;
}

void ProjectileServer::AddSymbols(const Holder<SymbolMgr>& projlist) {
	unsigned int rows = (unsigned int) projlist->GetSize();
	while(rows--) {
		unsigned int value = projlist->GetValueIndex(rows);
		if (value>MAX_PROJ_IDX) {
			continue;
		}
		if (value >= (unsigned int)projectilecount) {
			// this should never happen!
			error("ProjectileServer", "Too high projectilenumber while adding projectiles\n");
		}
		projectiles[value].resname = ResRef::MakeUpperCase(projlist->GetStringIndex(rows));
	}
}

unsigned int ProjectileServer::GetHighestProjectileNumber()
{
	if (projectilecount>=0) {
		// already read the projectiles
		return (unsigned int) projectilecount;
	}

	// built-in gemrb projectiles and game/mod-provided projectiles
	unsigned int gemresource = core->LoadSymbol("gemprjtl");
	Holder<SymbolMgr> gemprojlist = core->GetSymbol(gemresource);
	unsigned int resource = core->LoadSymbol("projectl");
	Holder<SymbolMgr> projlist = core->GetSymbol(resource);

	// first, we must calculate how many projectiles we have
	if (gemprojlist) {
		projectilecount = PrepareSymbols(gemprojlist) + 1;
	}
	if (projlist) {
		unsigned int temp = PrepareSymbols(projlist) + 1;
		if (projectilecount == -1 || temp > (unsigned int)projectilecount)
			projectilecount = temp;
	}

	// then, allocate space for them all
	if (projectilecount == -1) {
		// no valid projectiles files..
		projectilecount = 1;
	}	
	projectiles = new ProjectileEntry[projectilecount];
	
	// finally, we must read the projectile resrefs
	if (projlist) {
		AddSymbols(projlist);
		core->DelSymbol(resource);
	}
	// gemprojlist is second because it always overrides game/mod-supplied projectiles
	if (gemprojlist) {
		AddSymbols(gemprojlist);
		core->DelSymbol(gemresource);
	}

	return (unsigned int) projectilecount;
}

//return various flags for the explosion type
int ProjectileServer::GetExplosionFlags(unsigned int idx)
{
	if (explosioncount==-1) {
		if (InitExplosion()<0) {
			Log(ERROR, "ProjectileServer", "Problem with explosions!");
			explosioncount=0;
		}
	}
	if (idx>=(unsigned int) explosioncount) {
		return 0;
	}

	return explosions[idx].flags;
}

ResRef ProjectileServer::GetExplosion(unsigned int idx, int type)
{
	if (explosioncount==-1) {
		if (InitExplosion()<0) {
			Log(ERROR, "ProjectileServer", "Problem with explosions");
			explosioncount=0;
		}
	}
	if (idx>=(unsigned int) explosioncount) {
		return ResRef();
	}
	ResRef const *ret = &explosions[idx].resources[type];
	if (!ret || ret->IsStar()) return ResRef();

	return *ret;
}

}
