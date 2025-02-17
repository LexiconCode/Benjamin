import QtQuick 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.0
import QtQml 2.12
import QtQuick.Extras 1.4
import QtQuick.Controls 2.3

Dialog
{
    title: "Delete File ?"
    height: 160
    width: 350
    focus: true
    x: (root.width - width) / 2
    y: (root.height - height) / 2
    property string dialog_result: ""
    property string dialog_label: ""

    signal accept(string result)

    background: Rectangle
    {
        anchors.fill: parent
        color: "#2e2e2e"
    }

    Text
    {
        id: verify_label
        anchors.centerIn: parent
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        text: dialog_label
        lineHeight: 1.4
        color: "#b4b4b4"

        Keys.onPressed:
        {
            if( event.key===Qt.Key_Space ||
                event.key===Qt.Key_Y )
            {
                accept("Y");
                close();
            }
            else
            {
                accept("N");
                close();
            }
        }
    }

    Component.onCompleted:
    {
        verify_label.forceActiveFocus();
    }

}
