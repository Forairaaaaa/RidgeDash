function(ridgedash_add_dist_target target_name)
    if(RIDGEDASH_PLATFORM_IS_WEB)
        set(runtime_dir "${CMAKE_CURRENT_BINARY_DIR}/runtime")
        add_custom_target(copy_${target_name}_dist ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RIDGEDASH_ROOT}/dist"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dir}/${target_name}.html"
                "${RIDGEDASH_ROOT}/dist/index.html"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dir}/${target_name}.js"
                "${RIDGEDASH_ROOT}/dist/${target_name}.js"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dir}/${target_name}.wasm"
                "${RIDGEDASH_ROOT}/dist/${target_name}.wasm"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dir}/${target_name}.data"
                "${RIDGEDASH_ROOT}/dist/${target_name}.data"
            DEPENDS ${target_name}
            VERBATIM
        )
    else()
        add_custom_target(copy_${target_name}_dist ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RIDGEDASH_ROOT}/dist"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${target_name}>"
                "${RIDGEDASH_ROOT}/dist/${target_name}"
            COMMAND ${CMAKE_COMMAND} -E remove_directory
                "${RIDGEDASH_ROOT}/dist/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${RIDGEDASH_ROOT}/assets"
                "${RIDGEDASH_ROOT}/dist/assets"
            COMMAND ${CMAKE_COMMAND}
                -DDIST_ASSETS=${RIDGEDASH_ROOT}/dist/assets
                -P "${RIDGEDASH_ROOT}/cmake/strip_dist_assets.cmake"
            DEPENDS ${target_name}
            VERBATIM
        )
    endif()
endfunction()
