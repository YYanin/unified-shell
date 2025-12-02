/**
 * repo.c - Package Repository Management for Unified Shell
 * 
 * This module implements the core package repository functionality for the
 * ushell package manager. It handles:
 * 
 * - Directory structure creation and management
 * - Package index loading and saving
 * - Package metadata parsing
 * - Installation status tracking
 * 
 * The package manager uses a local repository structure:
 *   ~/.ushell/
 *   |-- packages/          # Installed packages
 *   |-- repo/              # Local repository
 *   |   |-- available/     # Available packages
 *   |   |-- cache/         # Downloaded packages
 *   |   |-- index.txt      # Package index
 *   |-- apt.conf           # Configuration
 * 
 * Index File Format:
 *   Each package entry consists of key-value pairs followed by a blank line:
 *   
 *   PackageName: toolname
 *   Version: 1.0
 *   Description: A useful tool
 *   Filename: toolname-1.0.tar.gz
 *   Depends: libc
 *   
 *   PackageName: anothertool
 *   ...
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
#include <ctype.h>

/* ============================================================================
 * Global State
 * ============================================================================ */

/* 
 * Global package index - holds all known packages
 * Initialized by apt_load_index()
 */
PackageIndex g_package_index = {
    .packages = {{0}},
    .count = 0
};

/*
 * Global apt configuration - holds directory paths
 * Initialized by apt_init()
 */
AptConfig g_apt_config = {
    .base_dir = "",
    .packages_dir = "",
    .repo_dir = "",
    .available_dir = "",
    .cache_dir = "",
    .index_file = "",
    .config_file = "",
    .initialized = 0
};

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * apt_get_base_dir - Get the base directory for apt
 * 
 * Constructs the path to ~/.ushell based on the HOME environment variable.
 * 
 * @param buf:  Buffer to store the path
 * @param size: Size of the buffer
 * 
 * @return Pointer to buf on success, NULL on failure
 * 
 * Example:
 *   char path[APT_PATH_LEN];
 *   apt_get_base_dir(path, sizeof(path));
 *   // path now contains "/home/user/.ushell"
 */
char* apt_get_base_dir(char *buf, size_t size) {
    /* Get user's home directory from environment */
    const char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "apt: HOME environment variable not set\n");
        return NULL;
    }
    
    /* Construct path: $HOME/.ushell */
    int ret = snprintf(buf, size, "%s/.ushell", home);
    if (ret < 0 || (size_t)ret >= size) {
        fprintf(stderr, "apt: path too long\n");
        return NULL;
    }
    
    return buf;
}

/**
 * apt_path_exists - Check if a path exists
 * 
 * Uses stat() to check for path existence. Works for both files and directories.
 * 
 * @param path: Path to check
 * 
 * @return 1 if exists, 0 otherwise
 */
int apt_path_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

/**
 * apt_mkdir_p - Create directory and parent directories
 * 
 * Similar to 'mkdir -p', creates the directory and any necessary parent
 * directories. If the directory already exists, does nothing.
 * 
 * @param path: Directory path to create
 * @param mode: Permission mode (e.g., 0755)
 * 
 * @return 0 on success, -1 on failure
 */
static int apt_mkdir_p(const char *path, mode_t mode) {
    char tmp[APT_PATH_LEN];
    char *p = NULL;
    size_t len;
    
    /* Copy path to temporary buffer for manipulation */
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    /* Remove trailing slash if present */
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }
    
    /* Create each directory in the path */
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';  /* Temporarily truncate at this point */
            
            /* Create this directory if it doesn't exist */
            if (!apt_path_exists(tmp)) {
                if (mkdir(tmp, mode) != 0) {
                    fprintf(stderr, "apt: cannot create directory '%s': %s\n", 
                            tmp, strerror(errno));
                    return -1;
                }
            }
            
            *p = '/';  /* Restore the slash */
        }
    }
    
    /* Create the final directory */
    if (!apt_path_exists(tmp)) {
        if (mkdir(tmp, mode) != 0) {
            fprintf(stderr, "apt: cannot create directory '%s': %s\n", 
                    tmp, strerror(errno));
            return -1;
        }
    }
    
    return 0;
}

