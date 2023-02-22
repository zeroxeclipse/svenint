#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#endif

#include "files/au_beep_synthtone01wav_raw.h"
#include "files/au_bunny_raw.h"
#include "files/au_glew32_raw.h"
#include "files/au_logo_raw.h"
#include "files/au_mainlua_raw.h"
#include "files/au_menu_click01wav_raw.h"
#include "files/au_menu_image_raw.h"
#include "files/au_radar_roundtga_raw.h"
#include "files/au_sven_internal_raw.h"
#include "files/au_talkwav_raw.h"
#include "files/au_testlua_raw.h"

#include "../shared/au_app_info.h"
#include "../shared/au_platform.h"
#include "../shared/au_utils.h"

#ifdef AU_PLATFORM_WINDOWS
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 142

#define WINDOW_NAME "AUTOUPDATE"
#define APP_NAME "Auto Update"

HWND g_hWndButton[2];
#define IDC_BT_0 101
#define IDC_BT_1 (IDC_BT_0+4)

struct update_file_t
{
    std::string path;

    char *bytes;
    unsigned int size;

    bool replaceExisting;
};

const char *g_pszPathsToCreate[] =
{
    "\\svenmod\\",
    "\\svenmod\\plugins\\",
    "\\sven_internal\\",
    "\\sven_internal\\config\\",
    "\\sven_internal\\config\\shaders\\",
    "\\sven_internal\\images\\",
    "\\sven_internal\\input_manager\\",
    "\\sven_internal\\message_spammer\\",
    "\\sven_internal\\models_manager\\",
    "\\sven_internal\\scripts\\",
    "\\svencoop\\",
    "\\svencoop\\sound\\",
    "\\svencoop\\sound\\sven_internal\\",
    "\\svencoop\\sven_internal\\",
    "\\svencoop\\sven_internal\\cfg\\",
    "\\svencoop\\sven_internal\\tex\\",
};

// TODO: compress?
update_file_t g_UpdateList[] =
{
    // Path: ../Sven Co-op/svenmod/plugins/
    { "svenmod/plugins/sven_internal.dll", au_sven_internal_bytes, au_sven_internal_size, true },
    
    // Path: ../Sven Co-op/
    { "glew32.dll", au_glew32_bytes, au_glew32_size, true },

    // Path: ../Sven Co-op/sven_internal/
    { "sven_internal/images/logo.png", au_logo_bytes, au_logo_size, false },
    { "sven_internal/images/menu_image.png", au_menu_image_bytes, au_menu_image_size, false },
    { "sven_internal/message_spammer/bunny.txt", au_bunny_bytes, au_bunny_size, false},
    { "sven_internal/models_manager/ignored_players.txt", "# Format: STEAM64ID", strlen("# Format: STEAM64ID"), false},
    { "sven_internal/models_manager/random_models.txt", "# Format: MODELNAME", strlen("# Format: MODELNAME"), false},
    { "sven_internal/models_manager/target_players.txt", "# Format: STEAM64ID = MODELNAME", strlen("# Format: STEAM64ID = MODELNAME"), false},
    { "sven_internal/scripts/main.lua", au_mainlua_bytes, au_mainlua_size, false },
    { "sven_internal/scripts/test.lua", au_testlua_bytes, au_testlua_size, false },

    // Path: ../Sven Co-op/svencoop/sound/sven_internal/
    { "svencoop/sound/sven_internal/beep_synthtone01.wav", au_beep_synthtone01wav_bytes, au_beep_synthtone01wav_size, false },
    { "svencoop/sound/sven_internal/menu_click01.wav", au_menu_click01wav_bytes, au_menu_click01wav_size, false },
    { "svencoop/sound/sven_internal/talk.wav", au_talkwav_bytes, au_talkwav_size, false },

    // Path: ../Sven Co-op/svencoop/sven_internal/cfg/
    { "svencoop/sven_internal/cfg/weapon_none.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_crowbar.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_glock.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_python.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_mp5.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_chaingun.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_crossbow.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_shotgun.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_rpg.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_gauss.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_egon.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_hornetgun.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_handgrenade.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_tripmine.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_satchel.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_snark.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_uzi.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_medkit.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_crowbar_electric.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_pipewrench.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_minigun.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_grapple.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_sniperrifle.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_m249.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_m16.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_sporelauncher.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_desert_eagle.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_shockrifle.cfg", NULL, 0, false },
    { "svencoop/sven_internal/cfg/weapon_displacer.cfg", NULL, 0, false },

    // Path: ../Sven Co-op/svencoop/sven_internal/tex/
    { "svencoop/sven_internal/tex/radar_round.tga", au_radar_roundtga_bytes, au_radar_roundtga_size, false },

    // Path: ../Sven Co-op/svencoop/sven_internal.cfg
    { "svencoop/sven_internal.cfg", "echo Config sven_internal.cfg executed", strlen("echo Config sven_internal.cfg executed"), false}
};

