# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from auto.pro.

add_subdirectory(accessibility)
if(NOT ANDROID) # temporarily disabled until QTBUG-100991 is fixed
add_subdirectory(controls)
endif()
add_subdirectory(cursor)
if(BUILD_SHARED_LIBS) # serious performance issues when built statically QTBUG-133755
add_subdirectory(customization)
endif()
add_subdirectory(deferred)
if(QT_FEATURE_quick_designer)
    add_subdirectory(designer)
endif()
if(NOT ANDROID) # QTBUG-100258
add_subdirectory(focus)
endif()
add_subdirectory(font)
add_subdirectory(palette)
add_subdirectory(platform)
add_subdirectory(pointerhandlers)
add_subdirectory(pressandhold)
add_subdirectory(qquickapplicationwindow)
add_subdirectory(qquickcontextmenu)
add_subdirectory(qquickcontrol)
add_subdirectory(qquickcolor)
add_subdirectory(qquickcontainer)
add_subdirectory(qquickdrawer)
add_subdirectory(qquickheaderview)
add_subdirectory(qquickiconimage)
add_subdirectory(qquickiconlabel)
add_subdirectory(qquickimaginestyle)
if (QT_FEATURE_quickcontrols2_material)
    add_subdirectory(qquickmaterialstyle)
    add_subdirectory(qquickmaterialstyleconf)
endif()
add_subdirectory(qquickmenu)
add_subdirectory(qquickmenubar)
add_subdirectory(qquickninepatchimage)
add_subdirectory(qquickoverlay)
add_subdirectory(qquickpopup)
add_subdirectory(qquickstyle)
add_subdirectory(qquicktextarea)
add_subdirectory(qquicktextfield)
add_subdirectory(qquickuniversalstyle)
add_subdirectory(qquickuniversalstyleconf)
add_subdirectory(revisions)
if(NOT ANDROID) # QTBUG-100191
add_subdirectory(sanity)
add_subdirectory(snippets)
endif()
add_subdirectory(styleimports)
add_subdirectory(styleimportscompiletimematerial)
add_subdirectory(styleimportscompiletimeqmlonly)
add_subdirectory(translation)

if (QT_FEATURE_quick_tableview)
    add_subdirectory(qquicktableviewdelegate)
    add_subdirectory(qquickheaderviewdelegate) # inherits QQuickTableViewDelegate
endif()

if (QT_FEATURE_quick_treeview)
    add_subdirectory(qquicktreeviewdelegate)
endif()

