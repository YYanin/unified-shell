#!/usr/bin/env python3
"""
MCP Client Example - Simple Python client for unified shell MCP server

This example demonstrates how to:
- Connect to the MCP server
- Initialize the connection
- List available tools
- Execute commands via call_tool
- Handle responses and errors
- Process notifications

Usage:
    python3 mcp_client_example.py
    
Prerequisites:
    - Shell running with USHELL_MCP_ENABLED=1
    - MCP server listening on port 9000 (or set MCP_PORT env var)
"""

import socket
import json
import os
import sys


class MCPClient:
    """Simple MCP (Model Context Protocol) client for shell interaction"""
    
    def __init__(self, host='localhost', port=9000):
        """
        Initialize MCP client
        
        Args:
            host: Server hostname (default: localhost)
            port: Server port (default: 9000)
        """
        self.host = host
        self.port = port
        self.socket = None
        self.buffer = ""
        
    def connect(self):
        """Connect to MCP server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            print(f"[+] Connected to MCP server at {self.host}:{self.port}")
            return True
        except ConnectionRefusedError:
            print(f"[-] Connection refused: Is the MCP server running on port {self.port}?")
            return False
        except Exception as e:
            print(f"[-] Connection error: {e}")
            return False
    
    def send_request(self, request_id, method, params=None):
        """
        Send JSON-RPC request to server
        
        Args:
            request_id: Unique request identifier (string)
            method: MCP method name (initialize, list_tools, call_tool)
            params: Method parameters (dict or None)
        
        Returns:
            True if sent successfully, False otherwise
        """
        if params is None:
            params = {}
        
        message = {
            "id": request_id,
            "method": method,
            "params": params
        }
        
        try:
            json_str = json.dumps(message) + "\n"
            self.socket.send(json_str.encode('utf-8'))
            return True
        except Exception as e:
            print(f"[-] Send error: {e}")
            return False
    
    def receive_message(self):
        """
        Receive newline-delimited JSON message from server
        
        Returns:
            Parsed JSON object, or None on error
        """
        try:
            while '\n' not in self.buffer:
                chunk = self.socket.recv(4096).decode('utf-8')
                if not chunk:
                    return None
                self.buffer += chunk
            
            line, self.buffer = self.buffer.split('\n', 1)
            return json.loads(line)
        except json.JSONDecodeError as e:
            print(f"[-] JSON parse error: {e}")
            return None
        except Exception as e:
            print(f"[-] Receive error: {e}")
            return None
    
    def close(self):
        """Close connection to server"""
        if self.socket:
            self.socket.close()
            self.socket = None
            print("[+] Connection closed")
    
    def initialize(self):
        """Initialize MCP session"""
        print("\n--- Initializing MCP Session ---")
        self.send_request("1", "initialize", {})
        response = self.receive_message()
        
        if response and response.get("type") == "response":
            result = response.get("result", {})
            print(f"[+] Server: {result.get('server', 'unknown')}")
            print(f"[+] Version: {result.get('version', 'unknown')}")
            return True
        else:
            print(f"[-] Initialization failed: {response}")
            return False
    
    def list_tools(self):
        """List all available tools"""
        print("\n--- Listing Available Tools ---")
        self.send_request("2", "list_tools", {})
        response = self.receive_message()
        
        if response and response.get("type") == "response":
            tools = response.get("result", {}).get("tools", [])
            print(f"[+] Found {len(tools)} tools:")
            
            # Show first 10 tools as example
            for tool in tools[:10]:
                name = tool.get("name", "unknown")
                desc = tool.get("description", "No description")
                print(f"    - {name}: {desc}")
            
            if len(tools) > 10:
                print(f"    ... and {len(tools) - 10} more tools")
            
            return tools
        else:
            print(f"[-] list_tools failed: {response}")
            return []
    
    def call_tool(self, tool_name, args=None):
        """
        Execute a tool with arguments
        
        Args:
            tool_name: Name of tool to execute
            args: Tool arguments (dict)
        
        Returns:
            Tool execution result, or None on error
        """
        if args is None:
            args = {}
        
        print(f"\n--- Calling Tool: {tool_name} ---")
        
        # Send request
        request_id = f"call_{tool_name}"
        self.send_request(request_id, "call_tool", {
            "tool": tool_name,
            "args": args
        })
        
        # Receive notifications and response
        while True:
            message = self.receive_message()
            if not message:
                print("[-] No response received")
                return None
            
            msg_type = message.get("type")
            
            if msg_type == "notification":
                event = message.get("event", "unknown")
                msg = message.get("message", "")
                print(f"[*] Notification: {event} - {msg}")
            
            elif msg_type == "response":
                result = message.get("result", {})
                output = result.get("output", "")
                exit_code = result.get("exit_code", -1)
                
                print(f"[+] Tool executed successfully")
                print(f"    Exit code: {exit_code}")
                if output:
                    print(f"    Output:\n{output}")
                
                return result
            
            elif msg_type == "error":
                error_msg = message.get("error", "Unknown error")
                print(f"[-] Error: {error_msg}")
                return None


def example_basic_usage():
    """Example 1: Basic MCP operations"""
    print("=" * 60)
    print("Example 1: Basic MCP Operations")
    print("=" * 60)
    
    # Get port from environment or use default
    port = int(os.getenv('MCP_PORT', '9000'))
    
    # Create client and connect
    client = MCPClient(port=port)
    if not client.connect():
        return False
    
    try:
        # Initialize session
        if not client.initialize():
            return False
        
        # List available tools
        tools = client.list_tools()
        
        # Execute simple commands
        client.call_tool("pwd", {})
        client.call_tool("echo", {"text": "Hello from MCP client!"})
        
        return True
        
    finally:
        client.close()


def example_shell_context():
    """Example 2: Get shell context"""
    print("\n" + "=" * 60)
    print("Example 2: Get Shell Context")
    print("=" * 60)
    
    port = int(os.getenv('MCP_PORT', '9000'))
    client = MCPClient(port=port)
    
    if not client.connect():
        return False
    
    try:
        client.initialize()
        
        # Get comprehensive shell context
        result = client.call_tool("get_shell_context", {})
        if result:
            print("\n[+] Shell context retrieved successfully")
        
        return True
        
    finally:
        client.close()


def example_command_search():
    """Example 3: Search for commands"""
    print("\n" + "=" * 60)
    print("Example 3: Command Search")
    print("=" * 60)
    
    port = int(os.getenv('MCP_PORT', '9000'))
    client = MCPClient(port=port)
    
    if not client.connect():
        return False
    
    try:
        client.initialize()
        
        # Search for file-related commands
        print("\n[*] Searching for 'list files' commands...")
        result = client.call_tool("search_commands", {
            "query": "list files"
        })
        
        if result:
            # Parse and display search results
            import json
            search_result = json.loads(result.get("output", "{}"))
            results = search_result.get("results", [])
            
            print(f"\n[+] Found {len(results)} matching commands:")
            for cmd in results:
                print(f"    - {cmd['name']}: {cmd['description']} (score: {cmd['score']})")
        
        return True
        
    finally:
        client.close()


def example_command_suggestion():
    """Example 4: Get command suggestions"""
    print("\n" + "=" * 60)
    print("Example 4: Command Suggestion")
    print("=" * 60)
    
    port = int(os.getenv('MCP_PORT', '9000'))
    client = MCPClient(port=port)
    
    if not client.connect():
        return False
    
    try:
        client.initialize()
        
        # Get command suggestion from natural language
        queries = [
            "find all python files",
            "show current directory",
            "list files with details"
        ]
        
        for query in queries:
            print(f"\n[*] Query: '{query}'")
            result = client.call_tool("suggest_command", {"query": query})
            
            if result:
                # Parse suggestion
                import json
                suggestion = json.loads(result.get("output", "{}"))
                cmd = suggestion.get("command", "")
                explanation = suggestion.get("explanation", "")
                print(f"    Suggested: {cmd}")
                print(f"    Reason: {explanation}")
        
        return True
        
    finally:
        client.close()


def main():
    """Run all examples"""
    print("\n" + "=" * 60)
    print("MCP Client Example - Unified Shell Integration")
    print("=" * 60)
    
    # Check if server is accessible
    port = int(os.getenv('MCP_PORT', '9000'))
    print(f"\nConnecting to MCP server on port {port}...")
    print("(Make sure shell is running with USHELL_MCP_ENABLED=1)\n")
    
    # Run examples
    examples = [
        ("Basic Operations", example_basic_usage),
        ("Shell Context", example_shell_context),
        ("Command Search", example_command_search),
        ("Command Suggestions", example_command_suggestion)
    ]
    
    results = []
    for name, func in examples:
        try:
            success = func()
            results.append((name, success))
        except Exception as e:
            print(f"\n[-] Example '{name}' failed with exception: {e}")
            results.append((name, False))
    
    # Print summary
    print("\n" + "=" * 60)
    print("Summary")
    print("=" * 60)
    for name, success in results:
        status = "PASS" if success else "FAIL"
        print(f"  [{status}] {name}")
    
    print("\nAll examples completed!")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[!] Interrupted by user")
        sys.exit(0)
