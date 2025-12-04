#!/usr/bin/env python3
"""
ushell_ai.py - Natural language to shell command helper for unified shell (ushell).

This script is called by ushell when a line starts with '@'.
It:
  1. Reads a natural-language query from argv[1].
  2. Retrieves a catalog of available commands and options:
       - Prefer asking ushell itself (e.g. `ushell --commands-json`).
       - Fall back to a local JSON file if needed.
  3. Uses a language model (or a simple heuristic stub) to suggest a shell command.
  4. Prints exactly ONE line: the suggested command.

The shell remains responsible for:
  - Parsing the suggested command.
  - Asking the user for confirmation.
  - Executing it safely.

Implementation details:
  - Working heuristic-only path that needs no network (default).
  - Optional OpenAI model support if OPENAI_API_KEY is configured.
  - Simple keyword-based scoring for command selection (RAG-like approach).
  - Graceful fallback on errors.

Environment variables:
  - USHELL_CATALOG_CMD: Command to get catalog (default: "ushell --commands-json")
  - USHELL_CATALOG_FILE: Path to fallback JSON file (default: "commands.json")
  - USHELL_AI_DEBUG: Enable debug output to stderr (0/1)
  - OPENAI_API_KEY: OpenAI API key (optional, for LLM mode)
  - USHELL_LLM_MODEL: Model name (default: "gpt-4o-mini")
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional


# Environment-configurable settings
CATALOG_CMD = os.environ.get("USHELL_CATALOG_CMD", "ushell --commands-json")
CATALOG_FILE = os.environ.get("USHELL_CATALOG_FILE", "commands.json")


@dataclass
class CommandOption:
    """
    Represents a single command-line option/argument.
    
    Attributes:
        short: Short option name (e.g., "-h")
        long: Long option name (e.g., "--help")
        arg: Argument name if option takes a value
        help: Help text describing the option
    """
    short: Optional[str]
    long: Optional[str]
    arg: Optional[str]
    help: str


@dataclass
class CommandInfo:
    """
    Complete information about a shell command.
    
    Attributes:
        name: Command name (e.g., "cd", "ls", "myls")
        summary: Brief one-line description
        description: Detailed description of what command does
        usage: Usage syntax string
        options: List of available options/flags
    """
    name: str
    summary: str
    description: str
    usage: str
    options: List[CommandOption]


def debug(msg: str) -> None:
    """
    Print debug message to stderr if USHELL_AI_DEBUG is set.
    
    Args:
        msg: Debug message to print
    """
    if os.environ.get("USHELL_AI_DEBUG"):
        print(f"[ushell_ai] {msg}", file=sys.stderr)


def run_catalog_command() -> Optional[List[Dict[str, Any]]]:
    """
    Try to ask the shell itself for the current command catalog.
    
    Expected behavior from the shell:
      - A built-in or flag like `ushell --commands-json` that prints JSON:
        { "commands": [ { "name": "...", "summary": "...", ... }, ... ] }
    
    This is a simple protocol where the shell exposes structured metadata
    and this helper consumes it for better AI suggestions.
    
    Returns:
        List of command dictionaries, or None on failure
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

    # Handle both {"commands": [...]} and [...] formats
    if isinstance(data, dict) and isinstance(data.get("commands"), list):
        return data["commands"]
    if isinstance(data, list):
        return data
    debug("Catalog JSON had unexpected structure")
    return None


def load_catalog_from_file(path: Path) -> Optional[List[Dict[str, Any]]]:
    """
    Load command catalog from a local JSON file.
    
    This is a fallback when the shell doesn't provide a --commands-json flag.
    
    Args:
        path: Path to JSON file containing command catalog
        
    Returns:
        List of command dictionaries, or None on failure
    """
    if not path.is_file():
        debug(f"Catalog file not found: {path}")
        return None
    try:
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, json.JSONDecodeError) as exc:
        debug(f"Failed to read catalog file {path}: {exc}")
        return None

    # Handle both {"commands": [...]} and [...] formats
    if isinstance(data, dict) and isinstance(data.get("commands"), list):
        return data["commands"]
    if isinstance(data, list):
        return data
    debug("Catalog JSON from file had unexpected structure")
    return None


