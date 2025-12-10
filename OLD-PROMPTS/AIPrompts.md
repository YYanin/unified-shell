# AI Integration Prompts for Unified Shell

This document contains step-by-step prompts to implement AI-assisted command suggestions in the unified shell using the @ symbol prefix. The AI integration allows users to ask natural language questions and receive shell command suggestions without giving the AI direct shell output access or control.

## Overview

The AI integration adds a natural language interface to the unified shell where:
- Users prefix their input with @ to trigger AI mode
- The shell sends the query to a Python-based AI helper
- The AI helper returns a suggested command (no execution)
- The shell displays the suggestion and asks for user confirmation
- If confirmed, the shell executes the command through normal parsing

Architecture:
- Shell (ushell) remains in control of all execution
- AI helper (Python script) acts as suggestion engine only
- No direct shell output or control given to AI
- User confirmation required before executing suggestions

---

## Feature: AI Command Suggestions with @ Prefix

### Prompt 1: Setup Python AI Helper Infrastructure

#### Prompt
```
Set up the Python-based AI helper infrastructure for the unified shell:

1. Ensure Python 3 is available:
   - Check: python3 --version
   - Should be Python 3.7 or higher

2. Create aiIntegr directory structure if not exists:
   - mkdir -p unified-shell/aiIntegr
   - This will house AI integration code

3. Copy or adapt mysh_llm.py to unified-shell/aiIntegr/:
   - Rename to ushell_ai.py for consistency
   - Update script shebang: #!/usr/bin/env python3
   - Make executable: chmod +x ushell_ai.py

4. Create a simple command catalog JSON file:
   - unified-shell/aiIntegr/commands.json
   - Include all built-in commands (cd, pwd, echo, export, set, etc.)
   - Include all custom tools (myls, mycat, mycp, etc.)
   - Include package manager (apt) commands
   - Include job control (jobs, fg, bg) commands
   - Format: {"commands": [{"name": "...", "summary": "...", "usage": "...", "options": [...]}]}

5. Set up environment variable for catalog:
   - export USHELL_CATALOG_FILE=./aiIntegr/commands.json
   - Add to shell startup or README

6. Test Python script independently:
   - ./aiIntegr/ushell_ai.py "list all c files"
   - Should return a suggested command line
```

#### Manual Tests
1. **Python availability**:
   ```bash
   python3 --version
   # Expected: Python 3.x.x (x >= 7)
   ```

2. **AI helper script execution**:
   ```bash
   cd unified-shell
   ./aiIntegr/ushell_ai.py "show current directory"
   # Expected: pwd (or similar command)
   ```

3. **Catalog loading**:
   ```bash
   export USHELL_AI_DEBUG=1
   ./aiIntegr/ushell_ai.py "list files"
   # Expected: Debug output showing catalog loaded
   # Expected: ls or myls command suggestion
   ```

4. **Heuristic fallback**:
   ```bash
   # Without OpenAI API key
   ./aiIntegr/ushell_ai.py "count lines in file"
   # Expected: Simple heuristic suggestion (e.g., wc)
   ```

---

### Prompt 2: Detect @ Prefix in Shell REPL

#### Prompt
```
Modify the shell's main REPL loop to detect @ prefix for AI queries:

1. Update src/main.c in the main() REPL loop:
   - After reading input line, check if it starts with '@'
   - Trim whitespace before checking
   - If '@' detected, extract the query (everything after @)
   - Pass query to new function: handle_ai_query()

2. Create new function in src/main.c:
   - int handle_ai_query(const char *query)
   - This will be the entry point for AI processing
   - For now, just print: "AI query detected: [query]"
   - Return 0 on success

3. Handle edge cases:
   - Empty query after @ (just "@" with nothing)
   - Query with only whitespace after @
   - Special characters in query that need shell escaping

4. Update shell prompt behavior:
   - If line starts with @, don't add to normal history yet
   - Don't try to parse as regular command
   - Skip normal command execution path

5. Preserve Ctrl+C and Ctrl+D handling:
   - User should still be able to cancel @ queries
   - Ctrl+D should still exit normally
```

