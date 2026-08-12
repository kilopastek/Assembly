#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass, field
from pathlib import Path


PRIMARY_KEYWORDS = ("Given", "When", "Then")
CONTINUATION_KEYWORDS = ("And", "But")
ALL_KEYWORDS = PRIMARY_KEYWORDS + CONTINUATION_KEYWORDS


@dataclass
class Step:
    keyword: str
    effective_keyword: str
    text: str
    line: int


@dataclass
class Scenario:
    name: str
    line: int
    tags: list[str] = field(default_factory=list)
    steps: list[Step] = field(default_factory=list)


@dataclass
class Feature:
    name: str
    scenarios: list[Scenario] = field(default_factory=list)


def sanitize_identifier(value: str) -> str:
    value = re.sub(r"[^A-Za-z0-9_]+", "_", value.strip())
    value = re.sub(r"_+", "_", value).strip("_")

    if not value:
        value = "Unnamed"

    if value[0].isdigit():
        value = "_" + value

    return value


def cpp_string(value: str) -> str:
    return (
        value
        .replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
    )


def parse_feature(path: Path) -> Feature:
    feature: Feature | None = None
    current_scenario: Scenario | None = None
    pending_tags: list[str] = []
    last_primary_keyword: str | None = None

    lines = path.read_text(encoding="utf-8").splitlines()

    for line_no, raw_line in enumerate(lines, 1):
        line = raw_line.strip()

        if not line:
            continue

        if line.startswith("#"):
            continue

        if line.startswith("@"):
            pending_tags.extend(line.split())
            continue

        if line.startswith("Feature:"):
            if feature is not None:
                raise ValueError(
                    f"{path}:{line_no}: "
                    "plusieurs Feature dans le même fichier"
                )

            feature = Feature(
                name=line.split(":", 1)[1].strip()
            )

            current_scenario = None
            last_primary_keyword = None
            continue

        if line.startswith("Scenario:"):
            if feature is None:
                raise ValueError(
                    f"{path}:{line_no}: "
                    "Scenario déclaré avant Feature"
                )

            scenario_name = (
                line.split(":", 1)[1].strip()
            )

            if not scenario_name:
                raise ValueError(
                    f"{path}:{line_no}: "
                    "nom de Scenario vide"
                )

            current_scenario = Scenario(
                name=scenario_name,
                line=line_no,
                tags=pending_tags,
            )

            pending_tags = []
            last_primary_keyword = None

            feature.scenarios.append(current_scenario)

            continue

        matched_keyword: str | None = None
        step_text: str | None = None

        for keyword in ALL_KEYWORDS:
            prefix = keyword + " "

            if line.startswith(prefix):
                matched_keyword = keyword
                step_text = line[len(prefix):].strip()
                break

        if matched_keyword is None:
            raise ValueError(
                f"{path}:{line_no}: "
                f"syntaxe non prise en charge : {line}"
            )

        if current_scenario is None:
            raise ValueError(
                f"{path}:{line_no}: "
                "step déclaré hors d'un Scenario"
            )

        if not step_text:
            raise ValueError(
                f"{path}:{line_no}: "
                f"{matched_keyword} sans texte"
            )

        if matched_keyword in PRIMARY_KEYWORDS:
            effective_keyword = matched_keyword
            last_primary_keyword = matched_keyword

        else:
            if last_primary_keyword is None:
                raise ValueError(
                    f"{path}:{line_no}: "
                    f"{matched_keyword} sans "
                    "Given/When/Then précédent"
                )

            effective_keyword = last_primary_keyword

        current_scenario.steps.append(
            Step(
                keyword=matched_keyword,
                effective_keyword=effective_keyword,
                text=step_text,
                line=line_no,
            )
        )

    if feature is None:
        raise ValueError(
            f"{path}: aucune Feature trouvée"
        )

    if not feature.scenarios:
        raise ValueError(
            f"{path}: aucun Scenario trouvé"
        )

    return feature


