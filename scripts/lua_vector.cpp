#include <stdio.h>

#include "lua_vector.h"

#define VECTOR_TYPE "Vector"
#define VECTOR_NAME "Vector"

bool lua_isvector(lua_State *pLuaState, int i)
{
	if ( luaL_checkudata(pLuaState, i, VECTOR_TYPE) == NULL )
	{
		return false;
	}

	return true;
}

Vector *lua_getvector(lua_State *pLuaState, int i)
{
	if ( luaL_checkudata(pLuaState, i, VECTOR_TYPE) == NULL )
	{
		luaL_typeerror(pLuaState, i, VECTOR_TYPE);
	}

	return (Vector *)lua_touserdata(pLuaState, i);
}

Vector lua_getvectorByValue(lua_State *pLuaState, int i)
{
	if ( lua_isnumber(pLuaState, i) )
	{
		float flValue = (vec_t)lua_tonumber(pLuaState, i);

		return Vector(flValue, flValue, flValue);
	}

	if ( luaL_checkudata(pLuaState, i, VECTOR_TYPE) == NULL )
	{
		luaL_typeerror(pLuaState, i, VECTOR_TYPE);
	}

	return *(Vector *)lua_touserdata(pLuaState, i);
}

static Vector *lua_allocvector(lua_State *pLuaState)
{
	Vector *v = (Vector *)lua_newuserdata(pLuaState, sizeof(Vector));

	luaL_getmetatable(pLuaState, VECTOR_TYPE);
	lua_setmetatable(pLuaState, -2);

	return v;
}

Vector *lua_newvector(lua_State *pLuaState, const Vector *Value)
{
	Vector *v;

	v = lua_allocvector(pLuaState);
	v->x = Value->x;
	v->y = Value->y;
	v->z = Value->z;

	return v;
}

static int Vector_new(lua_State *pLuaState)
{
	Vector *v;

	lua_settop(pLuaState, 3);
	v = lua_allocvector(pLuaState);

	v->x = (vec_t)luaL_optnumber(pLuaState, 1, 0);
	v->y = (vec_t)luaL_optnumber(pLuaState, 2, 0);
	v->z = (vec_t)luaL_optnumber(pLuaState, 3, 0);

	return 1;
}

static int Vector_index(lua_State *pLuaState)
{
	const char *pszKey = luaL_checkstring(pLuaState, 2);

	if (pszKey[1] == '\0')
	{
		Vector *v = lua_getvector(pLuaState, 1);

		switch (pszKey[0])
		{
		case 'x':
		case 'r':
		case '1':
			lua_pushnumber(pLuaState, v->x);
			return 1;

		case 'y':
		case 'g':
		case '2':
			lua_pushnumber(pLuaState, v->y);
			return 1;

		case 'z':
		case 'b':
		case '3':
			lua_pushnumber(pLuaState, v->z);
			return 1;
		}
	}

	lua_getfield(pLuaState, LUA_REGISTRYINDEX, VECTOR_TYPE);
	lua_pushstring(pLuaState, pszKey);
	lua_rawget(pLuaState, -2);

	return 1;
}

static int Vector_newindex(lua_State *pLuaState)
{
	const char *pszKey = luaL_checkstring(pLuaState, 2);

	if (pszKey[1] == '\0')
	{
		Vector *v = lua_getvector(pLuaState, 1);
		float flValue = (vec_t)luaL_checknumber(pLuaState, 3);
		
		switch (pszKey[0])
		{
		case 'x':
		case 'r':
		case '0':
			v->x = flValue;
			break;

		case 'y':
		case 'g':
		case '1':
			v->y = flValue;
			break;

		case 'z':
		case 'b':
		case '2':
			v->z = flValue;
			break;

		default:
			break;
		}
	}

	return 1;
}

static int Vector_tostring(lua_State *pLuaState)
{
	char s[64];
	Vector *v = lua_getvector(pLuaState, 1);

	snprintf(s, 64, "(%s : (%.6f, %.6f, %.6f))", VECTOR_TYPE, VectorExpand(*v));
	
	lua_pushstring(pLuaState, s);

	return 1;
}

