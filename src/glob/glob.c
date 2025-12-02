/**
 * @file glob.c
 * @brief Implementation of wildcard/glob expansion
 * 
 * This module implements pattern matching and expansion for shell wildcards.
 * Supports *, ?, and [] character classes.
 */

#include "glob.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_MATCHES 1024

/**
 * @brief Check if pattern contains wildcard characters
 */
static bool has_wildcards(const char *pattern) {
    return strchr(pattern, '*') != NULL || 
           strchr(pattern, '?') != NULL || 
           strchr(pattern, '[') != NULL;
}

/**
 * @brief Match a character class [abc] or [a-z] or [!abc]
 * 
 * @param pattern Pointer to character after '['
 * @param ch Character to match
 * @param pattern_end Output: pointer to character after ']'
 * @return 1 if match, 0 otherwise
 */
static int match_char_class(const char *pattern, char ch, const char **pattern_end) {
    bool negate = false;
    bool matched = false;
    
    if (*pattern == '!') {
        negate = true;
        pattern++;
    }
    
    while (*pattern && *pattern != ']') {
        // Check for range like a-z
        if (pattern[1] == '-' && pattern[2] != ']' && pattern[2] != '\0') {
            if (ch >= pattern[0] && ch <= pattern[2]) {
                matched = true;
            }
            pattern += 3;
        } else {
            // Single character
            if (ch == *pattern) {
                matched = true;
            }
            pattern++;
        }
    }
    
    if (*pattern == ']') {
        *pattern_end = pattern + 1;
    } else {
        *pattern_end = pattern;
    }
    
    return negate ? !matched : matched;
}

/**
 * @brief Recursively match pattern against string
 */
int match_pattern(const char *pattern, const char *str) {
    // Base cases
    if (*pattern == '\0' && *str == '\0') {
        return 1;  // Both exhausted - match
    }
    
    if (*pattern == '\0') {
        return 0;  // Pattern exhausted but string remains
    }
    
    // Handle wildcards
    if (*pattern == '*') {
        // Try matching * with empty string
        if (match_pattern(pattern + 1, str)) {
            return 1;
        }
        // Try matching * with one or more characters
        if (*str != '\0' && match_pattern(pattern, str + 1)) {
            return 1;
        }
        return 0;
    }
    
    if (*pattern == '?') {
        // ? matches any single character
        if (*str == '\0') {
            return 0;  // No character to match
        }
        return match_pattern(pattern + 1, str + 1);
    }
    
    if (*pattern == '[') {
        // Character class
        const char *pattern_end;
        if (*str == '\0') {
            return 0;
        }
        if (match_char_class(pattern + 1, *str, &pattern_end)) {
            return match_pattern(pattern_end, str + 1);
        }
        return 0;
    }
    
    // Regular character - must match exactly
    if (*str == '\0' || *pattern != *str) {
        return 0;
    }
    
    return match_pattern(pattern + 1, str + 1);
}

/**
 * @brief Expand glob pattern to matching filenames
 */
char **expand_glob(const char *pattern, int *count) {
    *count = 0;
    
    // If no wildcards, return NULL (use literal)
    if (!has_wildcards(pattern)) {
        return NULL;
    }
    
    // Open current directory
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return NULL;
    }
    
    // Allocate array for matches
    char **matches = malloc(sizeof(char *) * MAX_MATCHES);
    if (matches == NULL) {
        closedir(dir);
        return NULL;
    }
    
    // Scan directory
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Skip hidden files unless pattern starts with .
        if (entry->d_name[0] == '.' && pattern[0] != '.') {
            continue;
        }
        
        // Check if name matches pattern
        if (match_pattern(pattern, entry->d_name)) {
            if (*count >= MAX_MATCHES) {
                fprintf(stderr, "glob: too many matches (max %d)\n", MAX_MATCHES);
                break;
            }
            
            matches[*count] = strdup(entry->d_name);
            if (matches[*count] == NULL) {
                perror("strdup");
                break;
            }
            (*count)++;
        }
    }
    
    closedir(dir);
    
    // If no matches, free and return NULL
    if (*count == 0) {
        free(matches);
        return NULL;
    }
    
    // Sort matches alphabetically (simple bubble sort)
    for (int i = 0; i < *count - 1; i++) {
        for (int j = 0; j < *count - i - 1; j++) {
            if (strcmp(matches[j], matches[j + 1]) > 0) {
                char *temp = matches[j];
                matches[j] = matches[j + 1];
                matches[j + 1] = temp;
            }
        }
    }
    
    return matches;
}

/**
 * @brief Free array of glob matches
 */
void free_glob_matches(char **matches, int count) {
    if (matches == NULL) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        free(matches[i]);
    }
    free(matches);
}
