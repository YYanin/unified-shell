/**
 * apt.h - Simple Package Manager for Unified Shell
 * 
 * This header defines the package manager system for ushell, providing
 * a local package repository structure similar to apt/dpkg but simplified
 * for educational purposes.
 * 
 * Features:
 * - Local package repository in ~/.ushell/
 * - Package index management
 * - Basic install/remove operations
 * - Package search and listing
 * 
 * Directory Structure:
 *   ~/.ushell/
 *   |-- packages/          # Installed packages
 *   |-- repo/              # Local repository
 *   |   |-- available/     # Available packages
 *   |   |-- cache/         # Downloaded packages
 *   |   |-- index.txt      # Package index
 *   |-- apt.conf           # Configuration
 */

#ifndef APT_H
#define APT_H

#include "environment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Constants and Limits
 * ============================================================================ */

/* Maximum number of packages in the repository */
#define APT_MAX_PACKAGES 256

/* Field size limits for package metadata */
#define APT_NAME_LEN 64
#define APT_VERSION_LEN 16
#define APT_DESC_LEN 256
#define APT_FILENAME_LEN 128
#define APT_DEPS_LEN 256
#define APT_PATH_LEN 1024

/* Configuration file keys */
#define APT_CONF_REPO_URL "repo_url"
#define APT_CONF_CACHE_DIR "cache_dir"

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/**
 * Package - Represents a single package in the repository
 * 
 * Fields:
 *   name         - Unique package identifier (e.g., "mytools")
 *   version      - Version string (e.g., "1.0.0")
 *   description  - Human-readable description
 *   filename     - Archive filename (e.g., "mytools-1.0.tar.gz")
 *   dependencies - Comma-separated list of dependencies
 *   installed    - 1 if installed, 0 otherwise
 */
typedef struct {
    char name[APT_NAME_LEN];
    char version[APT_VERSION_LEN];
    char description[APT_DESC_LEN];
    char filename[APT_FILENAME_LEN];
    char dependencies[APT_DEPS_LEN];
    int installed;
} Package;

/**
 * PackageIndex - Collection of packages in the repository
 * 
 * Fields:
 *   packages     - Array of package structures
 *   count        - Number of packages in the array
 */
typedef struct {
    Package packages[APT_MAX_PACKAGES];
    int count;
} PackageIndex;

/**
 * AptConfig - Configuration for the package manager
 * 
 * Fields:
 *   base_dir     - Base directory (~/.ushell)
 *   packages_dir - Installed packages directory
 *   repo_dir     - Repository directory
 *   available_dir- Available packages directory
 *   cache_dir    - Cache directory for downloads
 *   index_file   - Path to index.txt
 *   config_file  - Path to apt.conf
 *   initialized  - 1 if apt system is initialized
 */
typedef struct {
    char base_dir[APT_PATH_LEN];
    char packages_dir[APT_PATH_LEN];
    char repo_dir[APT_PATH_LEN];
    char available_dir[APT_PATH_LEN];
    char cache_dir[APT_PATH_LEN];
    char index_file[APT_PATH_LEN];
    char config_file[APT_PATH_LEN];
    int initialized;
} AptConfig;

/* ============================================================================
 * Global State
 * ============================================================================ */

/* 
 * These are defined in src/apt/repo.c
 * They hold the current package index and configuration
 */
extern PackageIndex g_package_index;
extern AptConfig g_apt_config;

/* ============================================================================
 * Initialization Functions
 * ============================================================================ */

/**
 * apt_init - Initialize the package manager system
 * 
 * Creates the necessary directory structure under ~/.ushell/:
 *   - packages/     (installed packages)
 *   - repo/         (local repository)
 *   - repo/available/ (available packages)
 *   - repo/cache/   (download cache)
 *   - apt.conf      (configuration file)
 *   - repo/index.txt (package index)
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_init(void);

/**
 * apt_create_directories - Create apt directory structure
 * 
 * Creates all required directories if they don't exist.
 * Called by apt_init().
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_create_directories(void);

/**
 * apt_is_initialized - Check if apt system is ready
 * 
 * Returns: 1 if initialized, 0 otherwise
 */
int apt_is_initialized(void);

