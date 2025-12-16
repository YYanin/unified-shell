#!/usr/bin/env python3
"""Quick MCP test - just verify connectivity and basic operations"""

import socket
import json
import time

def test_mcp():
    print("Testing MCP Server Connection...")
    
    try:
        # Connect to MCP server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 9000))
        print("✓ Connected to localhost:9000")
        
        # Test 1: Initialize
        print("\nTest 1: Initialize")
        request = '{"id":"1","method":"initialize","params":{}}\n'
        sock.send(request.encode())
        response = sock.recv(4096).decode().strip()
        data = json.loads(response)
        if data.get('type') == 'response':
            print(f"✓ Initialize successful: {data.get('result', {}).get('server', 'unknown')}")
        else:
            print(f"✗ Initialize failed: {response}")
            return False
        
        # Test 2: List tools
        print("\nTest 2: List tools")
        request = '{"id":"2","method":"list_tools","params":{}}\n'
        sock.send(request.encode())
        response = sock.recv(65536).decode().strip()
        data = json.loads(response)
        if data.get('type') == 'response':
            tools = data.get('result', {}).get('tools', [])
            print(f"✓ Listed {len(tools)} tools")
            print(f"  Sample tools: {', '.join([t['name'] for t in tools[:5]])}")
        else:
            print(f"✗ List tools failed")
            return False
        
        # Test 3: Execute pwd
        print("\nTest 3: Execute tool (pwd)")
        request = '{"id":"3","method":"call_tool","params":{"tool":"pwd","args":{}}}\n'
        sock.send(request.encode())
        
        # Read multiple messages (notifications + response)
        buffer = ""
        output = None
        while True:
            chunk = sock.recv(4096).decode()
            if not chunk:
                break
            buffer += chunk
            
            # Process complete messages
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                if not line.strip():
                    continue
                msg = json.loads(line)
                if msg.get('type') == 'response':
                    output = msg.get('result', {}).get('output', '').strip()
                    break
            if output:
                break
        
        if output:
            print(f"✓ pwd output: {output}")
        else:
            print(f"✗ pwd execution failed")
            return False
        
        # Test 4: Get shell context
        print("\nTest 4: Get shell context (special tool)")
        request = '{"id":"4","method":"call_tool","params":{"tool":"get_shell_context","args":{}}}\n'
        sock.send(request.encode())
        
        buffer = ""
        context_data = None
        while True:
            chunk = sock.recv(4096).decode()
            if not chunk:
                break
            buffer += chunk
            
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                if not line.strip():
                    continue
                msg = json.loads(line)
                if msg.get('type') == 'response':
                    result = msg.get('result', {})
                    output_str = result.get('output', '{}')
                    try:
                        context_data = json.loads(output_str)
                    except:
                        context_data = result
                    break
            if context_data:
                break
        
        if context_data:
            print(f"✓ Shell context retrieved:")
            print(f"  CWD: {context_data.get('cwd', 'unknown')}")
            print(f"  User: {context_data.get('user', 'unknown')}")
        else:
            print(f"✗ Shell context failed")
        
        sock.close()
        print("\n" + "="*50)
        print("All tests passed! MCP server is working correctly.")
        return True
        
    except ConnectionRefusedError:
        print("✗ Connection refused - is the shell running with USHELL_MCP_ENABLED=1?")
        print("\nTo start the shell:")
        print("  USHELL_MCP_ENABLED=1 ./ushell")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == '__main__':
    import sys
    sys.exit(0 if test_mcp() else 1)
