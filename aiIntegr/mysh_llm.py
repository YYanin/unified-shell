#!/usr/bin/env python3
"""
mysh_llm.py – Natural language → shell command helper for the course shell.

This script is intended to be called by the shell when a line starts with '@'.
It:
  1. Reads a natural-language query from argv[1].
  2. Retrieves a catalog of available commands + options:
       - Prefer asking the shell itself (e.g. `mysh --commands-json`).
       - Fall back to a local JSON file if needed.
  3. Uses a language model (or a simple heuristic stub) to suggest a shell command.
  4. Prints exactly ONE line: the suggested command.

The shell remains responsible for:
  - Parsing the suggested command.
  - Asking the user for confirmation.
  - Executing it safely.

This file is deliberately "partly implemented":
  - There is a working, heuristic-only path that needs no network.
  - There is also a stub for calling an OpenAI model if you configure it.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional


CATALOG_CMD = os.environ.get("MYSH_CATALOG_CMD", "mysh --commands-json")
CATALOG_FILE = os.environ.get("MYSH_CATALOG_FILE", "commands.json")


@dataclass
class CommandOption:
    short: Optional[str]
    long: Optional[str]
    arg: Optional[str]
    help: str


@dataclass
class CommandInfo:
    name: str
    summary: str
    description: str
    usage: str
    options: List[CommandOption]


def debug(msg: str) -> None:
    if os.environ.get("MYSH_LLM_DEBUG"):
        print(f"[mysh_llm] {msg}", file=sys.stderr)


def run_catalog_command() -> Optional[List[Dict[str, Any]]]:
    """
    Try to ask the shell itself for the current command catalog.

    Expected behavior from the shell:
      - A built-in or flag like `mysh --commands-json` that prints JSON:
        { "commands": [ { "name": "...", "summary": "...", ... }, ... ] }

    This is a simple, custom protocol that plays a similar role to MCP:
    the shell exposes structured metadata; this helper consumes it.
    """
    try:
        debug(f"Running catalog command: {CATALOG_CMD}")
        result = subprocess.run(
            CATALOG_CMD,
            shell=True,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except (subprocess.CalledProcessError, OSError) as exc:
        debug(f"Catalog command failed: {exc}")
        return None

    try:
        data = json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        debug(f"Failed to decode JSON from catalog command: {exc}")
        return None

    if isinstance(data, dict) and isinstance(data.get("commands"), list):
        return data["commands"]
    if isinstance(data, list):
        return data
    debug("Catalog JSON had unexpected structure")
    return None


def load_catalog_from_file(path: Path) -> Optional[List[Dict[str, Any]]]:
    if not path.is_file():
        debug(f"Catalog file not found: {path}")
        return None
    try:
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, json.JSONDecodeError) as exc:
        debug(f"Failed to read catalog file {path}: {exc}")
        return None

    if isinstance(data, dict) and isinstance(data.get("commands"), list):
        return data["commands"]
    if isinstance(data, list):
        return data
    debug("Catalog JSON from file had unexpected structure")
    return None


def load_command_catalog() -> List[CommandInfo]:
    """
    Try shell-supplied catalog first, then fall back to a local JSON file.
    """
    commands_raw = run_catalog_command()
    if commands_raw is None:
        commands_raw = load_catalog_from_file(Path(CATALOG_FILE))
    if commands_raw is None:
        debug("No catalog available; returning empty list")
        return []

    result: List[CommandInfo] = []
    for item in commands_raw:
        try:
            options_data = item.get("options", []) or []
            options: List[CommandOption] = []
            for opt in options_data:
                options.append(
                    CommandOption(
                        short=opt.get("short"),
                        long=opt.get("long"),
                        arg=opt.get("arg"),
                        help=opt.get("help", ""),
                    )
                )
            result.append(
                CommandInfo(
                    name=item["name"],
                    summary=item.get("summary", ""),
                    description=item.get("description", ""),
                    usage=item.get("usage", item["name"]),
                    options=options,
                )
            )
        except KeyError as exc:
            debug(f"Skipping malformed command entry (missing key {exc})")
    return result


def score_command(query: str, cmd: CommandInfo) -> int:
    """
    Very simple keyword scoring used as a stand-in for embeddings.
    """
    q_tokens = {tok.lower() for tok in query.split() if tok}
    score = 0

    def add_if_matches(text: str, weight: int = 1) -> None:
        nonlocal score
        for tok in q_tokens:
            if tok in text.lower():
                score += weight

    add_if_matches(cmd.name, weight=3)
    add_if_matches(cmd.summary, weight=2)
    add_if_matches(cmd.description, weight=1)
    for opt in cmd.options:
        if opt.long:
            add_if_matches(opt.long, weight=1)
        if opt.help:
            add_if_matches(opt.help, weight=1)

    return score


def select_relevant_commands(query: str, catalog: List[CommandInfo], k: int = 5) -> List[CommandInfo]:
    if not catalog:
        return []
    scored = [(score_command(query, cmd), cmd) for cmd in catalog]
    scored.sort(key=lambda pair: pair[0], reverse=True)
    return [cmd for score, cmd in scored[:k] if score > 0] or [scored[0][1]]


def build_prompt(query: str, catalog: List[CommandInfo]) -> str:
    """
    Build a prompt for a general-purpose LLM, including a small
    RAG-style subset of commands/options.
    """
    relevant = select_relevant_commands(query, catalog, k=5)

    lines: List[str] = []
    lines.append(
        "You are a command suggestion engine for the custom 'mysh' shell.\n"
        "Your job is to translate a natural language request into exactly one shell command line.\n"
        "Constraints:\n"
        "- Only use commands and options that exist in the catalog below.\n"
        "- Prefer simple, robust commands.\n"
        "- Return only the command line, with no explanation or extra text.\n"
    )
    lines.append("Available commands (subset):")
    for cmd in relevant:
        lines.append(f"- {cmd.name}: {cmd.summary}")
        if cmd.usage:
            lines.append(f"  Usage: {cmd.usage}")
        if cmd.options:
            lines.append("  Options:")
            for opt in cmd.options:
                opt_names = " ".join(
                    name
                    for name in [opt.short, opt.long]
                    if name
                )
                arg = f" {opt.arg}" if opt.arg else ""
                lines.append(f"    {opt_names}{arg}: {opt.help}")
    lines.append("")
    lines.append(f"User request: {query}")
    lines.append("Suggested mysh command line:")
    return "\n".join(lines)


def call_llm(prompt: str) -> Optional[str]:
    """
    Optional: call an OpenAI model to get a suggestion.

    This requires:
      - `pip install openai`
      - `export OPENAI_API_KEY=...`

    If these are not configured, this function returns None and the caller
    should fall back to a heuristic suggestion.
    """
    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        debug("OPENAI_API_KEY not set; skipping real LLM call")
        return None

    try:
        # Modern OpenAI client usage; students may need to adapt to API changes.
        from openai import OpenAI  # type: ignore[import-not-found]
    except ImportError:
        debug("openai package not installed; skipping real LLM call")
        return None

    client = OpenAI(api_key=api_key)
    model = os.environ.get("MYSH_LLM_MODEL", "gpt-4o-mini")

    try:
        response = client.chat.completions.create(
            model=model,
            messages=[
                {
                    "role": "system",
                    "content": "You are a command suggestion engine for the 'mysh' shell.",
                },
                {"role": "user", "content": prompt},
            ],
            max_tokens=128,
            n=1,
        )
    except Exception as exc:  # noqa: BLE001
        debug(f"OpenAI API call failed: {exc}")
        return None

    try:
        text = response.choices[0].message.content or ""
    except (AttributeError, IndexError, KeyError) as exc:
        debug(f"Unexpected OpenAI response structure: {exc}")
        return None

    # Take the first non-empty line as the suggested command.
    for line in text.splitlines():
        stripped = line.strip()
        if stripped:
            return stripped
    return None


def heuristic_suggestion(query: str, catalog: List[CommandInfo]) -> str:
    """
    Simple fallback that does not require any external model:
      - Picks the most relevant command.
      - Returns a very basic command line, like 'ls' or 'ls .'.
    """
    if not catalog:
        return "echo \"LLM helper: no commands available\""

    best = select_relevant_commands(query, catalog, k=1)[0]

    # Very primitive: handle two common verbs.
    q_lower = query.lower()
    if "list" in q_lower or "show" in q_lower:
        if best.name == "ls":
            return "ls"
    if "count" in q_lower and best.name == "wc":
        return "wc"

    # Default: just run the command with no options.
    return best.name


def suggest_command(query: str) -> str:
    catalog = load_command_catalog()
    prompt = build_prompt(query, catalog)

    # Try a real LLM if configured.
    suggestion = call_llm(prompt)
    if suggestion:
        debug(f"LLM suggestion: {suggestion}")
        return suggestion

    # Fallback: heuristic suggestion.
    fallback = heuristic_suggestion(query, catalog)
    debug(f"Heuristic suggestion: {fallback}")
    return fallback


def main(argv: List[str]) -> int:
    if len(argv) < 2:
        print("echo \"Usage: mysh_llm <natural language query>\"", end="")
        return 0

    query = argv[1].strip()
    if not query:
        print("echo \"LLM helper: empty query\"", end="")
        return 0

    cmd = suggest_command(query)
    # IMPORTANT: print exactly one line, no extra text.
    print(cmd, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

