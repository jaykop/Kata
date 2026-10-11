"""열린 GitHub 이슈의 우선순위 등급과 작업 순서를 계산해 출력한다.

중요도(priority 라벨)와 규모(size 라벨)의 곱으로 자체 등급을 정하고, Blocked by 관계를 따라
선행 작업이 자기가 막는 작업의 등급을 물려받게 한 뒤 마일스톤별 순서표를 만든다.
규칙은 AGENTS.md "작업 추적" 항목, 사용법은 docs/manual/Issue-Priority.md를 따른다.

기본 실행은 읽기만 한다. `--apply`를 주면 grade 라벨을 자체 등급과 맞춘다.
이 동작은 공개 저장소에 게시되는 변경이므로 에이전트는 사용자 확인 뒤에만 실행한다.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

# 라벨 값과 점수. 규모는 작을수록 점수가 높아 a×b가 "중요하고 작은 일"을 앞세운다.
PRIORITY_SCORES = {"priority: must": 3, "priority: should": 2, "priority: could": 1}
SIZE_SCORES = {"size: S": 3, "size: M": 2, "size: L": 1, "size: XL": None}
# 용량 합산에 쓰는 칸의 가운데 값(시간). 상한으로 더하면 거의 모든 마일스톤이 경고에 걸려 신호가 약해진다.
SIZE_HOURS = {"size: S": 1.0, "size: M": 2.0, "size: L": 5.5}
GRADES = {9: "A1", 6: "A2", 4: "B1", 3: "B2", 2: "C1", 1: "C2"}
GRADE_PREFIX = "grade: "
MUST_SHARE_LIMIT = 0.6
CAPACITY_PATTERN = re.compile(r"용량:\s*([\d.]+)\s*시간")

ISSUE_QUERY = """
query($owner: String!, $name: String!, $after: String) {
  repository(owner: $owner, name: $name) {
    issues(states: OPEN, first: 100, after: $after) {
      pageInfo { hasNextPage endCursor }
      nodes {
        number
        title
        labels(first: 50) { nodes { name } }
        milestone { title dueOn description }
        blockedBy(first: 50) { nodes { number state } }
        subIssues(first: 1) { totalCount }
      }
    }
  }
}
"""


@dataclass
class Milestone:
    title: str
    due: str | None
    capacity: float | None


@dataclass
class Issue:
    number: int
    title: str
    labels: list[str]
    milestone: Milestone | None
    # 열린 선행 작업 번호만 담는다. 닫힌 선행 작업은 더 이상 막지 않는다.
    blocked_by: list[int]
    is_bundle: bool
    priority_label: str | None = None
    size_label: str | None = None
    a: int | None = None
    b: int | None = None
    blocks: list[int] = field(default_factory=list)
    effective_score: int = 0

    @property
    def score(self) -> int | None:
        if self.a is None or self.b is None:
            return None
        return self.a * self.b

    @property
    def grade(self) -> str | None:
        return GRADES.get(self.score) if self.score is not None else None

    @property
    def effective_grade(self) -> str | None:
        return GRADES.get(self.effective_score)

    @property
    def ready(self) -> bool:
        return not self.blocked_by


def run_gh(args: list[str]) -> str:
    """gh CLI를 실행하고 표준 출력을 돌려준다. 실패하면 오류 메시지와 함께 종료한다."""
    result = subprocess.run(["gh", *args], capture_output=True, text=True, encoding="utf-8")
    if result.returncode != 0:
        sys.exit(f"gh {' '.join(args[:3])} failed: {result.stderr.strip()}")
    return result.stdout


def detect_repo() -> str:
    # 스크립트가 있는 저장소의 원격을 기준으로 삼아, 다른 디렉터리에서 실행해도 같은 저장소를 읽는다.
    root = Path(__file__).resolve().parent.parent
    result = subprocess.run(
        ["gh", "repo", "view", "--json", "nameWithOwner", "--jq", ".nameWithOwner"],
        capture_output=True, text=True, encoding="utf-8", cwd=root,
    )
    if result.returncode != 0 or not result.stdout.strip():
        sys.exit("Could not detect the repository. Pass --repo owner/name.")
    return result.stdout.strip()


def fetch_issues(repo: str) -> list[Issue]:
    owner, name = repo.split("/", 1)
    issues: list[Issue] = []
    after: str | None = None
    while True:
        args = ["api", "graphql", "-f", f"query={ISSUE_QUERY}", "-F", f"owner={owner}", "-F", f"name={name}"]
        if after:
            args += ["-F", f"after={after}"]
        data = json.loads(run_gh(args))["data"]["repository"]["issues"]
        for node in data["nodes"]:
            milestone = None
            if node["milestone"]:
                match = CAPACITY_PATTERN.search(node["milestone"]["description"] or "")
                milestone = Milestone(
                    title=node["milestone"]["title"],
                    due=(node["milestone"]["dueOn"] or "")[:10] or None,
                    capacity=float(match.group(1)) if match else None,
                )
            issues.append(Issue(
                number=node["number"],
                title=node["title"],
                labels=[label["name"] for label in node["labels"]["nodes"]],
                milestone=milestone,
                blocked_by=[n["number"] for n in node["blockedBy"]["nodes"] if n["state"] == "OPEN"],
                is_bundle=node["subIssues"]["totalCount"] > 0,
            ))
        if not data["pageInfo"]["hasNextPage"]:
            return issues
        after = data["pageInfo"]["endCursor"]


def assign_scores(issues: list[Issue], warnings: list[str]) -> None:
    """라벨에서 a·b를 읽고 라벨 누락·중복과 마일스톤 배치 규칙 위반을 경고에 모은다."""
    for issue in issues:
        priorities = [l for l in issue.labels if l in PRIORITY_SCORES]
        sizes = [l for l in issue.labels if l in SIZE_SCORES]
        if issue.is_bundle:
            # 상위 이슈는 하위 이슈를 묶는 역할만 하므로 채점하지 않는다.
            continue
        if len(priorities) > 1 or len(sizes) > 1:
            warnings.append(f"#{issue.number}: multiple priority or size labels {priorities + sizes}")
        if not priorities or not sizes:
            warnings.append(f"#{issue.number}: missing priority or size label")
        if priorities:
            issue.priority_label = priorities[0]
            issue.a = PRIORITY_SCORES[priorities[0]]
        if sizes:
            issue.size_label = sizes[0]
            issue.b = SIZE_SCORES[sizes[0]]
        if issue.milestone and issue.size_label == "size: XL":
            warnings.append(f"#{issue.number}: size XL in milestone '{issue.milestone.title}', split into sub-issues")
        if issue.milestone and issue.grade and issue.grade.startswith("C"):
            warnings.append(f"#{issue.number}: grade {issue.grade} in milestone '{issue.milestone.title}', split or move to backlog")


def propagate(issues: dict[int, Issue], warnings: list[str]) -> None:
    """선행 작업이 자기가 막는 작업 중 가장 높은 점수를 물려받도록 유효 점수를 계산한다."""
    for issue in issues.values():
        for blocker in issue.blocked_by:
            if blocker in issues:
                issues[blocker].blocks.append(issue.number)

    memo: dict[int, int] = {}
    visiting: set[int] = set()

    def effective(number: int) -> int:
        if number in memo:
            return memo[number]
        if number in visiting:
            # 순환이면 이 경로의 상속을 끊고 경고만 남긴다. 순환 자체는 사용자가 풀어야 한다.
            warnings.append(f"#{number}: dependency cycle detected")
            return 0
        visiting.add(number)
        issue = issues[number]
        best = issue.score or 0
        for blocked in issue.blocks:
            best = max(best, effective(blocked))
        visiting.discard(number)
        memo[number] = best
        return best

    for number, issue in issues.items():
        issue.effective_score = effective(number)


def check_milestone_order(issues: dict[int, Issue], order: dict[str | None, int], warnings: list[str]) -> None:
    # 선행 작업이 더 늦은 마일스톤이나 백로그에 있으면, 막힌 작업이 제 마일스톤 안에 끝날 수 없다.
    for issue in issues.values():
        if not issue.milestone:
            continue
        for blocker_number in issue.blocked_by:
            blocker = issues.get(blocker_number)
            if blocker is None:
                continue
            blocker_key = blocker.milestone.title if blocker.milestone else None
            if order[blocker_key] > order[issue.milestone.title]:
                where = f"milestone '{blocker_key}'" if blocker_key else "backlog"
                warnings.append(f"#{issue.number}: blocker #{blocker_number} is in later {where}")


def check_grade_labels(issues: list[Issue], warnings: list[str]) -> list[tuple[int, list[str], list[str]]]:
    """현재 grade 라벨과 자체 등급을 비교해 (번호, 추가할 라벨, 제거할 라벨) 목록을 만든다."""
    changes = []
    for issue in issues:
        current = [l for l in issue.labels if l.startswith(GRADE_PREFIX)]
        expected = [GRADE_PREFIX + issue.grade] if issue.grade and not issue.is_bundle else []
        add = [l for l in expected if l not in current]
        remove = [l for l in current if l not in expected]
        if add or remove:
            changes.append((issue.number, add, remove))
            warnings.append(f"#{issue.number}: grade label mismatch (add {add}, remove {remove})")
    return changes


def sort_key(issue: Issue) -> tuple:
    # 작업 가능한 것 먼저, 그다음 유효 점수·자체 중요도가 높은 순, 같으면 번호 순.
    return (not issue.ready, -issue.effective_score, -(issue.a or 0), issue.number)


def format_row(index: str, issue: Issue) -> str:
    grade = issue.grade or ("XL" if issue.size_label == "size: XL" else "-")
    effective = issue.effective_grade or "-"
    a = str(issue.a) if issue.a is not None else "-"
    size = issue.size_label.split(": ")[1] if issue.size_label else "-"
    row = f"  {index:>3}  {grade:<4} {effective:<4} {a:>1}  {size:<2}  #{issue.number} {issue.title}"
    if not issue.ready:
        row += "  [blocked by " + ", ".join(f"#{n}" for n in issue.blocked_by) + "]"
    return row


def print_group(title: str, members: list[Issue], milestone: Milestone | None) -> None:
    graded = [i for i in members if not i.is_bundle]
    planned = sum(SIZE_HOURS.get(i.size_label or "", 0.0) for i in graded)
    must = sum(SIZE_HOURS.get(i.size_label or "", 0.0) for i in graded if i.a == 3)
    header = title
    if milestone:
        header += f"  due {milestone.due or '-'}"
        if milestone.capacity:
            header += (f"  capacity {milestone.capacity:g}h  planned {planned:g}h ({planned / milestone.capacity:.0%})"
                       f"  must {must:g}h ({must / milestone.capacity:.0%})")
            if must > milestone.capacity * MUST_SHARE_LIMIT:
                header += "  WARNING: must share over 60%, reduce the milestone goal"
        else:
            header += f"  planned {planned:g}h  (no capacity line)"
    print(header)
    print("  ord  own  eff  a  b   issue")
    rank = 0
    for issue in sorted(graded, key=sort_key):
        if issue.ready:
            rank += 1
            print(format_row(str(rank), issue))
        else:
            print(format_row("-", issue))
    bundles = [i for i in members if i.is_bundle]
    if bundles:
        print("  bundles: " + ", ".join(f"#{i.number}" for i in sorted(bundles, key=lambda i: i.number)))
    print()


def main() -> None:
    if sys.stdout.encoding and sys.stdout.encoding.lower() not in ("utf-8", "utf8"):
        # 콘솔 인코딩에 없는 문자가 제목에 있어도 출력이 중단되지 않게 한다.
        sys.stdout.reconfigure(errors="replace")

    parser = argparse.ArgumentParser(description="Show issue priority grades and work order per milestone.")
    parser.add_argument("--repo", help="owner/name (default: detected from this repository)")
    parser.add_argument("--milestone", help="show only milestones whose title contains this text")
    parser.add_argument("--apply", action="store_true", help="sync grade labels on GitHub (publishes changes)")
    args = parser.parse_args()

    repo = args.repo or detect_repo()
    issue_list = fetch_issues(repo)
    issues = {i.number: i for i in issue_list}
    warnings: list[str] = []

    assign_scores(issue_list, warnings)
    propagate(issues, warnings)

    milestones: dict[str, Milestone] = {}
    for issue in issue_list:
        if issue.milestone:
            milestones.setdefault(issue.milestone.title, issue.milestone)
    # 마감일이 없는 마일스톤은 마감일 있는 마일스톤 뒤, 백로그(None)는 맨 뒤.
    ordered = sorted(milestones.values(), key=lambda m: (m.due is None, m.due or "", m.title))
    order: dict[str | None, int] = {m.title: index for index, m in enumerate(ordered)}
    order[None] = len(ordered)

    check_milestone_order(issues, order, warnings)
    changes = check_grade_labels(issue_list, warnings)

    print(f"{repo}  open issues {len(issue_list)}\n")
    for milestone in ordered:
        if args.milestone and args.milestone not in milestone.title:
            continue
        members = [i for i in issue_list if i.milestone and i.milestone.title == milestone.title]
        print_group(milestone.title, members, milestone)
    if not args.milestone:
        print_group("Backlog", [i for i in issue_list if not i.milestone], None)

    if warnings:
        print(f"Warnings ({len(warnings)})")
        for warning in warnings:
            print(f"  - {warning}")
    else:
        print("Warnings (0)")

    if args.apply and changes:
        print("\nApplying grade labels")
        for number, add, remove in changes:
            command = ["issue", "edit", str(number), "--repo", repo]
            for label in add:
                command += ["--add-label", label]
            for label in remove:
                command += ["--remove-label", label]
            run_gh(command)
            print(f"  #{number}: +{add} -{remove}")
    elif changes:
        print("\nRun with --apply to sync grade labels.")


if __name__ == "__main__":
    main()
