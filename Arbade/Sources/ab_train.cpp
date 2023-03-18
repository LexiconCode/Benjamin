#include "ab_train.h"
#include <QDebug>
#include <QQmlProperty>

AbTrain::AbTrain(QObject *ui, QObject *parent) : QObject(parent)
{
    wsl = new AbInitWSL();
    root = ui;
    wsl_dialog = root->findChild<QObject*>("WslDialog");
    console = root->findChild<QObject*>("Console");

    connect(root, SIGNAL(sendKey(int)), this, SLOT(processKey(int)));
    connect(wsl_dialog, SIGNAL(driveEntered(QString)),
            wsl, SLOT(createWSL(QString)));

    con_read = new AbConsoleReader();
    con_thread = new QThread();
    con_read->moveToThread(con_thread);
    con_thread->start();
    connect(con_read, SIGNAL(readyData(QString)),
            this, SLOT(writeConsole(QString)));
    connect(this, SIGNAL(readConsole()),
            con_read, SLOT(run()));
    connect(con_read, SIGNAL(readyCommand()),
            this, SLOT(WriteToPipe()));
}

AbTrain::~AbTrain()
{

}

void AbTrain::processKey(int key)
{
    if( key==Qt::Key_T )
    {
        initWsl();
    }
    else
    {
        return;
    }
}

void AbTrain::initWsl()
{
    qDebug() << "TRAINING STARTED ....";
    wsl_path = wsl->getWslPath();
    if( wsl_path.isEmpty() )
    {
        QMetaObject::invokeMethod(root, "initWsl");
    }

    if( !QFile::exists(wsl_path + "\\ext4.vhdx") )
    {
        QString drive = QString(wsl_path[0]);
        wsl->createWSL(drive);
    }

    qDebug() << "createKalB";
    createKalB();
}

void AbTrain::createKalB()
{
    QQmlProperty::write(root, "ab_show_console", 1);
    QString current_dir = QDir::currentPath();
    QDir::setCurrent(wsl_path);

    openApp();
}

void AbTrain::writeConsole(QString line)
{
    QQmlProperty::write(console, "line_buf", line);
    QMetaObject::invokeMethod(console, "addLine");
}

int AbTrain::openApp()
{
    SECURITY_ATTRIBUTES saAttr;

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // stdout
    if( !CreatePipe(&con_read->handle, &h_out_write, &saAttr, 0) )
    {
       qDebug() << "StdoutRd CreatePipe failed";
    }
    if( !SetHandleInformation(con_read->handle, HANDLE_FLAG_INHERIT, 0) )
    {
       qDebug() << "Stdout SetHandleInformation failed";
    }

    // stdin
    if( !CreatePipe(&h_in_read, &h_in_write, &saAttr, 0) )
    {
       qDebug() << "Stdin CreatePipe failed";
    }
    if( !SetHandleInformation(h_in_write, HANDLE_FLAG_INHERIT, 0) )
    {
       qDebug() << "Stdin SetHandleInformation failed";
    }

//    CreateChildProcess("KalB.exe");
    CreateChildProcess("cmd.exe");

    qDebug() << "ghable emit";
    emit readConsole();

    return 0;
}

void AbTrain::CreateChildProcess(QString cmd)
{
   BOOL bSuccess = FALSE;

   PROCESS_INFORMATION piProcInfo;
   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

   STARTUPINFOA siStartInfo;
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = h_out_write;
   siStartInfo.hStdOutput = h_out_write;
   siStartInfo.hStdInput = h_in_read;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

   LPSTR cmd_a = const_cast<char *>(cmd.toStdString().c_str());
   bSuccess = CreateProcessA(NULL,
      cmd_a,     // command line
      NULL,          // process security attributes
      NULL,          // primary thread security attributes
      TRUE,          // handles are inherited
      0,             // creation flags
      NULL,          // use parent's environment
      NULL,          // use parent's current directory
      &siStartInfo,  // STARTUPINFO pointer
      &piProcInfo);  // receives PROCESS_INFORMATION

   if ( !bSuccess )
   {
       qDebug() << "CreateProcess failed";
   }
   else
   {
      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
      CloseHandle(h_out_write);
      CloseHandle(h_in_read);
   }
}

void AbTrain::WriteToPipe()
{
    DWORD dwWritten;
    QString cmd = "dir\n";
//    QString cmd = "ls";

    WriteFile(h_in_write, cmd.toStdString().c_str(),
              cmd.length(), &dwWritten, NULL);

   if ( !CloseHandle(h_in_write) )
   {
       qDebug() << "StdInWr CloseHandle failed";
   }
}

