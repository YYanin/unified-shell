# AI Integration for Unified Shell (ushell)

This directory contains the AI-powered natural language command suggestion system for the unified shell.

## Overview

The AI integration allows users to ask natural language questions and receive shell command suggestions without leaving the shell. Simply prefix your query with `@` and the shell will:

1. Send your query to the AI helper (`ushell_ai.py`)
2. Display the suggested command
3. Ask for your confirmation before executing

**Key Safety Features:**
- AI never gets direct shell control or output access
- User confirmation required for all suggestions
- Works offline with heuristic mode (no API required)
- Privacy controls to limit data sharing

## Quick Start

### Basic Usage (No Setup Required)

The AI helper works out-of-the-box with built-in heuristic matching:

```bash
cd unified-shell
./ushell

# Type: @list all files
# The AI will suggest: myls (or ls)
# Type: y to execute, n to cancel, e to edit
```

### Enhanced Mode with OpenAI API (Optional)

For better, more intelligent suggestions, you can optionally configure OpenAI integration:

1. **Install Python dependencies:**
   ```bash
   # First, install pip if not available
   sudo apt install python3-pip

   # Then install OpenAI package
   pip3 install -r aiIntegr/requirements.txt
   # Or directly: pip3 install openai
   ```

2. **Set up API key:**
   ```bash
   export OPENAI_API_KEY="sk-your-api-key-here"
   ```

3. **Start using:**
   ```bash
   ./ushell
   # Type: @find all python files larger than 1MB
   # AI will use GPT model for intelligent suggestions
   ```

## Architecture

```
User Query (@list files)
         |
         v
Shell (ushell) detects @ prefix
         |
         v
Shell calls ushell_ai.py with query
         |
         v
ushell_ai.py processes query:
  - Loads command catalog
  - Builds context-aware prompt
  - Tries OpenAI API (if configured)
  - Falls back to heuristics
         |
         v
Returns single command suggestion
         |
         v
Shell displays suggestion
         |
         v
User confirms (y/n/e)
         |
         v
Shell executes through normal pipeline
```

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `OPENAI_API_KEY` | (none) | OpenAI API key for GPT models. Without this, heuristic mode is used. |
| `USHELL_LLM_MODEL` | `gpt-4o-mini` | Which OpenAI model to use. Options: `gpt-4o-mini`, `gpt-4o`, `gpt-3.5-turbo` |
| `USHELL_AI_HELPER` | `./aiIntegr/ushell_ai.py` | Path to AI helper script |
| `USHELL_CATALOG_FILE` | `./aiIntegr/commands.json` | Fallback command catalog (static) |
| `USHELL_CATALOG_CMD` | `ushell --commands-json` | Command to get live catalog from shell |
| `USHELL_AI_DEBUG` | `0` | Enable debug output (set to `1` for verbose logging) |
| `USHELL_AI_CONTEXT` | `1` | Enable shell state context sharing (set to `0` to disable) |

### Configuration Examples

**Minimal (Heuristic Mode):**
```bash
# No configuration needed - just works!
./ushell
```

**OpenAI with GPT-4o-mini (Recommended):**
```bash
export OPENAI_API_KEY="sk-..."
export USHELL_LLM_MODEL="gpt-4o-mini"
./ushell
```

**OpenAI with GPT-4o (Higher Quality):**
```bash
export OPENAI_API_KEY="sk-..."
export USHELL_LLM_MODEL="gpt-4o"
./ushell
```

**Debug Mode:**
```bash
export USHELL_AI_DEBUG=1
./ushell
# Debug messages will appear on stderr
```

**Privacy Mode (No Context Sharing):**
```bash
export USHELL_AI_CONTEXT=0
./ushell
# AI won't receive current directory, history, or environment variables
```

## Features

### 1. Dual Mode Operation

#### Heuristic Mode (Default, No API Key)
- Pattern matching against common queries
- Keyword-based command selection
- Works completely offline
- No cost, no rate limits
- Good for common operations

**Example queries that work well:**
- "list files" → `myls`
- "copy file" → `mycp`
- "remove directory" → `myrmdir`
- "count lines" → `wc`

#### OpenAI Mode (With API Key)
- Natural language understanding
- Complex query handling
- Context-aware suggestions
- Better with unusual requests

**Example queries:**
- "find all C files modified in the last week" → intelligent `find` command
- "show me the 10 largest files" → `myls` with sorting
- "count the number of TODO comments in source files" → pipeline with `grep` and `wc`

### 2. Command Catalog Integration

The AI helper uses a catalog of available commands to provide accurate suggestions:

