# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Limit set of tests to run for static Qt builds.
if(QT_BUILD_MINIMAL_STATIC_TESTS)
    return()
endif()

if(QT_FEATURE_private_tests)
    add_subdirectory(qquickrectangleshape)
endif()
