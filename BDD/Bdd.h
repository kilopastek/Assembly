#pragma once

#include "StepRegistrar.h"

#define BDD_DETAIL_JOIN_IMPL(A, B) A##B
#define BDD_DETAIL_JOIN(A, B) \
    BDD_DETAIL_JOIN_IMPL(A, B)

#define BDD_STEP_IMPL(TYPE, TEXT, ID)                         \
    static void BDD_DETAIL_JOIN(                              \
        bdd_step_function_, ID)(                              \
            bdd::ScenarioContext& context,                    \
            const bdd::StepArguments& arguments);             \
                                                              \
    static bdd::StepRegistrar                                 \
        BDD_DETAIL_JOIN(                                      \
            bdd_step_registrar_, ID)(                         \
                bdd::StepType::TYPE,                          \
                TEXT,                                         \
                &BDD_DETAIL_JOIN(                             \
                    bdd_step_function_, ID));                 \
                                                              \
    static void BDD_DETAIL_JOIN(                              \
        bdd_step_function_, ID)(                              \
            bdd::ScenarioContext& context,                    \
            const bdd::StepArguments& arguments)

#define BDD_STEP_EXPAND(TYPE, TEXT, ID) \
    BDD_STEP_IMPL(TYPE, TEXT, ID)

#define BDD_GIVEN(TEXT) \
    BDD_STEP_EXPAND(Given, TEXT, __LINE__)

#define BDD_WHEN(TEXT) \
    BDD_STEP_EXPAND(When, TEXT, __LINE__)

#define BDD_THEN(TEXT) \
    BDD_STEP_EXPAND(Then, TEXT, __LINE__)
