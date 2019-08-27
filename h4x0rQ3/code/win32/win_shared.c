/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <intrin.h>

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds( void )
{
	static qboolean	initialized = qfalse;
	static DWORD sys_timeBase;
	int	sys_curtime;

	if ( !initialized ) {
		sys_timeBase = timeGetTime();
		initialized = qtrue;
	}

	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}


/*
================
Sys_RandomBytes
================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	HCRYPTPROV  prov;

	if( !CryptAcquireContext( &prov, NULL, NULL,
		PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )  {

		return qfalse;
	}

	if( !CryptGenRandom( prov, len, (BYTE *)string ) )  {
		CryptReleaseContext( prov, 0 );
		return qfalse;
	}
	CryptReleaseContext( prov, 0 );
	return qtrue;
}


#ifdef UNICODE
LPWSTR AtoW( const char *s ) 
{
	static WCHAR buffer[MAXPRINTMSG*2];
	MultiByteToWideChar( CP_OEMCP, 0, s, strlen( s ) + 1, (LPWSTR) buffer, ARRAYSIZE( buffer ) );
	return buffer;
}

const char *WtoA( const LPWSTR s ) 
{
	static char buffer[MAXPRINTMSG*2];
	WideCharToMultiByte( CP_OEMCP, 0, s, -1, buffer, ARRAYSIZE( buffer ), NULL, NULL );
	return buffer;
}
#endif


/*
================
Sys_GetCurrentUser
================
*/
char *Sys_GetCurrentUser( void )
{
	static char s_userName[256];

	TCHAR buffer[256];
	DWORD size = ARRAYSIZE( buffer );

	if ( !GetUserName( buffer, &size ) || !s_userName[0] ) {
		strcpy( s_userName, "player" );
	} else {
		strcpy( s_userName, WtoA( buffer ) );
	}

	return s_userName;
}


/*
================
Sys_DefaultHomePath
================
*/
const char *Sys_DefaultHomePath( void ) 
{
#ifdef USE_PROFILES
	TCHAR szPath[MAX_PATH];
	static char path[MAX_OSPATH];
	FARPROC qSHGetFolderPath;
	HMODULE shfolder = LoadLibrary("shfolder.dll");
	
	if(shfolder == NULL) {
		Com_Printf("Unable to load SHFolder.dll\n");
		return NULL;
	}

	qSHGetFolderPath = GetProcAddress(shfolder, "SHGetFolderPathA");
	if(qSHGetFolderPath == NULL)
	{
		Com_Printf("Unable to find SHGetFolderPath in SHFolder.dll\n");
		FreeLibrary(shfolder);
		return NULL;
	}

	if( !SUCCEEDED( qSHGetFolderPath( NULL, CSIDL_APPDATA,
		NULL, 0, szPath ) ) )
	{
		Com_Printf("Unable to detect CSIDL_APPDATA\n");
		FreeLibrary(shfolder);
		return NULL;
	}
	Q_strncpyz( path, szPath, sizeof(path) );
	Q_strcat( path, sizeof(path), "\\Quake3" );
	FreeLibrary(shfolder);
	if( !CreateDirectory( path, NULL ) )
	{
		if( GetLastError() != ERROR_ALREADY_EXISTS )
		{
			Com_Printf("Unable to create directory \"%s\"\n", path);
			return NULL;
		}
	}
	return path;
#else
    return NULL;
#endif
}


