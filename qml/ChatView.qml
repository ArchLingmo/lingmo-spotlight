import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// Conversation with the AI: questions on the right, streamed answers on the left
ColumnLayout {
    id: chat

    readonly property real wantedHeight: aiChat.count > 0 || aiChat.error !== ""
                                         ? 460 : hint.implicitHeight + 32

    spacing: 0

    // Seconds since the question was sent, so a slow model visibly isn't stuck
    property int waited: 0
    Timer {
        interval: 1000
        repeat: true
        running: aiChat.busy
        onRunningChanged: if (running) chat.waited = 0
        onTriggered: chat.waited++
    }

    ListView {
        id: messages
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.topMargin: 10
        visible: aiChat.count > 0
        clip: true
        spacing: 10
        model: aiChat
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        // Follow the answer as it streams in
        onCountChanged: Qt.callLater(positionViewAtEnd)
        Connections {
            target: aiChat
            function onDataChanged() { if (messages.atYEnd || aiChat.busy) Qt.callLater(messages.positionViewAtEnd) }
        }

        delegate: Item {
            id: bubble

            required property int index
            required property string role
            required property string text
            required property bool thinking
            required property string stats
            required property bool running
            required property bool ok
            required property string requestId
            required property string title
            required property string details
            required property string confirmState
            readonly property bool mine: role === "user"
            readonly property bool isTool: role === "tool"
            readonly property bool isConfirm: role === "confirm"
            readonly property bool waiting: !mine && text === "" && aiChat.busy && index === aiChat.count - 1

            width: ListView.view.width
            // Answers keep room for "Copy" so the list doesn't jump on hover
            height: isTool ? toolRow.implicitHeight
                  : isConfirm ? confirmCard.height
                  : !card.visible ? 0
                  : card.height + (bubble.mine ? 0 : copyButton.implicitHeight + 2)

            HoverHandler { id: hover }

            // Assistente tool activity, one compact line: "Ran: lscpu"
            RowLayout {
                id: toolRow
                visible: bubble.isTool
                width: parent.width
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 7
                    Layout.preferredHeight: 7
                    Layout.leftMargin: 6
                    radius: 4
                    color: bubble.running ? LingmoUI.Theme.highlightColor
                         : bubble.ok ? LingmoUI.Theme.greenColor : LingmoUI.Theme.redColor
                    SequentialAnimation on opacity {
                        running: bubble.running
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.3; duration: 500 }
                        NumberAnimation { to: 1.0; duration: 500 }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: bubble.text
                    elide: Text.ElideMiddle
                    font.pointSize: LingmoUI.Units.fontCaption
                    color: LingmoUI.Theme.secondaryTextColor
                }
            }

            // A write the Assistente wants to make: exactly what, Allow / Decline
            Rectangle {
                id: confirmCard
                visible: bubble.isConfirm
                width: parent.width
                height: visible ? confirmColumn.implicitHeight + 24 : 0
                radius: LingmoUI.Theme.bigRadius
                color: LingmoUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.04)
                border.width: bubble.confirmState === "pending" ? 1.5 : 0
                border.color: LingmoUI.Theme.highlightColor

                ColumnLayout {
                    id: confirmColumn
                    x: 14
                    y: 12
                    width: parent.width - 28
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: bubble.title
                        font.pointSize: LingmoUI.Units.fontBody
                        font.bold: true
                        wrapMode: Text.Wrap
                        color: LingmoUI.Theme.textColor
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(detailsText.implicitHeight + 12, 150)
                        radius: LingmoUI.Theme.smallRadius
                        clip: true
                        color: LingmoUI.Theme.darkMode ? Qt.rgba(0, 0, 0, 0.25) : Qt.rgba(1, 1, 1, 0.7)
                        Flickable {
                            id: detailsFlick
                            anchors.fill: parent
                            anchors.margins: 6
                            contentHeight: detailsText.implicitHeight
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar {}
                            TextEdit {
                                id: detailsText
                                width: detailsFlick.width
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap
                                text: bubble.details
                                font.family: "monospace"
                                font.pointSize: LingmoUI.Units.fontCaption
                                color: LingmoUI.Theme.textColor
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: qsTr("Open in Assistant")
                            visible: bubble.confirmState === "pending"
                            font.pointSize: LingmoUI.Units.fontCaption
                            color: LingmoUI.Theme.highlightColor
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    aiChat.openInAssistant()
                                    spotlight.hide()
                                }
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: bubble.confirmState !== "pending"
                            text: bubble.confirmState === "allowed" ? qsTr("Allowed")
                                : bubble.confirmState === "handed" ? qsTr("Continued in the Assistant")
                                : qsTr("Declined: nothing was changed")
                            font.pointSize: LingmoUI.Units.fontCaption
                            color: bubble.confirmState === "allowed" ? LingmoUI.Theme.greenColor
                                                                     : LingmoUI.Theme.secondaryTextColor
                        }
                        Item { Layout.fillWidth: true; visible: bubble.confirmState === "pending" }
                        LingmoUI.ActionButton {
                            visible: bubble.confirmState === "pending"
                            text: qsTr("Decline")
                            onClicked: aiChat.confirm(bubble.requestId, false)
                        }
                        LingmoUI.ActionButton {
                            visible: bubble.confirmState === "pending"
                            text: qsTr("Allow")
                            primary: true
                            onClicked: aiChat.confirm(bubble.requestId, true)
                        }
                    }
                }
            }

            Rectangle {
                id: card
                // No "Thinking…" counter while the assistant waits for Allow/Decline
                visible: !bubble.isTool && !bubble.isConfirm
                         && !(bubble.text === "" && !bubble.mine && aiChat.pendingConfirmation)
                anchors.right: bubble.mine ? parent.right : undefined
                anchors.left: bubble.mine ? undefined : parent.left
                width: Math.min(body.implicitWidth + 28, bubble.width * (bubble.mine ? 0.8 : 1))
                height: body.implicitHeight + 20
                radius: LingmoUI.Theme.bigRadius
                color: bubble.mine ? LingmoUI.Theme.highlightColor
                                   : (LingmoUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.04))

                TextEdit {
                    id: body
                    x: 14
                    y: 10
                    width: Math.min(implicitWidth, bubble.width * (bubble.mine ? 0.8 : 1) - 28)
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    textFormat: bubble.mine ? TextEdit.PlainText : TextEdit.MarkdownText
                    text: bubble.text !== "" ? bubble.text
                          : bubble.waiting ? (bubble.thinking ? qsTr("Reasoning… %1 s") : qsTr("Thinking… %1 s")).arg(chat.waited)
                          : ""
                    font.pointSize: LingmoUI.Units.fontSubheading
                    color: bubble.mine ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.textColor
                    selectionColor: LingmoUI.Theme.highlightColor
                    selectedTextColor: LingmoUI.Theme.highlightedTextColor
                    // Links in answers open in the browser
                    onLinkActivated: (link) => {
                        if (/^https?:\/\//.test(link))
                            Qt.openUrlExternally(link)
                    }
                }
            }

            // How long the model took, then "Copy" on hover
            Label {
                id: statsLabel
                anchors.top: card.bottom
                anchors.left: card.left
                anchors.leftMargin: 6
                anchors.topMargin: 2
                visible: card.visible && !bubble.mine && bubble.stats !== ""
                text: bubble.stats
                font.pointSize: LingmoUI.Units.fontCaption
                color: LingmoUI.Theme.secondaryTextColor
            }
            Label {
                id: copyButton
                anchors.top: card.bottom
                anchors.left: statsLabel.visible ? statsLabel.right : card.left
                anchors.leftMargin: statsLabel.visible ? 12 : 6
                anchors.topMargin: 2
                visible: card.visible && !bubble.mine && bubble.text !== "" && hover.hovered
                         && !(aiChat.busy && bubble.index === aiChat.count - 1)
                text: copied.running ? qsTr("Copied") : qsTr("Copy")
                font.pointSize: LingmoUI.Units.fontCaption
                color: LingmoUI.Theme.highlightColor

                Timer { id: copied; interval: 1500 }
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -4
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        aiChat.copy(bubble.index)
                        copied.restart()
                    }
                }
            }
        }
    }

    // Error from the model server (not running, model missing…)
    Label {
        Layout.fillWidth: true
        Layout.margins: 16
        visible: aiChat.error !== ""
        text: aiChat.error
        wrapMode: Text.Wrap
        font.pointSize: LingmoUI.Units.fontBody
        color: LingmoUI.Theme.redColor
    }

    Item { Layout.fillHeight: true; visible: aiChat.count === 0 }

    // Keys and model
    Label {
        id: hint
        Layout.fillWidth: true
        Layout.leftMargin: 20
        Layout.rightMargin: 20
        Layout.bottomMargin: 10
        Layout.topMargin: aiChat.count > 0 ? 6 : 16
        wrapMode: Text.Wrap
        font.pointSize: LingmoUI.Units.fontCaption
        color: LingmoUI.Theme.secondaryTextColor
        text: (aiChat.count === 0 ? qsTr("Ask the AI anything. ") : "")
              + (aiChat.busy ? qsTr("Esc stops the answer") : qsTr("Enter sends · Esc leaves the chat · Ctrl+L new conversation"))
              + (aiChat.model !== "" ? "  ·  " + aiChat.model : "")
              + (aiChat.usingAssistant && aiChat.count > 0 && !aiChat.busy ? "  ·  " + qsTr("Ctrl+O opens in the Assistant") : "")
    }
}
