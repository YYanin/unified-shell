/**
 * depends.c - Dependency Management for Unified Shell Package Manager
 * 
 * This module implements dependency resolution and management for the apt
 * subsystem. It handles:
 * 
 *   1. Dependency parsing and extraction
 *   2. Circular dependency detection
 *   3. Automatic dependency resolution
 *   4. Dependency installation ordering
 *   5. Reverse dependency checking (what depends on this package)
 * 
 * Dependency Format:
 *   In index.txt: "Depends: pkg1, pkg2, pkg3"
 *   Or: "Depends: none" or empty for no dependencies
 */

#include "apt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum depth for dependency resolution to prevent infinite loops */
#define MAX_DEPENDENCY_DEPTH 10

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * apt_parse_dependencies - Parse dependency string into array
 * 
 * Takes a comma-separated dependency string and splits it into an
 * array of individual package names.
 * 
 * @param deps_str: Dependency string (e.g., "pkg1, pkg2, pkg3")
 * @param deps_array: Output array to store dependency names
 * @param max_deps: Maximum number of dependencies to parse
 * 
 * Returns: Number of dependencies parsed
 */
static int apt_parse_dependencies(const char *deps_str, char deps_array[][APT_NAME_LEN], int max_deps) {
    if (deps_str == NULL || strlen(deps_str) == 0 || 
        strcmp(deps_str, "none") == 0 || strcmp(deps_str, "") == 0) {
        return 0;
    }
    
    /* Make a copy for tokenization */
    char deps_copy[APT_DEPS_LEN];
    strncpy(deps_copy, deps_str, sizeof(deps_copy) - 1);
    deps_copy[sizeof(deps_copy) - 1] = '\0';
    
    int count = 0;
    char *token = strtok(deps_copy, ",");
    
    while (token != NULL && count < max_deps) {
        /* Trim leading/trailing whitespace */
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') {
            *end = '\0';
            end--;
        }
        
        /* Skip empty tokens */
        if (strlen(token) == 0) {
            token = strtok(NULL, ",");
            continue;
        }
        
        /* Copy to output array */
        strncpy(deps_array[count], token, APT_NAME_LEN - 1);
        deps_array[count][APT_NAME_LEN - 1] = '\0';
        count++;
        
        token = strtok(NULL, ",");
    }
    
    return count;
}

/**
 * apt_check_circular_dependency - Detect circular dependencies
 * 
 * Recursively checks if installing a package would create a circular
 * dependency chain. Uses a visited array to track packages in the
 * current dependency path.
 * 
 * @param pkgname: Package to check
 * @param visited: Array of visited package names in current path
 * @param depth: Current recursion depth
 * 
 * Returns: 1 if circular dependency detected, 0 otherwise
 */
static int apt_check_circular_dependency(const char *pkgname, char visited[][APT_NAME_LEN], int depth) {
    /* Check depth limit to prevent infinite recursion */
    if (depth >= MAX_DEPENDENCY_DEPTH) {
        fprintf(stderr, "Dependency resolution depth exceeded (possible circular dependency)\n");
        return 1;
    }
    
    /* Check if this package is already in the visited path */
    for (int i = 0; i < depth; i++) {
        if (strcmp(visited[i], pkgname) == 0) {
            /* Circular dependency detected */
            fprintf(stderr, "Circular dependency detected: ");
            for (int j = i; j < depth; j++) {
                fprintf(stderr, "%s -> ", visited[j]);
            }
            fprintf(stderr, "%s\n", pkgname);
            return 1;
        }
    }
    
    /* Add current package to visited path */
    strncpy(visited[depth], pkgname, APT_NAME_LEN - 1);
    visited[depth][APT_NAME_LEN - 1] = '\0';
    
    /* Get package dependencies */
    Package *pkg = apt_find_package(pkgname);
    if (pkg == NULL) {
        return 0;  /* Package not found, not a circular dependency issue */
    }
    
    /* Parse dependencies */
    char deps[APT_MAX_PACKAGES][APT_NAME_LEN];
    int dep_count = apt_parse_dependencies(pkg->dependencies, deps, APT_MAX_PACKAGES);
    
    /* Recursively check each dependency */
    for (int i = 0; i < dep_count; i++) {
        if (apt_check_circular_dependency(deps[i], visited, depth + 1)) {
            return 1;
        }
    }
    
    return 0;
}

