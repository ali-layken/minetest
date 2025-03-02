/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "common/c_internal.h"
#include "debug.h"
#include "log.h"
#include "porting.h"
#include "settings.h"

std::string script_get_backtrace(lua_State *L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_BACKTRACE);
	lua_call(L, 0, 1);
	std::string result = luaL_checkstring(L, -1);
	lua_pop(L, 1);
	return result;
}

#define LEVELS1 12 /* size of the first part of the stack */
#define LEVELS2 10 /* size of the second part of the stack */

int lua_absindex(lua_State *L, int i)
{
	if (i < 0 && i > LUA_REGISTRYINDEX)
		i += lua_gettop(L) + 1;
	return i;
}

void lua_copy(lua_State *L, int from, int to)
{
	int abs_to = lua_absindex(L, to);
	luaL_checkstack(L, 1, "not enough stack slots");
	lua_pushvalue(L, from);
	lua_replace(L, abs_to);
}

static int findfield(lua_State *L, int objidx, int level)
{
	if (level == 0 || !lua_istable(L, -1))
		return 0;				   /* not found */
	lua_pushnil(L);					   /* start 'next' loop */
	while (lua_next(L, -2)) {			   /* for each pair in table */
		if (lua_type(L, -2) == LUA_TSTRING) {	   /* ignore non-string keys */
			if (lua_rawequal(L, objidx, -1)) { /* found object? */
				lua_pop(L, 1); /* remove value (but keep name) */
				return 1;
			} else if (findfield(L, objidx,
						   level - 1)) { /* try recursively */
				lua_remove(L, -2); /* remove table (but keep name) */
				lua_pushliteral(L, ".");
				lua_insert(L, -2); /* place '.' between the two names */
				lua_concat(L, 3);
				return 1;
			}
		}
		lua_pop(L, 1); /* remove value */
	}
	return 0; /* not found */
}

static int pushglobalfuncname(lua_State *L, lua_Debug *ar)
{
	int top = lua_gettop(L);
	lua_getinfo(L, "f", ar); /* push function */
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	if (findfield(L, top + 1, 2)) {
		lua_copy(L, -1, top + 1); /* move name to proper place */
		lua_pop(L, 2);		  /* remove pushed values */
		return 1;
	} else {
		lua_settop(L, top); /* remove function and global table */
		return 0;
	}
}

static void pushfuncname(lua_State *L, lua_Debug *ar)
{
	if (*ar->namewhat != '\0') /* is there a name? */
		lua_pushfstring(L, "function " LUA_QS, ar->name);
	else if (*ar->what == 'm') /* main? */
		lua_pushliteral(L, "main chunk");
	else if (*ar->what == 'C') {
		if (pushglobalfuncname(L, ar)) {
			lua_pushfstring(L, "function " LUA_QS, lua_tostring(L, -1));
			lua_remove(L, -2); /* remove name */
		} else
			lua_pushliteral(L, "?");
	} else
		lua_pushfstring(L, "function <%s:%d>", ar->short_src, ar->linedefined);
}

static int countlevels(lua_State *L)
{
	lua_Debug ar;
	int li = 1, le = 1;
	/* find an upper bound */
	while (lua_getstack(L, le, &ar)) {
		li = le;
		le *= 2;
	}
	/* do a binary search */
	while (li < le) {
		int m = (li + le) / 2;
		if (lua_getstack(L, m, &ar))
			li = m + 1;
		else
			le = m;
	}
	return le - 1;
}


void luaL_traceback(lua_State *L, lua_State *L1, const char *msg, int level)
{
	lua_Debug ar;
	int top = lua_gettop(L);
	int numlevels = countlevels(L1);
	int mark = (numlevels > LEVELS1 + LEVELS2) ? LEVELS1 : 0;
	if (msg)
		lua_pushfstring(L, "%s\n", msg);
	lua_pushliteral(L, "stack traceback:");
	while (lua_getstack(L1, level++, &ar)) {
		if (level == mark) {		       /* too many levels? */
			lua_pushliteral(L, "\n\t..."); /* add a '...' */
			level = numlevels - LEVELS2;   /* and skip to last ones */
		} else {
			lua_getinfo(L1, "Slnt", &ar);
			lua_pushfstring(L, "\n\t%s:", ar.short_src);
			if (ar.currentline > 0)
				lua_pushfstring(L, "%d:", ar.currentline);
			lua_pushliteral(L, " in ");
			pushfuncname(L, &ar);
			lua_concat(L, lua_gettop(L) - top);
		}
	}
	lua_concat(L, lua_gettop(L) - top);
}



