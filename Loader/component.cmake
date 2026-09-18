# add_library(test_components
#    src/ComponentFactory.cpp
#    src/ComponentStore.cpp
#)

# add_test_components(
#    test_components

#    External/Message
#    External/Command
#    Superviser/Status
#)

function(add_test_components target)

    foreach(component_path IN LISTS ARGN)

        get_filename_component(
            component_name
            "${component_path}"
            NAME
        )

        set(component_xml
            "${COMPONENTS_ROOT}/${component_path}/${component_name}.comp.xml"
        )

        set(generated_dir
            "${CMAKE_CURRENT_BINARY_DIR}/generated/${component_path}"
        )

        set(generated_header
            "${generated_dir}/${component_name}.generated.hpp"
        )

        set(component_impl_xml
            "${COMPONENTS_ROOT}/${component_path}/${component_name}/CPP/${component_name}.comp.impl.xml"
        )

        add_custom_command(
            OUTPUT "${generated_header}"

            COMMAND
                ${CMAKE_COMMAND} -E make_directory
                "${generated_dir}"

            COMMAND
                ${Python3_EXECUTABLE}
                "${TEST_COMPONENT_GENERATOR}"
                --xml "${component_xml}"
                --output "${generated_header}"

            DEPENDS
                "${component_xml}"
                "${component_impl_xml}"
                "${TEST_COMPONENT_GENERATOR}"

            COMMENT
                "Generating test bindings for ${component_name}"
        )

        target_sources(${target} PRIVATE
            "${generated_header}"
        )

    endforeach()

endfunction()