/* ============================================================================
 * Public Dependency Functions
 * ============================================================================ */

/**
 * apt_get_dependencies - Get list of dependencies for a package
 * 
 * Parses the package's Depends field and returns an array of dependency
 * names. Caller is responsible for freeing the returned array.
 * 
 * @param pkgname: Package name to get dependencies for
 * @param dep_count: Output parameter for number of dependencies
 * 
 * Returns: Array of dependency names (caller must free), NULL on error
 */
char** apt_get_dependencies(const char *pkgname, int *dep_count) {
    if (pkgname == NULL || dep_count == NULL) {
        return NULL;
    }
    
    *dep_count = 0;
    
    /* Find package */
    Package *pkg = apt_find_package(pkgname);
    if (pkg == NULL) {
        return NULL;
    }
    
    /* Parse dependencies */
    char deps[APT_MAX_PACKAGES][APT_NAME_LEN];
    int count = apt_parse_dependencies(pkg->dependencies, deps, APT_MAX_PACKAGES);
    
    if (count == 0) {
        return NULL;
    }
    
    /* Allocate array for dependency names */
    char **dep_array = (char **)malloc(count * sizeof(char *));
    if (dep_array == NULL) {
        fprintf(stderr, "apt: memory allocation failed\n");
        return NULL;
    }
    
    /* Copy dependency names */
    for (int i = 0; i < count; i++) {
        dep_array[i] = strdup(deps[i]);
        if (dep_array[i] == NULL) {
            /* Free previously allocated strings */
            for (int j = 0; j < i; j++) {
                free(dep_array[j]);
            }
            free(dep_array);
            fprintf(stderr, "apt: memory allocation failed\n");
            return NULL;
        }
    }
    
    *dep_count = count;
    return dep_array;
}

/**
 * apt_check_dependencies - Check if all dependencies are satisfied
 * 
 * Verifies that all packages listed as dependencies are installed.
 * 
 * @param pkgname: Package name to check dependencies for
 * @param missing: Output buffer for missing dependency names (comma-separated)
 * @param missing_size: Size of missing buffer
 * 
 * Returns: Number of missing dependencies, -1 on error
 */
int apt_check_dependencies(const char *pkgname, char *missing, size_t missing_size) {
    if (pkgname == NULL) {
        return -1;
    }
    
    /* Clear missing buffer */
    if (missing && missing_size > 0) {
        missing[0] = '\0';
    }
    
    /* Get dependencies */
    int dep_count = 0;
    char **deps = apt_get_dependencies(pkgname, &dep_count);
    
    if (deps == NULL || dep_count == 0) {
        return 0;  /* No dependencies */
    }
    
    /* Check each dependency */
    int missing_count = 0;
    
    for (int i = 0; i < dep_count; i++) {
        if (!apt_is_installed(deps[i])) {
            missing_count++;
            
            /* Add to missing list */
            if (missing && missing_size > 0) {
                if (strlen(missing) > 0) {
                    strncat(missing, ", ", missing_size - strlen(missing) - 1);
                }
                strncat(missing, deps[i], missing_size - strlen(missing) - 1);
            }
        }
    }
    
    /* Free dependency array */
    for (int i = 0; i < dep_count; i++) {
        free(deps[i]);
    }
    free(deps);
    
    return missing_count;
}

/**
 * apt_resolve_dependencies - Recursively resolve all dependencies
 * 
 * Builds a complete ordered list of packages that need to be installed
 * to satisfy all dependencies for the given package. The list is ordered
 * such that dependencies are installed before packages that depend on them.
 * 
 * @param pkgname: Package to resolve dependencies for
 * @param install_order: Output array for packages in installation order
 * @param max_packages: Maximum size of install_order array
 * 
 * Returns: Number of packages to install (including pkgname), -1 on error
 */
