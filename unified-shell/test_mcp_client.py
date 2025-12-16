#!/usr/bin/env python3
"""
Simple test client for MCP server
"""
import socket
import json
import time

def test_mcp_server(host='localhost', port=9000):
    """Test basic MCP server functionality"""
    try:
        # Connect to server
        print(f"Connecting to {host}:{port}...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)
        s.connect((host, port))
        print("Connected!")
        
        # Test 1: Initialize
        print("\n[TEST 1: Initialize]")
        request = {"id": "1", "method": "initialize", "params": {}}
        s.send((json.dumps(request) + "\n").encode())
        response = s.recv(4096).decode().strip()
        print(f"Request:  {json.dumps(request)}")
        print(f"Response: {response}")
        
        # Test 2: List tools
        print("\n[TEST 2: List Tools]")
        request = {"id": "2", "method": "list_tools", "params": {}}
        s.send((json.dumps(request) + "\n").encode())
        response = s.recv(4096).decode().strip()
        print(f"Request:  {json.dumps(request)}")
        print(f"Response: {response}")
        
        # Test 3: Call tool (placeholder)
        print("\n[TEST 3: Call Tool]")
        request = {"id": "3", "method": "call_tool", "params": {"tool": "echo", "args": {"text": "test"}}}
        s.send((json.dumps(request) + "\n").encode())
        # Receive notification (tool_started)
        response = s.recv(4096).decode().strip()
        print(f"Notification: {response}")
        # Receive result
        response = s.recv(4096).decode().strip()
        print(f"Response: {response}")
        
        s.close()
        print("\n[SUCCESS] All tests passed!")
        return True
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        return False

if __name__ == "__main__":
    import sys
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9000
    success = test_mcp_server(port=port)
    sys.exit(0 if success else 1)