/**
 * apt_trim - Trim whitespace from string
 * 
 * Removes leading and trailing whitespace from a string in-place.
 * 
 * @param str: String to trim (modified in place)
 * 
 * @return Pointer to the trimmed string
 */
static char* apt_trim(char *str) {
    char *end;
    
    /* Skip leading whitespace */
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    /* Handle empty string */
    if (*str == '\0') {
        return str;
    }
    
    /* Find end and trim trailing whitespace */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    /* Null terminate */
    end[1] = '\0';
    
    return str;
}

/* ============================================================================
 * Initialization Functions
 * ============================================================================ */

/**
 * apt_create_directories - Create the apt directory structure
 * 
 * Creates the following directory hierarchy:
 *   ~/.ushell/
 *   |-- packages/          # Installed packages
 *   |-- repo/              # Local repository
 *   |   |-- available/     # Available packages
 *   |   |-- cache/         # Downloaded packages
 * 
 * @return 0 on success, -1 on failure
 */
int apt_create_directories(void) {
    /* Create base directory: ~/.ushell */
    if (apt_mkdir_p(g_apt_config.base_dir, 0755) != 0) {
        return -1;
    }
    
    /* Create packages directory: ~/.ushell/packages */
    if (apt_mkdir_p(g_apt_config.packages_dir, 0755) != 0) {
        return -1;
    }
    
    /* Create repo directory: ~/.ushell/repo */
    if (apt_mkdir_p(g_apt_config.repo_dir, 0755) != 0) {
        return -1;
    }
    
    /* Create available directory: ~/.ushell/repo/available */
    if (apt_mkdir_p(g_apt_config.available_dir, 0755) != 0) {
        return -1;
    }
    
    /* Create cache directory: ~/.ushell/repo/cache */
    if (apt_mkdir_p(g_apt_config.cache_dir, 0755) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * apt_create_default_config - Create default apt.conf file
 * 
 * Creates a default configuration file with basic settings.
 * The configuration file format is key=value pairs.
 * 
 * @return 0 on success, -1 on failure
 */
int apt_create_default_config(void) {
    FILE *fp;
    
    fp = fopen(g_apt_config.config_file, "w");
    if (fp == NULL) {
        fprintf(stderr, "apt: cannot create config file: %s\n", strerror(errno));
        return -1;
    }
    
    /* Write default configuration */
    fprintf(fp, "# apt.conf - ushell package manager configuration\n");
    fprintf(fp, "# Created by apt init\n\n");
    fprintf(fp, "# Repository settings\n");
    fprintf(fp, "repo_url=local\n");
    fprintf(fp, "cache_enabled=1\n\n");
    fprintf(fp, "# Package settings\n");
    fprintf(fp, "auto_resolve_deps=1\n");
    
    fclose(fp);
    return 0;
}

/**
 * apt_create_default_index - Create default package index
 * 
 * Creates a sample index.txt with example package entries.
 * These serve as examples of the package format.
 * 
 * @return 0 on success, -1 on failure
 */
int apt_create_default_index(void) {
    FILE *fp;
    
    fp = fopen(g_apt_config.index_file, "w");
    if (fp == NULL) {
        fprintf(stderr, "apt: cannot create index file: %s\n", strerror(errno));
        return -1;
    }
    
    /* Write sample package entries */
    fprintf(fp, "# ushell package index\n");
    fprintf(fp, "# Format: key: value pairs, blank line between packages\n\n");
    
    /* Sample package 1: hello */
    fprintf(fp, "PackageName: hello\n");
    fprintf(fp, "Version: 1.0.0\n");
    fprintf(fp, "Description: A simple hello world program\n");
    fprintf(fp, "Filename: hello-1.0.0.tar.gz\n");
    fprintf(fp, "Depends:\n\n");
    
    /* Sample package 2: mathlib */
    fprintf(fp, "PackageName: mathlib\n");
    fprintf(fp, "Version: 2.1.0\n");
    fprintf(fp, "Description: Mathematical functions library\n");
    fprintf(fp, "Filename: mathlib-2.1.0.tar.gz\n");
    fprintf(fp, "Depends:\n\n");
    
    /* Sample package 3: textutils */
    fprintf(fp, "PackageName: textutils\n");
    fprintf(fp, "Version: 1.5.2\n");
    fprintf(fp, "Description: Text processing utilities\n");
    fprintf(fp, "Filename: textutils-1.5.2.tar.gz\n");
    fprintf(fp, "Depends: hello\n\n");
    
    fclose(fp);
    return 0;
}

/**
 * apt_init - Initialize the package manager system
 * 
 * This function:
 * 1. Sets up the configuration paths
 * 2. Creates the directory structure
 * 3. Creates default config and index files if needed
 * 4. Loads the package index
 * 
 * Should be called on first use of apt commands.
 * 
 * @return 0 on success, -1 on failure
 */
int apt_init(void) {
    /* Check if already initialized */
    if (g_apt_config.initialized) {
        return 0;
    }
    
    /* Get base directory path */
    if (apt_get_base_dir(g_apt_config.base_dir, sizeof(g_apt_config.base_dir)) == NULL) {
        return -1;
    }
    
    /* Construct all paths */
    snprintf(g_apt_config.packages_dir, sizeof(g_apt_config.packages_dir),
             "%s/packages", g_apt_config.base_dir);
    snprintf(g_apt_config.repo_dir, sizeof(g_apt_config.repo_dir),
             "%s/repo", g_apt_config.base_dir);
    snprintf(g_apt_config.available_dir, sizeof(g_apt_config.available_dir),
             "%s/repo/available", g_apt_config.base_dir);
    snprintf(g_apt_config.cache_dir, sizeof(g_apt_config.cache_dir),
             "%s/repo/cache", g_apt_config.base_dir);
    snprintf(g_apt_config.index_file, sizeof(g_apt_config.index_file),
             "%s/repo/index.txt", g_apt_config.base_dir);
    snprintf(g_apt_config.config_file, sizeof(g_apt_config.config_file),
             "%s/apt.conf", g_apt_config.base_dir);
    
    /* Create directory structure */
    if (apt_create_directories() != 0) {
        return -1;
    }
    
    /* Create default config if it doesn't exist */
    if (!apt_path_exists(g_apt_config.config_file)) {
        if (apt_create_default_config() != 0) {
            return -1;
        }
    }
    
    /* Create default index if it doesn't exist */
    if (!apt_path_exists(g_apt_config.index_file)) {
        if (apt_create_default_index() != 0) {
            return -1;
        }
    }
    
    /* Mark as initialized */
    g_apt_config.initialized = 1;
    
    return 0;
}

/**
 * apt_is_initialized - Check if apt system is ready
 * 
 * @return 1 if initialized, 0 otherwise
 */
int apt_is_initialized(void) {
    return g_apt_config.initialized;
}

/* ============================================================================
 * Index Management Functions
 * ============================================================================ */

/**
 * apt_parse_package_entry - Parse a single package entry from index file
 * 
 * Reads key-value pairs until a blank line or EOF is encountered.
 * Expected keys: PackageName, Version, Description, Filename, Depends
 * 
 * @param fp:  File pointer (positioned at start of entry)
 * @param pkg: Package structure to fill
 * 
 * @return 0 on success, -1 on error, 1 on EOF
 */
int apt_parse_package_entry(FILE *fp, Package *pkg) {
    char line[1024];
    int found_data = 0;
    
    /* Initialize package structure */
    memset(pkg, 0, sizeof(Package));
    
    /* Read lines until blank line or EOF */
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *trimmed = apt_trim(line);
        
        /* Skip comments */
        if (trimmed[0] == '#') {
            continue;
        }
        
        /* Blank line marks end of entry */
        if (strlen(trimmed) == 0) {
            if (found_data) {
                return 0;  /* Successfully parsed an entry */
            }
            continue;  /* Skip leading blank lines */
        }
        
        /* Parse key: value pair */
        char *colon = strchr(trimmed, ':');
        if (colon == NULL) {
            continue;  /* Skip malformed lines */
        }
        
        /* Split into key and value */
        *colon = '\0';
        char *key = apt_trim(trimmed);
        char *value = apt_trim(colon + 1);
        
        /* Store value based on key */
        if (strcmp(key, "PackageName") == 0) {
            strncpy(pkg->name, value, APT_NAME_LEN - 1);
            found_data = 1;
        } else if (strcmp(key, "Version") == 0) {
            strncpy(pkg->version, value, APT_VERSION_LEN - 1);
        } else if (strcmp(key, "Description") == 0) {
            strncpy(pkg->description, value, APT_DESC_LEN - 1);
        } else if (strcmp(key, "Filename") == 0) {
            strncpy(pkg->filename, value, APT_FILENAME_LEN - 1);
        } else if (strcmp(key, "Depends") == 0) {
            strncpy(pkg->dependencies, value, APT_DEPS_LEN - 1);
        }
    }
    
    /* EOF reached */
    if (found_data) {
        return 0;  /* Last entry was valid */
    }
    
    return 1;  /* EOF with no data */
}

/**
 * apt_write_package_entry - Write a package entry to file
 * 
 * Writes a package in the standard index format.
 * 
 * @param fp:  File pointer to write to
 * @param pkg: Package to write
 * 
 * @return 0 on success, -1 on failure
 */
int apt_write_package_entry(FILE *fp, const Package *pkg) {
    if (fp == NULL || pkg == NULL) {
        return -1;
    }
    
    fprintf(fp, "PackageName: %s\n", pkg->name);
    fprintf(fp, "Version: %s\n", pkg->version);
    fprintf(fp, "Description: %s\n", pkg->description);
    fprintf(fp, "Filename: %s\n", pkg->filename);
    fprintf(fp, "Depends: %s\n", pkg->dependencies);
    fprintf(fp, "\n");  /* Blank line between entries */
    
    return 0;
}

/**
 * apt_load_index - Load package index from disk
 * 
 * Reads the index file and populates g_package_index with all packages.
 * Also updates the installed status for each package.
 * 
 * @return Number of packages loaded, -1 on error
 */
int apt_load_index(void) {
    FILE *fp;
    Package pkg;
    int result;
    
    /* Ensure apt is initialized */
    if (!apt_is_initialized()) {
        if (apt_init() != 0) {
            return -1;
        }
    }
    
    /* Reset package index */
    g_package_index.count = 0;
    
    /* Open index file */
    fp = fopen(g_apt_config.index_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "apt: cannot open index file: %s\n", strerror(errno));
        return -1;
    }
    
    /* Read all package entries */
    while ((result = apt_parse_package_entry(fp, &pkg)) == 0) {
        /* Check for index overflow */
        if (g_package_index.count >= APT_MAX_PACKAGES) {
            fprintf(stderr, "apt: too many packages in index (max %d)\n", APT_MAX_PACKAGES);
            break;
        }
        
        /* Copy package to index */
        memcpy(&g_package_index.packages[g_package_index.count], &pkg, sizeof(Package));
        g_package_index.count++;
    }
    
    fclose(fp);
    
    /* Update installed status for all packages */
    apt_update_installed_status();
    
    return g_package_index.count;
}