int script_exception_wrapper(lua_State *L, lua_CFunction f)
{
	try {
		return f(L);  // Call wrapped function and return result.
	} catch (const char *s) {  // Catch and convert exceptions.
		lua_pushstring(L, s);
	} catch (std::exception &e) {
		lua_pushstring(L, e.what());
	}
	return lua_error(L);  // Rethrow as a Lua error.
}

/*
 * Note that we can't get tracebacks for LUA_ERRMEM or LUA_ERRERR (without
 * hacking Lua internals).  For LUA_ERRMEM, this is because memory errors will
 * not execute the error handler, and by the time lua_pcall returns the
 * execution stack will have already been unwound.  For LUA_ERRERR, there was
 * another error while trying to generate a backtrace from a LUA_ERRRUN.  It is
 * presumed there is an error with the internal Lua state and thus not possible
 * to gather a coherent backtrace.  Realistically, the best we can do here is
 * print which C function performed the failing pcall.
 */
void script_error(lua_State *L, int pcall_result, const char *mod, const char *fxn)
{
	if (pcall_result == 0)
		return;

	const char *err_type;
	switch (pcall_result) {
	case LUA_ERRRUN:
		err_type = "Runtime";
		break;
	case LUA_ERRMEM:
		err_type = "OOM";
		break;
	case LUA_ERRERR:
		err_type = "Double fault";
		break;
	default:
		err_type = "Unknown";
	}

	if (!mod)
		mod = "??";

	if (!fxn)
		fxn = "??";

	const char *err_descr = lua_tostring(L, -1);
	if (!err_descr)
		err_descr = "<no description>";

	char buf[256];
	porting::mt_snprintf(buf, sizeof(buf), "%s error from mod '%s' in callback %s(): ",
		err_type, mod, fxn);

	std::string err_msg(buf);
	err_msg += err_descr;

	if (pcall_result == LUA_ERRMEM) {
		err_msg += "\nCurrent Lua memory usage: "
			+ itos(lua_gc(L, LUA_GCCOUNT, 0) >> 10) + " MB";
	}

	throw LuaError(err_msg);
}

// Push the list of callbacks (a lua table).
// Then push nargs arguments.
// Then call this function, which
// - runs the callbacks
// - replaces the table and arguments with the return value,
//     computed depending on mode
void script_run_callbacks_f(lua_State *L, int nargs,
	RunCallbacksMode mode, const char *fxn)
{
	FATAL_ERROR_IF(lua_gettop(L) < nargs + 1, "Not enough arguments");

	// Insert error handler
	PUSH_ERROR_HANDLER(L);
	int error_handler = lua_gettop(L) - nargs - 1;
	lua_insert(L, error_handler);

	// Insert run_callbacks between error handler and table
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "run_callbacks");
	lua_remove(L, -2);
	lua_insert(L, error_handler + 1);

	// Insert mode after table
	lua_pushnumber(L, (int) mode);
	lua_insert(L, error_handler + 3);

	// Stack now looks like this:
	// ... <error handler> <run_callbacks> <table> <mode> <arg#1> <arg#2> ... <arg#n>

	int result = lua_pcall(L, nargs + 2, 1, error_handler);
	if (result != 0)
		script_error(L, result, NULL, fxn);

	lua_remove(L, error_handler);
}

static void script_log(lua_State *L, const std::string &message,
	std::ostream &log_to, bool do_error, int stack_depth)
{
	lua_Debug ar;

	log_to << message << " ";
	if (lua_getstack(L, stack_depth, &ar)) {
		FATAL_ERROR_IF(!lua_getinfo(L, "Sl", &ar), "lua_getinfo() failed");
		log_to << "(at " << ar.short_src << ":" << ar.currentline << ")";
	} else {
		log_to << "(at ?:?)";
	}
	log_to << std::endl;

	if (do_error)
		script_error(L, LUA_ERRRUN, NULL, NULL);
	else
		infostream << script_get_backtrace(L) << std::endl;
}

DeprecatedHandlingMode get_deprecated_handling_mode()
{
	static thread_local bool configured = false;
	static thread_local DeprecatedHandlingMode ret = DeprecatedHandlingMode::Ignore;

	// Only read settings on first call
	if (!configured) {
		std::string value = g_settings->get("deprecated_lua_api_handling");
		if (value == "log") {
			ret = DeprecatedHandlingMode::Log;
		} else if (value == "error") {
			ret = DeprecatedHandlingMode::Error;
		}
		configured = true;
	}

	return ret;
}

void log_deprecated(lua_State *L, const std::string &message, int stack_depth)
{
	DeprecatedHandlingMode mode = get_deprecated_handling_mode();
	if (mode != DeprecatedHandlingMode::Ignore)
		script_log(L, message, warningstream, mode == DeprecatedHandlingMode::Error, stack_depth);
}