char *Sys_GetLongPathName()
{
    char szShortPath[MAX_PATH];
    static char szLongPath[MAX_PATH];
    char *pszPath = NULL;

    szShortPath[0] = 0;
    szLongPath[0] = 0;

    if ( GetModuleFileNameA(NULL, szShortPath, sizeof(szShortPath)) )
    {
        GetLongPathNameA( szShortPath, szLongPath, sizeof(szLongPath) );
        pszPath = strrchr( szLongPath, '\\' );

        if ( pszPath[0] )
            pszPath[1] = 0;

        size_t len = strlen(szLongPath);

        if ( len > 0 )
        {
            if ( szLongPath[len - 1] == '\\' )
                szLongPath[len - 1] = 0;
        }
    }

    return szLongPath;
}

void DoAutoUpdate(HWND hWnd)
{
    auto ShowErrorMessage = [](HWND hWnd, const char *pszMessage)
    {
        MessageBox(hWnd, pszMessage, APP_NAME, MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
    };

    auto FileExists = [](LPCTSTR szPath) -> BOOL
    {
        DWORD dwAttrib = GetFileAttributes(szPath);

        return ( dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) );
    };

    std::string sBasePath = Sys_GetLongPathName();

    for (int i = 0; i < ARRAYSIZE(g_pszPathsToCreate); i++)
    {
        std::string sPath = sBasePath + g_pszPathsToCreate[i];

        if ( !CreateDirectory(sPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS )
        {
            ShowErrorMessage(hWnd, (std::string("Failed to create directory: ") + sPath).c_str());
            return;
        }
    }
    
    for (int i = 0; i < ARRAYSIZE(g_UpdateList); i++)
    {
        update_file_t &updateFile = g_UpdateList[i];

        if ( !updateFile.replaceExisting )
        {
            std::string sPath = sBasePath + updateFile.path;

            for (size_t j = 0; j < sPath.length(); j++)
            {
                if ( sPath[j] == '/' )
                    sPath[j] = '\\';
            }

            if ( FileExists( sPath.c_str() ) )
                continue;
        }

        FILE *file = fopen( updateFile.path.c_str(), "wb" );

        if ( file != NULL )
        {
            if ( updateFile.bytes != NULL && updateFile.size > 0 )
            {
                fwrite( updateFile.bytes, sizeof(char), updateFile.size, file );
            }

            fclose( file );
        }
        else
        {
            ShowErrorMessage(hWnd, (std::string("Unable to create/replace file: ") + updateFile.path).c_str());
            return;
        }
    }

    //FILE *file = fopen(AUTOUPDATE_APP_FILENAME, "wb");

	//if ( file != NULL )
	//{
	//	fwrite( app_bytes, sizeof(app_bytes[0]), app_size, file);
	//	fclose( file );
	//}

    MessageBox(hWnd, "Successfully updated.", APP_NAME, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
}

void SelfDelete()
{
	char szModuleName[MAX_PATH];
	char szCmd[512];

	STARTUPINFO startupInfo = { 0 };
	PROCESS_INFORMATION procInfo = { 0 };

	GetModuleFileName(NULL, szModuleName, MAX_PATH);

	sprintf(szCmd, "cmd.exe /C -w 1000 > Nul & Del /f /q \"%s\"", szModuleName);

	CreateProcess(NULL, szCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &procInfo);

	CloseHandle( procInfo.hThread );
	CloseHandle( procInfo.hProcess );
}

static bool GrabMutex(HANDLE *hMutex)
{
    *hMutex = CreateMutex(NULL, FALSE, TEXT("__autoupdate_singleton_mutex"));

    DWORD dwWaitResult = WaitForSingleObject(*hMutex, 0);

    if ( dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_ABANDONED )
        return true;

    return false;
}

BOOL OnCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL Result = TRUE;
    HWND hwndCtl = (HWND)lParam;
    WORD Command = LOWORD(wParam);
    WORD NotifyCode = HIWORD(wParam);

    switch (Command)
    {
    case IDC_BT_0:
    {
        DoAutoUpdate(hWnd);

        SendMessage(hWnd, WM_CLOSE, 0, 0);
        break;
    }

    case IDC_BT_1:
        //MessageBox(hWnd, "Bad", APP_NAME, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        break;
    };

    return Result;
}

void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);
    RECT rect;
    
    GetClientRect(hWnd, &rect);
    
    //rect.left = 100;
    rect.top = 12;
    //rect.right -= (rect.right / 4);
    
    DrawText(hDC, "An update for \"" AUTOUPDATE_APP_NAME "\" is available. Version: " AUTOUPDATE_APP_VERSION_STRING "\nProceed update?\n",
             -1, &rect, DT_EXTERNALLEADING | DT_NOPREFIX | DT_WORDBREAK | DT_CENTER);
    
    EndPaint(hWnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT retval = 0;
    static HWND hwndLastfocus;
    
    switch (uMsg)
    {
    case WM_CREATE:
    {
        LPCREATESTRUCT lpCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);

        for (int i = 0; i < 2; ++i)
        {
            g_hWndButton[i] = CreateWindowEx(0,
                                            "button",
                                            "",
                                            BS_PUSHBUTTON | BS_TEXT | WS_CHILD | WS_TABSTOP | WS_VISIBLE,
                                            0,
                                            0,
                                            0,
                                            0,
                                            hWnd,
                                            (HMENU)(UINT_PTR)IDC_BT_0 + i,
                                            lpCreateStruct->hInstance,
                                            LPVOID(0));
            
            if ( !g_hWndButton[i] )
            {
                //MessageBox(hWnd, "Cannot CreateWindow", APP_NAME, MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
                return -1;
            }
        }

        SetWindowText(g_hWndButton[0], "Yes");
        SetWindowText(g_hWndButton[1], "No");

        SendDlgItemMessage(hWnd, IDC_BT_0, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
        SetFocus(GetDlgItem(hWnd, IDC_BT_0));
    }
    return 1;

    case WM_ACTIVATE:
        if ( LOWORD(wParam) == WA_INACTIVE )
            hwndLastfocus = GetFocus();
        
        break;

    case WM_SETFOCUS:
        if ( hwndLastfocus )
            SetFocus(hwndLastfocus);
        
        break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        HDWP hDWP;
        RECT rc;

        if ( hDWP = BeginDeferWindowPos(2) )
        {
            GetClientRect(hWnd, &rc);

        #define BUTTON_POS_Y 60
        #define BUTTON_WIDTH 100
        #define BUTTON_HEIGHT 28
        #define BUTTON_GAP_BETWEEN 20

            int ButtonX = (WINDOW_WIDTH / 2) - ((BUTTON_GAP_BETWEEN / 2) + BUTTON_WIDTH);
            int ButtonY = BUTTON_POS_Y;

            hDWP = DeferWindowPos(hDWP, GetDlgItem(hWnd, IDC_BT_0), NULL, ButtonX, ButtonY, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER | SWP_NOREDRAW);

            ButtonX += BUTTON_WIDTH + BUTTON_GAP_BETWEEN;

            hDWP = DeferWindowPos(hDWP, GetDlgItem(hWnd, IDC_BT_1), NULL, ButtonX, ButtonY, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER | SWP_NOREDRAW);

            EndDeferWindowPos(hDWP);
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_NOFRAME | RDW_UPDATENOW);
        }
    }
    break;

    case WM_PAINT:
        OnPaint(hWnd);
        break;

    case WM_COMMAND:
        return OnCommand(hWnd, uMsg, wParam, lParam);

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        retval = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return retval;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    HANDLE hMutex = NULL;
    HWND hWnd = HWND_DESKTOP;

    if ( !GrabMutex(&hMutex) )
        return EXIT_FAILURE;

    WNDCLASSEX wcex =
    {
        sizeof(WNDCLASSEX),
        CS_VREDRAW | CS_HREDRAW,
        WndProc,
        0,
        0,
        hInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        NULL,
        WINDOW_NAME,
        NULL,
    };

    ATOM atom = RegisterClassEx(&wcex);
    
    if ( !atom )
    {
        atom = RegisterClass((LPWNDCLASS)&wcex.style);
        
        if ( !atom )
        {
            if ( hMutex )
                CloseHandle(hMutex);

            //MessageBox(hWnd, "Cannot RegisterClass", APP_NAME, MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
            return EXIT_FAILURE;
        }
    }

    int nWidth = WINDOW_WIDTH;
    int nHeight = WINDOW_HEIGHT;
    
    hWnd = CreateWindowEx(WS_EX_WINDOWEDGE,
                          MAKEINTATOM(atom),
                          APP_NAME,
                          WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME), // no minimize, maximize, resize
                          ((GetSystemMetrics(SM_CXSCREEN) - nWidth) / 2),
                          ((GetSystemMetrics(SM_CYSCREEN) - nHeight) / 2),
                          nWidth,
                          nHeight,
                          HWND_DESKTOP,
                          NULL,
                          hInstance,
                          NULL);
    if ( !hWnd )
    {
        if ( hMutex )
            CloseHandle(hMutex);

        //MessageBox(hWnd, "Cannot CreateWindow", APP_NAME, MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
        return EXIT_FAILURE;
    }

    ShowWindow(hWnd, SW_NORMAL);
    UpdateWindow(hWnd);

    MSG msg;
    
    while ( GetMessage(&msg, NULL, 0, 0) )
    {
        if ( !IsDialogMessage(hWnd, &msg) )
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if ( hMutex )
        CloseHandle(hMutex);

    SelfDelete();

    return (int)msg.wParam;
}