/**
 * apt_save_index - Save package index to disk
 * 
 * Writes all packages from g_package_index to the index file.
 * 
 * @return 0 on success, -1 on failure
 */
int apt_save_index(void) {
    FILE *fp;
    
    /* Ensure apt is initialized */
    if (!apt_is_initialized()) {
        return -1;
    }
    
    /* Open index file for writing */
    fp = fopen(g_apt_config.index_file, "w");
    if (fp == NULL) {
        fprintf(stderr, "apt: cannot write index file: %s\n", strerror(errno));
        return -1;
    }
    
    /* Write header */
    fprintf(fp, "# ushell package index\n");
    fprintf(fp, "# Auto-generated - do not edit manually\n\n");
    
    /* Write all packages */
    for (int i = 0; i < g_package_index.count; i++) {
        apt_write_package_entry(fp, &g_package_index.packages[i]);
    }
    
    fclose(fp);
    return 0;
}

/**
 * apt_update_installed_status - Update installed flags for all packages
 * 
 * Scans the packages directory to see which packages are installed.
 * A package is considered installed if a directory with its name exists
 * in ~/.ushell/packages/.
 * 
 * @return Number of installed packages found
 */
int apt_update_installed_status(void) {
    int installed_count = 0;
    char pkg_path[APT_PATH_LEN];
    
    /* Check each package */
    for (int i = 0; i < g_package_index.count; i++) {
        /* Construct path to package directory */
        snprintf(pkg_path, sizeof(pkg_path), "%s/%s",
                 g_apt_config.packages_dir,
                 g_package_index.packages[i].name);
        
        /* Check if directory exists */
        if (apt_path_exists(pkg_path)) {
            g_package_index.packages[i].installed = 1;
            installed_count++;
        } else {
            g_package_index.packages[i].installed = 0;
        }
    }
    
    return installed_count;
}