/*
================
Sys_SteamPath
================
*/
const char *Sys_SteamPath( void )
{
	static TCHAR steamPath[ MAX_OSPATH ]; // will be converted from TCHAR to ANSI

#if defined(STEAMPATH_NAME) || defined(STEAMPATH_APPID)
	HKEY steamRegKey;
	DWORD pathLen = MAX_OSPATH;
	qboolean finishPath = qfalse;
#endif

#ifdef STEAMPATH_APPID
	// Assuming Steam is a 32-bit app
	if ( !steamPath[0] && RegOpenKeyEx(HKEY_LOCAL_MACHINE, AtoW("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App " STEAMPATH_APPID), 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &steamRegKey ) == ERROR_SUCCESS ) 
	{
		pathLen = sizeof( steamPath );
		if ( RegQueryValueEx( steamRegKey, AtoW("InstallLocation"), NULL, NULL, (LPBYTE)steamPath, &pathLen ) != ERROR_SUCCESS )
			steamPath[ 0 ] = '\0';
	}

#ifdef STEAMPATH_NAME
	if ( !steamPath[0] && RegOpenKeyEx(HKEY_CURRENT_USER, AtoW("Software\\Valve\\Steam"), 0, KEY_QUERY_VALUE, &steamRegKey ) == ERROR_SUCCESS )
	{
		pathLen = sizeof( steamPath );
		if ( RegQueryValueEx( steamRegKey, AtoW("SteamPath"), NULL, NULL, (LPBYTE)steamPath, &pathLen ) != ERROR_SUCCESS ) {
			pathLen = sizeof( steamPath );
			if ( RegQueryValueEx( steamRegKey, AtoW("InstallPath"), NULL, NULL, (LPBYTE)steamPath, &pathLen ) != ERROR_SUCCESS )
				steamPath[ 0 ] = '\0';
		}

		if ( steamPath[ 0 ] )
			finishPath = qtrue;
	}
#endif

	if ( steamPath[ 0 ] )
	{
		if ( pathLen == sizeof( steamPath ) )
			pathLen--;

		*( ((char*)steamPath) + pathLen )  = '\0';
#ifdef UNICODE
		strcpy( (char*)steamPath, WtoA( steamPath ) );
#endif
		if ( finishPath )
			Q_strcat( (char*)steamPath, MAX_OSPATH, "\\SteamApps\\common\\" STEAMPATH_NAME );
	}
#endif

	return (const char*)steamPath;
}


/*
================
Sys_SnapVector
================
*/
#if idx64
void Sys_SnapVector( float *vector ) 
{
	__m128 vf0, vf1, vf2;
	__m128i vi;

	vf0 = _mm_setr_ps( vector[0], vector[1], vector[2], 0.0 );

	vi = _mm_cvtps_epi32( vf0 );
	vf0 = _mm_cvtepi32_ps( vi );

	vf1 = _mm_shuffle_ps(vf0, vf0, _MM_SHUFFLE(1,1,1,1));
	vf2 = _mm_shuffle_ps(vf0, vf0, _MM_SHUFFLE(2,2,2,2));

	_mm_store_ss( &vector[0], vf0 );
	_mm_store_ss( &vector[1], vf1 );
	_mm_store_ss( &vector[2], vf2 );
}
#else
void Sys_SnapVector( float *vector ) 
{
	static const DWORD cw037F = 0x037F;
	DWORD cwCurr;
__asm {
	fnstcw word ptr [cwCurr]
	mov ecx, vector
	fldcw word ptr [cw037F]

	fld   dword ptr[ecx+8]
	fistp dword ptr[ecx+8]
	fild  dword ptr[ecx+8]
	fstp  dword ptr[ecx+8]

	fld   dword ptr[ecx+4]
	fistp dword ptr[ecx+4]
	fild  dword ptr[ecx+4]
	fstp  dword ptr[ecx+4]

	fld   dword ptr[ecx+0]
	fistp dword ptr[ecx+0]
	fild  dword ptr[ecx+0]
	fstp  dword ptr[ecx+0]

	fldcw word ptr cwCurr
	}; // __asm
}
#endif


/*
================
Sys_SetAffinityMask
================
*/
void Sys_SetAffinityMask( int mask ) 
{
	static DWORD_PTR dwOldProcessMask;
	static DWORD_PTR dwSystemMask;

	// initialize
	if ( !dwOldProcessMask ) {
		dwSystemMask = 0x1;
		dwOldProcessMask = 0x1;
		GetProcessAffinityMask( GetCurrentProcess(), &dwOldProcessMask, &dwSystemMask );
	}

	 // set default mask
	if ( !mask ) {
		if ( dwOldProcessMask )
			mask = dwOldProcessMask;
		else
			mask = dwSystemMask;
	}

	if ( SetProcessAffinityMask( GetCurrentProcess(), mask ) ) {
		Sleep( 0 );
		Com_Printf( "setting CPU affinity mask to %i\n", mask );
	} else {
		Com_Printf( S_COLOR_YELLOW "error setting CPU affinity mask %i\n", mask );
	}
}

