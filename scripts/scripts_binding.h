#ifndef LUA_SCRIPTS_BINDING_H
#define LUA_SCRIPTS_BINDING_H

#ifdef _WIN32
#pragma once
#endif

#include "lua/lua.hpp"
#include "scripts.h"

#include "lua_usercmd.h"
#include "lua_player_move.h"
#include "lua_entity_dictionary.h"
#include "lua_entity_vars.h"

#include <math/mathlib.h>
#include <steamtypes.h>
#include <color.h>
#include <platform.h>

#include <map>
#include <string>

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

#define DEFINE_SCRIPTFUNC( name ) static int ScriptFunc_##name( lua_State *pLuaState )
#define DEFINE_GETTER( metatable, type ) static int ScriptFunc_MetaMethod_index( lua_State *pLuaState ) \
{ \
	const char *pszKey = luaL_checkstring( pLuaState, 2 ); \
	return VLua::InvokePropertyGetter( pLuaState, metatable, pszKey, VLuaDeduceFieldType( type ) ); \
}
#define DEFINE_SETTER( metatable, type ) static int ScriptFunc_MetaMethod_newindex( lua_State *pLuaState ) \
{ \
	const char *pszKey = luaL_checkstring( pLuaState, 2 ); \
	return VLua::InvokePropertySetter( pLuaState, metatable, pszKey, VLuaDeduceFieldType( type ) ); \
}
#define SCRIPTFUNC( name ) ScriptFunc_##name
#define VLUA_RET_ARGS( count ) count

#define REG_BEGIN( name ) static const luaL_Reg name[] = {
#define REG_END() { 0, 0 } };
#define REG_SCRIPTFUNC( name, funcname ) { name, ScriptFunc_##funcname },
#define REG_GETTER( ) { "__index", ScriptFunc_MetaMethod_index },
#define REG_SETTER( ) { "__newindex", ScriptFunc_MetaMethod_newindex },

#define VLuaTypeOf(s, m) VLuaDeduceValueFieldType(((s*)0)->m)
#define VLuaOffsetOf(s, m) ((int)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#define VLuaSizeOf(s, m) (int)sizeof(s::m)
#define VLuaPropertyDesc(s, m) { VLuaTypeOf(s, m), VLuaOffsetOf(s, m), VLuaSizeOf(s, m), false }
#define VLuaPropertyDescFieldtype(s, m, fieldtype) { fieldtype, VLuaOffsetOf(s, m), VLuaSizeOf(s, m), false }

#define VLUA_DECLARE_DEDUCE_FIELDTYPE( fieldType, type ) template<> struct VLuaFieldTypeDeducer<type> { enum { VLUA_FIELD_TYPE = fieldType }; };
#define VLuaDeduceFieldType(T) (int)VLuaFieldTypeDeducer<T>::VLUA_FIELD_TYPE

//-----------------------------------------------------------------------------
// Templates
//-----------------------------------------------------------------------------

template <typename T> struct VLuaFieldTypeDeducer { };
template <typename T> FORCEINLINE constexpr int VLuaDeduceValueFieldType( T value ) { return VLuaFieldTypeDeducer<T>::VLUA_FIELD_TYPE; }

//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------

typedef lua_CFunction VLuaCFunction;
typedef luaL_Reg VLuaReg;

typedef enum vlua_fieldtypes_t
{
	VLUA_FIELD_TYPE_VOID = 0,		// No type or value

	VLUA_FIELD_TYPE_FLOAT,			// Single precision floating-point
	VLUA_FIELD_TYPE_DOUBLE,			// Double precision floating-point

	VLUA_FIELD_TYPE_CSTRING,		// A zero terminated string

	VLUA_FIELD_TYPE_INTEGER,		// Any integer or enum
	VLUA_FIELD_TYPE_SHORT,			// 2 byte integer
	VLUA_FIELD_TYPE_INTEGER64,		// 64bit integer

	VLUA_FIELD_TYPE_BOOLEAN,		// boolean
	VLUA_FIELD_TYPE_BYTE,			// a byte
	VLUA_FIELD_TYPE_CHARACTER,		// a char

	VLUA_FIELD_TYPE_UINT,			// unsigned integer
	VLUA_FIELD_TYPE_UINT16,			// unsigned short integer
	VLUA_FIELD_TYPE_UINT64,			// unsigned 64bit integer

	VLUA_FIELD_TYPE_COLOR32,		// 8-bit per channel r,g,b,a (32bit color)

	VLUA_FIELD_TYPE_VECTOR,			// Vector
	VLUA_FIELD_TYPE_VECTOR_POINTER,	// Vector
	VLUA_FIELD_TYPE_VECTOR2D,		// Vector2D
	VLUA_FIELD_TYPE_QANGLE,			// QAngle
	VLUA_FIELD_TYPE_QUATERNION,		// Quaternion

	VLUA_FIELD_TYPE_CFUNCTION,		// C function
	VLUA_FIELD_TYPE_SCRIPTREF,		// Script reference

	// User data
	VLUA_FIELD_TYPE_STRING,			// string_t
	VLUA_FIELD_TYPE_USERCMD,		// usercmd_t
	VLUA_FIELD_TYPE_PLAYERMOVE,		// playermove_t
	VLUA_FIELD_TYPE_EDICT,			// edict_t
	VLUA_FIELD_TYPE_ENTVARS,		// entvars_t

	VLUA_FIELD_TYPE_TYPECOUNT,		// MUST BE LAST
	VLUA_FIELD_TYPE_TYPEUNKNOWN = VLUA_FIELD_TYPE_TYPECOUNT
} vlua_fieldtype_t;

//-----------------------------------------------------------------------------
// Declare script fieldtypes
//-----------------------------------------------------------------------------

VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_VOID, void );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_FLOAT, float );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_DOUBLE, double );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_CSTRING, const char * );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_CSTRING, char * );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_INTEGER, int );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_SHORT, short );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_INTEGER64, int64 );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_BOOLEAN, bool );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_BYTE, unsigned char );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_CHARACTER, char );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_UINT, uint32 );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_UINT16, uint16 );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_UINT64, uint64 );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_COLOR32, Color );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_COLOR32, const Color & );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_VECTOR, Vector );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_VECTOR, const Vector & );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_VECTOR_POINTER, Vector * );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_VECTOR2D, Vector2D );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_VECTOR2D, const Vector2D & );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_QANGLE, QAngle );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_QANGLE, const QAngle & );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_QUATERNION, Quaternion );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_QUATERNION, const Quaternion & );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_CFUNCTION, VLuaCFunction );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_SCRIPTREF, scriptref_t );
// User data
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_USERCMD, usercmd_t * );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_PLAYERMOVE, playermove_t * );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_EDICT, edict_t * );
VLUA_DECLARE_DEDUCE_FIELDTYPE( VLUA_FIELD_TYPE_ENTVARS, entvars_t * );

//-----------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------

struct VLuaPropertyDescription
{
	int fieldtype;
	int offset;
	int size;
	bool acquire_pointer;
};

struct VLuaScriptVariant
{
	inline VLuaScriptVariant() {}

	union
	{
		float m_float;
		double m_double;
		const char *m_cstring;
		int m_int;
		short m_short;
		int64 m_int64;
		bool m_bool;
		unsigned char m_uchar;
		char m_char;
		uint32 m_uint32;
		uint16 m_uint16;
		uint64 m_uint64;

		Color *m_color;

		Vector *m_pVector;
		Vector m_vector;
		Vector2D *m_pVector2D;
		Vector2D m_vector2D;
		QAngle *m_pQangle;
		QAngle m_Qangle;
		Quaternion *m_pQuaternion;
		Quaternion m_quaternion;

		VLuaCFunction m_cfunction;
		scriptref_t m_scriptref;

		usercmd_t *m_usercmd;
		playermove_t *m_playermove;
		edict_t *m_edict;
		entvars_t *m_entvars;
	};
};

//-----------------------------------------------------------------------------
// VLua Scripts Binding Namespace
//-----------------------------------------------------------------------------

namespace VLua
{
	//-----------------------------------------------------------------------------
	// Initialize / shutdown module
	//-----------------------------------------------------------------------------

	void Init( lua_State *state );
	void Shutdown( void );

	//-----------------------------------------------------------------------------
	// Registers a variable in the global space
	//-----------------------------------------------------------------------------

	void RegisterGlobalVariable( const char *pszVariableName, void *pVariable, int fieldtype );

	template <class T> FORCEINLINE void RegisterGlobalVariable( const char *pszVariableName, T variable )
	{
		RegisterGlobalVariable( pszVariableName, &variable, VLuaDeduceFieldType( T ) );
	}

	//-----------------------------------------------------------------------------
	// Registers a C function in the global space
	// Same as RegisterGlobalVariable( "MyFunctionName", MyFunction, VLuaDeduceFieldType( MyFunction ) );
	//-----------------------------------------------------------------------------

	void RegisterFunction( const char *pszFunctionName, VLuaCFunction pfnFunction );
	
	//-----------------------------------------------------------------------------
	// Registers a table
	//-----------------------------------------------------------------------------

	void RegisterTable( const char *pszTable );
	
	//-----------------------------------------------------------------------------
	// Registers a metatable
	//-----------------------------------------------------------------------------

	void RegisterMetaTable( const char *pszMetatable );

	//-----------------------------------------------------------------------------
	// Registers a table functions
	//-----------------------------------------------------------------------------

	void RegisterTableFunctions( const char *pszTable, const VLuaReg funcs[] );
	
	//-----------------------------------------------------------------------------
	// Registers a metatable functions
	//-----------------------------------------------------------------------------

	void RegisterMetaTableFunctions( const char *pszMetatable, const VLuaReg funcs[] );
	
	//-----------------------------------------------------------------------------
	// 
	//-----------------------------------------------------------------------------

	void SetupPropertiesInitialization( void );
	
	//-----------------------------------------------------------------------------
	// To don't lose perfomance, VLua doesn't remove binded properties after VM shutdown
	//-----------------------------------------------------------------------------

	bool ArePropertiesInitialized( void );
	
	//-----------------------------------------------------------------------------
	// Binds a property to given metatable
	//-----------------------------------------------------------------------------

	void BindProperty( const char *pszMetatable, const char *pszPropertyName, VLuaPropertyDescription propertyDesc, bool getter = true, bool setter = true );

	//-----------------------------------------------------------------------------
	// Invoke property getter from metatable
	//-----------------------------------------------------------------------------
	
	int InvokePropertyGetter( lua_State *pLuaState, const char *pszMetatable, const char *pszKey, int fieldtype );
	
	//-----------------------------------------------------------------------------
	// Invoke property setter from metatable
	//-----------------------------------------------------------------------------
	
	int InvokePropertySetter( lua_State *pLuaState, const char *pszMetatable, const char *pszKey, int fieldtype );

	//-----------------------------------------------------------------------------
	// Push a script type onto stack
	//-----------------------------------------------------------------------------

	void PushScriptVariant( void *pVariable, int fieldtype, bool pointer = false );

	//-----------------------------------------------------------------------------
	// Push a script type onto stack
	//-----------------------------------------------------------------------------

	void ConvertToScriptVariant( void *pVariable, int fieldtype, int idx );
}

#endif // LUA_SCRIPTS_BINDING_H