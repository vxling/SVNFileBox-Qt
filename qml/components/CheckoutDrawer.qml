import QtQuick
import QtQuick.Controls

Drawer {
    id: checkoutDrawer
    edge: Qt.RightEdge
    width: parent.width * 0.5
    height: parent.height - 36
    y: 0
    position: 0

    CheckoutDrawerContent {
        anchors.fill: parent
        onConfirmClicked: doCheckout()
        onCancelClicked: checkoutDrawer.close()
    }
}