**Live Catalog (Preferred):**
```bash
# Shell exports its command list
ushell --commands-json
```

**Static Catalog (Fallback):**
- File: `aiIntegr/commands.json`
- Contains all built-in commands, tools, and apt subcommands
- Manually maintained

### 3. Shell State Context (Optional)

When enabled (default), the AI receives context about your current shell session:

**Included:**
- Current working directory
- Username
- Recent command history (last 5 commands)
- Environment variables (filtered)

**Excluded (Privacy):**
- Variables containing: PASSWORD, TOKEN, KEY, SECRET, CREDENTIAL
- Command output
- Values truncated to 200 characters max

**Example of context helping:**
```bash
cd /tmp
@create a test file
# AI knows you're in /tmp and may suggest: mytouch test.txt
# (relative path, not absolute)
```

### 4. User Confirmation Flow

Every AI suggestion requires explicit confirmation:

```
AI Suggestion: myls -la
Execute this command? (y/n/e):
```

**Options:**
- **y (yes)**: Execute the suggested command immediately
- **n (no)**: Cancel and return to prompt
- **e (edit)**: View suggestion and type edited version

**Safety:**
- Default action is cancel (safe)
- Ctrl+C cancels at any time
- Both query and executed command saved to history

## Cost Considerations

### OpenAI API Pricing (as of 2024)

| Model | Input (per 1M tokens) | Output (per 1M tokens) | Per Query (est.) |
|-------|----------------------|------------------------|------------------|
| gpt-4o-mini | $0.15 | $0.60 | $0.0001 - $0.0003 |
| gpt-4o | $2.50 | $10.00 | $0.002 - $0.005 |
| gpt-3.5-turbo | $0.50 | $1.50 | $0.0003 - $0.0008 |

**Typical Query Breakdown:**
- Prompt: ~200-500 tokens (command catalog + query + context)
- Response: ~10-30 tokens (single command line)
- **Estimated cost per query: $0.0001 - $0.0003 with gpt-4o-mini**

### Cost Optimization Tips

1. **Use gpt-4o-mini** (default) for best cost/quality ratio
2. **Disable context** if not needed: `export USHELL_AI_CONTEXT=0`
3. **Use heuristic mode** for common queries (free)
4. **Monitor usage** in OpenAI dashboard

### Free Alternative

The heuristic mode works completely offline with zero cost:
```bash
# Don't set OPENAI_API_KEY
./ushell
# Uses pattern matching - no API calls
```

## Privacy and Security

### What Data is Sent to OpenAI (when API key is set)

**Sent:**
- Your natural language query
- Subset of available commands (5 most relevant)
- Current working directory (if context enabled)
- Username (if context enabled)
- Recent commands (if context enabled)
- Non-sensitive environment variables (if context enabled)

**Never Sent:**
- Command output
- File contents
- Sensitive environment variables (PASSWORD, TOKEN, KEY, etc.)
- Full command history
- Private data from your system

### Privacy Controls

**Disable all context sharing:**
```bash
export USHELL_AI_CONTEXT=0
# Only your query and command catalog sent
```

**Use heuristic mode only (no external calls):**
```bash
# Don't set OPENAI_API_KEY
# Everything stays on your machine
```

**Enable debug to see what's sent:**
```bash
export USHELL_AI_DEBUG=1
# Shows exactly what context is generated
```

### Data Retention

- OpenAI retains API requests for 30 days (as of 2024)
- After 30 days, data is deleted
- Data may be used to improve models (unless opted out)
- See OpenAI's privacy policy for details

## Troubleshooting

### "AI helper not found or not executable"

**Problem:** Shell can't find `ushell_ai.py`

**Solution:**
```bash
# Check file exists
ls -l aiIntegr/ushell_ai.py

# Make executable
chmod +x aiIntegr/ushell_ai.py

# Or set custom path
export USHELL_AI_HELPER=/path/to/ushell_ai.py
```

### "openai package not installed"

**Problem:** OpenAI Python package missing

**Solution:**
```bash
# Install pip first
sudo apt install python3-pip

# Install openai
pip3 install openai

# Or install from requirements
pip3 install -r aiIntegr/requirements.txt

# Verify
python3 -c "import openai; print('OK')"
```

### "AI helper produced no output"

**Problem:** Python script crashed or returned nothing

**Solution:**
```bash
# Test script directly
./aiIntegr/ushell_ai.py "list files"
# Should print a command

# Enable debug mode
export USHELL_AI_DEBUG=1
./ushell
# Check stderr for error messages

# Try heuristic mode
unset OPENAI_API_KEY
./ushell
```

### "OpenAI API call failed"

