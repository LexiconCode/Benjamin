#include "db_lua.h"
#include <QtDebug>
#include <QDir>

DbLua::DbLua()
{
    lst = luaL_newstate();
    luaL_openlibs(lst);

    connectPipe();
}

void DbLua::connectPipe()
{
    // 0: Default Wait Time
    int np_is_available = WaitNamedPipeA(DB_PIPE_ADDRESS, 0);
    if( np_is_available )
    {
        hPipe = CreateFileA(DB_PIPE_ADDRESS, GENERIC_WRITE, // dwDesiredAccess
                            0, nullptr,    // lpSecurityAttributes
                            OPEN_EXISTING,  // dwCreationDisposition
                            0, nullptr);    // hTemplateFile

        if( hPipe==INVALID_HANDLE_VALUE )
        {
            qDebug() << "Error 120: Cannot create " DB_PIPE_ADDRESS;
        }
    }
    else
    {
        hPipe = INVALID_HANDLE_VALUE;
        qDebug() << "Error 121: Pipe " DB_PIPE_ADDRESS
                    " not found";
    }
}

void DbLua::run(QString word)
{ ///FIXME: solve directory problem
    QString current_dir = QDir::currentPath();
    QDir::setCurrent(KAL_SI_DIR_WIN);

    luaL_loadfile(lst, "main_w.lua");

    lua_pushstring(lst, word.toStdString().c_str());
    lua_setglobal(lst, "in_word");
    lua_pcall(lst, 0, LUA_MULTRET, 0);

    lua_getglobal(lst, "output");
    int output = lua_tonumber(lst, -1);

    lua_getglobal(lst, "k_type");
    QString ktype = lua_tostring(lst, -1);

    qDebug() << "LUA : " << output << ktype;

    //Always send debug first
    sendDebug(word);
    sendKey(ktype, output);

    QDir::setCurrent(current_dir);
}

DbLua::~DbLua()
{
    lua_close(lst);
    CloseHandle(hPipe);
}

void DbLua::sendKey(QString type, int keycode)
{
    QString line = type + BT_PN_SEPARATOR;
    line += QString::number(keycode) + "\n";

    sendPipe(line.toStdString().c_str());
}

void DbLua::sendDebug(QString word)
{
    QString line = "debug" BT_PN_SEPARATOR;
    line += word + "\n";

    sendPipe(line.toStdString().c_str());
}

void DbLua::sendPipe(const char *data)
{
    DWORD len = strlen(data);
    if( hPipe==INVALID_HANDLE_VALUE )
    {
        qDebug() << "Try to reconnect to"
                 << DB_PIPE_ADDRESS;
        connectPipe();
        if( hPipe==INVALID_HANDLE_VALUE )
        {
            return;
        }
    }

    DWORD dwWritten;
    int success = WriteFile(hPipe, data, len, &dwWritten, NULL);
    if( !success )
    {
        qDebug() << "Error: NamedPipe writing failed," << GetLastError();
    }

    if( dwWritten!=len )
    {
        qDebug() << "Error: Wrong writing length."
                    "Try to revive channel";
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
        sendPipe(data);
    }
}