def generate_cpp(
    feature_path: Path,
    feature: Feature,
) -> str:
    feature_id = sanitize_identifier(feature.name)

    out: list[str] = []

    out.append("// Fichier généré automatiquement.\n")
    out.append("// Ne pas modifier manuellement.\n\n")
    out.append("#include <gtest/gtest.h>\n")
    out.append('#include "bdd/ScenarioRunner.hpp"\n\n')
    out.append("namespace generated_bdd {\n\n")

    for scenario in feature.scenarios:
        scenario_id = sanitize_identifier(scenario.name)

        out.append(f"TEST({feature_id}, {scenario_id})\n")
        out.append("{\n")
        out.append("    bdd::ScenarioContext context;\n")
        out.append("    auto& runner = bdd::ScenarioRunner::instance();\n\n")

        requirement_tags = [
            tag[1:]
            for tag in scenario.tags
            if tag.startswith("@REQ-")
        ]

        for requirement in requirement_tags:
            out.append(
                "    ::testing::Test::RecordProperty("
                f'"Requirement", '
                f'"{cpp_string(requirement)}");\n'
            )

        if requirement_tags:
            out.append("\n")

        for step in scenario.steps:
            trace_text = (f"{step.keyword} {step.text}")

            out.append(
                f'    SCOPED_TRACE('
                f'"{cpp_string(trace_text)}");\n'
            )

            out.append("    runner.runStep(\n")
            out.append("        context,\n")
            out.append(
                "        bdd::StepType::"
                f"{step.effective_keyword},\n"
            )
            out.append(
                f'        "{cpp_string(step.text)}");'
                f"  // {feature_path.name}:{step.line}\n"
            )
            out.append("\n")

        out.append("}\n\n")

    out.append("}  // namespace generated_bdd\n")

    return "".join(out)


def write_if_different(
    output_file: Path,
    content: str,
) -> bool:
    if output_file.exists():
        current = output_file.read_text(
            encoding="utf-8"
        )

        if current == content:
            return False

    output_file.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    output_file.write_text(
        content,
        encoding="utf-8",
    )

    return True


def generation_required(
    feature_file: Path,
    output_file: Path,
) -> bool:
    if not output_file.exists():
        return True

    return (feature_file.stat().st_mtime > output_file.stat().st_mtime)


def generate_feature(
    feature_file: Path,
    output_file: Path,
    force: bool = False,
) -> bool:
    if (not force and not generation_required(feature_file, output_file)):
        print( f"[BDD] à jour : {output_file}")
        return False

    feature = parse_feature(feature_file)
    generated = generate_cpp(feature_file, feature)
    changed = write_if_different(output_file, generated)

    if changed:
        print(f"[BDD] généré : {output_file}")
    else:
        print(f"[BDD] inchangé : {output_file}")

    return changed


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Génère un test GoogleTest "
            "depuis un fichier Gherkin."
        )
    )

    parser.add_argument(
        "feature",
        type=Path,
        help="Fichier .feature d'entrée",
    )

    parser.add_argument(
        "--cpp-out",
        required=True,
        type=Path,
        help="Fichier .generated.cpp de sortie",
    )

    parser.add_argument(
        "--force",
        action="store_true",
        help="Force la régénération",
    )

    args = parser.parse_args()

    feature_file: Path = args.feature
    output_file: Path = args.cpp_out

    if not feature_file.exists():
        print(
            f"Erreur : fichier introuvable : "
            f"{feature_file}"
        )
        return 1

    if not feature_file.is_file():
        print(
            f"Erreur : ce n'est pas un fichier : "
            f"{feature_file}"
        )
        return 1

    try:
        generate_feature(
            feature_file=feature_file,
            output_file=output_file,
            force=args.force,
        )

    except Exception as exception:
        print(f"Erreur BDD : {exception}")
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())