def load_command_catalog() -> List[CommandInfo]:
    """
    Load command catalog from shell or file, parse into CommandInfo objects.
    
    Tries shell-supplied catalog first (via --commands-json flag),
    then falls back to a local JSON file.
    
    Returns:
        List of CommandInfo objects (empty list if no catalog available)
    """
    # Try to get catalog from shell first
    commands_raw = run_catalog_command()
    if commands_raw is None:
        # Fall back to local file
        commands_raw = load_catalog_from_file(Path(CATALOG_FILE))
    if commands_raw is None:
        debug("No catalog available; returning empty list")
        return []

    # Parse raw JSON into CommandInfo objects
    result: List[CommandInfo] = []
    for item in commands_raw:
        try:
            # Parse options array
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
            # Create CommandInfo object
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
    Score how relevant a command is to the given query.
    
    Uses simple keyword matching as a stand-in for embeddings.
    Weights name matches higher than summary, which is higher than description.
    
    Args:
        query: User's natural language query
        cmd: Command to score
        
    Returns:
        Integer score (higher = more relevant)
    """
    q_tokens = {tok.lower() for tok in query.split() if tok}
    score = 0

    def add_if_matches(text: str, weight: int = 1) -> None:
        """Helper to add score if any query token appears in text."""
        nonlocal score
        for tok in q_tokens:
            if tok in text.lower():
                score += weight

    # Weight matches by importance
    add_if_matches(cmd.name, weight=3)          # Command name is most important
    add_if_matches(cmd.summary, weight=2)       # Summary is second
    add_if_matches(cmd.description, weight=1)   # Description is third
    
    # Also check option names and help text
    for opt in cmd.options:
        if opt.long:
            add_if_matches(opt.long, weight=1)
        if opt.help:
            add_if_matches(opt.help, weight=1)

    return score


def select_relevant_commands(query: str, catalog: List[CommandInfo], k: int = 5) -> List[CommandInfo]:
    """
    Select the k most relevant commands for the query.
    
    Uses keyword-based scoring to rank commands by relevance.
    
    Args:
        query: User's natural language query
        catalog: List of all available commands
        k: Number of commands to return
        
    Returns:
        List of up to k most relevant commands
    """
    if not catalog:
        return []
    
    # Score all commands
    scored = [(score_command(query, cmd), cmd) for cmd in catalog]
    # Sort by score (highest first)
    scored.sort(key=lambda pair: pair[0], reverse=True)
    
    # Return top k with score > 0, or at least the best one
    return [cmd for score, cmd in scored[:k] if score > 0] or [scored[0][1]]


def build_prompt(query: str, catalog: List[CommandInfo], context: Optional[Dict[str, Any]] = None) -> str:
    """
    Build a prompt for a general-purpose LLM.
    
    Includes a RAG-style subset of relevant commands/options to help
    the LLM generate accurate suggestions that actually exist in ushell.
    
    Args:
        query: User's natural language query
        catalog: Full command catalog
        context: Optional shell state (cwd, user, history, env)
        
    Returns:
        Formatted prompt string for LLM
    """
    # Select most relevant commands for context
    relevant = select_relevant_commands(query, catalog, k=5)

    lines: List[str] = []
    
    # System instructions
    lines.append(
        "You are a command suggestion engine for the 'ushell' (unified shell).\n"
        "Your job is to translate a natural language request into exactly one shell command line.\n"
        "Constraints:\n"
        "- Only use commands and options that exist in the catalog below.\n"
        "- Prefer simple, robust commands.\n"
        "- Return only the command line, with no explanation or extra text.\n"
    )
    
    # Add shell context if available
    if context:
        lines.append("Current shell state:")
        if "cwd" in context:
            lines.append(f"  Working directory: {context['cwd']}")
        if "user" in context:
            lines.append(f"  User: {context['user']}")
        if "history" in context and context["history"]:
            lines.append(f"  Recent commands: {', '.join(context['history'][:3])}")
        lines.append("")
    
    # Add relevant commands as context
    lines.append("Available commands (subset):")
    for cmd in relevant:
        lines.append(f"- {cmd.name}: {cmd.summary}")
        if cmd.usage:
            lines.append(f"  Usage: {cmd.usage}")
        if cmd.options:
            lines.append("  Options:")
            for opt in cmd.options:
                # Format option names (e.g., "-h --help")
                opt_names = " ".join(
                    name
                    for name in [opt.short, opt.long]
                    if name
                )
                arg = f" {opt.arg}" if opt.arg else ""
                lines.append(f"    {opt_names}{arg}: {opt.help}")
    
    # Add user query
    lines.append("")
    lines.append(f"User request: {query}")
    lines.append("Suggested ushell command line:")
    
    return "\n".join(lines)


def call_llm(prompt: str) -> Optional[str]:
    """
    Call an OpenAI model to get a command suggestion.
    
    This requires:
      - pip install openai
      - export OPENAI_API_KEY=...
    
    If these are not configured, this function returns None and the caller
    should fall back to a heuristic suggestion.
    
    Args:
        prompt: Formatted prompt with query and command context
        
    Returns:
        Suggested command string, or None on failure/not-configured
    """
    # Check for API key
    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        debug("OPENAI_API_KEY not set; skipping real LLM call")
        return None

    # Try to import openai package
    try:
        from openai import OpenAI  # type: ignore[import-not-found]
    except ImportError:
        debug("openai package not installed; skipping real LLM call")
        return None

    # Initialize OpenAI client
    client = OpenAI(api_key=api_key)
    model = os.environ.get("USHELL_LLM_MODEL", "gpt-4o-mini")

    # Make API call
    try:
        response = client.chat.completions.create(
            model=model,
            messages=[
                {
                    "role": "system",
                    "content": "You are a command suggestion engine for the 'ushell' shell.",
                },
                {"role": "user", "content": prompt},
            ],
            max_tokens=128,  # Commands are short, don't need many tokens
            n=1,
        )
    except Exception as exc:  # noqa: BLE001
        debug(f"OpenAI API call failed: {exc}")
        return None

    # Extract suggestion from response
    try:
        text = response.choices[0].message.content or ""
    except (AttributeError, IndexError, KeyError) as exc:
        debug(f"Unexpected OpenAI response structure: {exc}")
        return None

    # Take the first non-empty line as the suggested command
    for line in text.splitlines():
        stripped = line.strip()
        if stripped:
            return stripped
    return None


def heuristic_suggestion(query: str, catalog: List[CommandInfo], context: Optional[Dict[str, Any]] = None) -> str:
    """
    Simple fallback that does not require any external model.
    
    Uses keyword matching combined with explicit heuristics for common verbs
    to suggest appropriate commands. Can use shell context to improve suggestions
    (e.g., use relative paths based on current directory).
    
    Args:
        query: User's natural language query
        catalog: Full command catalog
        context: Optional shell state (cwd, user, history, env)
        
    Returns:
        Simple command suggestion (always returns something)
    """
    if not catalog:
        return "echo \"AI helper: no commands available\""

    # Create a map of command names for quick lookup
    cmd_map = {cmd.name: cmd for cmd in catalog}
    
    # Extract useful info from context for context-aware suggestions
    cwd = context.get("cwd", ".") if context else "."
    
    # Note: For now, we just make context available. Future enhancements
    # could use cwd to suggest relative paths, use history to predict
    # likely next commands, etc.
    
    # Apply explicit heuristics for common verbs first
    q_lower = query.lower()
    
    # "find" or "search" for files -> prefer myfd or find
    if ("find" in q_lower or "search" in q_lower) and ("file" in q_lower or "c file" in q_lower or ".c" in q_lower):
        if "myfd" in cmd_map:
            return "myfd"
        if "find" in cmd_map:
            return "find"
    
    # "search" for packages -> apt search
    if "search" in q_lower and "package" in q_lower:
        if "apt search" in cmd_map:
            return "apt search"
        if "apt" in cmd_map:
            return "apt"
    
    # "list" or "show" files/directories -> ls/myls
    if ("list" in q_lower or "show" in q_lower) and ("file" in q_lower or "dir" in q_lower or "content" in q_lower):
        if "myls" in cmd_map:
            return "myls"
        if "ls" in cmd_map:
            return "ls"
    
    # "show" jobs -> jobs
    if "show" in q_lower and "job" in q_lower:
        if "jobs" in cmd_map:
            return "jobs"
    
    # "count" lines/words -> wc
    if "count" in q_lower and ("line" in q_lower or "word" in q_lower):
        if "wc" in cmd_map:
            return "wc"
    
    # "copy" file -> mycp
    if "copy" in q_lower and "file" in q_lower:
        if "mycp" in cmd_map:
            return "mycp"
        if "cp" in cmd_map:
            return "cp"
    
    # "remove" or "delete" directory -> myrmdir
    # Check remove/delete before move (since "remove" contains "move")
    if ("remove" in q_lower or "delete" in q_lower) and ("directory" in q_lower or "dir" in q_lower):
        if "myrmdir" in cmd_map:
            return "myrmdir"
    
    # "remove" or "delete" file -> myrm
    if ("remove" in q_lower or "delete" in q_lower) and "file" in q_lower:
        if "myrm" in cmd_map:
            return "myrm"
        if "rm" in cmd_map:
            return "rm"
    
    # "move" or "rename" -> mymv (must come after "remove" check)
    if "move" in q_lower or "rename" in q_lower:
        if "mymv" in cmd_map:
            return "mymv"
        if "mv" in cmd_map:
            return "mv"
    
    # "create" directory -> mymkdir
    if "create" in q_lower and ("directory" in q_lower or "dir" in q_lower):
        if "mymkdir" in cmd_map:
            return "mymkdir"
        if "mkdir" in cmd_map:
            return "mkdir"
    
    # "create" file -> mytouch
    if "create" in q_lower and "file" in q_lower:
        if "mytouch" in cmd_map:
            return "mytouch"
        if "touch" in cmd_map:
            return "touch"
    
    # "display" or "show" file contents -> mycat/cat
    if ("display" in q_lower or "show" in q_lower) and "content" in q_lower:
        if "mycat" in cmd_map:
            return "mycat"
        if "cat" in cmd_map:
            return "cat"

    # Fall back to best scoring command if no heuristic matched
    best = select_relevant_commands(query, catalog, k=1)[0]
    return best.name


def suggest_command(query: str, context: Optional[Dict[str, Any]] = None) -> str:
    """
    Main suggestion logic: try LLM, fall back to heuristic.
    
    Args:
        query: User's natural language query
        context: Optional shell state context (cwd, user, history, env)
        
    Returns:
        Suggested command line (always returns something)
    """
    # Load command catalog
    catalog = load_command_catalog()
    
    # Build prompt for LLM (with optional context)
    prompt = build_prompt(query, catalog, context)

    # Try real LLM if configured
    suggestion = call_llm(prompt)
    if suggestion:
        debug(f"LLM suggestion: {suggestion}")
        return suggestion

    # Fallback: simple heuristic suggestion (with optional context)
    fallback = heuristic_suggestion(query, catalog, context)
    debug(f"Heuristic suggestion: {fallback}")
    return fallback


def main(argv: List[str]) -> int:
    """
    Main entry point for the AI helper script.
    
    Args:
        argv: Command-line arguments 
               argv[1] should be the query
               optional: --context <file> to provide shell state context
        
    Returns:
        Exit code (0 for success)
    """
    # Parse arguments for optional context file
    context = None
    query_arg = None
    
    i = 1
    while i < len(argv):
        if argv[i] == "--context" and i + 1 < len(argv):
            # Load context from file
            context_file = argv[i + 1]
            try:
                with open(context_file, 'r') as f:
                    context = json.load(f)
                debug(f"Loaded context from {context_file}")
            except (IOError, json.JSONDecodeError) as e:
                debug(f"Failed to load context from {context_file}: {e}")
            i += 2
        else:
            # This is the query
            query_arg = argv[i]
            i += 1
    
    # Check for query argument
    if not query_arg:
        print("echo \"Usage: ushell_ai.py [--context <file>] <natural language query>\"", end="")
        return 0

    # Extract and validate query
    query = query_arg.strip()
    if not query:
        print("echo \"AI helper: empty query\"", end="")
        return 0

    # Generate suggestion with optional context
    cmd = suggest_command(query, context)
    
    # IMPORTANT: print exactly one line, no extra text
    # The shell will read this and present it to the user
    print(cmd, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
