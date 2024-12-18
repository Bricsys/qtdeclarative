pragma Strict
import QtQml

TakeNumber {
    id: foo

    function literal0() {
        foo.takeInt(0)
        foo.propertyInt = 0;
        foo.takeNegativeInt(0)
        foo.propertyNegativeInt = 0;
        foo.takeQSizeType(0)
        foo.propertyQSizeType = 0;
        foo.takeQLongLong(0)
        foo.propertyQLongLong = 0;
    }

    function literal56() {
        foo.takeInt(56)
        foo.propertyInt = 56;
        foo.takeNegativeInt(-56)
        foo.propertyNegativeInt = -56;
        foo.takeQSizeType(56)
        foo.propertyQSizeType = 56;
        foo.takeQLongLong(56)
        foo.propertyQLongLong = 56;
    }

    function variable0() {
        var a = 0
        foo.takeInt(a)
        foo.propertyInt = a;
        foo.takeNegativeInt(-a)
        foo.propertyNegativeInt = -a;
        foo.takeQSizeType(a)
        foo.propertyQSizeType = a;
        foo.takeQLongLong(a)
        foo.propertyQLongLong = a;
    }

    function variable484() {
        var a = 484
        foo.takeInt(a)
        foo.propertyInt = a;
        foo.takeNegativeInt(-a)
        foo.propertyNegativeInt = -a;
        foo.takeQSizeType(a)
        foo.propertyQSizeType = a;
        foo.takeQLongLong(a)
        foo.propertyQLongLong = a;
    }
}
