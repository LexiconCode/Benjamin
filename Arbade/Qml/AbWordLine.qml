import QtQuick 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.0
import QtQml 2.12
import QtQuick.Extras 1.4
import QtQuick.Controls 2.3
import QtQuick.Window 2.10

Rectangle
{
    id: wordline
    height: text_area.height
    property string word_id: ""
    property string word_text: ""
    property string word_count: "0"
    property int    set_focus: 0

    signal wordChanged(string text_w)
    signal arrowPressed(int direction)
    signal setFocusNum(int id)
    signal removeLine()

    Rectangle
    {
        id: linenum_bg
        width: 50
        height: text_area.height
        anchors.top: parent.top
        anchors.left: parent.left

        color: "#666666"
    }

    Label
    {
        id: linenum_lbl
        anchors.top: parent.top
        anchors.horizontalCenter: linenum_bg.horizontalCenter
        anchors.topMargin: 3

        font.pixelSize: 16
        text: word_id
        color: "#b4b4b4"
    }

    Rectangle
    {
        id: text_bg
        height: text_area.height
        anchors.top: parent.top
        anchors.left: linenum_bg.right
        width: wordline.width - 20
        color: "#4e4e4e"
    }

    TextArea
    {
        id: text_area
        Accessible.name: "document"
        width: wordline.width - 70
        anchors.left: linenum_lbl.right
        anchors.leftMargin: 30
        anchors.top: parent.top
        anchors.topMargin: -2

        text: word_text
        font.pixelSize: 16
        padding: 3
        focus: false
        selectByMouse: true
        color: "#c9c9c9"
        selectedTextColor: "#333"
        selectionColor: "#ccc"

        onTextChanged:
        {
            wordChanged(text);
        }

        Keys.onEscapePressed:
        {
            focus_item.forceActiveFocus();
        }

        Keys.onPressed:
        {
            if( event.key===Qt.Key_Backspace && text==="" )
            {
                var id = parseInt(word_id);
                if( id===editor_box.word_count-2 )
                {
                    removeLine();
                }
                else if( id===editor_box.word_count-1 )
                {
                    arrowPressed(Qt.Key_Up);
                }
            }
            else if( event.key===Qt.Key_Up ||
                     event.key===Qt.Key_Down )
            {
                arrowPressed(event.key);
            }
        }

        onFocusChanged:
        {
            setFocusNum(parseInt(word_id));
        }
    }

    Label
    {
        id: stat_lbl
        anchors.top: parent.top
        anchors.right: text_bg.right
        anchors.rightMargin: 20
        anchors.topMargin: 3
        horizontalAlignment: Text.AlignHCenter

        font.pixelSize: 16
        text: word_count
        color:
        {
            var mean = parseInt(root.ab_mean_var.split("!")[0]);
            var variance = parseInt(root.ab_mean_var.split("!")[1]);
            var count = parseInt(word_count.substring(1,word_count.length-1));
            if( count<mean-variance )
            {
                "#cb6565"; // red
            }
            else if( count>mean+variance )
            {
                "#80bf73"; // green
            }
            else
            {
                "#9a9a9a"; // gray
            }
        }
    }

    onSet_focusChanged:
    {
        if( set_focus )
        {
            text_area.forceActiveFocus();
            text_area.cursorPosition = text_area.text.length;
        }
    }

    onWord_textChanged:
    {
        text_area.text = word_text;
    }
}

