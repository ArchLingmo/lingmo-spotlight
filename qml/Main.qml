import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

Window {
    id: root

    readonly property int rowHeight: 52
    readonly property int sectionHeight: 28
    readonly property int searchHeight: 64
    readonly property int maxListHeight: 440
    readonly property bool hasResults: resultsView.count > 0 && !chatMode
    // "ai: …" / "ia: …" or a picked "Ask the AI" result: talk to the AI in place of searching
    property bool chatMode: false
    // Arrow keys or the mouse picked a row since the last keystroke: keep it
    property bool userNavigated: false
    property var lastPointer

    width: 700
    height: searchHeight + (chatMode ? chatView.wantedHeight + 1
                                     : hasResults ? Math.min(resultsView.contentHeight + 12, maxListHeight) + 1 : 0)
    visible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool

    Behavior on height {
        NumberAnimation { duration: LingmoUI.Units.veryShortDuration; easing.type: LingmoUI.Units.easing }
    }

    // Called by Spotlight::show() before the window appears
    function reset() {
        // A confirmation still waiting: reopen on it
        chatMode = aiChat.pendingConfirmation
        lastPointer = undefined
        searchField.text = ""
        resultsView.currentIndex = 0
        searchField.forceActiveFocus()
    }

    function enterChat(question) {
        chatMode = true
        searchField.text = ""
        searchEngine.query = ""
        if (question)
            aiChat.ask(question)
        searchField.forceActiveFocus()
    }

    function leaveChat() {
        chatMode = false
        searchField.text = ""
    }

    function submit() {
        if (!chatMode) {
            activateCurrent()
            return
        }
        if (searchField.text.trim() !== "" && !aiChat.busy) {
            aiChat.ask(searchField.text)
            searchField.text = ""
        }
    }

    Connections {
        target: searchEngine
        function onChatRequested(question) { root.enterChat(question) }
    }

    // Results arrive in steps (files and contents later): the best one stays selected
    Connections {
        target: searchEngine.results
        function onModelReset() {
            if (!root.userNavigated)
                resultsView.currentIndex = 0
        }
    }

    function activateCurrent() {
        if (resultsView.currentIndex >= 0 && searchEngine.activate(resultsView.currentIndex))
            spotlight.hide()
    }

    // Clicking anywhere else closes it, like macOS; not while the assistant
    // waits for Allow/Decline, so the question isn't lost
    onActiveChanged: if (!active && visible && !aiChat.pendingConfirmation) spotlight.hide()

    LingmoUI.WindowHelper { id: windowHelper }

    LingmoUI.WindowShadow {
        view: root
        geometry: Qt.rect(root.x, root.y, root.width, root.height)
        radius: background.radius
    }

    // "Vidro Lingmo" (Settings > Appearance > Glass): fills the window and asks
    // KWin for the blur; solid with Glass off or without a compositor
    LingmoUI.GlassSurface {
        id: background
        anchors.fill: parent
        radius: 16
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Search field
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.searchHeight
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 12

            LingmoUI.IconItem {
                Layout.preferredWidth: 26
                Layout.preferredHeight: 26
                visible: !root.chatMode || aiChat.providerIcon !== ""
                source: root.chatMode ? aiChat.providerIcon : "system-search"
                opacity: root.chatMode ? 1.0 : 0.7
            }

            // Chat mode tag, like a search scope
            Rectangle {
                visible: root.chatMode
                Layout.preferredHeight: 26
                Layout.preferredWidth: aiTag.implicitWidth + 16
                radius: LingmoUI.Theme.smallRadius
                color: LingmoUI.Theme.highlightColor
                Label {
                    id: aiTag
                    anchors.centerIn: parent
                    text: qsTr("AI")
                    font.pointSize: LingmoUI.Units.fontBody
                    font.bold: true
                    color: LingmoUI.Theme.highlightedTextColor
                }
            }

            TextField {
                id: searchField
                Layout.fillWidth: true
                font.pointSize: LingmoUI.Units.fontTitle
                placeholderText: root.chatMode ? qsTr("Ask the AI…") : qsTr("Spotlight Search")
                color: LingmoUI.Theme.textColor
                placeholderTextColor: LingmoUI.Theme.secondaryTextColor
                selectByMouse: true
                background: Item {}

                onTextChanged: {
                    if (root.chatMode) {
                        // The space typed after "ai:" isn't part of the question
                        if (/^\s+$/.test(text))
                            text = ""
                        return
                    }
                    const prefix = /^\s*(ai|ia)\s*:\s*/i
                    if (prefix.test(text)) {
                        const question = text.replace(prefix, "")
                        root.enterChat("")
                        text = question
                        return
                    }
                    root.userNavigated = false
                    searchEngine.query = text
                    resultsView.currentIndex = 0
                }

                Keys.onDownPressed: if (!root.chatMode) { root.userNavigated = true; resultsView.incrementCurrentIndex() }
                Keys.onUpPressed: if (!root.chatMode) { root.userNavigated = true; resultsView.decrementCurrentIndex() }
                Keys.onReturnPressed: root.submit()
                Keys.onEnterPressed: root.submit()
                Keys.onEscapePressed: {
                    if (root.chatMode && aiChat.busy)
                        aiChat.stop()
                    else if (root.chatMode)
                        root.leaveChat()
                    else
                        spotlight.hide()
                }
                Keys.onPressed: (event) => {
                    // Backspace on an empty field leaves the chat, like removing a search scope
                    if (root.chatMode && event.key === Qt.Key_Backspace && text === "") {
                        root.leaveChat()
                        event.accepted = true
                    } else if (root.chatMode && event.key === Qt.Key_L && (event.modifiers & Qt.ControlModifier)) {
                        aiChat.clear()
                        event.accepted = true
                    } else if (root.chatMode && event.key === Qt.Key_O && (event.modifiers & Qt.ControlModifier)
                               && aiChat.usingAssistant) {
                        // Continue this conversation in the Assistente app
                        aiChat.openInAssistant()
                        spotlight.hide()
                        event.accepted = true
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            visible: root.hasResults || root.chatMode
            color: LingmoUI.Theme.dividerColor
        }

        ListView {
            id: resultsView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6
            visible: root.hasResults
            clip: true
            model: searchEngine.results
            currentIndex: 0
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds
            keyNavigationWraps: true

            section.property: "category"
            section.delegate: Label {
                width: ListView.view.width
                height: root.sectionHeight
                leftPadding: 14
                verticalAlignment: Text.AlignBottom
                bottomPadding: 4
                text: section
                font.pointSize: LingmoUI.Units.fontCaption
                font.bold: true
                color: LingmoUI.Theme.secondaryTextColor
            }

            highlight: Rectangle {
                radius: LingmoUI.Theme.mediumRadius
                color: LingmoUI.Theme.highlightColor
            }

            delegate: ResultDelegate {
                width: ListView.view.width
                height: root.rowHeight
                selected: ListView.isCurrentItem
                onClicked: {
                    resultsView.currentIndex = index
                    root.activateCurrent()
                }
                onHovered: (globalPos) => {
                    const last = root.lastPointer
                    if (last !== undefined && last.x === globalPos.x && last.y === globalPos.y)
                        return
                    const moved = last !== undefined
                    root.lastPointer = globalPos
                    if (!moved)
                        return  // first sighting of the pointer: it hasn't moved yet
                    root.userNavigated = true
                    resultsView.currentIndex = index
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }

        ChatView {
            id: chatView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.chatMode
        }
    }
}
