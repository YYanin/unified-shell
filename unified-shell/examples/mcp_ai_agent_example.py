#!/usr/bin/env python3
"""
MCP AI Agent Example - Mock AI agent using unified shell MCP server

This example demonstrates an AI agent that:
- Discovers available tools via list_tools
- Selects appropriate tools based on natural language queries
- Executes multi-step operations
- Interprets results and chains commands

Note: This is a mock agent with hardcoded logic (no LLM required)
Real AI agents would use LLMs for tool selection and planning.

Usage:
    python3 mcp_ai_agent_example.py "Find all Python files"
    python3 mcp_ai_agent_example.py "What's in my home directory?"
"""

import socket
import json
import sys
import os
import re


class MCPAIAgent:
    """Mock AI agent that uses MCP tools to accomplish tasks"""
    
    def __init__(self, host='localhost', port=9000):
        self.host = host
        self.port = port
        self.socket = None
        self.buffer = ""
        self.tools = []
        self.request_counter = 0
    
    def connect(self):
        """Connect to MCP server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            return True
        except Exception as e:
            print(f"Error connecting to MCP server: {e}")
            return False
    
    def send_request(self, method, params=None):
        """Send MCP request"""
        if params is None:
            params = {}
        
        self.request_counter += 1
        message = {
            "id": str(self.request_counter),
            "method": method,
            "params": params
        }
        
        json_str = json.dumps(message) + "\n"
        self.socket.send(json_str.encode('utf-8'))
    
    def receive_message(self):
        """Receive message from server"""
        while '\n' not in self.buffer:
            chunk = self.socket.recv(4096).decode('utf-8')
            if not chunk:
                return None
            self.buffer += chunk
        
        line, self.buffer = self.buffer.split('\n', 1)
        return json.loads(line)
    
    def initialize(self):
        """Initialize MCP session"""
        print("[Agent] Initializing connection to shell...")
        self.send_request("initialize", {})
        response = self.receive_message()
        
        if response and response.get("type") == "response":
            print(f"[Agent] Connected to {response['result'].get('server', 'shell')}")
            return True
        return False
    
    def discover_tools(self):
        """Discover available tools"""
        print("[Agent] Discovering available tools...")
        self.send_request("list_tools", {})
        response = self.receive_message()
        
        if response and response.get("type") == "response":
            self.tools = response.get("result", {}).get("tools", [])
            print(f"[Agent] Found {len(self.tools)} tools")
            return True
        return False
    
    def call_tool(self, tool_name, args=None):
        """Execute a tool"""
        if args is None:
            args = {}
        
        print(f"[Agent] Executing tool: {tool_name}")
        self.send_request("call_tool", {"tool": tool_name, "args": args})
        
        # Collect all messages (notifications + response)
        messages = []
        while True:
            msg = self.receive_message()
            if not msg:
                break
            
            messages.append(msg)
            
            if msg.get("type") == "notification":
                event = msg.get("event", "")
                if event == "tool_completed":
                    continue  # Wait for actual response
            elif msg.get("type") in ["response", "error"]:
                break
        
        # Return the final response
        for msg in reversed(messages):
            if msg.get("type") in ["response", "error"]:
                return msg
        
        return None
    
    def analyze_query(self, query):
        """
        Analyze natural language query and determine appropriate action
        
        This is a mock analysis - real AI agents would use LLM for this
        """
        query_lower = query.lower()
        
        # Pattern matching for different query types
        patterns = {
            'find_files': [
                (r'find.*(python|\.py)', 'find', {'name': '*.py'}),
                (r'find.*(text|\.txt)', 'find', {'name': '*.txt'}),
                (r'find.*files', 'find', {}),
                (r'search.*files', 'find', {}),
            ],
            'list_directory': [
                (r'what.*in.*(directory|folder|here)', 'ls', {}),
                (r'list.*(files|directory|folder)', 'ls', {}),
                (r'show.*contents', 'ls', {}),
            ],
            'current_location': [
                (r'where am i', 'pwd', {}),
                (r'current (directory|location|path)', 'pwd', {}),
            ],
            'shell_context': [
                (r'what.*environment', 'get_shell_context', {}),
                (r'show.*context', 'get_shell_context', {}),
            ],
            'search_commands': [
                (r'what commands.*for', 'search_commands', None),  # Will extract query
                (r'how (do|to).*', 'suggest_command', None),  # Will extract query
            ]
        }
        
        # Try to match query to a pattern
        for category, pattern_list in patterns.items():
            for pattern, tool, args in pattern_list:
                if re.search(pattern, query_lower):
                    if args is None:
                        # Extract the actual query for search/suggest
                        args = {"query": query}
                    
                    return {
                        'tool': tool,
                        'args': args,
                        'category': category
                    }
        
        # Default: try to suggest a command
        return {
            'tool': 'suggest_command',
            'args': {'query': query},
            'category': 'suggestion'
        }
    
    def execute_query(self, query):
        """Execute a natural language query"""
        print(f"\n{'=' * 60}")
        print(f"Query: {query}")
        print('=' * 60)
        
        # Analyze query to determine action
        print("[Agent] Analyzing query...")
        action = self.analyze_query(query)
        
        print(f"[Agent] Planning: Use tool '{action['tool']}'")
        
        # Execute the tool
        result = self.call_tool(action['tool'], action['args'])
        
        if not result:
            print("[Agent] Failed to get response")
            return False
        
        # Interpret result
        if result.get("type") == "error":
            print(f"[Agent] Error: {result.get('error', 'Unknown error')}")
            return False
        
        # Extract and display result
        result_data = result.get("result", {})
        
        # Different handling based on tool type
        if action['tool'] in ['pwd', 'ls', 'echo']:
            output = result_data.get("output", "").strip()
            print(f"\n[Agent] Result:\n{output}")
        
        elif action['tool'] == 'get_shell_context':
            context = result_data
            print(f"\n[Agent] Shell Context:")
            print(f"  Working Directory: {context.get('cwd', 'unknown')}")
            print(f"  User: {context.get('user', 'unknown')}")
            history = context.get('history', [])
            if history:
                print(f"  Recent Commands: {', '.join(history[:3])}")
        
        elif action['tool'] == 'search_commands':
            try:
                # Output is JSON string in result
                search_data = json.loads(result_data.get("output", "{}"))
                results = search_data.get("results", [])
                print(f"\n[Agent] Found {len(results)} relevant commands:")
                for cmd in results[:5]:
                    print(f"  - {cmd['name']}: {cmd['description']}")
            except:
                print(f"\n[Agent] Search results:\n{result_data.get('output', '')}")
        
        elif action['tool'] == 'suggest_command':
            try:
                suggestion = json.loads(result_data.get("output", "{}"))
                cmd = suggestion.get("command", "")
                explanation = suggestion.get("explanation", "")
                print(f"\n[Agent] Suggested Command: {cmd}")
                print(f"[Agent] Explanation: {explanation}")
            except:
                print(f"\n[Agent] Suggestion:\n{result_data.get('output', '')}")
        
        else:
            # Generic output
            output = result_data.get("output", "")
            if output:
                print(f"\n[Agent] Output:\n{output}")
        
        return True
    
    def close(self):
        """Close connection"""
        if self.socket:
            self.socket.close()


def scenario_find_python_files():
    """Scenario 1: Find all Python files"""
    agent = MCPAIAgent(port=int(os.getenv('MCP_PORT', '9000')))
    
    if not agent.connect():
        return False
    
    try:
        agent.initialize()
        agent.discover_tools()
        
        # Execute query
        agent.execute_query("Find all Python files in the current directory")
        
        return True
    finally:
        agent.close()


def scenario_whats_in_directory():
    """Scenario 2: What's in my directory"""
    agent = MCPAIAgent(port=int(os.getenv('MCP_PORT', '9000')))
    
    if not agent.connect():
        return False
    
    try:
        agent.initialize()
        agent.discover_tools()
        
        # Execute query
        agent.execute_query("What's in my current directory?")
        
        return True
    finally:
        agent.close()


