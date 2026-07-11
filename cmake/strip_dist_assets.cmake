# Removes editor/source cruft that copy_directory drags into dist/assets:
#   *.bfxr        - bfxr project sources (only the exported .wav is needed)
#   *.DS_Store    - macOS Finder metadata
# Invoked at build time with -DDIST_ASSETS=<path> -P.
file(GLOB_RECURSE _junk
    "${DIST_ASSETS}/*.bfxr"
    "${DIST_ASSETS}/*.DS_Store"
)
if(_junk)
    file(REMOVE ${_junk})
endif()
