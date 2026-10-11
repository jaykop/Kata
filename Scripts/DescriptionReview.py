"""작업별 설명 검토 목록과 내용 스냅샷을 기록한다. Python 표준 라이브러리만 사용한다."""

import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import uuid


ROOT = Path(__file__).resolve().parent.parent
QUEUES = ROOT / "docs/localdocs/threads"


def now():
    return datetime.now(timezone.utc).isoformat()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def project_path(value):
    path = (ROOT / value).resolve()
    path.relative_to(ROOT)
    return path


def relative(value):
    return project_path(value).relative_to(ROOT).as_posix()


def git(*args):
    return subprocess.check_output(
        ["git", *args], cwd=ROOT, encoding="utf-8", errors="strict"
    ).strip()


def file_hash(value):
    path = project_path(value)
    return digest(path.read_bytes()) if path.is_file() else None


def queue_path(task):
    if not re.fullmatch(r"[a-z0-9][a-z0-9-]{0,95}", task):
        raise ValueError("Task ID must use lowercase letters, digits and hyphens")
    return project_path(f"docs/localdocs/threads/{task}.descriptions.json")


@contextmanager
def edit_queue(task, create=False):
    path = queue_path(task)
    path.parent.mkdir(parents=True, exist_ok=True)
    lock = path.with_suffix(path.suffix + ".lock")
    fd = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
    temp = path.with_name(path.name + "." + uuid.uuid4().hex + ".tmp")
    try:
        original = path.read_bytes() if path.exists() else None
        if create and original is not None:
            raise ValueError("Queue already exists; reuse it instead of resetting")
        if not create and original is None:
            raise ValueError("Queue does not exist; run start first")
        data = json.loads(original.decode("utf-8")) if original else {}
        if original and data.get("schemaVersion") != 1:
            raise ValueError("Unsupported queue schema version")
        yield data
        # 잠금을 쓰지 않는 편집기의 수정도 덮어쓰지 않는다.
        current = path.read_bytes() if path.exists() else None
        if current != original:
            raise ValueError("Queue changed during this operation; retry after reading it")
        data["updatedAt"] = now()
        temp.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        # Windows에서 동기화·읽기 도중 잠깐 파일 교체를 막으면 제한적으로 재시도한다.
        for attempt in range(4):
            latest = path.read_bytes() if path.exists() else None
            if latest != original:
                raise ValueError("Queue changed before replacement; read it again")
            try:
                os.replace(temp, path)
                break
            except PermissionError:
                if attempt == 3:
                    raise
                time.sleep(0.1 * (attempt + 1))
    finally:
        os.close(fd)
        lock.unlink()
        if temp.exists():
            temp.unlink()


def block(selector):
    lines = project_path(selector["path"]).read_text(encoding="utf-8-sig").splitlines()
    matches = [i for i, line in enumerate(lines) if selector["anchor"] in line]
    if len(matches) != 1:
        raise ValueError(f"Anchor must match one line: {selector['path']} / {selector['anchor']}")
    index = matches[0]
    start = index - selector.get("before", 0)
    end = index + 1 + selector.get("after", 0)
    mode = selector.get("mode", "lines")
    if mode == "heading":
        heading = re.match(r"^(#{1,6})\s", lines[index])
        if not heading:
            raise ValueError("Heading selector must point to a Markdown heading")
        level = len(heading[1])
        end = len(lines)
        for i in range(index + 1, len(lines)):
            other = re.match(r"^(#{1,6})\s", lines[i])
            if other and len(other[1]) <= level:
                end = i
                break
    elif mode == "span":
        ends = [i for i in range(index + 1, len(lines)) if selector["endAnchor"] in lines[i]]
        if len(ends) != 1:
            raise ValueError("End anchor must match one later line")
        end = ends[0] + (1 if selector.get("includeEnd", False) else 0)
    elif mode != "lines":
        raise ValueError("Unknown selector mode")
    if start < 0 or end > len(lines) or start >= end:
        raise ValueError("Selected block is outside the file")
    text = "\n".join(lines[start:end]) + "\n"
    return {"sha256": digest(text.encode("utf-8")), "lineHint": start + 1, "endLineHint": end}