#### Manual Tests
1. **Basic @ detection**:
   ```bash
   ./ushell
   # Type: @help me
   # Expected: "AI query detected: help me"
   ```

2. **Empty @ query**:
   ```bash
   # Type: @
   # Expected: Error message or prompt for query
   ```

3. **@ with whitespace**:
   ```bash
   # Type: @   
   # Expected: Error message (empty query after trimming)
   ```

4. **Normal commands still work**:
   ```bash
   # Type: echo test
   # Expected: test (normal execution, not AI query)
   ```

5. **Signal handling**:
   ```bash
   # Type: @test
   # Press Ctrl+C
   # Expected: Query cancelled, shell continues
   ```

---

### Prompt 3: Execute Python AI Helper from Shell

#### Prompt
```
Implement shell execution of the Python AI helper script:

1. Create new function in src/main.c:
   - char* call_ai_helper(const char *query)
   - Uses fork/exec or popen to run ushell_ai.py
   - Passes query as command-line argument
   - Captures stdout (the suggested command)
   - Returns malloc'd string with suggestion (caller must free)
   - Returns NULL on error

2. Handle Python script path:
   - Check environment variable: USHELL_AI_HELPER
   - Default: ./aiIntegr/ushell_ai.py
   - Verify script exists and is executable before calling
   - Print error if script not found

3. Process Python script output:
   - Read exactly one line from stdout
   - Strip trailing newline
   - Handle empty output (AI couldn't generate suggestion)
   - Handle stderr for debugging (if USHELL_AI_DEBUG set)

4. Add timeout protection:
   - AI helper should respond within 30 seconds
   - Use alarm() or similar mechanism
   - Kill child process if timeout exceeded
   - Return error to user with helpful message

5. Update handle_ai_query() to call this function:
   - char *suggestion = call_ai_helper(query);
   - Check if suggestion is NULL (error case)
   - Print suggestion to user (don't execute yet)
   - Free suggestion when done
```

#### Manual Tests
1. **Basic AI helper call**:
   ```bash
   ./ushell
   # Type: @list files
   # Expected: Suggestion printed (e.g., "Suggested: ls")
   ```

2. **AI helper not found**:
   ```bash
   export USHELL_AI_HELPER=/nonexistent/script.py
   ./ushell
   # Type: @test query
   # Expected: Error message about missing AI helper
   ```

3. **Complex query**:
   ```bash
   # Type: @show all c files modified today
   # Expected: Reasonable ls or find command suggestion
   ```

4. **Empty AI response**:
   ```bash
   # Create broken AI script that outputs nothing
   # Type: @test
   # Expected: Error message about empty suggestion
   ```

5. **Debug mode**:
   ```bash
   export USHELL_AI_DEBUG=1
   ./ushell
   # Type: @list files
   # Expected: Debug output visible, then suggestion
   ```

---

### Prompt 4: Implement User Confirmation for AI Suggestions

#### Prompt
```
Add user confirmation before executing AI-suggested commands:

1. Update handle_ai_query() to show suggestion and prompt:
   - Print: "AI Suggestion: [command]"
   - Print: "Execute this command? (y/n/e): "
   - y = execute the command
   - n = cancel, return to prompt
   - e = edit the command before executing

2. Read user response:
   - Use custom input function (don't use readline here)
   - Accept single character: y, n, e (case insensitive)
   - Accept Enter key as 'n' (safe default)
   - Handle Ctrl+C as cancel (same as 'n')

3. If user chooses 'y' (execute):
   - Pass suggestion through normal command execution
   - Use existing execute_command() or parse/execute pipeline
   - Add to command history
   - Show command output normally

4. If user chooses 'e' (edit):
   - Pre-fill readline buffer with suggested command
   - Let user edit interactively
   - Execute the edited version if they confirm
   - Add edited version to history, not original suggestion

5. If user chooses 'n' (cancel):
   - Print "Command cancelled"
   - Return to normal prompt
   - Don't add anything to history

6. Safety features:
   - Warn if command contains 'rm -rf' or similar dangerous patterns
   - Highlight potentially destructive commands in red (if color supported)
   - Allow user to review before confirming
```

