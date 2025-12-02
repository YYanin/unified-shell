/**
 * install.c - Package Installation System for Unified Shell
 * 
 * This module implements the package installation functionality for the apt
 * subsystem. It handles:
 * 
 *   1. Dependency checking
 *   2. Package archive extraction (tar.gz format)
 *   3. File installation to ~/.ushell/packages/
 *   4. Metadata management
 *   5. Making executables accessible via PATH
 * 
 * Package Format:
 *   pkgname-version.tar.gz containing:
 *     bin/           - Executables
 *     lib/           - Libraries (optional)
 *     METADATA       - Package information
 *     README         - Documentation (optional)
 */

#include "apt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * apt_check_dependencies_for_package - Check if package dependencies are installed
 * 
 * Wrapper function that calls the dependency checking from depends.c
 * 
 * @param pkgname: Package name to check dependencies for
 * 
 * Returns: 0 if all dependencies satisfied, -1 if missing dependencies
 */
static int apt_check_dependencies_for_package(const char *pkgname) {
    char missing[APT_DEPS_LEN];
    int missing_count = apt_check_dependencies(pkgname, missing, sizeof(missing));
    
    if (missing_count < 0) {
        return -1;  /* Error */
    }
    
    if (missing_count > 0) {
        fprintf(stderr, "apt install: missing dependencies: %s\n", missing);
        fprintf(stderr, "Install dependencies first:\n");
        fprintf(stderr, "  apt install %s\n", missing);
        fprintf(stderr, "Or use --auto-install flag to install dependencies automatically.\n");
        return -1;
    }
    
    return 0;
}

/**
 * apt_create_metadata - Create METADATA file for installed package
 * 
 * Creates a metadata file in the package directory with installation
 * information including name, version, install date, etc.
 * 
 * @param pkg: Package being installed
 * @param pkg_dir: Installation directory path
 * 
 * Returns: 0 on success, -1 on failure
 */
static int apt_create_metadata(const Package *pkg, const char *pkg_dir) {
    char metadata_path[APT_PATH_LEN];
    snprintf(metadata_path, sizeof(metadata_path), "%s/METADATA", pkg_dir);
    
    FILE *fp = fopen(metadata_path, "w");
    if (fp == NULL) {
        fprintf(stderr, "apt install: cannot create metadata file: %s\n", 
                strerror(errno));
        return -1;
    }
    
    /* Get current time for install date */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Write metadata */
    fprintf(fp, "Name: %s\n", pkg->name);
    fprintf(fp, "Version: %s\n", pkg->version);
    fprintf(fp, "Description: %s\n", pkg->description);
    fprintf(fp, "InstallDate: %s\n", date_str);
    fprintf(fp, "Filename: %s\n", pkg->filename);
    
    if (strlen(pkg->dependencies) > 0) {
        fprintf(fp, "Dependencies: %s\n", pkg->dependencies);
    }
    
    fclose(fp);
    
    return 0;
}

/**
 * apt_extract_package - Extract package archive to installation directory
 * 
 * Uses tar command to extract the package archive from the available
 * directory to the packages directory.
 * 
 * @param pkg: Package to extract
 * @param pkg_dir: Destination directory
 * 
 * Returns: 0 on success, -1 on failure
 */
static int apt_extract_package(const Package *pkg, const char *pkg_dir) {
    /* Build path to package archive */
    char archive_path[APT_PATH_LEN];
    snprintf(archive_path, sizeof(archive_path), "%s/%s",
             g_apt_config.available_dir, pkg->filename);
    
    /* Check if archive exists */
    if (access(archive_path, F_OK) != 0) {
        fprintf(stderr, "apt install: package archive not found: %s\n", 
                archive_path);
        fprintf(stderr, "Expected location: %s\n", g_apt_config.available_dir);
        return -1;
    }
    
    /* Build tar extraction command */
    /* Use tar -xzf to extract, --strip-components=1 to remove top-level dir */
    char cmd[APT_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "tar -xzf \"%s\" -C \"%s\" --strip-components=1 2>&1",
             archive_path, pkg_dir);
    
    /* Execute extraction */
    FILE *pipe = popen(cmd, "r");
    if (pipe == NULL) {
        fprintf(stderr, "apt install: failed to execute tar command\n");
        return -1;
    }
    
    /* Check for extraction errors */
    char error_buf[512] = "";
    size_t bytes_read = fread(error_buf, 1, sizeof(error_buf) - 1, pipe);
    error_buf[bytes_read] = '\0';
    
    int status = pclose(pipe);
    
    if (status != 0) {
        fprintf(stderr, "apt install: extraction failed\n");
        if (strlen(error_buf) > 0) {
            fprintf(stderr, "tar error: %s\n", error_buf);
        }
        return -1;
    }
    
    return 0;
}

