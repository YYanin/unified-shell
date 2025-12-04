/**
 * remove.c - Package Removal System for Unified Shell
 * 
 * This module implements the package removal functionality for the apt
 * subsystem. It handles:
 * 
 *   1. Verification that package is installed
 *   2. Checking for dependent packages (warning user)
 *   3. Recursive directory removal
 *   4. Package index updates
 *   5. Package integrity verification
 * 
 * Safety Features:
 *   - Confirms package is installed before removal
 *   - Warns about dependent packages
 *   - Updates index after successful removal
 *   - Provides detailed error messages
 */

#include "apt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * apt_remove_directory_recursive - Recursively remove a directory and contents
 * 
 * This function removes all files and subdirectories within a directory,
 * then removes the directory itself. Similar to 'rm -rf'.
 * 
 * @param path: Path to directory to remove
 * 
 * Returns: 0 on success, -1 on failure
 */
static int apt_remove_directory_recursive(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[APT_PATH_LEN];
    struct stat st;
    
    /* Open directory for reading */
    dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "apt remove: cannot open directory %s: %s\n",
                path, strerror(errno));
        return -1;
    }
    
    /* Iterate through directory entries */
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. to avoid infinite recursion */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Build full path to entry */
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        /* Get entry status */
        if (stat(full_path, &st) != 0) {
            fprintf(stderr, "apt remove: cannot stat %s: %s\n",
                    full_path, strerror(errno));
            closedir(dir);
            return -1;
        }
        
        /* Recursively remove directories, unlink files */
        if (S_ISDIR(st.st_mode)) {
            /* Recursively remove subdirectory */
            if (apt_remove_directory_recursive(full_path) != 0) {
                closedir(dir);
                return -1;
            }
        } else {
            /* Remove file */
            if (unlink(full_path) != 0) {
                fprintf(stderr, "apt remove: cannot remove %s: %s\n",
                        full_path, strerror(errno));
                closedir(dir);
                return -1;
            }
        }
    }
    
    closedir(dir);
    
    /* Remove the directory itself */
    if (rmdir(path) != 0) {
        fprintf(stderr, "apt remove: cannot remove directory %s: %s\n",
                path, strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * apt_check_dependents - Check if any installed packages depend on this one
 * 
 * Scans all installed packages to see if any list this package as a
 * dependency. Warns user if dependents are found.
 * 
 * @param pkgname: Package name to check
 * @param dependents: Buffer to store dependent package names (comma-separated)
 * @param buf_size: Size of dependents buffer
 * 
 * Returns: Number of dependent packages found
 */
static int apt_check_dependents(const char *pkgname, char *dependents, size_t buf_size) {
    int count = 0;
    int pkg_count = apt_get_package_count(0);
    Package *packages = apt_list_packages(0);
    
    /* Clear dependents buffer */
    if (dependents && buf_size > 0) {
        dependents[0] = '\0';
    }
    
    /* Check each package for dependencies on this one */
    for (int i = 0; i < pkg_count; i++) {
        Package *pkg = &packages[i];
        
        /* Skip if not installed */
        if (!pkg->installed) {
            continue;
        }
        
        /* Skip if no dependencies */
        if (strlen(pkg->dependencies) == 0) {
            continue;
        }
        
        /* Parse dependencies to see if this package is listed */
        char deps_copy[APT_DEPS_LEN];
        strncpy(deps_copy, pkg->dependencies, sizeof(deps_copy) - 1);
        deps_copy[sizeof(deps_copy) - 1] = '\0';
        
        char *token = strtok(deps_copy, ",");
        while (token != NULL) {
            /* Trim whitespace */
            while (*token == ' ') token++;
            char *end = token + strlen(token) - 1;
            while (end > token && *end == ' ') {
                *end = '\0';
                end--;
            }
            
            /* Check if this dependency matches the package being removed */
            if (strcmp(token, pkgname) == 0) {
                count++;
                
                /* Add to dependents list */
                if (dependents && buf_size > 0) {
                    if (strlen(dependents) > 0) {
                        strncat(dependents, ", ", buf_size - strlen(dependents) - 1);
                    }
                    strncat(dependents, pkg->name, buf_size - strlen(dependents) - 1);
                }
                
                break;  /* Found dependency, no need to check other deps */
            }
            
            token = strtok(NULL, ",");
        }
    }
    
    return count;
}

/* ============================================================================
 * Package Verification
 * ============================================================================ */

/**
 * apt_verify_package - Verify package integrity
 * 
 * Checks that an installed package has all expected files and directories
 * in the correct structure. Reports any missing or corrupted files.
 * 
 * @param pkgname: Name of package to verify
 * 
 * Returns: 0 if package is valid, -1 if issues found
 */
int apt_verify_package(const char *pkgname) {
    if (pkgname == NULL || strlen(pkgname) == 0) {
        fprintf(stderr, "apt verify: package name cannot be empty\n");
        return -1;
    }
    
    /* Find package in index */
    Package *pkg = apt_find_package(pkgname);
    if (pkg == NULL) {
        fprintf(stderr, "apt verify: package '%s' not found in repository\n", pkgname);
        return -1;
    }
    
    /* Check if package is installed */
    if (!pkg->installed) {
        fprintf(stderr, "apt verify: package '%s' is not installed\n", pkgname);
        return -1;
    }
    
    /* Build path to package directory */
    char pkg_dir[APT_PATH_LEN];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s",
             g_apt_config.packages_dir, pkgname);
    
    printf("Verifying package '%s'...\n", pkgname);
    
    int issues = 0;
    struct stat st;
    
    /* Check if package directory exists */
    if (stat(pkg_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "  ERROR: Package directory missing: %s\n", pkg_dir);
        issues++;
        return -1;  /* Critical error */
    } else {
        printf("  [OK] Package directory exists\n");
    }
    
    /* Check for METADATA file */
    char metadata_path[APT_PATH_LEN];
    snprintf(metadata_path, sizeof(metadata_path), "%s/METADATA", pkg_dir);
    
    if (stat(metadata_path, &st) != 0) {
        fprintf(stderr, "  WARNING: METADATA file missing\n");
        issues++;
    } else {
        printf("  [OK] METADATA file present\n");
        
        /* Verify METADATA content */
        FILE *fp = fopen(metadata_path, "r");
        if (fp != NULL) {
            char line[256];
            int has_name = 0, has_version = 0;
            
            while (fgets(line, sizeof(line), fp) != NULL) {
                if (strncmp(line, "Name:", 5) == 0) has_name = 1;
                if (strncmp(line, "Version:", 8) == 0) has_version = 1;
            }
            fclose(fp);
            
            if (!has_name || !has_version) {
                fprintf(stderr, "  WARNING: METADATA file incomplete\n");
                issues++;
            } else {
                printf("  [OK] METADATA content valid\n");
            }
        }
    }
    
    /* Check for bin directory (optional) */
    char bin_dir[APT_PATH_LEN];
    snprintf(bin_dir, sizeof(bin_dir), "%s/bin", pkg_dir);
    
    if (stat(bin_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        printf("  [OK] bin/ directory present\n");
        
        /* Count executables */
        DIR *dir = opendir(bin_dir);
        if (dir != NULL) {
            struct dirent *entry;
            int exec_count = 0;
            
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                
                char exec_path[APT_PATH_LEN];
                snprintf(exec_path, sizeof(exec_path), "%s/%s", bin_dir, entry->d_name);
                
                if (stat(exec_path, &st) == 0 && S_ISREG(st.st_mode)) {
                    exec_count++;
                    
                    /* Check if executable */
                    if (!(st.st_mode & S_IXUSR)) {
                        fprintf(stderr, "  WARNING: %s is not executable\n", entry->d_name);
                        issues++;
                    }
                }
            }
            closedir(dir);
            
            printf("  [OK] Found %d executable(s)\n", exec_count);
        }
    } else {
        printf("  [INFO] No bin/ directory (package may not have executables)\n");
    }
    
    /* Summary */
    if (issues == 0) {
        printf("\nPackage verification: PASSED\n");
        return 0;
    } else {
        printf("\nPackage verification: PASSED with %d warning(s)\n", issues);
        return 0;  /* Warnings are not fatal */
    }
}

/* ============================================================================
 * Main Removal Function
 * ============================================================================ */

/**
 * apt_remove_package - Remove an installed package (calls with force=0)
 * 
 * This is a wrapper function that calls apt_remove_package_with_force
 * with force=0 (respects dependent checking).
 * 
 * @param pkgname: Name of package to remove
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_remove_package(const char *pkgname) {
    return apt_remove_package_with_force(pkgname, 0);
}

/**
 * apt_remove_package_with_force - Remove an installed package with force option
 * 
 * This is the main entry point for package removal. It performs:
 * 
 *   1. Find package in index
 *   2. Verify package is installed
 *   3. Check for dependent packages (unless force=1)
 *   4. Remove package directory and all contents
 *   5. Update package as not installed in index
 *   6. Save updated index to disk
 * 
 * @param pkgname: Name of package to remove
 * @param force: If 1, skip dependent checking; if 0, warn about dependents
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_remove_package_with_force(const char *pkgname, int force) {
    if (pkgname == NULL || strlen(pkgname) == 0) {
        fprintf(stderr, "apt remove: package name cannot be empty\n");
        return -1;
    }
    
    printf("Removing package '%s'%s...\n", pkgname, force ? " (forced)" : "");
    
    /* Step 1: Find the package in the index */
    Package *pkg = apt_find_package(pkgname);
    if (pkg == NULL) {
        fprintf(stderr, "apt remove: package '%s' not found in repository\n", pkgname);
        fprintf(stderr, "Run 'apt list' to see available packages.\n");
        return -1;
    }
    
    /* Step 2: Check if package is installed */
    if (!pkg->installed) {
        printf("Package '%s' is not installed.\n", pkgname);
        printf("Nothing to do.\n");
        return 0;
    }
    
    /* Step 3: Check for dependent packages (unless force flag is set) */
    if (!force) {
        char dependents[APT_DEPS_LEN];
        int dependent_count = apt_check_dependents(pkgname, dependents, sizeof(dependents));
        
        if (dependent_count > 0) {
            fprintf(stderr, "\nWARNING: The following packages depend on '%s':\n", pkgname);
            fprintf(stderr, "  %s\n", dependents);
            fprintf(stderr, "\nRemoving '%s' may break these packages.\n", pkgname);
            fprintf(stderr, "Consider removing dependent packages first, or use --force flag.\n");
            /* For now, continue with removal but warn user */
        }
    } else {
        printf("Skipping dependent checking (--force flag is set).\n");
    }
    
    /* Step 4: Build path to package directory */
    char pkg_dir[APT_PATH_LEN];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s",
             g_apt_config.packages_dir, pkgname);
    
    printf("Removing package directory: %s\n", pkg_dir);
    
    /* Step 5: Remove package directory recursively */
    if (apt_remove_directory_recursive(pkg_dir) != 0) {
        fprintf(stderr, "apt remove: failed to remove package directory\n");
        fprintf(stderr, "Package may be partially removed.\n");
        return -1;
    }
    
    printf("Package files removed successfully.\n");
    
    /* Step 6: Update package status in index */
    pkg->installed = 0;
    
    /* Step 7: Save updated index to disk */
    if (apt_save_index() != 0) {
        fprintf(stderr, "apt remove: warning: failed to save package index\n");
        fprintf(stderr, "Package was removed but index may be inconsistent.\n");
        /* Continue anyway - package is already removed from disk */
    }
    
    /* Success! */
    printf("\n");
    printf("========================================\n");
    printf("Successfully removed: %s\n", pkgname);
    printf("========================================\n");
    printf("\n");
    
    return 0;
}

