// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef LUABIND_REF_HPP_INCLUDED
#define LUABIND_REF_HPP_INCLUDED

#include <luabind/config.hpp>

// most of the code in this file comes from
// lauxlib.c in lua distribution

/******************************************************************************
* Copyright (C) 1994-2003 Tecgraf, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

// TODO: move thses functions into their
// own compilation unit

namespace luabind { namespace detail
{
	enum
	{
		// the number of reserved references
		RESERVED_REFS = 2,

		// free list of references
		FREELIST_REF = 1,

		// array sizes (not used here)
		ARRAYSIZE_REF = 2
	};

	inline int checkint (lua_State *L, int topop)
	{
		int n = (int)lua_tonumber(L, -1);
		if (n == 0 && !lua_isnumber(L, -1)) n = -1;
		lua_pop(L, topop);
		return n;
	}


	inline void getsizes (lua_State *L)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, ARRAYSIZE_REF);
		if (lua_isnil(L, -1)) {  /* no `size' table? */
			lua_pop(L, 1);  /* remove nil */
			lua_newtable(L);  /* create it */
			lua_pushvalue(L, -1);  /* `size' will be its own metatable */
			lua_setmetatable(L, -2);
			lua_pushliteral(L, "__mode");
			lua_pushliteral(L, "k");
			lua_rawset(L, -3);  /* metatable(N).__mode = "k" */
			lua_pushvalue(L, -1);
			lua_rawseti(L, LUA_REGISTRYINDEX, ARRAYSIZE_REF);  /* store in register */
		}
	}

	inline void luaL_setn (lua_State *L, int t, int n)
	{
		lua_pushliteral(L, "n");
		lua_rawget(L, t);
		if (checkint(L, 1) >= 0) {  /* is there a numeric field `n'? */
			lua_pushliteral(L, "n");  /* use it */
			lua_pushnumber(L, n);
			lua_rawset(L, t);
		}
		else {  /* use `sizes' */
			getsizes(L);
			lua_pushvalue(L, t);
			lua_pushnumber(L, n);
			lua_rawset(L, -3);  /* sizes[t] = n */
			lua_pop(L, 1);  /* remove `sizes' */
		}
	}

	inline int luaL_getn (lua_State *L, int t)
	{
		int n;
		lua_pushliteral(L, "n");  /* try t.n */
		lua_rawget(L, t);
		if ((n = checkint(L, 1)) >= 0) return n;
		getsizes(L);  /* else try sizes[t] */
		lua_pushvalue(L, t);
		lua_rawget(L, -2);
		if ((n = checkint(L, 2)) >= 0) return n;
		for (n = 1; ; n++) {  /* else must count elements */
			lua_rawgeti(L, t, n);
			if (lua_isnil(L, -1)) break;
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		return n - 1;
	}


	// based on luaL_ref
	inline int ref(lua_State *L)
	{
		int t = LUA_REGISTRYINDEX;

		int ref;
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);  /* remove from stack */
			return LUA_REFNIL;  /* `nil' has a unique fixed reference */
		}

		lua_rawgeti(L, t, FREELIST_REF);  /* get first free element */
		ref = (int)lua_tonumber(L, -1);  /* ref = t[FREELIST_REF] */
		lua_pop(L, 1);  /* remove it from stack */
		if (ref != 0) {  /* any free element? */
			lua_rawgeti(L, t, ref);  /* remove it from list */
			lua_rawseti(L, t, FREELIST_REF);  /* (t[FREELIST_REF] = t[ref]) */
		}
		else {  /* no free elements */
			ref = ::luabind::detail::luaL_getn(L, t);
			if (ref < RESERVED_REFS)
			ref = RESERVED_REFS;  /* skip reserved references */
			ref++;  /* create new reference */
			::luabind::detail::luaL_setn(L, t, ref);
		}
		lua_rawseti(L, t, ref);
		return ref;

		// from an older lua version
#if 0
		lua_rawgeti(L, LUA_REGISTRYINDEX, 0);  /* get first free element */
		ref = (int)lua_tonumber(L, -1);  /* ref = t[0] */
		lua_pop(L, 1);  /* remove it from stack */
		if (ref != 0)
		{  /* any free element? */
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);  /* remove it from list */
			lua_rawseti(L, LUA_REGISTRYINDEX, 0);  /* (that is, t[0] = t[ref]) */
		}
		else
		{  /* no free elements */
			lua_pushliteral(L, "n");
			lua_pushvalue(L, -1);
			lua_rawget(L, LUA_REGISTRYINDEX);  /* get t.n */
			ref = (int)lua_tonumber(L, -1) + 1;  /* ref = t.n + 1 */
			lua_pop(L, 1);  /* pop t.n */
			lua_pushnumber(L, ref);
			lua_rawset(L, LUA_REGISTRYINDEX);  /* t.n = t.n + 1 */
		}
		lua_rawseti(L, LUA_REGISTRYINDEX, ref);
		return ref;
#endif
	}

	inline void unref(lua_State *L, int ref)
	{
		int t = LUA_REGISTRYINDEX;
		if (ref >= 0) {
			lua_rawgeti(L, t, FREELIST_REF);
			lua_rawseti(L, t, ref);  /* t[ref] = t[FREELIST_REF] */
			lua_pushnumber(L, ref);
			lua_rawseti(L, t, FREELIST_REF);  /* t[FREELIST_REF] = ref */
		}
	}

	inline void getref(lua_State* L, int r)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, r);
	}

}}

#endif // LUABIND_REF_HPP_INCLUDED