#### Manual Tests
1. **Execute confirmation**:
   ```bash
   ./ushell
   # Type: @show current directory
   # AI suggests: pwd
   # Type: y
   # Expected: pwd executes, output shown
   ```

2. **Cancel confirmation**:
   ```bash
   # Type: @list files
   # AI suggests: ls
   # Type: n
   # Expected: "Command cancelled", return to prompt
   ```

3. **Edit confirmation**:
   ```bash
   # Type: @list files
   # AI suggests: ls
   # Type: e
   # Expected: Prompt with "ls" pre-filled for editing
   # Edit to: ls -la
   # Press Enter
   # Expected: "ls -la" executes
   ```

4. **Default to cancel**:
   ```bash
   # Type: @test
   # AI suggests: echo test
   # Press Enter (no y/n/e)
   # Expected: Command cancelled (safe default)
   ```

5. **History handling**:
   ```bash
   # Type: @list files
   # Type: y
   # Press Up arrow
   # Expected: Original "@list files" in history
   # Press Up arrow again
   # Expected: Executed command (e.g., "ls") in history
   ```

---

### Prompt 5: Add Command Catalog Export from Shell

#### Prompt
```
Implement a way for the shell to export its command catalog to the AI helper:

1. Create new built-in command: commands
   - Implement in src/builtins/builtins.c
   - Usage: commands [--json]
   - Without --json: human-readable list of all commands
   - With --json: JSON format for AI consumption

2. Build JSON catalog from shell's built-in registry:
   - Include all built-in commands (cd, pwd, echo, etc.)
   - Include all tool commands (myls, mycat, etc.)
   - Include apt subcommands
   - Include job control commands
   - For each command, include:
     - name (string)
     - summary (string)
     - description (string, detailed help)
     - usage (string, usage syntax)
     - options (array of {short, long, arg, help})

3. Format JSON structure:
   ```json
   {
     "commands": [
       {
         "name": "cd",
         "summary": "Change directory",
         "description": "Change the current working directory to the specified path",
         "usage": "cd [directory]",
         "options": [
           {
             "short": null,
             "long": null,
             "arg": "directory",
             "help": "Target directory (default: $HOME)"
           }
         ]
       },
       ...
     ]
   }
   ```

4. Update AI helper script (ushell_ai.py):
   - Change CATALOG_CMD to: ushell --commands-json
   - Fall back to commands.json file if shell not available
   - Prefer live shell catalog over static file

5. Add --commands-json flag to main():
   - ./ushell --commands-json
   - Prints JSON catalog to stdout
   - Exits immediately (doesn't start REPL)
```

#### Manual Tests
1. **List commands (human readable)**:
   ```bash
   ./ushell
   # Type: commands
   # Expected: List all available commands with summaries
   ```

2. **Export JSON catalog**:
   ```bash
   ./ushell --commands-json
   # Expected: Valid JSON printed to stdout
   ./ushell --commands-json | jq .
   # Expected: Pretty-printed JSON (if jq available)
   ```

3. **Validate JSON structure**:
   ```bash
   ./ushell --commands-json | python3 -m json.tool > /dev/null
   # Expected: No errors (valid JSON)
   ```

4. **AI helper uses shell catalog**:
   ```bash
   export PATH=$PWD:$PATH  # Make ushell available in PATH
   ./aiIntegr/ushell_ai.py "list files"
   # Expected: Uses live catalog from ushell --commands-json
   ```

