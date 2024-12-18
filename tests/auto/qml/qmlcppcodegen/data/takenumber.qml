pragma Strict
import QtQml

TakeNumber {
    id: foo

    function literal0() {
        foo.takeInt(0)
        foo.takeNegativeInt(0)
        foo.takeQSizeType(0)
        foo.takeQLongLong(0)
    }

    function literal56() {
        foo.takeInt(56)
        foo.takeNegativeInt(-56)
        foo.takeQSizeType(56)
        foo.takeQLongLong(56)
    }

    function variable0() {
        var a = 0
        foo.takeInt(a)
        foo.takeNegativeInt(-a)
        foo.takeQSizeType(a)
        foo.takeQLongLong(a)
    }

    function variable484() {
        var a = 484
        foo.takeInt(a)
        foo.takeNegativeInt(-a)
        foo.takeQSizeType(a)
        foo.takeQLongLong(a)
    }
}
