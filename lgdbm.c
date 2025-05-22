/*
* lgdbm.c
* gdbm interface for Lua
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br>
* 09 Jun 2023 10:07:21
* This code is hereby placed in the public domain and also under the MIT license
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define _POSIX_C_SOURCE 200809L
#include "gdbm.h"
#ifndef GDBM_NOMMAP
#define GDBM_NOMMAP 0
#endif

#include "lua.h"
#include "lauxlib.h"

#define MYNAME		"gdbm"
#define MYVERSION	MYNAME " library for " LUA_VERSION " / Jun 2023 / "\
			"using "
#define MYTYPE		MYNAME " handle"

#if LUA_VERSION_NUM <= 501

#define luaL_setmetatable(L,t)          \
        luaL_getmetatable(L,t);         \
        lua_setmetatable(L,-2)

#define luaL_setfuncs(L,r,n)            \
        luaL_register(L,NULL,r)

#endif

/* convenience macros */

#define luaL_boxpointer(L,u)            \
        (*(void **)(lua_newuserdata(L, sizeof(void *))) = (u))

#define luaL_unboxpointer(L,i,t)        \
        *((void**)luaL_checkudata(L,i,t))


static const char *errorstring(void)
{
 if (errno!=0)
  return strerror(errno);
 else
  return gdbm_strerror(gdbm_errno);
}

static int report(lua_State *L, int rc)
{
 if (rc==0)
 {
  lua_settop(L,1);
  return 1;
 }
 else
 {
  lua_pushnil(L);
  lua_pushstring(L,errorstring());
  return 2;
 }
}

static datum encode(lua_State *L, int i)
{
 datum d;
 size_t l;
 d.dptr=(char*)luaL_checklstring(L,i,&l);
 d.dsize=(int)l;
 return d;
}

static int decode(lua_State *L, datum d)
{
 if (d.dptr==NULL)
  return report(L,1);
 else
 {
  lua_pushlstring(L,d.dptr,d.dsize);
  free(d.dptr);
  return 1;
 }
}

static lua_State *LL=NULL;

static void errorhandler(const char *message)
{
 luaL_error(LL,"(gdbm) %s: %s",message,errorstring());
}

static GDBM_FILE Pget(lua_State *L, int i)
{
 LL=L;
 errno=0;
 return luaL_unboxpointer(L,i,MYTYPE);
}

static int Lopen(lua_State *L)			/** open(db,mode) */
{
 const char *file=luaL_checkstring(L,1);
 const char *mode=luaL_optstring(L,2,"r");
 int flags;
 GDBM_FILE dbf;
 GDBM_FILE *p=lua_newuserdata(L,sizeof(GDBM_FILE));
 switch (*mode)
 {
  default:
  case 'r': flags=GDBM_READER;  break;
  case 'w': flags=GDBM_WRITER;  break;
  case 'c': flags=GDBM_WRCREAT; break;
  case 'n': flags=GDBM_NEWDB;   break;
 }
 for (; *mode; mode++)
 {
  switch (*mode)
  {
   case 'L': flags|=GDBM_NOLOCK; break;
   case 'M': flags|=GDBM_NOMMAP; break;
   case 'S': flags|=GDBM_SYNC;   break;
  }
 }
 LL=L;
 errno=0;
 dbf=gdbm_open(file,0,flags,0666,errorhandler);
 if (dbf==NULL)
 {
  lua_pushnil(L);
  lua_pushfstring(L,"cannot open %s: %s",file,errorstring());
  return 2;
 }
 else
 {
  *p=dbf;
  luaL_setmetatable(L,MYTYPE);
  return 1;
 }
}

static int Pclose(lua_State *L)
{
 GDBM_FILE dbf=Pget(L,1);
#if GDBM_VERSION_MINOR >= 17
 int rc=gdbm_close(dbf);
#else
 int rc=0; gdbm_close(dbf);
#endif
 lua_pushnil(L);
 lua_setmetatable(L,1);
 return rc;
}

static int Lclose(lua_State *L)			/** close(db) */
{
 int rc=Pclose(L);
 return report(L,rc);
}

static int Lgc(lua_State *L)
{
 Pclose(L);
 return 0;
}

static int Lreorganize(lua_State *L)		/** reorganize(db) */
{
 GDBM_FILE dbf=Pget(L,1);
 int rc=gdbm_reorganize(dbf);
 return report(L,rc);
}

static int Lsync(lua_State *L)			/** sync(db) */
{
 GDBM_FILE dbf=Pget(L,1);
#if GDBM_VERSION_MINOR >= 17
 int rc=gdbm_sync(dbf);
#else
 int rc=0; gdbm_sync(dbf);
#endif
 return report(L,rc);
}

static int Lexists(lua_State *L)		/** exists(db,key) */
{
 GDBM_FILE dbf=Pget(L,1);
 datum key=encode(L,2);
 lua_pushboolean(L,gdbm_exists(dbf,key));
 return 1;
}

static int Lfetch(lua_State *L)			/** fetch(db,key) */
{
 GDBM_FILE dbf=Pget(L,1);
 datum key=encode(L,2);
 datum dat=gdbm_fetch(dbf,key);
 return decode(L,dat);
}

static int Pstore(lua_State *L, int flags)
{
 GDBM_FILE dbf=Pget(L,1);
 datum key=encode(L,2);
 datum dat=encode(L,3);
 int rc=gdbm_store(dbf,key,dat,flags);
 return report(L,rc);
}

static int Linsert(lua_State *L)		/** insert(db,key,data) */
{
 return Pstore(L,GDBM_INSERT);
}

static int Lreplace(lua_State *L)		/** replace(db,key,data) */
{
 return Pstore(L,GDBM_REPLACE);
}