5. **Catalog completeness**:
   ```bash
   ./ushell --commands-json | grep -o '"name":"[^"]*"' | wc -l
   # Expected: Count >= 26 (16 built-ins + 10 tools)
   ```

---

### Prompt 6: Enhance AI Context with Shell State (Optional)

#### Prompt
```
Add optional shell state information to AI queries for better context:

1. Create function to gather shell state:
   - char* get_shell_state_json(void)
   - Returns JSON string with current shell state
   - Include: cwd, environment variables, recent history
   - Exclude sensitive data (passwords, tokens)

2. Shell state JSON structure:
   ```json
   {
     "cwd": "/home/user/project",
     "user": "alice",
     "shell_vars": {
       "VAR1": "value1",
       "VAR2": "value2"
     },
     "recent_commands": [
       "cd project",
       "ls -la"
     ]
   }
   ```

3. Update AI helper to accept optional --context flag:
   - ./ushell_ai.py "query" [--context context.json]
   - AI uses context to provide better suggestions
   - Example: If cwd=/tmp, suggest relative paths

4. Modify call_ai_helper() to pass context:
   - Generate state JSON
   - Write to temporary file
   - Pass filename as --context argument
   - Clean up temp file after AI returns

5. Privacy controls:
   - Add USHELL_AI_CONTEXT=0 to disable state sharing
   - Default: enabled (context helps AI)
   - Document privacy implications in help
   - Never send command output, only metadata

Note: This prompt is optional and can be implemented later
if basic AI integration works well.
```

#### Manual Tests
1. **Context generation**:
   ```bash
   ./ushell
   # Add test function to print get_shell_state_json()
   # Expected: Valid JSON with cwd, user, vars
   ```

2. **AI with context**:
   ```bash
   cd /tmp
   ./ushell
   # Type: @create a test file
   # Expected: Suggestion uses /tmp or relative path
   ```

3. **Context disabled**:
   ```bash
   export USHELL_AI_CONTEXT=0
   ./ushell
   # Type: @list files
   # Expected: AI works without context
   ```

4. **Privacy check**:
   ```bash
   export USHELL_AI_DEBUG=1
   ./ushell
   # Type: @test
   # Check debug output
   # Expected: No sensitive data (passwords, tokens) in context
   ```

---

### Prompt 7: Add OpenAI API Integration (Optional)

#### Prompt
```
Add optional OpenAI API integration for better AI suggestions:

1. Install OpenAI Python package:
   - pip3 install openai
   - Add to requirements.txt in aiIntegr/

2. Update ushell_ai.py to use OpenAI when available:
   - Check for OPENAI_API_KEY environment variable
   - If present, use GPT model (default: gpt-4o-mini)
   - If absent, fall back to heuristic suggestions

3. Configure OpenAI model:
   - export OPENAI_API_KEY=sk-...
   - export USHELL_LLM_MODEL=gpt-4o-mini (or gpt-3.5-turbo)
   - Document in README

4. Optimize prompt for shell commands:
   - System message: "You are a Unix shell command expert"
   - Include command catalog in prompt (RAG approach)
   - Request single-line command output only
   - Max tokens: 128 (commands are short)

5. Error handling:
   - Catch API errors (network, quota, invalid key)
   - Fall back to heuristic if API fails
   - Log errors to stderr if USHELL_AI_DEBUG set

6. Cost considerations:
   - Document API costs in README
   - Suggest using gpt-4o-mini for cost efficiency
   - Mention heuristic fallback for free usage

Note: This prompt is optional and requires OpenAI API key.
Users can use heuristic mode without any API keys.
```

#### Manual Tests
1. **OpenAI package installation**:
   ```bash
   pip3 install openai
   python3 -c "import openai; print('OK')"
   # Expected: OK
   ```

2. **API key check**:
   ```bash
   export OPENAI_API_KEY=sk-your-key-here
   ./aiIntegr/ushell_ai.py "list all python files"
   # Expected: Intelligent suggestion (e.g., ls *.py or find -name "*.py")
   ```

