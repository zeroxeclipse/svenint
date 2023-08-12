#include "scripts_binding.h"
#include "lua_vector.h"

#include "../modules/server.h"

#include <dbg.h>

namespace VLua
{
	bool bInitialized = false;
	bool bPropsInitialized = false;
	lua_State *pLuaState = NULL;

	struct vlua_metatable_t
	{
		std::map<std::string, VLuaPropertyDescription> getters;
		std::map<std::string, VLuaPropertyDescription> setters;
	};

	std::map<std::string, vlua_metatable_t> MetaTables;

	//-----------------------------------------------------------------------------
	// Initialize / shutdown module
	//-----------------------------------------------------------------------------

	void Init( lua_State *state )
	{
		pLuaState = state;
		bInitialized = true;
	}

	void Shutdown( void )
	{
		if ( bInitialized ) // da
			return;

		pLuaState = NULL;
		MetaTables.clear();
		bInitialized = false;
		bPropsInitialized = false;
	}

	//-----------------------------------------------------------------------------
	// Get a meta table
	//-----------------------------------------------------------------------------

	static vlua_metatable_t *CreateMetaTable( const char *pszMetatable )
	{
		auto it = MetaTables.find( pszMetatable );

		if ( it == MetaTables.end() )
		{
			MetaTables[ pszMetatable ] = vlua_metatable_t();
			return &MetaTables.at( pszMetatable );
		}

		return NULL;
	}

	//-----------------------------------------------------------------------------
	// Get a meta table
	//-----------------------------------------------------------------------------

	static vlua_metatable_t *GetMetaTable( const char *pszMetatable )
	{
		auto it = MetaTables.find( pszMetatable );

		if ( it != MetaTables.end() )
		{
			return &MetaTables.at( pszMetatable );
		}

		return NULL;
	}
	
	//-----------------------------------------------------------------------------
	// Register a global variable
	//-----------------------------------------------------------------------------

	void RegisterGlobalVariable( const char *pszVariableName, void *pVariable, int fieldtype )
	{
		PushScriptVariant( pVariable, fieldtype );
		lua_setglobal( pLuaState, pszVariableName );
	}

	//-----------------------------------------------------------------------------
	// Register a C function
	//-----------------------------------------------------------------------------

	void RegisterFunction( const char *pszFunctionName, VLuaCFunction pfnFunction )
	{
		lua_pushcfunction( pLuaState, pfnFunction );
		lua_setglobal( pLuaState, pszFunctionName );
	}
	
	//-----------------------------------------------------------------------------
	// Registers a table
	//-----------------------------------------------------------------------------

	void RegisterTable( const char *pszTable )
	{
		lua_newtable( pLuaState );
		lua_setglobal( pLuaState, pszTable );
	}

	//-----------------------------------------------------------------------------
	// Register a metatable
	//-----------------------------------------------------------------------------

	void RegisterMetaTable( const char *pszMetatable )
	{
		luaL_newmetatable( pLuaState, pszMetatable );
		lua_pop( pLuaState, 1 );

		CreateMetaTable( pszMetatable );
	}

	//-----------------------------------------------------------------------------
	// Register table functions
	//-----------------------------------------------------------------------------

	void RegisterTableFunctions( const char *pszTable, const VLuaReg funcs[] )
	{
		lua_getglobal( pLuaState, pszTable );
		luaL_setfuncs( pLuaState, funcs, NULL );
	}
	
	//-----------------------------------------------------------------------------
	// Register metatable functions
	//-----------------------------------------------------------------------------

	void RegisterMetaTableFunctions( const char *pszMetatable, const VLuaReg funcs[] )
	{
		luaL_getmetatable( pLuaState, pszMetatable );
		luaL_setfuncs( pLuaState, funcs, NULL );
	}

	//-----------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------
	
	void SetupPropertiesInitialization( void )
	{
		bPropsInitialized = true;
	}

	//-----------------------------------------------------------------------------
	// To don't lose perfomance, VLua doesn't remove binded properties after VM shutdown
	//-----------------------------------------------------------------------------

	bool ArePropertiesInitialized( void )
	{
		return bPropsInitialized;
	}
	
	//-----------------------------------------------------------------------------
	// Bind a property to given metatable
	//-----------------------------------------------------------------------------

	void BindProperty( const char *pszMetatable, const char *pszPropertyName, VLuaPropertyDescription propertyDesc, bool getter /* = true */, bool setter /* = true */ )
	{
		vlua_metatable_t *metatable = GetMetaTable( pszMetatable );

		if ( metatable == NULL )
			return;

		if ( getter )
		{
			metatable->getters[ pszPropertyName ] = propertyDesc;
		}

		if ( setter )
		{
			metatable->setters[ pszPropertyName ] = propertyDesc;
		}
	}

	//-----------------------------------------------------------------------------
	// Invoke property getter from metatable
	//-----------------------------------------------------------------------------

