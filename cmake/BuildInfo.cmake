# Regenerated on every build (dirty630_buildinfo target): stamps the editor's
# header line. VERSION (full major.minor.patch) and OUT are passed on the
# command line; a .0 patch is not displayed.
if(VERSION MATCHES "^([0-9]+\\.[0-9]+)\\.0$")
    set(VERSION "${CMAKE_MATCH_1}")
endif()
string(TIMESTAMP DIRTY630_BUILD_TIME "%d/%m/%Y %H:%M")
file(WRITE "${OUT}"
"#pragma once
#define DIRTY630_VERSION \"${VERSION}\"
#define DIRTY630_BUILD_TIME \"${DIRTY630_BUILD_TIME}\"
")