**Problem:** API error (invalid key, network, quota exceeded)

**Solutions:**

1. **Invalid API key:**
   ```bash
   # Check key format (starts with sk-)
   echo $OPENAI_API_KEY
   
   # Test key
   curl https://api.openai.com/v1/models \
     -H "Authorization: Bearer $OPENAI_API_KEY"
   ```

2. **Network issues:**
   ```bash
   # Check connectivity
   ping api.openai.com
   
   # AI will fall back to heuristic mode automatically
   ```

3. **Quota exceeded:**
   ```bash
   # Check usage in OpenAI dashboard
   # AI will fall back to heuristic mode
   
   # Or use heuristic mode only
   unset OPENAI_API_KEY
   ```

### Poor Suggestions

**Problem:** AI suggests wrong or unhelpful commands

**Solutions:**

1. **Be more specific:**
   ```bash
   # Instead of: @show files
   # Try: @list all C files in src directory
   ```

2. **Try OpenAI mode:**
   ```bash
   export OPENAI_API_KEY="sk-..."
   # GPT models understand complex queries better
   ```

3. **Enable context:**
   ```bash
   export USHELL_AI_CONTEXT=1
   # Context helps AI understand your situation
   ```

4. **Use edit mode:**
   ```bash
   # When AI suggests something close:
   # Type: e
   # Edit the suggestion to fix it
   ```

### Context Not Working

**Problem:** AI doesn't seem to use current directory or history

**Solution:**
```bash
# Verify context is enabled
echo $USHELL_AI_CONTEXT  # Should be 1 or empty (default: enabled)

# Enable debug to see context
export USHELL_AI_DEBUG=1
./ushell
# Type: @test
# Check stderr - should show "Loaded context from..."
```

## Development

### Testing the AI Helper

**Test heuristic mode:**
```bash
./aiIntegr/ushell_ai.py "list files"
# Output: myls (or similar)
```

**Test with context:**
```bash
# Create test context
echo '{"cwd":"/tmp","user":"test"}' > /tmp/test_context.json

./aiIntegr/ushell_ai.py --context /tmp/test_context.json "test query"
# Should work with context
```

**Test OpenAI mode:**
```bash
export OPENAI_API_KEY="sk-..."
export USHELL_AI_DEBUG=1
./aiIntegr/ushell_ai.py "find all python files"
# Should show API call in debug output
```

### Extending the Heuristic Engine

Edit `ushell_ai.py` function `heuristic_suggestion()`:

```python
# Add new pattern
if "your_keyword" in q_lower:
    if "your_command" in cmd_map:
        return "your_command args"
```

### Adding New Commands to Catalog

Commands are exported automatically from the shell:
```bash
# Built-in commands: edit src/builtins/builtins.c
# Tools: edit src/tools/tool_dispatch.c
# Then rebuild: make

# Verify
./ushell --commands-json | jq '.[] | select(.name=="newcmd")'
```

## Example Queries

### File Operations
```bash
@list all files                    → myls
@show C files                      → myls *.c
@copy readme to backup             → mycp readme.txt backup.txt
@remove old log files              → myrm *.log
@create a directory called test    → mymkdir test
@create a new file                 → mytouch file.txt
```

### Search and Find
```bash
@find all python files             → myfd .py
@find C files in src               → myfd .c src/
@search for TODO in code           → grep -r TODO src/
```

### System Info
```bash
@show current directory            → pwd
@show environment variables        → env
@count files                       → myls | wc -l
@show disk usage                   → du -sh *
```

### Package Management
```bash
@search for git package            → apt search git
@install python                    → apt install python
@show installed packages           → apt list
```

### Complex Queries (OpenAI)
```bash
@find all files larger than 1MB modified in last week
→ find . -type f -size +1M -mtime -7

@show the 10 largest files in this directory
→ myls -lSh | head -10

@count the number of TODO comments in all C files
→ grep -r "TODO" src/*.c | wc -l
```

## Support

For issues or questions:
1. Check this README's troubleshooting section
2. Enable debug mode: `export USHELL_AI_DEBUG=1`
3. Test AI helper directly: `./aiIntegr/ushell_ai.py "test"`
4. See main README.md for general shell documentation
5. Check OpenAI API status: https://status.openai.com/

## Future Enhancements

Potential improvements for future versions:

- Multi-turn conversations (keep context across queries)
- Command explanations ("@explain ls -la")
- Error help ("@fix: command not found")
- Local LLM support (Ollama, llama.cpp)
- Learning from user feedback (accept/reject tracking)
- Shell script generation from descriptions
- Voice input integration

## License

Same as unified shell project - see main README.md
