﻿import QtQuick 2.2
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.3
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.0
import QtQuick.Extras 1.4
import QtQuick.Layouts 1.0
import QtQuick.Window 2.1
import Qt.labs.settings 1.0
import QtMultimedia 5.5
import QtQml 2.12

ApplicationWindow
{
    id: root

    y: 100
    x: 30
    width: 1650
    height: 900
    minimumHeight: 500
    minimumWidth: 760

    color: "#2e2e2e"
    title: "ArBade"
    visible: true

    property int sig_del_file: 0
    property real play_pos: 0

    property string ab_category: "home"
    property string ab_words: "<One> <Roger> <Spotify>"
    property string ab_address: ""
    property string ab_word_list: ""
    property string ab_word_stat: ""
    property string ab_auto_comp: ""
    property string ab_mean_var: ""
    property string ab_focus_text: ""
    property string ab_dif_words: ""
    property int ab_focus_word: -1
    property int ab_count: 0
    property int ab_total_count: 100
    property int ab_total_count_v: 100
    property real ab_elapsed_time: 0
    property int ab_status: ab_const.ab_STATUS_STOP
    property real ab_rec_time: 3
    property real ab_rec_time_v: 3
    property int ab_num_words: 3
    property int ab_num_words_v: 3
    property real ab_pause_time: 1.0
    property real ab_power: 0
    property int ab_verifier: 0
    property int ab_show_console: 0

    property real ab_start_now: 0

    signal loadsrc()
    signal delFile()
    signal deleteSample(string sample)
    signal copyFile()
    signal sendKey(int key)
    signal setStatus(int st)
    signal setVerifier(int ver)
    signal setFocusWord(int fw)
    signal saveWordList()
    signal setCategory()
    signal setDifWords()

    Component.onCompleted:
    {
        ab_start_now = Date.now();
    }

    onAb_statusChanged:
    {
        if( ab_verifier )
        {
            if( ( ab_status===ab_const.ab_STATUS_PLAY ||
                  ab_status===ab_const.ab_STATUS_BREAK) &&
                !audio_timer.running )
            {
                audio_timer.start();
            }
            else if( ab_status===ab_const.ab_STATUS_PAUSE ||
                     ab_status===ab_const.ab_STATUS_STOP )
            {
                audio_timer.stop();
            }
        }
    }

    onAb_word_statChanged:
    {
        editor_box.loadWordBoxes();
    }

    onAb_mean_varChanged:
    {
        status_bar.mean = ab_mean_var.split("!")[0];
        status_bar.variance = ab_mean_var.split("!")[1];
    }

    onAb_wordsChanged:
    {
        rec_list.word_samples.unshift(ab_words);
        rec_list.updateRecList();
    }

    Settings
    {
        property alias totalcount:      root.ab_total_count
        property alias category_name:   root.ab_category
        property alias rectime:         root.ab_rec_time
        property alias numwords:        root.ab_num_words
        property alias pausetime:       root.ab_pause_time
        property alias focusword:       root.ab_focus_word
        property alias verifier:        root.ab_verifier
    }

    Item
    {
        id: focus_item
        focus: true
        Keys.onPressed:
        {
            execKey(event.key);
        }
    }

    AbStatus
    {
        id: status_bar
        height: 120
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        pause_time: ab_pause_time
        num_words: ab_num_words
        rec_time: ab_rec_time
        status: ab_status
        words: ab_words
        category:
        {
            if( ab_verifier===1 )
            {
                "unverified"
            }
            else
            {
                ab_category
            }
        }
        count: ab_count
        count_total:
        {
            if( ab_verifier )
            {
                ab_total_count_v
            }
            else
            {
                ab_total_count
            }
        }

        elapsed_time:
        {
            if( ab_verifier )
            {
                play_pos
            }
            else
            {
                ab_elapsed_time
            }
        }
        power: ab_power
        focus_word: ab_focus_text
    }

    AbHelp
    {
        id: ab_help

        height: 120
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    AbConst
    {
        id: ab_const
    }

    AbDialog
    {
        id: get_value_dialog

        auto_complete_list: ab_auto_comp.split("!").
                                filter(i => i);
        onAcceptDialog:
        {
            if( title===category_title )
            {
                ab_category = value;
                setCategory();
            }
            else if( title===cnt_title )
            {
                var total_count = parseInt(value);
                ab_total_count = total_count;
            }
            else if( title===focus_word_title )
            {
                if( value==="" )
                {
                    ab_focus_word = -1;
                }
                else
                {
                    ab_focus_word = parseInt(value);
                }
                setFocusWord(ab_focus_word);
            }
        }
    }

    AbDialogWsl
    {
        id: dialog_wsl
        objectName: "WslDialog"
    }

    AbQuery
    {
        id: verify_dialog

        onAccept:
        {
            if( result==="Y" )
            {
                sig_del_file = 1;
            }
            else if( result==="N" )
            {
                sig_del_file = 0;
            }
            rest_timer.interval = 1;
            if( audioPlayer.playbackState===Audio.PausedState )
            {
                audioPlayer.stop();
            }
            else if( audioPlayer.playbackState===Audio.StoppedState )
            {
                rest_timer.start();
            }
        }
    }

    MouseArea
    {
        anchors.fill: parent

        onClicked: focus_item.forceActiveFocus();
    }

    AbEditor
    {
        id: editor_box
        objectName: "WordList"

        width: 1200
        anchors.top: ab_help.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.bottom: status_bar.top
        anchors.bottomMargin: 65

        onUpdateWordList:
        {
            ab_word_list = word_list;
            saveWordList();
        }
        onUpdateDifWords:
        {
            ab_dif_words = dif_words;
            setDifWords();
        }
        onEnableButtons:
        {
            buttons_box.btn_enable = enable;
        }
    }

    AbButtons
    {
        id: buttons_box
        anchors.top: editor_box.bottom
        anchors.topMargin: -30
        anchors.horizontalCenter: editor_box.horizontalCenter
    }

    AbRecList
    {
        id: rec_list

        height: editor_box.height
        anchors.left: editor_box.right
        anchors.top: ab_help.bottom
        anchors.topMargin: 20
        anchors.bottom: status_bar.top
        anchors.right: parent.right
        anchors.rightMargin: 20

        onDelSample:
        {
            deleteSample(sample);
        }
    }

    AbConsole
    {
        id: console_box
        anchors.top: editor_box.top
        anchors.left: editor_box.left
        anchors.right: editor_box.right
        anchors.bottom: buttons_box.bottom
        anchors.rightMargin: 20
        visible: ab_show_console
        objectName: "Console"
    }

    Audio
    {
        id: audioPlayer
        source: ab_address

        onStopped:
        {
            rest_timer.start();
        }
    }

    Timer
    {
        id: audio_timer
        interval: 50
        repeat: true

        onTriggered:
        {// (*100.0 -> 0-100 %) (/1000 -> ms->sec)
            play_pos += 50.0*100.0/1000.0/
                    (ab_rec_time+ab_pause_time)
            if( play_pos>100 )
            {
                play_pos=100
            }
        }
    }

    Timer
    {
        id: rest_timer
        interval: ab_const.ab_REST_PAUSE
        repeat: false

        onTriggered:
        {
            interval = ab_const.ab_REST_PAUSE;
            rest_timer.stop();
            if( sig_del_file )
            {
                sig_del_file = 0;
                delFile();
            }
            else
            {
                copyFile();
            }

            loadsrc();
            play_pos = 0;
        }
    }

    //Fonts:
    FontLoader
    {
        id: fontRobotoRegular
        source: "qrc:/Roboto-Regular.ttf"
    }

    FontLoader
    {
        id: fontAwesomeSolid
        source: "qrc:/fa6-solid.ttf"
    }

    function execKey(key)
    {
        if( key===Qt.Key_O || key===Qt.Key_W || key===Qt.Key_T )
        {
            sendKey(key);
        }
        else if( key===Qt.Key_Escape )
        {
            if( console_box.visible )
            {
                console_box.visible = false;
            }
            sendKey(key);
        }
        else if( key===Qt.Key_Left )
        {
            ab_num_words--;
        }
        else if( key===Qt.Key_Right )
        {
            ab_num_words++;
        }
        else if( key===Qt.Key_K )
        {
            ab_rec_time += .1;
        }
        else if( key===Qt.Key_J )
        {
            ab_rec_time -= .1;
        }
        else if( key===Qt.Key_Up )
        {
            ab_pause_time += .1;
        }
        else if( key===Qt.Key_Down )
        {
            ab_pause_time -= .1;
        }
        else if( key===Qt.Key_Space )
        {
            if( ab_verifier )
            {
                if( ab_status===ab_const.ab_STATUS_STOP )
                {
                    loadsrc();
                }
                else if( ab_status===ab_const.ab_STATUS_BREAK )
                {
                    return;
                }
                else
                {
                    if( audioPlayer.playbackState===Audio.PlayingState )
                    {
                        audioPlayer.pause();
                    }
                    ab_status = ab_const.ab_STATUS_PAUSE;
                    setStatus(ab_status);
                    rest_timer.stop();
                    verify_dialog.dialog_label = "Are you sure "+
                                         "you want to delete?\n"+
                                         "( Yes:space / No:q )"
                    verify_dialog.open();
                    verify_dialog.forceActiveFocus();
                }
            }
            else
            {
                if( ab_status===ab_const.ab_STATUS_REC ||
                    ab_status===ab_const.ab_STATUS_BREAK )
                {
                    ab_status = ab_const.ab_STATUS_REQPAUSE;
                    setStatus(ab_status);
                }
                else if( ab_status===ab_const.ab_STATUS_PAUSE ||
                         ab_status===ab_const.ab_STATUS_STOP )
                {
                    ab_status = ab_const.ab_STATUS_REC;
                    setStatus(ab_status);
                }
            }
        }
        else if( key===Qt.Key_Q )
        {
            close();
        }
        else if( key===Qt.Key_S )
        {
            if( ab_verifier===0 )
            {
                get_value_dialog.title = get_value_dialog.category_title;
                get_value_dialog.dialog_label = get_value_dialog.value_label;
                get_value_dialog.dialog_text = get_value_dialog.category_title;
                get_value_dialog.visible = true;
            }
        }
        else if( key===Qt.Key_C )
        {
            get_value_dialog.title = get_value_dialog.cnt_title;
            get_value_dialog.dialog_label = get_value_dialog.value_label;
            get_value_dialog.dialog_text = get_value_dialog.cnt_title;
            get_value_dialog.visible = true;
        }
        else if( key===Qt.Key_F )
        {
            get_value_dialog.title = get_value_dialog.focus_word_title;
            get_value_dialog.dialog_label = get_value_dialog.id_label;
            get_value_dialog.dialog_text = get_value_dialog.focus_word_title;
            get_value_dialog.visible = true;
        }
        else if( key===Qt.Key_V )
        {
            ab_status = ab_const.ab_STATUS_STOP;
            setStatus(ab_status);
            audioPlayer.stop()
            if( ab_verifier )
            {
                ab_verifier = 0;
            }
            else
            {
                ab_verifier = 1;
            }
            setVerifier(ab_verifier);
        }
        else if( key===Qt.Key_R )
        {

        }
    }

    function playkon()
    {
        audioPlayer.play();
    }

    function incCount()
    {
        ab_count++;
    }

    function initWsl()
    {
        dialog_wsl.visible = true;
    }
}
