#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdlib.h>
#include <string.h>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/** USING DEFINITIONS AS DESCRIBED IN STATS.IDS */
#define MAX_STATS 164
#define MAX_SCRIPTS 5   //or 8 ?

class GEM_EXPORT Character
{
private:
	long BaseStats[MAX_STATS];
	long Modified[MAX_STATS];
	char  Name[33];
	char  Scripts[MAX_SCRIPTS][9];
	char  Dialog[9];
	char  Icon[9];
public:
	Character(void);
	~Character(void);
	/** Returns a Stat value */
	long  GetStat(unsigned char StatIndex);
	/** Returns the difference */
	long  GetMod(unsigned char StatIndex);
	/** Returns a Stat Base Value */
	long  GetBase(unsigned char StatIndex);
	/** Sets a Stat Base Value */
	bool  SetBase(unsigned char StatIndex, long Value);
	/** Sets the modified value in different ways, returns difference */
	int   NewMod(unsigned char StatIndex, long ModifierValue, long ModifierType);
	/** Sets the Character Name */
	void  SetName(const char * string)
	{
		if(string == NULL)
			return;
		strncpy(Name, string, 32);
	}
	/** Sets a Script ResRef */
	void  SetScript(int ScriptIndex, const char * ResRef)
	{
		if(ResRef == NULL)
			return;
		if(ScriptIndex>=MAX_SCRIPTS)
			return;
		strncpy(Scripts[ScriptIndex], ResRef, 8);
	}
	/** Sets the Dialog ResRef */
	void  SetDialog(const char * ResRef)
	{
		if(ResRef == NULL)
			return;
		strncpy(Dialog, ResRef, 8);
	}
	/** Sets the Icon ResRef */
	void  SetIcon(const char * ResRef)
	{
		if(ResRef == NULL)
			return;
		strncpy(Icon, ResRef, 8);
	}
	/** Gets the Character Name */
	char *GetName(void)
	{
		return Name;
	}
	/** Gets a Script ResRef */
	char *GetScript(int ScriptIndex)
	{
		return Scripts[ScriptIndex];
	}
	/** Gets the Dialog ResRef */
	char *GetDialog(void)
	{
		return Dialog;
	}
	/** Gets the Icon ResRef */
	char *GetIcon(void)
	{
		return Icon;
	}
};

#endif
