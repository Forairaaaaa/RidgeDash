function(ridgedash_configure_3ds_assets target_name)
    find_program(TEX3DS_EXECUTABLE tex3ds REQUIRED)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    set(sprite_manifest "${RIDGEDASH_ROOT}/packaging/3ds/sprites.t3s")
    set(romfs_dir "${CMAKE_CURRENT_BINARY_DIR}/romfs")
    set(sprite_sheet "${romfs_dir}/gfx/sprites.t3x")
    set(font_png "${CMAKE_CURRENT_BINARY_DIR}/generated/default_font.png")
    set(font_manifest "${CMAKE_CURRENT_BINARY_DIR}/generated/font.t3s")
    set(font_sheet "${romfs_dir}/gfx/font.t3x")
    set(audio_stamp "${romfs_dir}/audio/.stamp")
    file(GLOB sprite_sources CONFIGURE_DEPENDS "${RIDGEDASH_ROOT}/assets/sprites/*.png")
    file(GLOB sfx_sources CONFIGURE_DEPENDS "${RIDGEDASH_ROOT}/assets/audio/sfx/*.wav")
    file(MAKE_DIRECTORY "${romfs_dir}/gfx")
    file(REMOVE_RECURSE "${romfs_dir}/audio/music")

    set(RIDGEDASH_3DS_FONT_PNG "${font_png}")
    configure_file("${RIDGEDASH_ROOT}/packaging/3ds/font.t3s.in" "${font_manifest}" @ONLY)

    add_custom_command(
        OUTPUT "${font_png}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
        COMMAND "${Python3_EXECUTABLE}"
                "${RIDGEDASH_ROOT}/packaging/3ds/generate_default_font.py"
                "${RIDGEDASH_ROOT}/dependencies/raylib/src/rtext.c"
                "${font_png}"
        DEPENDS
            "${RIDGEDASH_ROOT}/packaging/3ds/generate_default_font.py"
            "${RIDGEDASH_ROOT}/dependencies/raylib/src/rtext.c"
        COMMENT "Generating the raylib default font texture"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${font_sheet}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${romfs_dir}/gfx"
        COMMAND "${TEX3DS_EXECUTABLE}" -i "${font_manifest}" -o "${font_sheet}"
        DEPENDS "${font_manifest}" "${font_png}"
        COMMENT "Packing the raylib default font for Citro2D"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${sprite_sheet}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${romfs_dir}/gfx"
        COMMAND "${TEX3DS_EXECUTABLE}" -i "${sprite_manifest}" -o "${sprite_sheet}"
        DEPENDS "${sprite_manifest}" ${sprite_sources}
        WORKING_DIRECTORY "${RIDGEDASH_ROOT}/packaging/3ds"
        COMMENT "Packing RidgeDash sprites for Citro2D"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${audio_stamp}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${romfs_dir}/audio/sfx"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${RIDGEDASH_ROOT}/assets/audio/engine_loop.wav"
                "${romfs_dir}/audio/engine_loop.wav"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${sfx_sources} "${romfs_dir}/audio/sfx"
        COMMAND ${CMAKE_COMMAND} -E touch "${audio_stamp}"
        DEPENDS "${RIDGEDASH_ROOT}/assets/audio/engine_loop.wav" ${sfx_sources}
        COMMENT "Copying 3DS WAV audio into RomFS"
        VERBATIM
    )

    add_custom_target(${target_name}_3ds_assets DEPENDS "${sprite_sheet}" "${font_sheet}" "${audio_stamp}")
    add_dependencies(${target_name} ${target_name}_3ds_assets)

    set(smdh_file "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.smdh")
    set(three_dsx_file "${CMAKE_CURRENT_BINARY_DIR}/runtime/${target_name}.3dsx")
    ctr_generate_smdh("${smdh_file}"
        NAME "RidgeDash"
        DESCRIPTION "A tiny endless off-road driving game"
        AUTHOR "Forairaaaaa"
        ICON "${RIDGEDASH_ROOT}/packaging/3ds/icon.png"
    )
    ctr_create_3dsx(${target_name}
        OUTPUT "${three_dsx_file}"
        ROMFS "${romfs_dir}"
        SMDH "${smdh_file}"
    )
endfunction()
