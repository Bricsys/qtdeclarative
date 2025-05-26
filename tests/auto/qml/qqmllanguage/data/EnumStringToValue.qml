import QtQml
import StaticTest

QtObject {
    enum QmlEnum { A, B }

    property int p1: Qt.enumStringToValue(EnumStringToValue.QmlEnum, "B")
    property int p2: Qt.enumStringToValue(CppEnum.Scoped, "S2")
    property int p3: Qt.enumStringToValue(CppEnum.Unscoped, "U2")
    property int p4: Qt.enumStringToValue(EnumNamespace.Scoped, "S2")
    property int p5: Qt.enumStringToValue(EnumNamespace.Unscoped, "U2")


    property var p7: Qt.enumStringToValue(NOPE, "A")
    property var p8: Qt.enumStringToValue(CppEnum.Scoped, NOPE)

    property int p9: Qt.enumStringToValue(ConflictingEnums.E1, "A")
    property int p10: Qt.enumStringToValue(ConflictingEnums.E2, "A")
}
