#!/bin/bash
# Wrapper script to run ushell_ai.py with venv python
exec /home/nordiffico/Documents/Coding/VenvEnvironments/linuxSystems/bin/python3 "$(dirname "$0")/ushell_ai.py" "$@"