def scenario_shell_context():
    """Scenario 3: Get shell environment context"""
    agent = MCPAIAgent(port=int(os.getenv('MCP_PORT', '9000')))
    
    if not agent.connect():
        return False
    
    try:
        agent.initialize()
        agent.discover_tools()
        
        # Execute query
        agent.execute_query("Show me the current shell environment")
        
        return True
    finally:
        agent.close()


def main():
    """Main entry point"""
    print("=" * 60)
    print("MCP AI Agent Example - Mock Intelligent Shell Assistant")
    print("=" * 60)
    print()
    
    # Check for command line query
    if len(sys.argv) > 1:
        query = " ".join(sys.argv[1:])
        
        agent = MCPAIAgent(port=int(os.getenv('MCP_PORT', '9000')))
        if not agent.connect():
            print("Failed to connect to MCP server")
            print("Make sure shell is running with USHELL_MCP_ENABLED=1")
            return 1
        
        try:
            agent.initialize()
            agent.discover_tools()
            agent.execute_query(query)
        finally:
            agent.close()
        
        return 0
    
    # Run demonstration scenarios
    print("Running demonstration scenarios...")
    print("(Pass a query as argument to test custom queries)\n")
    
    scenarios = [
        ("Find Python Files", scenario_find_python_files),
        ("List Directory", scenario_whats_in_directory),
        ("Shell Context", scenario_shell_context),
    ]
    
    for name, func in scenarios:
        print(f"\n{'=' * 60}")
        print(f"Scenario: {name}")
        print('=' * 60)
        try:
            func()
        except Exception as e:
            print(f"Scenario failed: {e}")
    
    print("\n" + "=" * 60)
    print("All scenarios completed!")
    print("=" * 60)
    
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        sys.exit(1)