def normalize_selector(selector):
    selector = dict(selector)
    selector["path"] = relative(selector["path"])
    if not isinstance(selector.get("anchor"), str) or not selector["anchor"]:
        raise ValueError("Selector requires a literal anchor")
    for key in ("before", "after"):
        if not isinstance(selector.get(key, 0), int) or selector.get(key, 0) < 0:
            raise ValueError("before/after must be nonnegative line counts")
    return selector


def snapshot(entry):
    return {"description": block(entry["selector"]),
            "evidence": [block(item) for item in entry["evidence"]]}


def fingerprints(value):
    # 줄 위치만 바뀌었을 때는 검토를 유지하고, 선택한 내용의 변화를 비교한다.
    return [value["description"]["sha256"], *[item["sha256"] for item in value["evidence"]]]


def refresh(data):
    for entry in data["entries"]:
        try:
            current = snapshot(entry)
            entry.pop("locationError", None)
            reference = entry.get("review", {}).get("snapshot", entry["snapshot"])
            if fingerprints(current) != fingerprints(reference):
                entry["status"] = "pending"
                entry.pop("ticket", None)
            entry["snapshot"] = current
        except (OSError, ValueError, KeyError) as error:
            entry["status"] = "pending"
            entry["locationError"] = str(error)
            entry.pop("ticket", None)


def get_entry(data, entry_id):
    for entry in data["entries"]:
        if entry["id"] == entry_id:
            return entry
    raise ValueError("Entry not found")


