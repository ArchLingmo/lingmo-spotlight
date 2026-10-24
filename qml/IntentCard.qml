import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// An app action (Lingmo Intents) picked in Spotlight: what will run, with
// the values found in the typed text. Actions that send or change something
// only run on "Run" (Enter); the app checks the permission settings again.
Item {
    id: card

    // IntentProvider::pending
    property var info: ({})
    property string doneText: ""
    readonly property bool canRun: info.missing === "" && !info.running
    implicitHeight: content.implicitHeight + 32

    signal runRequested()
    signal cancelRequested()

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            spacing: 12
            Layout.fillWidth: true

            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: LingmoUI.Theme.mediumRadius
                color: card.info.sends ? LingmoUI.Theme.orangeColor : LingmoUI.Theme.highlightColor
                LingmoUI.IconItem {
                    anchors.centerIn: parent
                    width: 22
                    height: 22
                    source: card.info.icon || "system-run"
                    color: LingmoUI.Theme.highlightedTextColor
                }
            }

            ColumnLayout {
                spacing: 2
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: card.doneText !== "" ? card.doneText : card.info.title || ""
                    font.pointSize: LingmoUI.Units.fontSubheading
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    color: LingmoUI.Theme.textColor
                }

                Label {
                    Layout.fillWidth: true
                    text: card.doneText !== "" ? qsTr("Done")
                        : card.info.running ? qsTr("Running…")
                        : card.info.confirm ? (card.info.sends ? qsTr("This sends something on your behalf. Check it before running.")
                                                               : qsTr("This changes something. Check it before running."))
                        : card.info.description || ""
                    wrapMode: Text.WordWrap
                    color: card.info.confirm && card.doneText === "" ? LingmoUI.Theme.orangeColor : LingmoUI.Theme.secondaryTextColor
                }
            }
        }

        // The parameters found in the text
        Rectangle {
            Layout.fillWidth: true
            visible: (card.info.lines || []).length > 0 && card.doneText === ""
            implicitHeight: values.implicitHeight + 16
            radius: LingmoUI.Theme.mediumRadius
            color: LingmoUI.Theme.secondBackgroundColor

            GridLayout {
                id: values
                anchors.fill: parent
                anchors.margins: 8
                columns: 2
                columnSpacing: 12
                rowSpacing: 4

                Repeater {
                    model: card.info.lines || []
                    delegate: Label {
                        required property var modelData
                        required property int index
                        Layout.row: index
                        Layout.column: 0
                        text: modelData.label
                        color: LingmoUI.Theme.secondaryTextColor
                    }
                }
                Repeater {
                    model: card.info.lines || []
                    delegate: Label {
                        required property var modelData
                        required property int index
                        Layout.row: index
                        Layout.column: 1
                        Layout.fillWidth: true
                        text: modelData.value
                        elide: Text.ElideRight
                        maximumLineCount: 3
                        wrapMode: Text.WordWrap
                        color: LingmoUI.Theme.textColor
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: card.info.missing !== "" && card.info.missing !== undefined
            text: qsTr("Missing: %1. Type it too, e.g. “criar alarme 7h”.").arg(card.info.missing)
            wrapMode: Text.WordWrap
            color: LingmoUI.Theme.orangeColor
        }

        Label {
            Layout.fillWidth: true
            visible: (card.info.error || "") !== ""
            text: card.info.error || ""
            wrapMode: Text.WordWrap
            color: LingmoUI.Theme.redColor
        }

        RowLayout {
            Layout.fillWidth: true
            visible: card.doneText === ""
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: qsTr("Enter runs · Esc cancels")
                color: LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontCaption
            }
            Button {
                text: qsTr("Cancel")
                flat: true
                onClicked: card.cancelRequested()
            }
            Button {
                text: card.info.sends ? qsTr("Send") : qsTr("Run")
                highlighted: true
                enabled: card.canRun
                onClicked: card.runRequested()
            }
        }
    }
}