3. **Fallback to heuristic**:
   ```bash
   unset OPENAI_API_KEY
   ./aiIntegr/ushell_ai.py "list files"
   # Expected: Simple heuristic suggestion (e.g., ls)
   ```

4. **API error handling**:
   ```bash
   export OPENAI_API_KEY=invalid-key
   ./aiIntegr/ushell_ai.py "test query"
   # Expected: Error message, then heuristic fallback
   ```

5. **Model selection**:
   ```bash
   export USHELL_LLM_MODEL=gpt-3.5-turbo
   ./aiIntegr/ushell_ai.py "count lines"
   # Expected: Uses specified model, returns suggestion
   ```

---

### Prompt 8: Add Help and Documentation

#### Prompt
```
Add comprehensive help and documentation for AI integration:

1. Update shell's help built-in command:
   - Add section about @ prefix for AI queries
   - Explain y/n/e confirmation options
   - Mention environment variables (OPENAI_API_KEY, etc.)

2. Create unified-shell/aiIntegr/README.md:
   - Overview of AI integration
   - Setup instructions (Python, dependencies)
   - Configuration options (API keys, models)
   - Privacy considerations
   - Troubleshooting guide
   - Example queries and expected outputs

3. Update main unified-shell/README.md:
   - Add "AI Integration" section
   - Document @ prefix feature
   - Link to aiIntegr/README.md for details

4. Add inline help in shell:
   - If user types just "@", show quick help
   - Print: "Usage: @<natural language query>"
   - Print: "Example: @list all c files"
   - Print: "See 'help' for more information"

5. Document environment variables:
   - OPENAI_API_KEY: OpenAI API key (optional)
   - USHELL_LLM_MODEL: AI model to use (default: gpt-4o-mini)
   - USHELL_AI_HELPER: Path to AI helper script
   - USHELL_CATALOG_FILE: Path to command catalog JSON
   - USHELL_AI_DEBUG: Enable debug output (0/1)
   - USHELL_AI_CONTEXT: Enable context sharing (0/1)

6. Add examples to docs:
   - "@list all python files"
   - "@find files modified today"
   - "@show disk usage"
   - "@count lines in main.c"
   - "@create a backup directory"
```

#### Manual Tests
1. **Shell help includes AI**:
   ```bash
   ./ushell
   # Type: help
   # Expected: Help text includes section on @ prefix
   ```

2. **Empty @ shows help**:
   ```bash
   # Type: @
   # Expected: Quick usage help for AI queries
   ```

3. **README completeness**:
   ```bash
   cat aiIntegr/README.md
   # Expected: Setup, usage, examples, troubleshooting
   ```

4. **Environment variables documented**:
   ```bash
   grep -i "OPENAI_API_KEY" aiIntegr/README.md
   # Expected: Documentation found
   ```

5. **Examples work**:
   ```bash
   # Try each example from documentation
   # Type: @list all python files
   # Type: @find files modified today
   # etc.
   # Expected: Reasonable suggestions for each
   ```

---

## Integration Testing

After implementing all prompts, perform these comprehensive tests:

### Test Suite 1: Basic Functionality

```bash
# Test 1: Simple query
./ushell
# Type: @list files
# Expected: Suggestion (ls or myls), confirm with y, executes

# Test 2: Complex query
# Type: @show all c files in src directory
# Expected: Suggestion (ls src/*.c or find), confirm with y

# Test 3: Edit suggestion
# Type: @list files
# Type: e (edit)
# Edit suggestion to add flags
# Expected: Edited command executes

# Test 4: Cancel suggestion
# Type: @test query
# Type: n (cancel)
# Expected: Returns to prompt, nothing executed
```

### Test Suite 2: Error Handling

