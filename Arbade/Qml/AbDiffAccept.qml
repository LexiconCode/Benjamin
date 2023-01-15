import QtQuick 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.0
import QtQml 2.12
import QtQuick.Extras 1.4
import QtQuick.Controls 2.3
import QtQuick.Window 2.10

Window
{
    title: "Delete File ?"
    height: accept_label.height + 100
    width: accept_label.width + 40
    property string dialog_result: ""
    property string dialog_label: ""
    property string botton_text: "#b6b6b6"
    property string botton_bg: "#4d4d4d"
    property string botton_border: "#bfbfbf"
    property string area_text: "#c9c9c9"

    color: "#2e2e2e"

    Text
    {
        id: accept_label
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.left: parent.left
        anchors.leftMargin: 20
        font.pixelSize: 20
        text: dialog_label
        lineHeight: 1.4
        color: area_text

        Keys.onPressed:
        {
            if( event.key===Qt.Key_Space ||
                event.key===Qt.Key_Y )
            {
                accept();
            }
            else if( event.key===Qt.Key_Q ||
                    event.key===Qt.Key_N )
            {
                reject();
            }
        }
    }

    Button
    {
        id: yes_button
        text: "Yes"
        height: 40
        width: (parent.width-45)/2
        anchors.left: parent.left
        anchors.leftMargin: 15
        anchors.top: accept_label.bottom
        anchors.topMargin: 10
        font.pixelSize: 18
        DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        palette.buttonText: botton_text
        background: Rectangle
        {
            anchors.fill: parent
            color: botton_bg
            border.color: botton_border
        }

        onClicked:
        {
            accept();
        }
    }

    Button
    {
        id: no_button
        text: "No"
        height: 40
        width: (parent.width-45)/2
        anchors.right: parent.right
        anchors.rightMargin: 15
        anchors.top: accept_label.bottom
        anchors.topMargin: 10
        font.pixelSize: 18
        DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        palette.buttonText: botton_text
        background: Rectangle
        {
            anchors.fill: parent
            color: botton_bg
            border.color: botton_border
        }

        onClicked:
        {
            reject();
        }
    }


    Component.onCompleted:
    {
        accept_label.forceActiveFocus()
    }

    function accept()
    {
        dialog_result = "Y"
        close()
    }

    function reject()
    {
        dialog_result = "N"
        close()
    }


}