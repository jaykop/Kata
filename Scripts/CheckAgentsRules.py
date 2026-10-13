"""AGENTS.md의 모듈 경계·싱글플레이 범위 규칙 가운데 빌드 없이 텍스트로 판정할 수 있는 것을 검사한다.

`Plugins/*/*.uplugin`의 플러그인 의존, `*.Build.cs`의 모듈 의존, Kata 플러그인 C++ 소스의 복제·RPC 선언을 읽어
AGENTS.md "모듈 경계"·"프로젝트와 현재 범위" 항목과 대조한다. 엔진 없이 실행되므로 GitHub 호스트 러너의 CI에서 쓴다.
규칙 목록, 판정 방식과 한계는 docs/manual/Agents-Rules-Check.md를 따른다.

파일을 읽기만 하며 Python 표준 라이브러리만 사용한다. 위반이 없으면 0, 위반이 있으면 1,
규칙 표가 전제하는 플러그인 구성이 저장소와 맞지 않으면 2로 종료한다.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

CORE_PLUGIN = "Kata"
SATELLITE_PLUGINS = ("KataTargeting", "KataAI", "KataCamera")
INTEGRATION_PLUGIN = "KataFramework"
INSPECTOR_PLUGIN = "KataGASInspector"
# 규칙을 적용하는 플러그인이다. 벤더 플러그인은 원문을 유지하므로 검사하지 않고 참조 대상으로만 판정한다.
KATA_PLUGINS = (CORE_PLUGIN, *SATELLITE_PLUGINS, INTEGRATION_PLUGIN, INSPECTOR_PLUGIN)

# 위성 간에 허용하는 의존(참조하는 쪽, 참조되는 쪽)이다. AGENTS.md "모듈 경계"가 정한 KataAI → KataTargeting만 둔다.
ALLOWED_SATELLITE_DEPENDENCIES = {("KataAI", "KataTargeting")}

# 코어 .uplugin이 의존할 수 있는 플러그인이다. 코어는 엔진 모듈과 GAS에만 의존한다.
CORE_ALLOWED_PLUGINS = {"GameplayAbilities"}

# 샘플 프로젝트에서만 활성화하는 플러그인이다. 엔진 플러그인은 저장소에 없으므로 플러그인·모듈 이름의 접두사로 판정한다.
SAMPLE_ONLY_PREFIXES = ("WindowsDualsense", "ChaosClothAsset", "SlateIM")

RUNTIME_MODULE_TYPES = {
    "Runtime", "RuntimeNoCommandlet", "RuntimeAndProgram", "CookedOnly",
    "ClientOnly", "ClientOnlyNoCommandlet", "ServerOnly",
}
EDITOR_MODULE_TYPES = {"Editor", "EditorNoCommandlet", "EditorAndProgram", "UncookedOnly"}

# 이름 규칙(EDITOR_NAME_RE)으로 잡히지 않는 엔진 에디터 전용 모듈이다. 전체 목록이 아니므로
# 런타임 모듈이 실수로 쓸 만한 에디터 모듈을 새로 발견하면 여기에 더한다.
ENGINE_EDITOR_MODULES = frozenset({
    "AnimGraph", "AnimationModifiers", "AssetDefinition", "AssetTools", "BlueprintGraph", "Blutility",
    "ClassViewer", "ContentBrowser", "ContentBrowserData", "DataValidation", "DetailCustomizations",
    "Kismet", "KismetCompiler", "KismetWidgets", "MainFrame", "MovieSceneTools", "Persona",
    "PlacementMode", "SceneOutliner", "Sequencer", "StructViewer", "UnrealEd", "WorkspaceMenuStructure",
})
# EditorFramework·EditorSubsystem처럼 Editor로 시작하거나 PropertyEditor·StateTreeEditorModule처럼 Editor로 끝나는 모듈이다.
EDITOR_NAME_RE = re.compile(r"^Editor|Editor(?:Module)?$")
IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")

CPP_SUFFIXES = {".h", ".hpp", ".cpp", ".inl"}

# UHT 지정자는 대소문자를 구분하지 않으므로 소문자로 비교한다.
REPLICATED_PROPERTY_SPECIFIERS = {"replicated", "replicatedusing"}
RPC_FUNCTION_SPECIFIERS = {
    "server", "client", "netmulticast", "reliable", "unreliable", "withvalidation",
    "servicerequest", "serviceresponse",
}
REPLICATION_CODE_PATTERNS = (
    (re.compile(r"\bGetLifetimeReplicatedProps\b"), "GetLifetimeReplicatedProps"),
    (re.compile(r"\bDOREPLIFETIME\w*"), "DOREPLIFETIME"),
    (re.compile(r"\bbReplicates\s*=\s*true\b"), "bReplicates = true"),
    (re.compile(r"\bbReplicateMovement\s*=\s*true\b"), "bReplicateMovement = true"),
    (re.compile(r"\bSetIsReplicated(?:ByDefault)?\s*\(\s*true\s*\)"), "SetIsReplicated(true)"),
    (re.compile(r"\bSetReplicates\s*\(\s*true\s*\)"), "SetReplicates(true)"),
    (re.compile(r"\bSetReplicateMovement\s*\(\s*true\s*\)"), "SetReplicateMovement(true)"),
)
REFLECTION_MACRO_RE = re.compile(r"\b(UPROPERTY|UFUNCTION)\s*\(")


@dataclass(frozen=True)
class Rule:
    section: str
    summary: str


# 출력하는 AGENTS.md 근거다. section은 AGENTS.md의 항목 제목을 그대로 쓴다.
RULES = {
    "core-no-satellite": Rule(
        "모듈 경계",
        "The core Kata plugin must not reference satellite (KataTargeting, KataAI, KataCamera) "
        "or integration (KataFramework) plugins."),
    "satellite-deps": Rule(
        "모듈 경계",
        "Satellites may depend on each other only as KataAI -> KataTargeting and must not reference KataFramework."),
    "core-plugin-allowlist": Rule(
        "모듈 경계",
        "The core plugin depends only on the engine and GAS; its .uplugin may list only GameplayAbilities."),
    "runtime-no-editor": Rule(
        "모듈 경계",
        "Runtime modules must not depend on editor-only modules (UnrealEd, AssetTools, Kata Editor/UncookedOnly "
        "modules), even inside Target.bBuildEditor branches."),
    "no-gas-inspector-ref": Rule(
        "프로젝트와 현재 범위",
        "Core, satellite and integration plugins must not reference the KataGASInspector development tool."),
    "sample-only-plugins": Rule(
        "프로젝트와 현재 범위",
        "WindowsDualsense_ds5w, ChaosClothAsset* and SlateIM are enabled only for the sample project; "
        "Kata plugins must not reference them."),
    "no-sample-module-ref": Rule(
        "프로젝트와 현재 범위",
        "The ProjectKata project is sample-only; plugins must not depend on its modules."),
    "no-replication": Rule(
        "프로젝트와 현재 범위",
        "Kata is single-player only: do not add replication, RPCs or client prediction."),
}


class ConfigError(Exception):
    """저장소 구성이 규칙 표의 전제와 달라 판정할 수 없을 때 쓴다."""


@dataclass
class Plugin:
    name: str
    root: Path
    descriptor: Path
    # 모듈 이름 → .uplugin의 Type이다.
    modules: dict[str, str] = field(default_factory=dict)
    # 의존 플러그인 이름과 .uplugin 안의 줄 번호다.
    dependencies: list[tuple[str, int]] = field(default_factory=list)


@dataclass(frozen=True)
class Violation:
    rule: str
    path: str
    line: int
    message: str


def line_of(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def read_text(path: Path) -> str:
    # 벤더 파일에 BOM이 있을 수 있으므로 utf-8-sig로 읽는다. 판정에는 식별자만 쓰므로 깨진 바이트는 대체한다.
    return path.read_text(encoding="utf-8-sig", errors="replace")


def named_entry_lines(text: str, section: str, names: list[str]) -> list[int]:
    """JSON 텍스트의 section 배열에서 각 "Name" 값이 나오는 줄을 순서대로 찾는다.

    json 모듈은 위치를 돌려주지 않으므로 텍스트에서 다시 찾는다. 찾지 못한 항목은 section 키의 줄을 쓴다.
    """
    start = text.find(f'"{section}"')
    if start < 0:
        return [1] * len(names)
    lines = []
    cursor = start
    for name in names:
        match = re.compile(r'"Name"\s*:\s*"' + re.escape(name) + '"').search(text, cursor)
        if match:
            lines.append(line_of(text, match.start()))
            cursor = match.end()
        else:
            lines.append(line_of(text, start))
    return lines


def load_descriptor(path: Path) -> tuple[str, dict]:
    text = read_text(path)
    try:
        return text, json.loads(text)
    except json.JSONDecodeError as error:
        raise ConfigError(f"{path.name}: invalid JSON ({error})") from error


def load_plugins(root: Path) -> dict[str, Plugin]:
    plugins: dict[str, Plugin] = {}
    for descriptor in sorted((root / "Plugins").glob("*/*.uplugin")):
        text, data = load_descriptor(descriptor)
        plugin = Plugin(name=descriptor.stem, root=descriptor.parent, descriptor=descriptor)
        for module in data.get("Modules", []):
            plugin.modules[module.get("Name", "")] = module.get("Type", "")
        names = [entry.get("Name", "") for entry in data.get("Plugins", [])]
        plugin.dependencies = list(zip(names, named_entry_lines(text, "Plugins", names)))
        plugins[plugin.name] = plugin

    # 플러그인 이름이 바뀌거나 새 Kata 플러그인이 생기면 규칙 표도 함께 고쳐야 하므로 조용히 통과시키지 않는다.
    missing = [name for name in KATA_PLUGINS if name not in plugins]
    if missing:
        raise ConfigError(
            f"expected plugin(s) not found under Plugins/: {', '.join(missing)}. "
            "Update the plugin table in Scripts/CheckAgentsRules.py together with AGENTS.md.")
    unknown = [name for name in plugins if name.startswith("Kata") and name not in KATA_PLUGINS]
    if unknown:
        raise ConfigError(
            f"unclassified Kata plugin(s): {', '.join(unknown)}. "
            "Add them to the plugin table in Scripts/CheckAgentsRules.py together with AGENTS.md.")
    return plugins


def load_project_modules(root: Path) -> dict[str, str]:
    projects = sorted(root.glob("*.uproject"))
    if not projects:
        raise ConfigError("no .uproject found at the project root.")
    _, data = load_descriptor(projects[0])
    return {module.get("Name", ""): module.get("Type", "") for module in data.get("Modules", [])}


def iter_csharp_strings(text: str):
    """C# 소스에서 주석을 건너뛰고 문자열 리터럴의 값과 시작 위치를 돌려준다.

    Build.cs의 모듈 의존은 문자열로만 적히므로, 주석에 적힌 모듈 이름을 의존으로 오인하지 않도록 주석을 먼저 건너뛴다.
    """
    i, n = 0, len(text)
    while i < n:
        if text.startswith("//", i):
            end = text.find("\n", i)
            i = n if end < 0 else end
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            i = n if end < 0 else end + 2
        elif text[i] == '"':
            verbatim = text[max(0, i - 2):i].find("@") >= 0
            j = i + 1
            while j < n:
                if verbatim and text[j] == '"':
                    if j + 1 < n and text[j + 1] == '"':
                        j += 2
                        continue
                    break
                if not verbatim and text[j] == "\\":
                    j += 2
                    continue
                if text[j] == '"' or (not verbatim and text[j] == "\n"):
                    break
                j += 1
            yield text[i + 1:j], i
            i = j + 1
        elif text[i] == "'":
            j = i + 1
            while j < n and text[j] not in "'\n":
                j += 2 if text[j] == "\\" else 1
            i = j + 1
        else:
            i += 1


def blank(segment: str) -> str:
    return re.sub(r"[^\n]", " ", segment)


def blank_cpp_comments_and_literals(text: str) -> str:
    """C++ 주석과 문자열·문자 리터럴의 내용을 공백으로 바꾼다.

    줄바꿈과 문자 위치를 유지해 원문의 줄 번호를 그대로 쓸 수 있게 한다. 툴팁이나 주석에 적힌 "Client" 같은 단어를
    지정자로 오인하지 않기 위한 처리다. 전처리 조건(#if 0)은 해석하지 않는다.
    """
    out: list[str] = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        # 원시 문자열 R"delim( ... )delim"은 이스케이프가 없으므로 닫는 구분자까지 통째로 지운다.
        raw = re.match(r'"([^\s()\\]{0,16})\(', text[i:i + 20]) if c == '"' and i > 0 and text[i - 1] == "R" else None
        if text.startswith("//", i):
            end = text.find("\n", i)
            end = n if end < 0 else end
            out.append(blank(text[i:end]))
            i = end
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            out.append(blank(text[i:end]))
            i = end
        elif raw:
            closing = ")" + raw.group(1) + '"'
            end = text.find(closing, i + raw.end())
            end = n if end < 0 else end + len(closing)
            out.append(blank(text[i:end]))
            i = end
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            end = min(j + 1, n)
            out.append(blank(text[i:end]))
            i = end
        else:
            out.append(c)
            i += 1
    return "".join(out)


def matching_paren(text: str, open_index: int) -> int:
    depth = 0
    for index in range(open_index, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return index
    return len(text)


def split_top_level(text: str, base: int) -> list[tuple[str, int]]:
    """괄호 밖의 쉼표로 나눈 항목과 각 항목의 원문 위치를 돌려준다. meta=(...) 안의 쉼표는 나누지 않는다."""
    items = []
    depth = 0
    start = 0
    for index, c in enumerate(text):
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            items.append((text[start:index], base + start))
            start = index + 1
    items.append((text[start:], base + start))
    return items


def is_sample_only(name: str) -> bool:
    return name.startswith(SAMPLE_ONLY_PREFIXES)


def is_editor_module(name: str, kata_editor_modules: set[str]) -> bool:
    return name in ENGINE_EDITOR_MODULES or name in kata_editor_modules or bool(EDITOR_NAME_RE.search(name))


def plugin_reference_rule(source: str, target: str) -> tuple[str, str] | None:
    """source 플러그인이 target 플러그인을 참조할 때 어긋나는 규칙 ID와 메시지를 돌려준다. 문제가 없으면 None이다."""
    if target == source:
        return None
    if target == INSPECTOR_PLUGIN:
        return "no-gas-inspector-ref", f"{source} references the {INSPECTOR_PLUGIN} development tool"
    if is_sample_only(target):
        return "sample-only-plugins", f"{source} references sample-only plugin {target}"
    if source == CORE_PLUGIN and (target in SATELLITE_PLUGINS or target == INTEGRATION_PLUGIN):
        return "core-no-satellite", f"core plugin {source} references {target}"
    if source in SATELLITE_PLUGINS:
        if target == INTEGRATION_PLUGIN:
            return "satellite-deps", f"satellite plugin {source} references integration plugin {target}"
        if target in SATELLITE_PLUGINS and (source, target) not in ALLOWED_SATELLITE_DEPENDENCIES:
            return "satellite-deps", f"satellite plugin {source} references satellite plugin {target}"
    return None


class Checker:
    def __init__(self, root: Path):
        self.root = root
        self.plugins = load_plugins(root)
        self.project_modules = load_project_modules(root)
        # 모듈 이름 → 소유 플러그인이다. 벤더 플러그인도 넣어 참조 대상으로 판정한다.
        self.module_owner: dict[str, str] = {}
        for plugin in self.plugins.values():
            for module in plugin.modules:
                self.module_owner[module] = plugin.name
        self.kata_editor_modules = {
            module
            for plugin in self.plugins.values()
            for module, module_type in plugin.modules.items()
            if module_type in EDITOR_MODULE_TYPES
        }
        self.kata_editor_modules |= {
            module for module, module_type in self.project_modules.items() if module_type in EDITOR_MODULE_TYPES
        }
        self.violations: list[Violation] = []
        self.counts = {"uplugin": 0, "build_cs": 0, "cpp": 0}

    def relative(self, path: Path) -> str:
        return path.relative_to(self.root).as_posix()

    def add(self, rule: str, path: Path, line: int, message: str) -> None:
        self.violations.append(Violation(rule, self.relative(path), line, message))

    def run(self) -> list[Violation]:
        for name in KATA_PLUGINS:
            plugin = self.plugins[name]
            self.check_descriptor(plugin)
            for build_cs in sorted(plugin.root.glob("Source/**/*.Build.cs")):
                self.check_build_cs(plugin, build_cs)
            for source in sorted(plugin.root.glob("Source/**/*")):
                if source.suffix in CPP_SUFFIXES and source.is_file():
                    self.check_replication(source)
        return sorted(self.violations, key=lambda v: (v.path, v.line, v.rule))

    def check_descriptor(self, plugin: Plugin) -> None:
        self.counts["uplugin"] += 1
        for target, line in plugin.dependencies:
            result = plugin_reference_rule(plugin.name, target)
            if result is None and plugin.name == CORE_PLUGIN and target not in CORE_ALLOWED_PLUGINS:
                result = ("core-plugin-allowlist", f"core plugin {plugin.name} depends on plugin {target}")
            if result:
                self.add(result[0], plugin.descriptor, line, result[1] + " (.uplugin Plugins)")

    def check_build_cs(self, plugin: Plugin, path: Path) -> None:
        self.counts["build_cs"] += 1
        module = path.name[: -len(".Build.cs")]
        module_type = plugin.modules.get(module)
        if module_type is None:
            raise ConfigError(f"{self.relative(path)}: module {module} is not declared in {plugin.descriptor.name}.")
        text = read_text(path)
        for value, offset in iter_csharp_strings(text):
            if not IDENTIFIER_RE.match(value) or value == module:
                continue
            line = line_of(text, offset)
            # 한 참조에는 규칙 하나만 보고한다. 플러그인 관계 규칙을 먼저 판정하고, 해당하지 않을 때 런타임→에디터를 본다.
            if value in self.project_modules:
                self.add("no-sample-module-ref", path, line, f"module {module} depends on sample project module {value}")
                continue
            owner = self.module_owner.get(value)
            result = plugin_reference_rule(plugin.name, owner) if owner else None
            if result is None and owner is None and is_sample_only(value):
                result = ("sample-only-plugins", f"{plugin.name} references sample-only module {value}")
            if result:
                self.add(result[0], path, line, f"{result[1]} (module {module} -> {value})")
                continue
            if module_type in RUNTIME_MODULE_TYPES and is_editor_module(value, self.kata_editor_modules):
                self.add("runtime-no-editor", path, line,
                         f"{module_type} module {module} depends on editor-only module {value}")

    def check_replication(self, path: Path) -> None:
        self.counts["cpp"] += 1
        clean = blank_cpp_comments_and_literals(read_text(path))
        for match in REFLECTION_MACRO_RE.finditer(clean):
            macro = match.group(1)
            open_index = match.end() - 1
            close_index = matching_paren(clean, open_index)
            specifiers = REPLICATED_PROPERTY_SPECIFIERS if macro == "UPROPERTY" else RPC_FUNCTION_SPECIFIERS
            for item, offset in split_top_level(clean[open_index + 1:close_index], open_index + 1):
                key = item.split("=", 1)[0].strip()
                if key.lower() in specifiers:
                    self.add("no-replication", path, line_of(clean, offset + len(item) - len(item.lstrip())),
                             f"{macro} specifier {key}")
        for pattern, label in REPLICATION_CODE_PATTERNS:
            for match in pattern.finditer(clean):
                self.add("no-replication", path, line_of(clean, match.start()), f"replication setup {label}")


def escape_annotation_data(value: str) -> str:
    return value.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def escape_annotation_property(value: str) -> str:
    return escape_annotation_data(value).replace(":", "%3A").replace(",", "%2C")


def print_rules() -> None:
    for rule_id, rule in RULES.items():
        print(f"{rule_id}\n    AGENTS.md \"{rule.section}\": {rule.summary}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Check AGENTS.md module-boundary and single-player rules without building the engine.")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent,
                        help="Project root (default: parent of the Scripts directory)")
    parser.add_argument("--list-rules", action="store_true", help="Print the rule table and exit")
    args = parser.parse_args(argv)

    # Windows 콘솔 코드 페이지에서 한국어 항목 제목을 출력하다 중단되지 않도록 인코딩 실패 문자는 이스케이프한다.
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="backslashreplace")

    if args.list_rules:
        print_rules()
        return 0

    github = os.environ.get("GITHUB_ACTIONS") == "true"
    try:
        checker = Checker(args.root.resolve())
        violations = checker.run()
    except ConfigError as error:
        if github:
            print(f"::error title=AGENTS.md rules config::{escape_annotation_data(str(error))}")
        print(f"error: {error}", file=sys.stderr)
        return 2

    for violation in violations:
        rule = RULES[violation.rule]
        print(f"{violation.path}:{violation.line}: [{violation.rule}] {violation.message}")
        print(f"    AGENTS.md \"{rule.section}\": {rule.summary}")
        if github:
            title = escape_annotation_property(f"AGENTS.md {violation.rule}")
            body = escape_annotation_data(f"{violation.message}\nAGENTS.md \"{rule.section}\": {rule.summary}")
            print(f"::error file={escape_annotation_property(violation.path)},line={violation.line},title={title}::{body}")

    counts = checker.counts
    scope = f"{counts['uplugin']} .uplugin, {counts['build_cs']} Build.cs, {counts['cpp']} C++ files"
    if violations:
        print(f"\n{len(violations)} violation(s) in {scope}. See docs/manual/Agents-Rules-Check.md.")
        return 1
    print(f"No violations in {scope} ({len(RULES)} rules).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