/**
 * apt_make_executables_accessible - Make package binaries executable
 * 
 * Scans the bin/ directory in the package and ensures all files have
 * execute permissions. This allows the binaries to be run via PATH.
 * 
 * @param pkg_dir: Package installation directory
 * 
 * Returns: 0 on success, -1 on failure
 */
static int apt_make_executables_accessible(const char *pkg_dir) {
    char bin_dir[APT_PATH_LEN];
    snprintf(bin_dir, sizeof(bin_dir), "%s/bin", pkg_dir);
    
    /* Check if bin directory exists */
    struct stat st;
    if (stat(bin_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        /* No bin directory - not an error, package may not have executables */
        return 0;
    }
    
    /* Open directory for reading */
    DIR *dir = opendir(bin_dir);
    if (dir == NULL) {
        fprintf(stderr, "apt install: warning: cannot open bin directory\n");
        return 0;  /* Not a fatal error */
    }
    
    /* Iterate through files and make them executable */
    struct dirent *entry;
    int file_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Build full path to file */
        char file_path[APT_PATH_LEN];
        snprintf(file_path, sizeof(file_path), "%s/%s", bin_dir, entry->d_name);
        
        /* Check if it's a regular file */
        if (stat(file_path, &st) == 0 && S_ISREG(st.st_mode)) {
            /* Make it executable (chmod +x) */
            if (chmod(file_path, st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH) == 0) {
                file_count++;
            } else {
                fprintf(stderr, "apt install: warning: cannot make %s executable\n",
                        entry->d_name);
            }
        }
    }
    
    closedir(dir);
    
    if (file_count > 0) {
        printf("Made %d executable(s) accessible.\n", file_count);
    }
    
    return 0;
}

/**
 * apt_verify_installation - Verify package was installed correctly
 * 
 * Performs basic sanity checks on the installed package to ensure
 * it was extracted correctly and has the expected structure.
 * 
 * @param pkg_dir: Package installation directory
 * 
 * Returns: 0 if verification passed, -1 if problems found
 */
static int apt_verify_installation(const char *pkg_dir) {
    struct stat st;
    
    /* Check if package directory exists */
    if (stat(pkg_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "apt install: verification failed: directory missing\n");
        return -1;
    }
    
    /* Check if METADATA file exists */
    char metadata_path[APT_PATH_LEN];
    snprintf(metadata_path, sizeof(metadata_path), "%s/METADATA", pkg_dir);
    
    if (stat(metadata_path, &st) != 0) {
        fprintf(stderr, "apt install: warning: METADATA file missing\n");
        /* Not a fatal error */
    }
    
    return 0;
}

/* ============================================================================
 * Main Installation Function
 * ============================================================================ */

/**
 * apt_install_package - Install a package by name
 * 
 * This is the main entry point for package installation. It performs
 * the complete installation process:
 * 
 *   1. Find package in index
 *   2. Check if already installed
 *   3. Verify dependencies
 *   4. Create package directory
 *   5. Extract package archive
 *   6. Create metadata
 *   7. Make executables accessible
 *   8. Mark as installed in index
 *   9. Update index file
 * 
 * @param pkgname: Name of package to install
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_install_package(const char *pkgname) {
    if (pkgname == NULL || strlen(pkgname) == 0) {
        fprintf(stderr, "apt install: package name cannot be empty\n");
        return -1;
    }
    
    printf("Installing package '%s'...\n", pkgname);
    
    /* Step 1: Find the package in the index */
    Package *pkg = apt_find_package(pkgname);
    if (pkg == NULL) {
        fprintf(stderr, "apt install: package '%s' not found in repository\n", 
                pkgname);
        fprintf(stderr, "Run 'apt search %s' to find similar packages.\n", pkgname);
        return -1;
    }
    
    /* Step 2: Check if already installed */
    if (pkg->installed) {
        printf("Package '%s' version %s is already installed.\n", 
               pkg->name, pkg->version);
        printf("To reinstall, first run: apt remove %s\n", pkg->name);
        return 0;
    }
    
    /* Step 3: Check dependencies */
    printf("Checking dependencies...\n");
    if (apt_check_dependencies_for_package(pkg->name) != 0) {
        fprintf(stderr, "apt install: dependency check failed\n");
        return -1;
    }
    printf("Dependencies satisfied.\n");
    
    /* Step 4: Create package installation directory */
    char pkg_dir[APT_PATH_LEN];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", 
             g_apt_config.packages_dir, pkg->name);
    
    printf("Creating package directory: %s\n", pkg_dir);
    if (mkdir(pkg_dir, 0755) != 0) {
        if (errno == EEXIST) {
            fprintf(stderr, "apt install: package directory already exists\n");
            fprintf(stderr, "This may indicate a partial installation.\n");
            fprintf(stderr, "Run 'apt remove %s' first.\n", pkg->name);
        } else {
            fprintf(stderr, "apt install: cannot create package directory: %s\n",
                    strerror(errno));
        }
        return -1;
    }
    
    /* Step 5: Extract package archive */
    printf("Extracting package archive...\n");
    if (apt_extract_package(pkg, pkg_dir) != 0) {
        fprintf(stderr, "apt install: extraction failed\n");
        /* Clean up - remove directory */
        rmdir(pkg_dir);
        return -1;
    }
    printf("Package extracted successfully.\n");
    
    /* Step 6: Create metadata file */
    printf("Creating package metadata...\n");
    if (apt_create_metadata(pkg, pkg_dir) != 0) {
        fprintf(stderr, "apt install: warning: metadata creation failed\n");
        /* Continue anyway - not fatal */
    }
    
    /* Step 7: Make executables accessible */
    printf("Setting up executables...\n");
    apt_make_executables_accessible(pkg_dir);
    
    /* Step 8: Verify installation */
    if (apt_verify_installation(pkg_dir) != 0) {
        fprintf(stderr, "apt install: warning: verification failed\n");
        /* Continue anyway */
    }
    
    /* Step 9: Mark package as installed in index */
    pkg->installed = 1;
    
    /* Step 10: Save updated index to disk */
    if (apt_save_index() != 0) {
        fprintf(stderr, "apt install: warning: failed to save package index\n");
        /* Package is installed, but index may be inconsistent */
    }
    
    /* Success! */
    printf("\n");
    printf("========================================\n");
    printf("Successfully installed: %s (version %s)\n", pkg->name, pkg->version);
    printf("========================================\n");
    printf("\n");
    
    /* Check if package has executables and inform user about PATH */
    char bin_dir[APT_PATH_LEN];
    snprintf(bin_dir, sizeof(bin_dir), "%s/bin", pkg_dir);
    struct stat st;
    if (stat(bin_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        printf("Package binaries are located in:\n");
        printf("  %s\n", bin_dir);
        printf("\n");
        printf("Add the following to your shell's PATH:\n");
        printf("  export PATH=\"$PATH:%s\"\n", bin_dir);
        printf("\n");
        printf("Or restart ushell to automatically include package paths.\n");
    }
    
    return 0;
}

