#!/usr/bin/env python3

import argparse
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


# =============================================================================
# Configuration
# =============================================================================

# Types directement fournis par le framework.
#
# À adapter avec les noms exacts de ton framework.
FRAMEWORK_TYPES = {
    "int8":   "system::int8_t",
    "uint8":  "system::uint8_t",
    "int16":  "system::int16_t",
    "uint16": "system::uint16_t",
    "int32":  "system::int32_t",
    "uint32": "system::uint32_t",
    "int64":  "system::int64_t",
    "uint64": "system::uint64_t",
    "float32": "system::float32_t",
    "float64": "system::float64_t",
    "bool":   "bool",
    "string": "system::string",
}


# =============================================================================
# Model
# =============================================================================

@dataclass
class Parameter:
    name: str
    xml_type: str
    cpp_type: str


@dataclass
class Event:
    name: str
    parameters: list[Parameter]


@dataclass
class Component:
    name: str
    sent_events: list[Event]
    received_events: list[Event]


# =============================================================================
# Type conversion
# =============================================================================

def cpp_type(xml_type: str, common_namespace: str) -> str:
    """
    Convert a type appearing in the .comp.xml into its C++ type.

    Framework primitive:
        int32 -> system::int32_t

    Application/common type:
        Status -> common::Status
    """

    if xml_type in FRAMEWORK_TYPES:
        return FRAMEWORK_TYPES[xml_type]

    if "::" in xml_type:
        # Already qualified.
        return xml_type

    return f"{common_namespace}::{xml_type}"


# =============================================================================
# XML parsing
# =============================================================================

def parse_parameters(
    event_node: ET.Element,
    common_namespace: str
) -> list[Parameter]:

    parameters = []

    for node in event_node:

        if not node.tag.startswith("parameter"):
            continue

        name = node.get("name")
        xml_type = node.get("type")

        if not name:
            raise RuntimeError(
                f"Missing 'name' attribute on <{node.tag}>"
            )

        if not xml_type:
            raise RuntimeError(
                f"Missing 'type' attribute on <{node.tag}>"
            )

        parameters.append(
            Parameter(
                name=name,
                xml_type=xml_type,
                cpp_type=cpp_type(
                    xml_type,
                    common_namespace
                )
            )
        )

    return parameters


def parse_event(
    node: ET.Element,
    common_namespace: str
) -> Event:

    name = node.get("name")

    if not name:
        raise RuntimeError(
            f"Missing 'name' attribute on <{node.tag}>"
        )

    return Event(
        name=name,
        parameters=parse_parameters(
            node,
            common_namespace
        )
    )


def parse_component(
    xml_file: Path,
    common_namespace: str
) -> Component:

    try:
        tree = ET.parse(xml_file)
    except ET.ParseError as error:
        raise RuntimeError(
            f"Invalid XML file '{xml_file}': {error}"
        ) from error

    root = tree.getroot()

    component_name = xml_file.name.removesuffix(
        ".comp.xml"
    )

    sent_events = []
    received_events = []

    operations = root.find(".//operations")

    if operations is None:
        raise RuntimeError(
            f"No <operations> element found in '{xml_file}'"
        )

    for node in operations:

        if node.tag == "eventSent":
            sent_events.append(
                parse_event(
                    node,
                    common_namespace
                )
            )

        elif node.tag == "eventReceive":
            received_events.append(
                parse_event(
                    node,
                    common_namespace
                )
            )

    return Component(
        name=component_name,
        sent_events=sent_events,
        received_events=received_events
    )


# =============================================================================
# C++ helpers
# =============================================================================

def cpp_parameters(event: Event) -> str:
    """
    Generate:

        const system::int32_t& param0,
        const common::Status& status
    """

    return ", ".join(
        f"const {parameter.cpp_type}& {parameter.name}"
        for parameter in event.parameters
    )


def cpp_arguments(event: Event) -> str:
    """
    Generate:

        param0, status
    """

    return ", ".join(
        parameter.name
        for parameter in event.parameters
    )


# =============================================================================
# FakeContainer generation
# =============================================================================

def generate_fake_container(component: Component) -> str:

    lines = [
        f"class {component.name}FakeContainer final",
        "    : public IContainer,",
        "      public TestContainerBase",
        "{",
        "public:"
    ]

    for event in component.sent_events:

        parameters = cpp_parameters(event)
        arguments = cpp_arguments(event)

        lines.extend([
            "",
            f"    void {event.name}_Event_Sent("
            f"{parameters}) override",
            "    {"
        ])

        if arguments:
            lines.append(
                f'        outputPorts().get("{event.name}")'
                f'.send({arguments});'
            )
        else:
            lines.append(
                f'        outputPorts().get("{event.name}")'
                f'.send();'
            )

        lines.append("    }")

    lines.append("};")

    return "\n".join(lines)


# =============================================================================
# Input registration generation
# =============================================================================

def generate_input_registration(
    component: Component
) -> str:

    lines = [
        "template<>",
        f"struct ComponentTestPorts<{component.name}>",
        "{",
        "    static void registerInputs(",
        f"        {component.name}& component,",
        "        InputPortRegistry& registry,",
        "        const std::string& instanceName)",
        "    {"
    ]

    for event in component.received_events:

        parameters = cpp_parameters(event)
        arguments = cpp_arguments(event)

        lines.extend([
            "",
            "        registry.registerPort(",
            "            instanceName,",
            f'            "{event.name}",'
        ])

        if parameters:
            lines.append(
                f"            [&component]({parameters})"
            )
        else:
            lines.append(
                "            [&component]()"
            )

        lines.append("            {")

        if arguments:
            lines.append(
                f"                component."
                f"{event.name}_Event_Receive("
                f"{arguments});"
            )
        else:
            lines.append(
                f"                component."
                f"{event.name}_Event_Receive();"
            )

        lines.extend([
            "            });"
        ])

    lines.extend([
        "    }",
        "};"
    ])

    return "\n".join(lines)


# =============================================================================
# Header generation
# =============================================================================

def generate_header(component: Component) -> str:

    return f"""\
#pragma once

// ============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT
// Component: {component.name}
// ============================================================================

#include <string>

#include "{component.name}.hpp"
#include "IContainer.hpp"
#include "TestContainerBase.hpp"
#include "InputPortRegistry.hpp"
#include "ComponentTestPorts.hpp"

namespace test::generated
{{

{generate_fake_container(component)}


{generate_input_registration(component)}

}} // namespace test::generated
"""


# =============================================================================
# Main
# =============================================================================

def main() -> int:

    parser = argparse.ArgumentParser(
        description=(
            "Generate test messaging code from a .comp.xml file"
        )
    )

    parser.add_argument(
        "--xml",
        required=True,
        type=Path,
        help="Input .comp.xml file"
    )

    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help="Generated .hpp file"
    )

    parser.add_argument(
        "--namespace",
        required=True,
        dest="common_namespace",
        help=(
            "C++ namespace used for types not provided "
            "by the framework"
        )
    )

    args = parser.parse_args()

    if not args.xml.is_file():
        print(
            f"error: file not found: {args.xml}",
            file=sys.stderr
        )
        return 1

    try:
        component = parse_component(
            args.xml,
            args.common_namespace
        )

        code = generate_header(component)

        args.output.parent.mkdir(
            parents=True,
            exist_ok=True
        )

        args.output.write_text(
            code,
            encoding="utf-8"
        )

    except RuntimeError as error:
        print(
            f"error: {error}",
            file=sys.stderr
        )
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())

# generate_test_component.py \
#    --xml External/Message/Message.comp.xml \
#    --output build/generated/External/Message.generated.hpp \
    --namespace application