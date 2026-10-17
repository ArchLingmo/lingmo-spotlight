import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

Item {
    id: control

    property bool selected: false
    signal clicked()
    // Global pointer position, so the list can tell a real move from a synthetic one
    signal hovered(point globalPos)

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: control.clicked()
        // Only follow the mouse when it actually moves: Qt also sends a move when
        // the list grows under a resting pointer, which mustn't steal the selection
        onPositionChanged: (mouse) => control.hovered(mapToGlobal(mouse.x, mouse.y))
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 16
        spacing: 12

        LingmoUI.IconItem {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            source: iconName
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                Layout.fillWidth: true
                text: title
                elide: Text.ElideRight
                font.pointSize: LingmoUI.Units.fontSubheading
                color: control.selected ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.textColor
            }

            Label {
                Layout.fillWidth: true
                text: subtitle
                visible: text !== ""
                elide: Text.ElideMiddle
                font.pointSize: LingmoUI.Units.fontCaption
                color: control.selected ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.secondaryTextColor
                opacity: control.selected ? 0.85 : 1.0
            }
        }

        Label {
            text: "↵"
            visible: control.selected
            font.pointSize: LingmoUI.Units.fontSubheading
            color: LingmoUI.Theme.highlightedTextColor
        }
    }
}