/* ============================================================================
 * PATH Management
 * ============================================================================ */

/**
 * apt_setup_path - Add package bin directories to PATH
 * 
 * Scans ~/.ushell/packages/ for installed packages and adds their
 * bin/ directories to the PATH environment variable if they exist.
 * This makes installed executables accessible from the shell without
 * specifying full paths.
 * 
 * Implementation:
 * 1. Get current PATH from environment
 * 2. Scan packages directory for installed packages
 * 3. Check if each package has a bin/ directory
 * 4. Append bin/ directories to PATH
 * 5. Update PATH environment variable
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_setup_path(void) {
    /* Check if apt is initialized */
    if (!apt_is_initialized()) {
        /* Not initialized yet - silently skip */
        /* This is not an error, apt may not be set up yet */
        return 0;
    }
    
    /* Get current PATH */
    const char *current_path = getenv("PATH");
    if (current_path == NULL) {
        current_path = "/usr/local/bin:/usr/bin:/bin";
    }
    
    /* Build new PATH with package bin directories */
    char new_path[8192];  /* Large buffer for PATH */
    strncpy(new_path, current_path, sizeof(new_path) - 1);
    new_path[sizeof(new_path) - 1] = '\0';
    
    /* Open packages directory */
    DIR *dir = opendir(g_apt_config.packages_dir);
    if (dir == NULL) {
        /* Packages directory doesn't exist yet - not an error */
        return 0;
    }
    
    /* Scan for installed packages */
    struct dirent *entry;
    int added_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Build path to package bin directory */
        char bin_dir[APT_PATH_LEN];
        snprintf(bin_dir, sizeof(bin_dir), "%s/%s/bin",
                 g_apt_config.packages_dir, entry->d_name);
        
        /* Check if bin directory exists */
        struct stat st;
        if (stat(bin_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
            /* Add to PATH if not already present */
            if (strstr(new_path, bin_dir) == NULL) {
                /* Check if we have space */
                if (strlen(new_path) + strlen(bin_dir) + 2 < sizeof(new_path)) {
                    strncat(new_path, ":", sizeof(new_path) - strlen(new_path) - 1);
                    strncat(new_path, bin_dir, sizeof(new_path) - strlen(new_path) - 1);
                    added_count++;
                } else {
                    fprintf(stderr, "apt_setup_path: warning: PATH too long, "
                            "cannot add %s\n", bin_dir);
                }
            }
        }
    }
    
    closedir(dir);
    
    /* Update PATH environment variable if we added any directories */
    if (added_count > 0) {
        if (setenv("PATH", new_path, 1) != 0) {
            fprintf(stderr, "apt_setup_path: failed to update PATH\n");
            return -1;
        }
    }
    
    return 0;
}