/* ============================================================================
 * Package Query Functions
 * ============================================================================ */

/**
 * apt_find_package - Find a package by name
 * 
 * Searches the package index for a package with the given name.
 * 
 * @param name: Package name to search for
 * 
 * @return Pointer to Package if found, NULL otherwise
 */
Package* apt_find_package(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < g_package_index.count; i++) {
        if (strcmp(g_package_index.packages[i].name, name) == 0) {
            return &g_package_index.packages[i];
        }
    }
    
    return NULL;
}

/**
 * apt_search_packages - Search packages by keyword
 * 
 * Searches package names and descriptions for the keyword.
 * Case-insensitive matching.
 * 
 * @param keyword:     Search term
 * @param results:     Array to store matching package pointers
 * @param max_results: Maximum number of results
 * 
 * @return Number of matching packages
 */
int apt_search_packages(const char *keyword, Package **results, int max_results) {
    int count = 0;
    
    if (keyword == NULL || results == NULL) {
        return 0;
    }
    
    /* Convert keyword to lowercase for case-insensitive search */
    char lower_keyword[256];
    size_t i;
    for (i = 0; keyword[i] && i < sizeof(lower_keyword) - 1; i++) {
        lower_keyword[i] = tolower((unsigned char)keyword[i]);
    }
    lower_keyword[i] = '\0';
    
    /* Search all packages */
    for (int j = 0; j < g_package_index.count && count < max_results; j++) {
        Package *pkg = &g_package_index.packages[j];
        
        /* Convert name and description to lowercase for comparison */
        char lower_name[APT_NAME_LEN];
        char lower_desc[APT_DESC_LEN];
        
        for (i = 0; pkg->name[i] && i < sizeof(lower_name) - 1; i++) {
            lower_name[i] = tolower((unsigned char)pkg->name[i]);
        }
        lower_name[i] = '\0';
        
        for (i = 0; pkg->description[i] && i < sizeof(lower_desc) - 1; i++) {
            lower_desc[i] = tolower((unsigned char)pkg->description[i]);
        }
        lower_desc[i] = '\0';
        
        /* Check if keyword appears in name or description */
        if (strstr(lower_name, lower_keyword) != NULL ||
            strstr(lower_desc, lower_keyword) != NULL) {
            results[count++] = pkg;
        }
    }
    
    return count;
}

/**
 * apt_list_packages - Get all packages
 * 
 * Returns the array of packages from the global index.
 * If installed_only is set, caller must check installed flag on each.
 * 
 * @param installed_only: If 1, caller should filter for installed packages
 * 
 * @return Pointer to package array
 */
Package* apt_list_packages(int installed_only) {
    (void)installed_only;  /* Caller handles filtering */
    return g_package_index.packages;
}

/**
 * apt_get_package_count - Get number of packages
 * 
 * @param installed_only: If 1, count only installed packages
 * 
 * @return Number of packages
 */
int apt_get_package_count(int installed_only) {
    if (!installed_only) {
        return g_package_index.count;
    }
    
    int count = 0;
    for (int i = 0; i < g_package_index.count; i++) {
        if (g_package_index.packages[i].installed) {
            count++;
        }
    }
    return count;
}

/**
 * apt_is_installed - Check if a package is installed
 * 
 * @param name: Package name
 * 
 * @return 1 if installed, 0 otherwise
 */
int apt_is_installed(const char *name) {
    Package *pkg = apt_find_package(name);
    if (pkg == NULL) {
        return 0;
    }
    return pkg->installed;
}