#else

void SelfDelete(char *argv[])
{
	remove( argv[0] );
}

void UpdateApp()
{
	AU_Printf("\nUpdate has been confirmed.\n");
	AU_Printf("\nUnpacking...\n");

	FILE *file = fopen(AUTOUPDATE_APP_FILENAME, "wb");

	if ( file != NULL )
	{
		fwrite( au_sven_internal_bytes, sizeof(au_sven_internal_bytes[0]), au_sven_internal_size, file);
		fclose( file );

		AU_Printf("\nFinished unpacking.\n");
	}
	else
	{
		AU_Printf("\nUnable to replace an app.\n");
	}
}

int main(int argc, char *argv[])
{
	char c;

	AU_Printf("An update for \"" AUTOUPDATE_APP_NAME "\" is available. Version: " AUTOUPDATE_APP_VERSION_STRING "\n\n");
	AU_Printf("Proceed update? [Y/N]: ");

	if ( scanf("%c", &c) && (c == 'y' || c == 'Y') )
	{
		UpdateApp();
	}
	else
	{
		AU_Printf("\nUpdate has been canceled.\n");
	}

	AU_Printf("\nPress any key to exit.\n");

#pragma warning(push)
#pragma warning(disable : 6031)

#ifdef AU_PLATFORM_WINDOWS
	getch();
#else
	getchar();
#endif

#pragma warning(pop)

	SelfDelete( argv );

	return 0;
}

#endif