```bash
# Test 5: AI helper not found
export USHELL_AI_HELPER=/nonexistent
./ushell
# Type: @test
# Expected: Error message, graceful fallback

# Test 6: Invalid query
# Type: @
# Expected: Help message

# Test 7: Python script error
# Create broken ushell_ai.py (syntax error)
# Type: @test
# Expected: Error message, shell continues

# Test 8: Ctrl+C during AI query
# Type: @test query
# Press Ctrl+C immediately
# Expected: Query cancelled, shell continues
```

### Test Suite 3: Advanced Features

```bash
# Test 9: Command catalog
./ushell --commands-json
# Expected: Valid JSON with all commands

# Test 10: AI uses catalog
# Type: @use the built-in package manager
# Expected: Suggestion includes 'apt' command

# Test 11: History tracking
# Type: @list files
# Type: y
# Press Up arrow twice
# Expected: Both "@list files" and executed command in history

# Test 12: Pipeline in suggestion
# Type: @count lines in all c files
# Expected: Suggestion might include pipeline (cat *.c | wc -l)
# Confirm and verify execution
```

### Test Suite 4: OpenAI Integration (if configured)

```bash
# Test 13: OpenAI API call
export OPENAI_API_KEY=sk-your-key
./ushell
# Type: @find all files larger than 1MB modified in last week
# Expected: Intelligent find command with complex flags

# Test 14: Model selection
export USHELL_LLM_MODEL=gpt-3.5-turbo
# Type: @list python files sorted by size
# Expected: Suggestion from specified model

# Test 15: API error fallback
export OPENAI_API_KEY=invalid
# Type: @test
# Expected: Falls back to heuristic, still works

# Test 16: Cost efficiency
# Count tokens/requests for typical queries
# Expected: Stays within reasonable limits (< 200 tokens per query)
```

---

## Success Criteria

The AI integration is complete and successful when:

1. DONE **@ Prefix Detection**: Shell recognizes @ prefix and routes to AI handler
2. DONE **AI Helper Execution**: Python script runs and returns suggestions
3. DONE **User Confirmation**: User can accept (y), reject (n), or edit (e) suggestions
4. DONE **Safe Execution**: Only confirmed commands execute, no direct AI control
5. DONE **Command Catalog**: Shell exports its command list for AI context
6. DONE **Heuristic Fallback**: Works without API keys using simple heuristics
7. DONE **OpenAI Integration**: Optionally uses GPT models when configured
8. DONE **Error Handling**: Gracefully handles missing scripts, API errors, etc.
9. DONE **Documentation**: Comprehensive help and README files
10. DONE **History Integration**: AI queries and executed commands in history

---

## Future Enhancements

Ideas for extending AI integration beyond initial implementation:

1. **Command Output Context**: Allow AI to see recent command outputs
2. **Multi-turn Conversations**: Keep context across multiple @ queries
3. **Inline Suggestions**: Show AI suggestions as you type (like GitHub Copilot)
4. **Command Explanations**: Ask AI to explain what a command does
5. **Error Help**: Automatically suggest fixes when commands fail
6. **Custom Models**: Support for local LLMs (Ollama, llama.cpp)
7. **Learning from Usage**: Track which suggestions user accepts/rejects
8. **Shell Script Generation**: Generate entire scripts from descriptions
9. **Natural Language Conditionals**: Convert "if X then Y" to shell if statements
10. **Voice Input**: Use speech-to-text for @ queries

---

## Notes and Constraints

Following constraints from AgentConstraints.md:

- NO new markdown files except this AIPrompts.md (as requested)
- All code includes detailed comments for understanding
- NO emojis, use ASCII art or DONE/NOT DONE markers
- Document interactions in AI_Interaction.md via echo

Additional AI-specific constraints:

- AI never gets direct shell control or output access
- User confirmation required for all suggested commands
- Privacy: no sensitive data sent to AI without user knowledge
- Works offline with heuristic mode (no API required)
- Compatible with existing shell features (history, pipes, etc.)
- Minimal dependencies (Python 3 + optional openai package)