/* ============================================================================
 * Index Management Functions
 * ============================================================================ */

/**
 * apt_load_index - Load package index from disk
 * 
 * Reads ~/.ushell/repo/index.txt and populates g_package_index.
 * Also checks which packages are currently installed.
 * 
 * Returns: Number of packages loaded, -1 on error
 */
int apt_load_index(void);

/**
 * apt_save_index - Save package index to disk
 * 
 * Writes g_package_index to ~/.ushell/repo/index.txt.
 * Called after package operations (install/remove).
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_save_index(void);

/**
 * apt_update_installed_status - Update installed flags
 * 
 * Scans ~/.ushell/packages/ and updates the installed
 * flag for each package in the index.
 * 
 * Returns: Number of installed packages found
 */
int apt_update_installed_status(void);

/* ============================================================================
 * Package Query Functions
 * ============================================================================ */

/**
 * apt_find_package - Find a package by name
 * 
 * @param name: Package name to search for
 * 
 * Returns: Pointer to Package if found, NULL otherwise
 */
Package* apt_find_package(const char *name);

/**
 * apt_search_packages - Search packages by keyword
 * 
 * Searches package names and descriptions for the keyword.
 * 
 * @param keyword:  Search term
 * @param results:  Array to store matching packages
 * @param max_results: Maximum number of results to return
 * 
 * Returns: Number of matching packages
 */
int apt_search_packages(const char *keyword, Package **results, int max_results);

/**
 * apt_list_packages - Get all packages
 * 
 * @param installed_only: If 1, only return installed packages
 * 
 * Returns: Pointer to package array (g_package_index.packages)
 */
Package* apt_list_packages(int installed_only);

/**
 * apt_get_package_count - Get number of packages
 * 
 * @param installed_only: If 1, count only installed packages
 * 
 * Returns: Number of packages
 */
int apt_get_package_count(int installed_only);

/* ============================================================================
 * Package I/O Functions
 * ============================================================================ */

/**
 * apt_parse_package_entry - Parse a single package entry
 * 
 * Parses a package entry from the index file format:
 *   PackageName: name
 *   Version: 1.0
 *   Description: text
 *   Filename: file.tar.gz
 *   Depends: dep1, dep2
 * 
 * @param fp:   File pointer positioned at start of entry
 * @param pkg:  Package structure to fill
 * 
 * Returns: 0 on success, -1 on error, 1 on EOF
 */
int apt_parse_package_entry(FILE *fp, Package *pkg);

/**
 * apt_write_package_entry - Write a package entry to file
 * 
 * @param fp:  File pointer to write to
 * @param pkg: Package to write
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_write_package_entry(FILE *fp, const Package *pkg);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * apt_get_base_dir - Get ~/.ushell directory path
 * 
 * @param buf:  Buffer to store path
 * @param size: Buffer size
 * 
 * Returns: Pointer to buf, or NULL on failure
 */
char* apt_get_base_dir(char *buf, size_t size);

/**
 * apt_path_exists - Check if path exists
 * 
 * @param path: Path to check
 * 
 * Returns: 1 if exists, 0 otherwise
 */
int apt_path_exists(const char *path);

/**
 * apt_is_installed - Check if a package is installed
 * 
 * @param name: Package name
 * 
 * Returns: 1 if installed, 0 otherwise
 */
int apt_is_installed(const char *name);

/**
 * apt_create_default_index - Create default index with sample packages
 * 
 * Creates a sample index.txt with example package entries.
 * Called when initializing a new repository.
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_create_default_index(void);

/**
 * apt_create_default_config - Create default apt.conf
 * 
 * Creates a default configuration file.
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_create_default_config(void);

/* ============================================================================
 * Package Installation Functions
 * ============================================================================ */