static int Vector_Add(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	Vector vResult = v1 + v2;

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Subtract(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	Vector vResult = v1 - v2;

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Multiply(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	Vector vResult = v1 * v2;

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Divide(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	Vector vResult = v1 / v2;

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Equal(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	return (v1 == v2 ? 1 : 0);
}

static int Vector_CopyTo(lua_State *pLuaState)
{
	Vector *v1 = lua_getvector(pLuaState, 1);
	Vector *v2 = lua_getvector(pLuaState, 2);

	v1->CopyToArray( (float *)v2 );

	return 0;
}

static int Vector_Zero(lua_State *pLuaState)
{
	Vector *v = lua_getvector(pLuaState, 1);

	v->Zero();

	return 0;
}

static int Vector_Negate(lua_State *pLuaState)
{
	Vector *v = lua_getvector(pLuaState, 1);

	v->Negate();

	return 0;
}

static int Vector_Scale(lua_State *pLuaState)
{
	Vector *v = lua_getvector(pLuaState, 1);

	float flValue = (vec_t)luaL_checknumber(pLuaState, 2);

	v->Scale(flValue);

	return 0;
}

static int Vector_Lerp(lua_State *pLuaState)
{
	Vector *v1 = lua_getvector(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	float dt = (vec_t)luaL_checknumber(pLuaState, 3);

	v1->Lerp(v2, dt);

	return 0;
}

static int Vector_NormalizeInPlace(lua_State *pLuaState)
{
	Vector *v1 = lua_getvector(pLuaState, 1);

	float flResult = v1->NormalizeInPlace();

	lua_pushnumber(pLuaState, flResult);

	return 1;
}

static int Vector_Normalize(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);

	Vector vResult = v1.Normalize();

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Project(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	Vector vResult = v1.Project(v2);

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Reject(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	Vector vResult = v1.Reject(v2);

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Reflect(lua_State *pLuaState)
{
	float factor;

	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	if ( !lua_isnumber(pLuaState, 3) )
	{
		factor = 2.f;
	}
	else
	{
		factor = (vec_t)lua_tonumber(pLuaState, 3);
	}

	Vector vResult = v1.Reflect(v2, factor);

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Dot(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	float flResult = v1.Dot(v2);

	lua_pushnumber(pLuaState, flResult);

	return 1;
}

static int Vector_Cross(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	Vector vResult = v1.Cross(v2);

	lua_newvector(pLuaState, &vResult);

	return 1;
}

static int Vector_Length(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);

	float flResult = v1.Length();

	lua_pushnumber(pLuaState, flResult);

	return 1;
}

static int Vector_LengthSqr(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);

	float flResult = v1.LengthSqr();

	lua_pushnumber(pLuaState, flResult);

	return 1;
}

static int Vector_Length2D(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);

	float flResult = v1.Length2D();

	lua_pushnumber(pLuaState, flResult);

	return 1;
}

static int Vector_Length2DSqr(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);

	float flResult = v1.Length2DSqr();

	lua_pushnumber(pLuaState, flResult);

	return 1;
}

static int Vector_DistTo(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	float flResult = v1.DistTo(v2);

	lua_pushnumber(pLuaState, flResult);

	return 1;
}

static int Vector_DistToSqr(lua_State *pLuaState)
{
	Vector v1 = lua_getvectorByValue(pLuaState, 1);
	Vector v2 = lua_getvectorByValue(pLuaState, 2);

	float flResult = v1.DistToSqr(v2);

	lua_pushnumber(pLuaState, flResult);

	return 1;
}

static const luaL_Reg Registrations[] =
{
	{ "__index",	Vector_index },
	{ "__newindex",	Vector_newindex	},
	{ "__tostring",	Vector_tostring	},
	{ "__add",		Vector_Add },
	{ "__sub",		Vector_Subtract	},
	{ "__mul",		Vector_Multiply	},
	{ "__div",		Vector_Divide },
	{ "__len",		Vector_Length },
	{ "__eq",		Vector_Equal },
	{ "CopyTo",		Vector_CopyTo },
	{ "Zero",		Vector_Zero },
	{ "Negate",		Vector_Negate },
	{ "Scale",		Vector_Scale },
	{ "Lerp",		Vector_Lerp },
	{ "NormalizeInPlace", Vector_NormalizeInPlace },
	{ "Normalize",	Vector_Normalize },
	{ "Project",	Vector_Project },
	{ "Reject",		Vector_Reject },
	{ "Reflect",	Vector_Reflect },
	{ "Dot",		Vector_Dot },
	{ "Cross",		Vector_Cross },
	{ "Length",		Vector_Length },
	{ "LengthSqr",	Vector_LengthSqr },
	{ "Length2D",	Vector_Length2D },
	{ "Length2DSqr", Vector_Length2DSqr },
	{ "DistTo",		Vector_DistTo },
	{ "DistToSqr",	Vector_DistToSqr },
	{ NULL,			NULL }
};

LUALIB_API int luaopen_vector(lua_State *pLuaState)
{
	luaL_newmetatable(pLuaState, VECTOR_TYPE);
	luaL_setfuncs(pLuaState, Registrations, NULL);

	lua_register(pLuaState, VECTOR_NAME, Vector_new);

	return 1;
}