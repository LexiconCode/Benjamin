import QtQuick 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.0
import QtQml 2.12
import QtQuick.Extras 1.4
import QtQuick.Controls 2.3
import QtQuick.Window 2.10

Rectangle
{
    id: container
    property string do_job: ""
    property string word_list: ""
    property string word_stat: ""
    property int start_num: 0
    property int wl_count: 0

    signal wordBoxChanged(int id, string word)

    ListModel
    {
        id: lm_wordbox
    }

    Component
    {
        id: ld_wordbox

        AbWordLine
        {
            width: container.width
            word_id: wid
            word_text: wt
            word_count: wc

            onWordChanged:
            {
                wordBoxChanged(word_id, word_text)
            }
        }
    }

    ListView
    {
        id: lv_wordbox

        anchors.left: parent.left
        anchors.top: parent.top
        width: parent.width
        height: childrenRect.height
        interactive: false

        model: lm_wordbox
        delegate: ld_wordbox
    }

    onWord_statChanged: // word stat changes after word list
    {
        updateWords();
    }

    onDo_jobChanged:
    {
        if( do_job==="do that fucking job" )
        {
            word_list = "";
            for( var i=0 ; i<wl_count ; i++ )
            {
                word_list += lm_wordbox.get(i).word_text + "\n";
            }

            do_job = "";
        }
    }

    function updateWords()
    {
        var wl_split = word_list.split("\n");
        var wlc_split = word_stat.split("\n");
        wl_count = wl_split.length;

        for( var i=0 ; i<wl_count ; i++ )
        {
            lm_wordbox.append({wid: zeroPad(i+start_num),
                               wt: wl_split[i],
                               wc: wlc_split[i]});
        }
    }

    function zeroPad(num)
    {
        var zero = 3 - num.toString().length + 1;
        return Array(+(zero > 0 && zero)).join("0") + num;
    }
}