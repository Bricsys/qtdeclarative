# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from quick.pro.

add_subdirectory(geometry)
add_subdirectory(nodes)
add_subdirectory(qquickpixmapcache)

# Limit set of tests to run for static Qt builds.
if(QT_BUILD_MINIMAL_STATIC_TESTS)
    return()
endif()
add_subdirectory(drawingmodes)

if(QT_FEATURE_private_tests)
    add_subdirectory(doc)
    add_subdirectory(examples)
    add_subdirectory(nokeywords)
    add_subdirectory(propertyrequirements)
    add_subdirectory(qquickanimations)
    add_subdirectory(qquickapplication)
    add_subdirectory(qquickbehaviors)
    add_subdirectory(qquickboundaryrule)
    add_subdirectory(qquickfontloader)
    add_subdirectory(qquickfontloader_static)
    add_subdirectory(qquickfontmetrics)
    add_subdirectory(qquickimageprovider)
    add_subdirectory(qquicklayouts)
    add_subdirectory(qquickpath)
    add_subdirectory(qquicksmoothedanimation)
    add_subdirectory(qquickspringanimation)
    add_subdirectory(qquickanimationcontroller)
    add_subdirectory(qquickstyledtext)
    add_subdirectory(qquickstates)
    add_subdirectory(qquicksystempalette)
    add_subdirectory(qquicktimeline)
    add_subdirectory(pointerhandlers)
    add_subdirectory(qquickaccessible)
    add_subdirectory(qquickanchors)
    add_subdirectory(qquickanimatedimage)
    add_subdirectory(qquickanimators)
    add_subdirectory(qquickdynamicpropertyanimation)
    add_subdirectory(qquickborderimage)
    add_subdirectory(qquickwindow)
    add_subdirectory(qquickwindowcontainer)
    add_subdirectory(qquickdeliveryagent)
    add_subdirectory(qquickdrag)
    add_subdirectory(qquickdragattached)
    add_subdirectory(qquickdroparea)
    add_subdirectory(qquickflickable)
    add_subdirectory(qquickflipable)
    add_subdirectory(qquickfocusscope)
    add_subdirectory(qquickgraphicsinfo)
    add_subdirectory(qquickgridview)
    add_subdirectory(qquickimage)
    add_subdirectory(qquickitem)
    add_subdirectory(qquickitem2)
    add_subdirectory(qquickitemlayer)
    add_subdirectory(qquicklistview)
    add_subdirectory(qquicklistview2)
    add_subdirectory(qquicktableview)
    add_subdirectory(qquicktreeview)
    add_subdirectory(qquickloader)
    add_subdirectory(qquickmousearea)
    add_subdirectory(qquickmultipointtoucharea)
    add_subdirectory(qquickpainteditem)
    add_subdirectory(qquicksafearea)
    add_subdirectory(qquickpathview)
    add_subdirectory(qquickpincharea)
    add_subdirectory(qquickpositioners)
    add_subdirectory(qquickrectangle)
    add_subdirectory(qquickrepeater)
    add_subdirectory(qquickshortcut)
    add_subdirectory(qquicktext)
    add_subdirectory(qquicktextdocument)
    add_subdirectory(qquicktextedit)
    add_subdirectory(qquicktextinput)
    add_subdirectory(qquickvisualdatamodel)
    add_subdirectory(qquickview)
    add_subdirectory(qquickview_extra)
    add_subdirectory(qquickcanvasitem)
    if(QT_FEATURE_quick_designer)
        add_subdirectory(qquickdesignersupport)
    endif()
    add_subdirectory(qquickscreen)
    add_subdirectory(touchmouse)
    add_subdirectory(scenegraph)
    add_subdirectory(sharedimage)
    add_subdirectory(qquickcolorgroup)
    add_subdirectory(qquickpalette)
    add_subdirectory(qquickrendercontrol)
    add_subdirectory(qquickshadereffect)
    add_subdirectory(qquickanimatedsprite)
    add_subdirectory(qquickspritesequence)
    add_subdirectory(qquickrhiitem)
    add_subdirectory(rendernode)

    if(QT_FEATURE_opengl)
        add_subdirectory(qquickframebufferobject)
    endif()

    add_subdirectory(softwarerenderer)
endif()
