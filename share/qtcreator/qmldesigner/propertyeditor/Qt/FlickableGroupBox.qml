import Qt 4.7
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
    caption: qsTr("Flickable")
    layout: VerticalLayout {
        QWidget {  // 1
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Content size")
                }

                DoubleSpinBox {
                    text: "W"
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    backendValue: backendValues.contentWidth
                    minimum: 0;
                    maximum: 8000;
                    baseStateFlag: isBaseState;
                }

                DoubleSpinBox {
                    singleStep: 1;
                    text: "H"
                    alignRight: false
                    spacing: 4
                    backendValue: backendValues.contentHeight
                    minimum: 0;
                    maximum: 8000;
                    baseStateFlag: isBaseState;
                }


            }
        } //QWidget  //1
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Flick direction")
                    toolTip: qsTr("Flickable direction")
                }

                ComboBox {
                    baseStateFlag: isBaseState
                    items : { ["AutoFlickDirection", "HorizontalFlick", "VerticalFlick", "HorizontalAndVerticalFlick"] }
                    currentText: backendValues.flickableDirection.value;
                    onItemsChanged: {
                        currentText =  backendValues.flickableDirection.value;
                    }
                    backendValue: backendValues.flickableDirection
                }
            }
        } //QWidget
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Behavior")
                    toolTip: qsTr("Bounds behavior")
                }

                ComboBox {
                    baseStateFlag: isBaseState
                    items : { ["StopAtBounds", "DragOverBounds", "DragAndOvershootBounds"] }
                    currentText: backendValues.boundsBehavior.value;
                    onItemsChanged: {
                        currentText =  backendValues.boundsBehavior.value;
                    }
                    backendValue: backendValues.boundsBehavior
                }
            }
        } //QWidget
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text:qsTr("Interactive")
                }
                CheckBox {
                    text: ""
                    backendValue: backendValues.interactive;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
            }
        }// QWidget
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Max. velocity")
                    toolTip: qsTr("Maximum flick velocity")
                }

                DoubleSpinBox {
                    text: ""
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    backendValue: backendValues.maximumFlickVelocity
                    minimum: 0;
                    maximum: 8000;
                    baseStateFlag: isBaseState;
                }
            }
        } //QWidget
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Deceleration")
                    toolTip: qsTr("Flick deceleration")
                }

                DoubleSpinBox {
                    text: ""
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    backendValue: backendValues.flickDeceleration
                    minimum: 0;
                    maximum: 8000;
                    baseStateFlag: isBaseState;
                }
            }
        } //QWidget
    }
}