int apt_resolve_dependencies(const char *pkgname, char install_order[][APT_NAME_LEN], int max_packages) {
    if (pkgname == NULL || install_order == NULL) {
        return -1;
    }
    
    /* Check for circular dependencies */
    char visited[MAX_DEPENDENCY_DEPTH][APT_NAME_LEN];
    if (apt_check_circular_dependency(pkgname, visited, 0)) {
        fprintf(stderr, "Cannot resolve dependencies due to circular dependency\n");
        return -1;
    }
    
    /* Track which packages are already in install order */
    int order_count = 0;
    
    /* Recursive helper to add dependencies */
    int resolve_recursive(const char *pkg) {
        /* Check if already in install order */
        for (int i = 0; i < order_count; i++) {
            if (strcmp(install_order[i], pkg) == 0) {
                return 0;  /* Already added */
            }
        }
        
        /* Check if already installed */
        if (apt_is_installed(pkg)) {
            return 0;  /* Already installed, skip */
        }
        
        /* Get dependencies of this package */
        int dep_count = 0;
        char **deps = apt_get_dependencies(pkg, &dep_count);
        
        /* Recursively add dependencies first */
        if (deps != NULL) {
            for (int i = 0; i < dep_count; i++) {
                if (resolve_recursive(deps[i]) != 0) {
                    /* Free and return error */
                    for (int j = 0; j < dep_count; j++) {
                        free(deps[j]);
                    }
                    free(deps);
                    return -1;
                }
            }
            
            /* Free dependency array */
            for (int i = 0; i < dep_count; i++) {
                free(deps[i]);
            }
            free(deps);
        }
        
        /* Add this package to install order */
        if (order_count >= max_packages) {
            fprintf(stderr, "Too many dependencies to resolve\n");
            return -1;
        }
        
        strncpy(install_order[order_count], pkg, APT_NAME_LEN - 1);
        install_order[order_count][APT_NAME_LEN - 1] = '\0';
        order_count++;
        
        return 0;
    }
    
    /* Start recursive resolution */
    if (resolve_recursive(pkgname) != 0) {
        return -1;
    }
    
    return order_count;
}

/**
 * apt_install_dependencies - Automatically install all dependencies
 * 
 * Resolves dependencies and installs them in the correct order.
 * This is the main function called when --auto-install flag is used.
 * 
 * @param pkgname: Package to install dependencies for
 * @param install_self: If 1, also install pkgname after dependencies
 * 
 * Returns: 0 on success, -1 on error
 */
int apt_install_dependencies(const char *pkgname, int install_self) {
    if (pkgname == NULL) {
        return -1;
    }
    
    printf("Resolving dependencies for '%s'...\n", pkgname);
    
    /* Resolve dependency order */
    char install_order[APT_MAX_PACKAGES][APT_NAME_LEN];
    int count = apt_resolve_dependencies(pkgname, install_order, APT_MAX_PACKAGES);
    
    if (count < 0) {
        fprintf(stderr, "Failed to resolve dependencies\n");
        return -1;
    }
    
    if (count == 0) {
        printf("No dependencies to install.\n");
        if (install_self) {
            /* Just install the package itself */
            return apt_install_package(pkgname);
        }
        return 0;
    }
    
    /* Display installation plan */
    printf("\nThe following packages will be installed:\n");
    for (int i = 0; i < count; i++) {
        printf("  %d. %s", i + 1, install_order[i]);
        if (strcmp(install_order[i], pkgname) == 0) {
            printf(" (requested)");
        } else {
            printf(" (dependency)");
        }
        printf("\n");
    }
    printf("\n");
    
    /* Install each package in order */
    for (int i = 0; i < count; i++) {
        printf("Installing dependency %d/%d: %s\n", i + 1, count, install_order[i]);
        
        /* Skip if already installed */
        if (apt_is_installed(install_order[i])) {
            printf("Package '%s' is already installed, skipping.\n", install_order[i]);
            continue;
        }
        
        /* Install the package */
        if (apt_install_package(install_order[i]) != 0) {
            fprintf(stderr, "\nFailed to install dependency: %s\n", install_order[i]);
            fprintf(stderr, "Installation aborted.\n");
            return -1;
        }
        
        printf("\n");
    }
    
    printf("All dependencies installed successfully.\n");
    
    /* Install the requested package if not already in the list */
    if (install_self) {
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(install_order[i], pkgname) == 0) {
                found = 1;
                break;
            }
        }
        
        if (!found) {
            printf("\nInstalling requested package: %s\n", pkgname);
            return apt_install_package(pkgname);
        }
    }
    
    return 0;
}