	int InvokePropertyGetter( lua_State *pLuaState, const char *pszMetatable, const char *pszKey, int fieldtype )
	{
		vlua_metatable_t *metatable = GetMetaTable( pszMetatable );

		if ( metatable == NULL )
			return 0;

		auto it = metatable->getters.find( pszKey );

		if ( it == metatable->getters.end() )
			return 0;

		char *prop;
		VLuaScriptVariant instance;
		VLuaPropertyDescription &propDesc = metatable->getters[ pszKey ];

		ConvertToScriptVariant( &instance, fieldtype, 1 );

		prop = (char *)*(int *)&instance + propDesc.offset;

		PushScriptVariant( prop, propDesc.fieldtype, propDesc.acquire_pointer );

		return 1;
	}
	
	//-----------------------------------------------------------------------------
	// Invoke property setter from metatable
	//-----------------------------------------------------------------------------

	int InvokePropertySetter( lua_State *pLuaState, const char *pszMetatable, const char *pszKey, int fieldtype )
	{
		vlua_metatable_t *metatable = GetMetaTable( pszMetatable );

		if ( metatable == NULL )
			return 0;

		auto it = metatable->setters.find( pszKey );

		if ( it == metatable->setters.end() )
			return 0;

		char *prop;
		VLuaScriptVariant instance, propValue;
		VLuaPropertyDescription &propDesc = metatable->setters[ pszKey ];

		ConvertToScriptVariant( &instance, fieldtype, 1 );
		ConvertToScriptVariant( &propValue, propDesc.fieldtype, 3 );

		prop = (char *)*(int *)&instance + propDesc.offset;
		memcpy( (void *)prop, (void *)&propValue, propDesc.size );

		PushScriptVariant( prop, propDesc.fieldtype );

		return 1;
	}

	//-----------------------------------------------------------------------------
	// Push a script type onto stack
	//-----------------------------------------------------------------------------

	void PushScriptVariant( void *pVariable, int fieldtype, bool pointer )
	{
		switch ( fieldtype )
		{
		case VLUA_FIELD_TYPE_VOID:
			lua_pushnil( pLuaState );
			break;
		case VLUA_FIELD_TYPE_FLOAT:
			lua_pushnumber( pLuaState, (lua_Number)( *(float *)pVariable ) );
			break;
		case VLUA_FIELD_TYPE_DOUBLE:
			lua_pushnumber( pLuaState, (lua_Number)( *(double *)pVariable ) );
			break;
		case VLUA_FIELD_TYPE_CSTRING:
			lua_pushstring( pLuaState, *(const char **)pVariable );
			break;
		case VLUA_FIELD_TYPE_INTEGER:
			lua_pushinteger( pLuaState, ( lua_Integer ) * (int *)pVariable );
			break;
		case VLUA_FIELD_TYPE_SHORT:
			lua_pushinteger( pLuaState, ( lua_Integer ) * (short *)pVariable );
			break;
		case VLUA_FIELD_TYPE_INTEGER64:
			lua_pushinteger( pLuaState, ( lua_Integer ) * (int64 *)pVariable );
			break;
		case VLUA_FIELD_TYPE_BOOLEAN:
			lua_pushboolean( pLuaState, (int)*(bool *)pVariable );
			break;
		case VLUA_FIELD_TYPE_BYTE:
			lua_pushinteger( pLuaState, ( lua_Integer ) * (unsigned char *)pVariable );
			break;
		case VLUA_FIELD_TYPE_CHARACTER:
			lua_pushinteger( pLuaState, ( lua_Integer ) * (char *)pVariable );
			break;
		case VLUA_FIELD_TYPE_UINT:
			lua_pushinteger( pLuaState, ( lua_Integer ) * (uint32 *)pVariable );
			break;
		case VLUA_FIELD_TYPE_UINT16:
			lua_pushinteger( pLuaState, ( lua_Integer ) * (uint16 *)pVariable );
			break;
		case VLUA_FIELD_TYPE_UINT64:
			lua_pushinteger( pLuaState, ( lua_Integer ) * (uint64 *)pVariable );
			break;
		case VLUA_FIELD_TYPE_COLOR32:
			Sys_Error( "Implement VLUA_FIELD_TYPE_COLOR32" );
			break;
		case VLUA_FIELD_TYPE_VECTOR:
			lua_newvector( pLuaState, (Vector *)pVariable );
			break;
		case VLUA_FIELD_TYPE_VECTOR_POINTER:
			lua_newvector( pLuaState, (Vector *)pVariable );
			break;
		case VLUA_FIELD_TYPE_VECTOR2D:
			Sys_Error( "Implement VLUA_FIELD_TYPE_VECTOR2D" );
			break;
		case VLUA_FIELD_TYPE_QANGLE:
			Sys_Error( "Implement VLUA_FIELD_TYPE_QANGLE" );
			break;
		case VLUA_FIELD_TYPE_QUATERNION:
			Sys_Error( "Implement VLUA_FIELD_TYPE_QUATERNION" );
			break;
		case VLUA_FIELD_TYPE_CFUNCTION:
			lua_pushcfunction( pLuaState, *(VLuaCFunction *)pVariable );
			break;
		case VLUA_FIELD_TYPE_SCRIPTREF:
			lua_rawgeti( pLuaState, LUA_REGISTRYINDEX, (int)( *(scriptref_t *)pVariable ) );
			break;
		case VLUA_FIELD_TYPE_STRING:
			lua_pushstring( pLuaState, gpGlobals->pStringBase + *(string_t *)pVariable );
			break;
		case VLUA_FIELD_TYPE_USERCMD:
			lua_pushusercmd( pLuaState, *(usercmd_t **)pVariable );
			break;
		case VLUA_FIELD_TYPE_PLAYERMOVE:
			lua_pushplayermove( pLuaState, *(playermove_t **)pVariable );
			break;
		case VLUA_FIELD_TYPE_EDICT:
			lua_pushedict( pLuaState, *(edict_t **)pVariable );
			break;
		case VLUA_FIELD_TYPE_ENTVARS:
			lua_pushentvars( pLuaState, pointer ? (entvars_t *)pVariable : *(entvars_t **)pVariable );
			break;
		case VLUA_FIELD_TYPE_GLOBALVARS:
			lua_pushglobalvars( pLuaState, *(globalvars_t **)pVariable );
			break;
		default:
			Sys_Error( "Passing VLUA_FIELD_TYPE_TYPEUNKNOWN!" );
			break;
		}
	}