/**
 * apt_install_package - Install a package by name
 * 
 * Performs complete package installation:
 *   1. Finds package in index
 *   2. Checks if already installed
 *   3. Verifies dependencies
 *   4. Creates package directory
 *   5. Extracts package archive
 *   6. Creates metadata
 *   7. Makes executables accessible
 *   8. Updates package index
 * 
 * @param pkgname: Name of package to install
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_install_package(const char *pkgname);

/**
 * apt_setup_path - Add package bin directories to PATH
 * 
 * Scans ~/.ushell/packages/ for installed packages and adds their
 * bin/ directories to the PATH environment variable. This makes
 * installed executables accessible from the shell.
 * 
 * Example: If packages foo and bar are installed, adds:
 *   ~/.ushell/packages/foo/bin
 *   ~/.ushell/packages/bar/bin
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_setup_path(void);

/* ============================================================================
 * Package Removal Functions
 * ============================================================================ */

/**
 * apt_remove_package - Remove an installed package
 * 
 * Wrapper that calls apt_remove_package_with_force with force=0.
 * 
 * @param pkgname: Name of package to remove
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_remove_package(const char *pkgname);

/**
 * apt_remove_package_with_force - Remove an installed package with force option
 * 
 * Performs complete package removal:
 *   1. Verifies package is installed
 *   2. Checks for dependent packages (unless force=1)
 *   3. Removes package directory recursively
 *   4. Updates package index
 * 
 * @param pkgname: Name of package to remove
 * @param force: If 1, skip dependent checking; if 0, warn about dependents
 * 
 * Returns: 0 on success, -1 on failure
 */
int apt_remove_package_with_force(const char *pkgname, int force);

/**
 * apt_verify_package - Verify package integrity
 * 
 * Checks that an installed package has all expected files:
 *   - Package directory exists
 *   - METADATA file present and valid
 *   - Executables have proper permissions
 * 
 * Reports any issues found.
 * 
 * @param pkgname: Name of package to verify
 * 
 * Returns: 0 if valid, -1 if critical issues found
 */
int apt_verify_package(const char *pkgname);

/* ============================================================================
 * Dependency Management Functions
 * ============================================================================ */

/**
 * apt_get_dependencies - Get list of dependencies for a package
 * 
 * Parses the package's Depends field and returns an array of dependency
 * names. Caller is responsible for freeing the returned array and its strings.
 * 
 * @param pkgname: Package name to get dependencies for
 * @param dep_count: Output parameter for number of dependencies
 * 
 * Returns: Array of dependency names (caller must free), NULL on error
 */
char** apt_get_dependencies(const char *pkgname, int *dep_count);

/**
 * apt_check_dependencies - Check if all dependencies are satisfied
 * 
 * Verifies that all packages listed as dependencies are installed.
 * If missing dependencies exist, they are listed in the missing buffer.
 * 
 * @param pkgname: Package name to check dependencies for
 * @param missing: Output buffer for missing dependency names (comma-separated)
 * @param missing_size: Size of missing buffer
 * 
 * Returns: Number of missing dependencies, -1 on error
 */
int apt_check_dependencies(const char *pkgname, char *missing, size_t missing_size);

/**
 * apt_resolve_dependencies - Recursively resolve all dependencies
 * 
 * Builds a complete ordered list of packages that need to be installed
 * to satisfy all dependencies for the given package. The list is ordered
 * such that dependencies are installed before packages that depend on them.
 * Includes circular dependency detection.
 * 
 * @param pkgname: Package to resolve dependencies for
 * @param install_order: Output array for packages in installation order
 * @param max_packages: Maximum size of install_order array
 * 
 * Returns: Number of packages to install (including pkgname), -1 on error
 */
int apt_resolve_dependencies(const char *pkgname, char install_order[][APT_NAME_LEN], int max_packages);

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
int apt_install_dependencies(const char *pkgname, int install_self);

/* ============================================================================
 * Built-in Command Entry Point
 * ============================================================================ */

/**
 * builtin_apt - Main entry point for apt command
 * 
 * Usage: apt <subcommand> [arguments]
 * 
 * Subcommands:
 *   init     - Initialize package system
 *   update   - Refresh package index
 *   list     - List all packages
 *   search   - Search for packages
 *   show     - Show package details
 *   install  - Install a package
 *   remove   - Remove a package
 * 
 * @param argv: Command arguments (argv[0] = "apt")
 * @param env:  Shell environment
 * 
 * Returns: 0 on success, non-zero on failure
 */
int builtin_apt(char **argv, Env *env);

#endif /* APT_H */