static int Ldelete(lua_State *L)		/** delete(db,key) */
{
 GDBM_FILE dbf=Pget(L,1);
 datum key=encode(L,2);
 int rc=gdbm_delete(dbf,key);
 return report(L,rc);
}

static int Lfirstkey(lua_State *L)		/** firstkey(db) */
{
 GDBM_FILE dbf=Pget(L,1);
 datum dat=gdbm_firstkey(dbf);
 return decode(L,dat);
}

static int Lnextkey(lua_State *L)		/** nextkey(db,key) */
{
 GDBM_FILE dbf=Pget(L,1);
 datum key=encode(L,2);
 datum dat=gdbm_nextkey(dbf,key);
 return decode(L,dat);
}

#if GDBM_VERSION_MINOR >= 9

static int Lexport(lua_State *L)		/** export(db,file) */
{
 GDBM_FILE dbf=Pget(L,1);
 const char* file=luaL_checkstring(L,2);
 int rc=gdbm_export(dbf,file,GDBM_NEWDB,0666);
 return report(L,rc<0);
}

static int Limport(lua_State *L)		/** import(db,file,[replace]) */
{
 GDBM_FILE dbf=Pget(L,1);
 const char *file=luaL_checkstring(L,2);
 int replace=lua_toboolean(L,3);
 int rc=gdbm_import(dbf,file, replace ? GDBM_REPLACE : GDBM_INSERT);
 return report(L,rc<0);
}

#endif

#if GDBM_VERSION_MINOR >= 11

static int Lcount(lua_State *L)			/** count(db) */
{
 GDBM_FILE dbf=Pget(L,1);
 gdbm_count_t count;
 int rc=gdbm_count(dbf,&count);
 lua_settop(L,0);
 lua_pushinteger(L,count);
 return report(L,rc);
}

#endif

#if GDBM_VERSION_MINOR >= 12

static int Ldump(lua_State *L)			/** dump(db,output) */
{
 GDBM_FILE dbf=Pget(L,1);
 const char *file=luaL_checkstring(L,2);
 int rc=gdbm_dump(dbf,file,GDBM_DUMP_FMT_ASCII,GDBM_NEWDB,0666);
 return report(L,rc);
}

static int Lload(lua_State *L)			/** load(db,input,[replace]) */
{
 GDBM_FILE dbf=Pget(L,1);
 const char *file=luaL_checkstring(L,2);
 int replace=lua_toboolean(L,3);
 int rc=gdbm_load(&dbf,file, replace ? GDBM_REPLACE : GDBM_INSERT,0,NULL);
 return report(L,rc);
}
#endif

#if GDBM_VERSION_MINOR >= 13

static int Lcopymeta(lua_State *L)		/** copymeta(dst,src) */
{
 GDBM_FILE dst=Pget(L,1);
 GDBM_FILE src=Pget(L,2);
 int rc=gdbm_copy_meta(dst,src);
 return report(L,rc);
}

static int Llasterror(lua_State *L)		/** lasterror(db) */
{
 GDBM_FILE dbf=Pget(L,1);
 lua_pushstring(L,gdbm_db_strerror(dbf));
 return 1;
}

static int Lneedsrecovery(lua_State *L)		/** needsrecovery(db) */
{
 GDBM_FILE dbf=Pget(L,1);
 lua_pushboolean(L,gdbm_needs_recovery(dbf));
 return 1;
}

static int Lrecover(lua_State *L)		/** recover(db) */
{
 GDBM_FILE dbf=Pget(L,1);
 int rc=gdbm_recover(dbf,NULL,0);
 return report(L,rc);
}

#endif

#if LUA_VERSION_NUM <= 502
static int Ltostring(lua_State *L)
{
 lua_pushfstring(L,"%s: %p",MYTYPE,(void*)Pget(L,1));
 return 1;
}
#define MYTOSTRING {"__tostring",Ltostring},
#else
#define MYTOSTRING
#endif

static const luaL_Reg R[] =
{
	MYTOSTRING
	{ "__gc",		Lgc		},
	{ "close",		Lclose		},
	{ "delete",		Ldelete		},
	{ "exists",		Lexists		},
	{ "fetch",		Lfetch		},
	{ "firstkey",		Lfirstkey	},
	{ "insert",		Linsert		},
	{ "nextkey",		Lnextkey	},
	{ "open",		Lopen		},
	{ "reorganize",		Lreorganize	},
	{ "replace",		Lreplace	},
	{ "sync",		Lsync		},
#if GDBM_VERSION_MINOR >= 9
	{ "export",		Lexport		},
	{ "import",		Limport		},
#endif
#if GDBM_VERSION_MINOR >= 11
	{ "count",		Lcount		},
#endif
#if GDBM_VERSION_MINOR >= 12
	{ "dump",		Ldump		},
	{ "load",		Lload		},
#endif
#if GDBM_VERSION_MINOR >= 13
	{ "copymeta",		Lcopymeta	},
	{ "lasterror",		Llasterror	},
	{ "needsrecovery",	Lneedsrecovery	},
	{ "recover",		Lrecover	},
#endif
	{ NULL,			NULL		}
};

LUALIB_API int luaopen_gdbm(lua_State *L)
{
 luaL_newmetatable(L,MYTYPE);
 luaL_setfuncs(L,R,0);
 lua_pushliteral(L,"version");			/** version */
 lua_pushliteral(L,MYVERSION);
 lua_pushstring(L,gdbm_version);
 lua_concat(L,2);
 lua_settable(L,-3);
 lua_pushliteral(L,"__index");
 lua_pushvalue(L,-2);
 lua_settable(L,-3);
 return 1;
}