	//-----------------------------------------------------------------------------
	// Convert to a script variant
	//-----------------------------------------------------------------------------

	void ConvertToScriptVariant( void *pVariable, int fieldtype, int idx )
	{
		switch ( fieldtype )
		{
		case VLUA_FIELD_TYPE_VOID:
			break;
		case VLUA_FIELD_TYPE_FLOAT:
			*(float *)pVariable = (float)lua_tonumber( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_DOUBLE:
			*(double *)pVariable = (double)lua_tonumber( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_CSTRING:
			*(const char **)pVariable = lua_tostring( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_INTEGER:
			*(int *)pVariable = (int)lua_tointeger( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_SHORT:
			*(short *)pVariable = (short)lua_tointeger( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_INTEGER64:
			*(int64 *)pVariable = (int64)lua_tointeger( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_BOOLEAN:
			*(bool *)pVariable = !!lua_toboolean( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_BYTE:
			*(unsigned char *)pVariable = (unsigned char)lua_tointeger( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_CHARACTER:
			*(char *)pVariable = (char)lua_tointeger( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_UINT:
			*(uint32 *)pVariable = (uint32)lua_tointeger( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_UINT16:
			*(uint16 *)pVariable = (uint16)lua_tointeger( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_UINT64:
			*(uint64 *)pVariable = (uint64)lua_tointeger( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_COLOR32:
			Sys_Error( "Implement VLUA_FIELD_TYPE_COLOR32" );
			break;
		case VLUA_FIELD_TYPE_VECTOR:
			*(Vector *)pVariable = *lua_getvector( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_VECTOR_POINTER:
			*(Vector **)pVariable = lua_getvector( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_VECTOR2D:
			Sys_Error( "Implement VLUA_FIELD_TYPE_VECTOR2D" );
			break;
		case VLUA_FIELD_TYPE_QANGLE:
			Sys_Error( "Implement VLUA_FIELD_TYPE_QANGLE" );
			break;
		case VLUA_FIELD_TYPE_QUATERNION:
			Sys_Error( "Implement VLUA_FIELD_TYPE_QUATERNION" );
			break;
		case VLUA_FIELD_TYPE_CFUNCTION:
			*(VLuaCFunction *)pVariable = (VLuaCFunction)lua_tocfunction( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_SCRIPTREF:
			*(scriptref_t *)pVariable = (scriptref_t)luaL_ref( pLuaState, LUA_REGISTRYINDEX );
			break;
		case VLUA_FIELD_TYPE_STRING:
		{
			string_t iString = (string_t)(int)lua_tointeger( pLuaState, idx );
			if ( iString > 0 )
				*(string_t *)pVariable = (string_t)(int)lua_tointeger( pLuaState, idx );
			//Sys_Error( "Implement VLUA_FIELD_TYPE_STRING" );
			break;
		}
		case VLUA_FIELD_TYPE_USERCMD:
			*(usercmd_t **)pVariable = lua_getusercmd( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_PLAYERMOVE:
			*(playermove_t **)pVariable = lua_getplayermove( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_EDICT:
			*(edict_t **)pVariable = lua_getedict( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_ENTVARS:
			*(entvars_t **)pVariable = lua_getentvars( pLuaState, idx );
			break;
		case VLUA_FIELD_TYPE_GLOBALVARS:
			*(globalvars_t **)pVariable = lua_getglobalvars( pLuaState, idx );
			break;
		default:
			Sys_Error( "Passing VLUA_FIELD_TYPE_TYPEUNKNOWN!" );
			break;
		}
	}
}