def capture(data):
    data["headCommit"] = git("rev-parse", "HEAD")
    changes = []
    for path in data["taskPaths"]:
        current = file_hash(path)
        initial = data["initialFiles"][path]
        if current == initial["sha256"]:
            continue
        patch = git("diff", "--no-ext-diff", "--no-textconv", "--unified=0",
                    data["baseCommit"], "--", path)
        hunks = re.findall(r"^@@.*@@.*$", patch, flags=re.MULTILINE)
        changes.append({"path": path, "sha256": current, "hunks": hunks,
                        "initiallyDirty": initial["dirty"],
                        "changeType": "deleted" if current is None else "modified-or-added"})
    data["changes"] = changes
    data["capturedAt"] = now()
    refresh(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("start", "scope", "capture", "register", "no-impact", "pending", "snapshot", "begin", "finish"))
    parser.add_argument("task")
    parser.add_argument("--paths", nargs="+")
    parser.add_argument("--issue", type=int)
    parser.add_argument("--items", help="Project-relative JSON array of review entries")
    parser.add_argument("--path", help="Task file whose descriptions are unaffected")
    parser.add_argument("--id")
    parser.add_argument("--token")
    parser.add_argument("--reviewer", default="Codex")
    parser.add_argument("--status", choices=("reviewed", "needs-decision"), default="reviewed")
    parser.add_argument("--note")
    parser.add_argument("--description-hash", help="Expected hash of the final, edited description block")
    args = parser.parse_args()
    with edit_queue(args.task, create=args.command == "start") as data:
        if args.command == "start":
            if not args.paths:
                raise ValueError("start requires explicit file paths")
            data.update(schemaVersion=1, taskId=args.task, issue=args.issue,
                        baseCommit=git("rev-parse", "HEAD"), headCommit=git("rev-parse", "HEAD"),
                        startedAt=now(), initialChanges=git("status", "--short"),
                        taskPaths=[], initialFiles={}, changes=[], entries=[], noImpact=[])
        if args.command in ("start", "scope"):
            if not args.paths:
                raise ValueError("scope requires explicit file paths")
            for value in args.paths:
                path = relative(value)
                if project_path(path).is_dir():
                    raise ValueError("Use explicit files, not directories")
                if path not in data["taskPaths"]:
                    data["taskPaths"].append(path)
                    data["initialFiles"][path] = {
                        "sha256": file_hash(path), "capturedAt": now(),
                        "dirty": bool(git("status", "--short", "--", path))}
        elif args.command == "capture":
            capture(data)
        elif args.command == "register":
            if not args.items:
                raise ValueError("register requires --items")
            items = json.loads(project_path(args.items).read_text(encoding="utf-8-sig"))
            for item in items:
                if not item.get("id") or not item.get("reason"):
                    raise ValueError("Entry requires id and reason")
                if item.get("kind") not in ("comment", "manual", "tag-devcomment"):
                    raise ValueError("Unknown entry kind")
                item["selector"] = normalize_selector(item["selector"])
                if item["selector"]["path"] not in data["taskPaths"]:
                    raise ValueError("Description file must be in taskPaths; add it before editing")
                item["evidence"] = [normalize_selector(s) for s in item.get("evidence", [])]
                if not item["evidence"]:
                    raise ValueError("Entry requires implementation or contract evidence")
                item["snapshot"] = snapshot(item)
                item["status"] = "pending"
                item["registeredAt"] = now()
                existing = next((e for e in data["entries"] if e["id"] == item["id"]), None)
                if existing:
                    if existing.get("review"):
                        item["review"] = existing["review"]
                    data["entries"][data["entries"].index(existing)] = item
                else:
                    data["entries"].append(item)
        elif args.command == "pending":
            capture(data)
            covered = {e["selector"]["path"] for e in data["entries"]}
            covered.update(s["path"] for e in data["entries"] for s in e["evidence"])
            covered.update(n["path"] for n in data["noImpact"] if n["sha256"] == file_hash(n["path"]))
            print(json.dumps({"taskId": data["taskId"], "changes": data["changes"],
                              "unassignedChanges": [c for c in data["changes"] if c["path"] not in covered],
                              "noImpact": data["noImpact"],
                              "entries": [e for e in data["entries"] if e["status"] != "reviewed"]},
                             ensure_ascii=False, indent=2))
        elif args.command == "no-impact":
            if not args.path or not args.note:
                raise ValueError("no-impact requires --path and --note")
            path = relative(args.path)
            if path not in data["taskPaths"]:
                raise ValueError("File must be in taskPaths")
            data["noImpact"] = [n for n in data["noImpact"] if n["path"] != path]
            data["noImpact"].append({"path": path, "sha256": file_hash(path), "reason": args.note, "recordedAt": now()})
        elif args.command == "snapshot":
            entry = get_entry(data, args.id)
            print(json.dumps(snapshot(entry), ensure_ascii=False, indent=2))
        elif args.command == "begin":
            refresh(data)
            entry = get_entry(data, args.id)
            if entry.get("locationError"):
                raise ValueError(entry["locationError"])
            entry["ticket"] = {"token": uuid.uuid4().hex, "snapshot": entry["snapshot"],
                               "reviewer": args.reviewer, "startedAt": now()}
            print(json.dumps(entry["ticket"], ensure_ascii=False, indent=2))
        elif args.command == "finish":
            entry = get_entry(data, args.id)
            ticket = entry.get("ticket", {})
            if not args.token or ticket.get("token") != args.token:
                raise ValueError("finish requires the token returned by begin")
            if not args.note:
                raise ValueError("finish requires a review result note")
            current = snapshot(entry)
            previous = ticket["snapshot"]
            if fingerprints(current)[1:] != fingerprints(previous)[1:]:
                raise ValueError("Implementation evidence changed during review; begin again")
            expected = args.description_hash or previous["description"]["sha256"]
            if current["description"]["sha256"] != expected:
                raise ValueError("Description changed; provide its final hash after reading it")
            entry["status"] = args.status
            entry["snapshot"] = current
            entry["review"] = {"reviewer": ticket["reviewer"], "finishedAt": now(),
                               "snapshot": current, "note": args.note, "status": args.status}
            entry.pop("ticket")
    if args.command != "pending":
        print(f"Saved: {queue_path(args.task).relative_to(ROOT).as_posix()}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f"DescriptionReview: {error}", file=sys.stderr)
        sys.exit(1)
