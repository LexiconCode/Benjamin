#include "mm_win32.h"
#include <QScreen>
#include <QGuiApplication>
#include <QThread>
#include <QDebug>
#include <psapi.h>

void mm_sendMouseKey(int key)
{
    if( key==MM_MOUSE_LKEY )
    {
        mm_sendMouseFlag(MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP);
    }
    else if( key==MM_MOUSE_MKEY )
    {
        mm_sendMouseFlag(MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_MIDDLEUP);
    }
    else if( key==MM_MOUSE_RKEY )
    {
        mm_sendMouseFlag(MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_RIGHTUP);
    }
}

void mm_sendMouseFlag(int flag)
{
    INPUT input;
    input.type=INPUT_MOUSE;
    input.mi.dx=0;
    input.mi.dy=0;
    input.mi.dwFlags=(flag);
    input.mi.mouseData=0;
    input.mi.dwExtraInfo=NULL;
    input.mi.time=0;
    SendInput(1,&input,sizeof(INPUT));
}

long mm_getPid(HWND hWnd)
{
    // get allegro pid of window handle
    DWORD dwProcessId;
    GetWindowThreadProcessId(hWnd, &dwProcessId);
    if(long(dwProcessId) < 0)
    {
        qDebug() <<"Warning: couldn't get pid of allegro from window handle";
        return -1;
    }
    return dwProcessId;
}

QString mm_getPName(long pid)
{
    HANDLE processHandle = NULL;
//    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if(processHandle == NULL)
    {
        qDebug() << "Warning: couldn't get process handle from pid" << pid;
        return "";
    }

    // get name of process handle
    char filename[MAX_PATH];
    if(GetProcessImageFileNameA(processHandle, filename, MAX_PATH) == 0)
    {
//        qDebug("Warning: couldn't get name of process handle");
        return "";
    }
    return QString(filename);
}

QString mm_getPNameA(HWND hwnd)
{
    long active_pid = mm_getPid(hwnd);
    QString app_path = mm_getPName(active_pid);
    QString app_name = app_path.split("\\").last();

    return app_name;
}

BOOL IsAltTabWindow(HWND hwnd)
{
    HWND hwndTry, hwndWalk = NULL;

    if (!IsWindowVisible(hwnd))
        return FALSE;

    hwndTry = GetAncestor(hwnd, GA_ROOTOWNER);
    while (hwndTry != hwndWalk)
    {
        hwndWalk = hwndTry;
        hwndTry = GetLastActivePopup(hwndWalk);
        if (IsWindowVisible(hwndTry))
            break;
    }
    if (hwndWalk != hwnd)
        return FALSE;

    // the following removes some task tray programs and "Program Manager"
    //ti.cbSize = sizeof(ti);
    //GetTitleBarInfo(hwnd, &ti);
    //if (ti.rgstate[0] & STATE_SYSTEM_INVISIBLE)
    //    return FALSE;

    // Tool windows should not be displayed either, these do not appear in the
    // task bar.
    if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return FALSE;

    return TRUE;
}