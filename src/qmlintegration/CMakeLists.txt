# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(QmlIntegration
    HEADER_MODULE
    NO_PRIVATE_MODULE
    SOURCES
        qqmlintegration.h
    LIBRARIES
        Qt::Core
    NO_GENERATE_CPP_EXPORTS
)
