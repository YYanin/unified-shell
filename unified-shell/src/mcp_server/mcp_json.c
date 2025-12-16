/*
 * mcp_json.c - Simple JSON helper for MCP protocol
 * 
 * Educational implementation for basic JSON operations.
 * For production use, replace with cJSON or jsmn library.
 */

#include "mcp_json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * mcp_json_extract_string - Extract string value from JSON object
 * 
 * Simple string extraction that looks for "field":"value" pattern.
 * Handles basic escaping and whitespace.
 */
int mcp_json_extract_string(const char *json, const char *field, char *output, size_t output_size) {
    if (!json || !field || !output || output_size == 0) {
        return 0;
    }
    
    /* Build search pattern: "field" */
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", field);
    
    /* Find field in JSON */
    const char *field_pos = strstr(json, pattern);
    if (!field_pos) {
        return 0;
    }
    
    /* Find colon after field name */
    const char *colon = strchr(field_pos, ':');
    if (!colon) {
        return 0;
    }
    
    /* Skip whitespace and opening quote */
    const char *value_start = colon + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t' || *value_start == '\n')) {
        value_start++;
    }
    
    /* Check if value is a string (starts with quote) */
    if (*value_start != '"') {
        /* Could be number, boolean, or null - copy until delimiter */
        size_t i = 0;
        while (*value_start && *value_start != ',' && *value_start != '}' && 
               *value_start != ']' && *value_start != '\n' && i + 1 < output_size) {
            if (!isspace((unsigned char)*value_start)) {
                output[i++] = *value_start;
            }
            value_start++;
        }
        output[i] = '\0';
        /* Trim trailing whitespace */
        while (i > 0 && isspace((unsigned char)output[i-1])) {
            output[--i] = '\0';
        }
        return i > 0 ? 1 : 0;
    }
    
    /* Skip opening quote */
    value_start++;
    
    /* Copy value until closing quote, handling escapes */
    size_t i = 0;
    int escaped = 0;
    while (*value_start && i + 1 < output_size) {
        if (escaped) {
            /* Handle escape sequence */
            switch (*value_start) {
                case 'n': output[i++] = '\n'; break;
                case 't': output[i++] = '\t'; break;
                case 'r': output[i++] = '\r'; break;
                case '\\': output[i++] = '\\'; break;
                case '"': output[i++] = '"'; break;
                default: output[i++] = *value_start; break;
            }
            escaped = 0;
        } else if (*value_start == '\\') {
            escaped = 1;
        } else if (*value_start == '"') {
            /* End of string value */
            break;
        } else {
            output[i++] = *value_start;
        }
        value_start++;
    }
    
    output[i] = '\0';
    return 1;
}

/*
 * mcp_json_escape - Escape special characters for JSON string
 * 
 * Replaces special characters with their escaped equivalents.
 */
void mcp_json_escape(const char *input, char *output, size_t output_size) {
    if (!input || !output || output_size == 0) {
        return;
    }
    
    size_t i = 0;
    while (*input && i + 2 < output_size) {  /* +2 for potential escape */
        switch (*input) {
            case '"':
                output[i++] = '\\';
                output[i++] = '"';
                break;
            case '\\':
                output[i++] = '\\';
                output[i++] = '\\';
                break;
            case '\n':
                output[i++] = '\\';
                output[i++] = 'n';
                break;
            case '\r':
                output[i++] = '\\';
                output[i++] = 'r';
                break;
            case '\t':
                output[i++] = '\\';
                output[i++] = 't';
                break;
            case '\b':
                output[i++] = '\\';
                output[i++] = 'b';
                break;
            case '\f':
                output[i++] = '\\';
                output[i++] = 'f';
                break;
            default:
                /* Only include printable ASCII characters */
                if ((unsigned char)*input >= 32 && (unsigned char)*input <= 126) {
                    output[i++] = *input;
                }
                break;
        }
        input++;
    }
    output[i] = '\0';
}

/*
 * mcp_json_build_response - Build a simple JSON response
 * 
 * Creates a JSON response object with id, type, and result fields.
 */
char* mcp_json_build_response(const char *id, const char *type, const char *result) {
    if (!type || !result) {
        return NULL;
    }
    
    /* Allocate buffer for response */
    size_t size = 1024 + strlen(result);
    char *response = malloc(size);
    if (!response) {
        return NULL;
    }
    
    /* Build JSON response */
    if (id) {
        snprintf(response, size, "{\"id\":\"%s\",\"type\":\"%s\",\"result\":%s}", 
                 id, type, result);
    } else {
        snprintf(response, size, "{\"id\":null,\"type\":\"%s\",\"result\":%s}", 
                 type, result);
    }
    
    return response;
}

/*
 * mcp_json_build_error - Build an error response
 * 
 * Creates a JSON error response with escaped error message.
 */
char* mcp_json_build_error(const char *id, const char *error_message) {
    if (!error_message) {
        return NULL;
    }
    
    /* Escape error message */
    char escaped[512];
    mcp_json_escape(error_message, escaped, sizeof(escaped));
    
    /* Allocate buffer for error response */
    size_t size = 512 + strlen(escaped);
    char *response = malloc(size);
    if (!response) {
        return NULL;
    }
    
    /* Build JSON error response */
    if (id) {
        snprintf(response, size, "{\"id\":\"%s\",\"type\":\"error\",\"error\":\"%s\"}", 
                 id, escaped);
    } else {
        snprintf(response, size, "{\"id\":null,\"type\":\"error\",\"error\":\"%s\"}", 
                 escaped);
    }
    
    return response;
}
