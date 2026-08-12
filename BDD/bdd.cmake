function(bdd_add_test)

    set(options "")
    set(oneValueArgs TARGET)
    set(multiValueArgs FEATURES SOURCES)

    cmake_parse_arguments(
        BDD
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # ---------------------------------------------------------
    # Vérification des arguments
    # ---------------------------------------------------------

    if(NOT BDD_TARGET)
        message(FATAL_ERROR "bdd_add_test: TARGET manquant")
    endif()

    if(NOT BDD_FEATURES)
        message(FATAL_ERROR "bdd_add_test: aucune FEATURE fournie")
    endif()

    if(NOT BDD_SOURCES)
        message(WARNING "bdd_add_test: aucun fichier de steps fourni pour ${BDD_TARGET}")
    endif()

    # ---------------------------------------------------------
    # Répertoire du code généré
    # ---------------------------------------------------------

    set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/bdd_generated/${BDD_TARGET}")

    # ---------------------------------------------------------
    # Génération des .cpp depuis les .feature
    # ---------------------------------------------------------

    set(generated_sources "")

    foreach(feature IN LISTS BDD_FEATURES)

        cmake_path(
            ABSOLUTE_PATH feature
            BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE feature_file
        )

        if(NOT EXISTS "${feature_file}")
            message(FATAL_ERROR "Feature BDD introuvable : ${feature_file}")
        endif()

        cmake_path(
            REPLACE_EXTENSION feature
            ".generated.cpp"
            OUTPUT_VARIABLE relative_generated
        )

        set(generated_cpp "${generated_dir}/${relative_generated}")

        add_custom_command(
            OUTPUT
                "${generated_cpp}"

            COMMAND
                "${CMAKE_COMMAND}" -E make_directory
                "${generated_dir}"

            COMMAND
                "${Python3_EXECUTABLE}"
                "${BDD_GENERATOR}"
                "${feature_file}"
                --cpp-out "${generated_cpp}"

            DEPENDS
                "${feature_file}"
                "${BDD_GENERATOR}"

            COMMENT
                "Generation BDD : ${feature}"

            VERBATIM
        )

        list(APPEND generated_sources "${generated_cpp}")

    endforeach()

    # ---------------------------------------------------------
    # Création de l'exécutable
    # ---------------------------------------------------------

    add_executable(
        "${BDD_TARGET}"
        ${BDD_SOURCES}
        ${generated_sources}
    )

    # ---------------------------------------------------------
    # Dépendances du framework BDD
    # ---------------------------------------------------------

    target_link_libraries(
        "${BDD_TARGET}"
        PRIVATE
            bdd_runtime
            GTest::gtest
    )

    # ---------------------------------------------------------
    # Découverte GoogleTest
    # ---------------------------------------------------------

    gtest_discover_tests("${BDD_TARGET}")

endfunction()
