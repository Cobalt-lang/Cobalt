/*
* linit.c
* Initialize libraries for Cobalt
* Read copyright notice at cobalt.h
*/


#define linit_c
#define LUA_LIB

/*
** If you embed Lua in your program and need to open the standard
** libraries, call luaL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
**
** You can also *preload* libraries, so that a later 'require' can
** open the library, which is already linked to the application.
** For that, do the following code:
**
**  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
**  lua_pushcfunction(L, luaopen_modname);
**  lua_setfield(L, -2, modname);
**  lua_pop(L, 1);  // remove PRELOAD table
*/

#include "lprefix.h"


#include <stddef.h>

#include "cobalt.h"

#include "lualib.h"
#include "lauxlib.h"
#include "llink.c"

/*
** these libs are loaded by cobalt.c and are readily available to any Lua
** program
*/
static const luaL_Reg loadedlibs[] = {
  /* Base */
  {"_G", luaopen_base},
  
  /* C API */
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_IOLIBNAME, luaopen_io}, 
  {LUA_ASYNCLIBNAME, luaopen_async},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string}, 
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},
  {LUA_CORENAME, luaopen_core},
  {LUA_DEVICENAME, luaopen_device},
  {LUA_FILESYSTEMNAME, luaopen_lfs},
  {LUA_COMPLEXNAME, luaopen_complex},
  {LUA_SIGNALNAME, luaopen_signal},
  {LUA_SOCKETNAME, luaopen_chan},
  {LUA_BITLIBNAME, luaopen_bit32},
  {LUA_BITOPNAME, luaopen_bit},
  {LUA_ALLOCNAME, luaopen_alloc},
  

  {NULL, NULL}
};

/*
** these libs are in the source code but must be `require`/`import`-d 
** into the global table
*/
static const luaL_Reg preloadedlibs[] = {
  /* Platform specifics */
  #if defined __unix__ || defined LUA_USE_POSIX || defined __APPLE__
  {LUA_UNIXNAME, luaopen_unix},
  #elif defined _WIN32 || defined _WIN64 || defined __CYGWIN__ || defined __MINGW32__ || defined LUA_USE_WINDOWS || defined LUA_USE_MINGW
  {LUA_WINNAME, luaopen_win},
  #endif

  /* C API */
  {LUA_STRUCTNAME, luaopen_struct},
  {LUA_COLORLIBNAME, luaopen_color},
  {LUA_2DLIBNAME, luaopen_2D},
  {LUA_3DLIBNAME, luaopen_3D},
  {LUA_TRANSFORMNAME, luaopen_transform},
  {LUA_JSONAME, luaopen_cjson_safe},

  // FFI
  #ifdef COBALT_FFI
  {LUA_FFINAME, luaopen_ffi},
  #endif
  
  // Bindings
  #ifdef COBALT_SDL
  {LUA_SDLNAME, luaopen_moonsdl2},
  #endif
  #ifdef COBALT_SOCKET
  {LUA_CURLNAME, luaopen_lcurl},
  #endif

  {NULL, NULL}
};

LUALIB_API void luaL_openlibs(lua_State *L)
{
  const luaL_Reg *lib;

  /* "require" functions from 'loadedlibs' and set results to global table */
  for (lib = loadedlibs; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);  /* remove lib */
  }

  /* add open functions from 'preloadedlibs' into 'package.preload' table */
  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
  for (lib = preloadedlibs; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_setfield(L, -2, lib->name);
  }

}