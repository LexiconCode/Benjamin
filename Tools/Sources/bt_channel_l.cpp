#include "bt_channel_l.h"
#include <unistd.h>

ReChannelL::ReChannelL(QObject *parent) : QObject(parent)
{
    ConnectDBus();
    conf = new BtConfidence;
}

ReChannelL::~ReChannelL()
{
    ;
}

void ReChannelL::ConnectDBus()
{
    QDBusConnection session = QDBusConnection::sessionBus();

    if (!session.isConnected())
    {
        qFatal("Cannot connect to the D-Bus session bus.");
        return;
    }

    session.connect("", "/", DBUS_NAME, "nato", this, SLOT(nato   (const QString &)));
    session.connect("", "/", DBUS_NAME, "exec", this, SLOT(execute(const QString &)));

    if(!session.registerService(DBUS_NAME))
    {
        qFatal("Another session is on DBus.");
        // This cannot be automatic because killing would also kill
        // this instant too
        return;
    }
}

void ReChannelL::execute(const QString &words)
{
    conf->parseConfidence();

    if( conf->isValidUtterance() )
    {
        conf->printWords(words);

        QString cmd = KAL_SI_DIR"main.sh \"";
        cmd += conf->getUtterance() + "\"";

        qDebug() << cmd;

        system(cmd.toStdString().c_str());
    }
}

void ReChannelL::nato(const QString &text)
{
    ;
}

void ReChannelL::startServer()
{

}