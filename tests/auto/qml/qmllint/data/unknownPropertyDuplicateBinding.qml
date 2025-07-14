import QtQuick

ItemExposingIdAsAliasPropery {
    inner.states: [
      State {name: "foo"}, // qmllint disable missing-property
      State {name: "bar"}  // qmllint disable missing-property
    ]
}
