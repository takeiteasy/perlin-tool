//
//  lua.c
//  sokol
//
//  Created by George Watson on 23/02/2023.
//

#define LUA_IMPL
#include "lua.h"

void LuaDumpTable(lua_State* L, int idx) {
    printf("--------------- LUA TABLE DUMP ---------------\n");
    lua_pushvalue(L, idx);
    lua_pushnil(L);
    int t, j = (idx < 0 ? -idx : idx), i = idx - 1;
    const char *key = NULL, *tmp = NULL;
    while ((t = lua_next(L, i))) {
        lua_pushvalue(L, idx - 1);
        key = lua_tostring(L, idx);
        switch (lua_type(L, idx - 1)) {
            case LUA_TSTRING:
                printf("%s (string, %d) => `%s'\n", key, j, lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:
                printf("%s (boolean, %d) => %s\n", key, j, lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("%s (integer, %d) => %g\n", key, j, lua_tonumber(L, i));
                break;
            default:
                tmp = lua_typename(L, i);
                printf("%s (%s, %d) => %s\n", key, tmp, j, tmp);
                if (!strncmp(lua_typename(L, t), "table", 5))
                    LuaDumpTable(L, i);
                break;
        }
        lua_pop(L, 2);
    }
    lua_pop(L, 1);
    printf("--------------- END TABLE DUMP ---------------\n");
}

int LuaDumpStack(lua_State* L) {
    int t, i = lua_gettop(L);
    const char* tmp = NULL;
    printf("--------------- LUA STACK DUMP ---------------\n");
    for (; i; --i) {
        
        switch ((t = lua_type(L, i))) {
            case LUA_TSTRING:
                printf("%d (string): `%s'\n", i, lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:
                printf("%d (boolean): %s\n", i, lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("%d (integer): %g\n",  i, lua_tonumber(L, i));
                break;
            default:
                tmp = lua_typename(L, t);
                printf("%d (%s): %s\n", i, lua_typename(L, t), tmp);
                break;
        }
    }
    printf("--------------- END STACK DUMP ---------------\n");
    return 0;
}

void LuaFail(lua_State *L, char *msg, bool die) {
    LuaDumpStack(L);
    fprintf(stderr, "\nERROR:\n  %s: %s\n\n", msg, lua_tostring(L, -1));
    if (die)
        exit(1);
}

typedef struct {
    Bitmap *bitmap;
} LuaBitmap;

static int LuaRGB(lua_State *L) {
    int r = (int)luaL_checkinteger(L, 1);
    int g = (int)luaL_checkinteger(L, 2);
    int b = (int)luaL_checkinteger(L, 3);
    int c = RGB(r, g, b);
    lua_pushinteger(L, c);
    return 1;
}

static int LuaBitmapPSet(lua_State *L) {
    LuaBitmap *lbitmap = (LuaBitmap*)luaL_checkudata(L, 1, "Bitmap");
    unsigned int x = (unsigned int)luaL_checkinteger(L, 2);
    unsigned int y = (unsigned int)luaL_checkinteger(L, 3);
    int color = (int)luaL_checkinteger(L, 4);
    lbitmap->bitmap->buf[y * lbitmap->bitmap->w + x] = color;
    return 0;
}

static int LuaBitmapPGet(lua_State *L) {
    LuaBitmap *lbitmap = (LuaBitmap*)luaL_checkudata(L, 1, "Bitmap");
    unsigned int x = (unsigned int)luaL_checkinteger(L, 2);
    unsigned int y = (unsigned int)luaL_checkinteger(L, 3);
    int color = lbitmap->bitmap->buf[y * lbitmap->bitmap->w + x];
    lua_pushinteger(L, color);
    return 1;
}

static int LuaBitmapGet(lua_State *L) {
    LuaBitmap *lbitmap = (LuaBitmap*)luaL_checkudata(L, 1, "Bitmap");
    unsigned int x = (unsigned int)luaL_checkinteger(L, 2);
    unsigned int y = (unsigned int)luaL_checkinteger(L, 3);
    unsigned char color = lbitmap->bitmap->buf[y * lbitmap->bitmap->w + x] & 0xFF;
    lua_pushinteger(L, color);
    return 1;
}

static int LuaBitmapWidth(lua_State *L) {
    LuaBitmap *lbitmap = (LuaBitmap*)luaL_checkudata(L, 1, "Bitmap");
    lua_pushinteger(L, lbitmap->bitmap->w);
    return 1;
}

static int LuaBitmapHeight(lua_State *L) {
    LuaBitmap *lbitmap = (LuaBitmap*)luaL_checkudata(L, 1, "Bitmap");
    lua_pushinteger(L, lbitmap->bitmap->h);
    return 1;
}

static const struct luaL_Reg BitmapMethods[] = {
    {"pset", LuaBitmapPSet},
    {"pget", LuaBitmapPGet},
    {"get", LuaBitmapGet},
    {"width", LuaBitmapWidth},
    {"height", LuaBitmapHeight},
    {NULL, NULL}
};

static const struct luaL_Reg BitmapFunctions[] = {
    {NULL, NULL}
};

lua_State* LoadLuaScript(const char *filename) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    
    luaL_newmetatable(L, "Bitmap");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, BitmapMethods, 0);
    luaL_newlib(L, BitmapFunctions);
    
    lua_pushcfunction(L, LuaRGB);
    lua_setglobal(L, "RGB");
    lua_pushcfunction(L, LuaSettings);
    lua_setglobal(L, "Setting");
    lua_pushcfunction(L, LuaDelta);
    lua_setglobal(L, "Delta");
    lua_pushcfunction(L, LuaDumpStack);
    lua_setglobal(L, "DumpStack");
    
    char asset[1024];
    sprintf(asset, "assets%s%s", PATH_SEPERATOR, filename);
    if (luaL_dofile(L, asset))
        LuaFail(L, "Errors found in lua script", false);
    return L;
}

void LuaCallPreframe(lua_State *L) {
    lua_getglobal(L, "preframe");
    if (lua_isfunction(L, -1) && lua_pcall(L, 0, 0, 0))
        LuaFail(L, "Failed to execute Lua script", false);
}

void LuaCallFrame(lua_State *L, unsigned char *heightmap, int w, int h) {
    lua_getglobal(L, "callback");
    if (!lua_isfunction(L, -1))
        return;
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            lua_getglobal(L, "callback");
            lua_pushnumber(L, heightmap[y * w + x]);
            lua_pushinteger(L, x);
            lua_pushinteger(L, y);
            lua_pushinteger(L, w);
            lua_pushinteger(L, h);
            if (lua_pcall(L, 5, 1, 0))
                LuaFail(L, "Failed to execute Lua script", false);
            if (!lua_isnumber(L, -1))
                LuaFail(L, "Invalid return value from Lua callback", false);
            heightmap[y * w + x] = CLAMP(lua_tonumber(L, -1), 0, 255);
            lua_pop(L, 1);
        }
}

void LuaCallPostframe(lua_State *L, Bitmap *bitmap) {
    lua_getglobal(L, "postframe");
    if (!lua_isfunction(L, -1))
        return;
    LuaBitmap *lbitmap = (LuaBitmap*)lua_newuserdata(L, sizeof(LuaBitmap));
    lbitmap->bitmap = bitmap;
    luaL_getmetatable(L, "Bitmap");
    lua_setmetatable(L, -2);
    if (lua_pcall(L, 1, 0, 0))
        LuaFail(L, "Failed to execute Lua script", false);
}
