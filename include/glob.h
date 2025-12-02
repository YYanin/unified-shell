#ifndef GLOB_H
#define GLOB_H

/**
 * @file glob.h
 * @brief Wildcard/glob expansion for shell patterns
 * 
 * This module implements wildcard expansion for patterns containing
 * *, ?, and [] characters. Patterns are matched against files in the
 * current directory and expanded to a list of matching filenames.
 */

/**
 * @brief Expand a glob pattern to matching filenames
 * 
 * Scans the current directory for files matching the pattern.
 * Supports:
 *   - * (match any sequence of characters)
 *   - ? (match single character)
 *   - [abc] (match any of a, b, c)
 *   - [a-z] (match range)
 *   - [!abc] (match anything except a, b, c)
 * 
 * @param pattern The glob pattern to expand
 * @param count Output parameter for number of matches
 * @return Array of matched filenames (caller must free), NULL if no matches
 */
char **expand_glob(const char *pattern, int *count);

/**
 * @brief Check if a string matches a glob pattern
 * 
 * Recursively matches pattern against string.
 * 
 * @param pattern The glob pattern
 * @param str The string to match against
 * @return 1 if match, 0 otherwise
 */
int match_pattern(const char *pattern, const char *str);

/**
 * @brief Free array of strings returned by expand_glob
 * 
 * @param matches Array of strings to free
 * @param count Number of strings in array
 */
void free_glob_matches(char **matches, int count);

#endif // GLOB_H
