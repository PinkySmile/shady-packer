#pragma once
#include <lua.hpp>
#include <filesystem>
#include "lualibs/soku.hpp"
#include "../Core/package.hpp"

namespace ShadyLua {
    void LualibBase(lua_State* L);
    void LualibMemory(lua_State* L);
    void LualibResource(lua_State* L);
    void LualibSoku(lua_State* L);
    void LualibLoader(lua_State* L, ShadyCore::PackageEx* package);
    void LualibGui(lua_State* L);
    void LualibBattle(lua_State* L);
}