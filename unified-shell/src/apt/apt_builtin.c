/**
 * apt_builtin.c - apt Built-in Command for Unified Shell
 * 
 * This module implements the apt built-in command that provides
 * package management functionality. It supports the following subcommands:
 * 
 *   apt init     - Initialize the package system
 *   apt update   - Refresh the package index
 *   apt list     - List all packages
 *   apt search   - Search for packages by keyword
 *   apt show     - Show detailed package information
 *   apt install  - Install a package (placeholder)
 *   apt remove   - Remove a package (placeholder)
 * 
 * Usage: apt <subcommand> [arguments]
 */

#include "apt.h"
#include "environment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ============================================================================
 * Subcommand Implementations
 * ============================================================================ */

/**
 * apt_cmd_init - Initialize the package system
 * 
 * Creates the directory structure and default files:
 *   ~/.ushell/packages/
 *   ~/.ushell/repo/
 *   ~/.ushell/repo/available/
 *   ~/.ushell/repo/cache/
 *   ~/.ushell/apt.conf
 *   ~/.ushell/repo/index.txt
 * 
 * @return 0 on success, 1 on failure
 */
static int apt_cmd_init(void) {
    printf("Initializing package system...\n");
    
    /* Initialize apt (creates directories and default files) */
    if (apt_init() != 0) {
        fprintf(stderr, "apt init: failed to initialize package system\n");
        return 1;
    }
    
    /* Load the package index */
    int count = apt_load_index();
    if (count < 0) {
        fprintf(stderr, "apt init: failed to load package index\n");
        return 1;
    }
    
    printf("Package system initialized.\n");
    printf("Created directory structure in ~/.ushell/\n");
    printf("Loaded %d package(s) from index.\n", count);
    
    return 0;
}

/**
 * apt_cmd_update - Refresh the package index
 * 
 * Reloads the package index from disk and updates installation status.
 * In a real package manager, this would fetch updates from a remote server.
 * 
 * @return 0 on success, 1 on failure
 */
static int apt_cmd_update(void) {
    /* Ensure apt is initialized */
    if (!apt_is_initialized()) {
        if (apt_init() != 0) {
            fprintf(stderr, "apt update: package system not initialized\n");
            fprintf(stderr, "Run 'apt init' first.\n");
            return 1;
        }
    }
    
    printf("Updating package index...\n");
    
    /* Reload the index from disk */
    int count = apt_load_index();
    if (count < 0) {
        fprintf(stderr, "apt update: failed to load package index\n");
        return 1;
    }
    
    /* Count installed packages */
    int installed = apt_get_package_count(1);
    
    printf("Package index loaded.\n");
    printf("Found %d package(s), %d installed.\n", count, installed);
    
    return 0;
}

/**
 * apt_cmd_list - List all packages
 * 
 * Displays all packages in the repository with their status.
 * Format: [installed] package-name  version  description
 * 
 * @param installed_only: If 1, only show installed packages
 * 
 * @return 0 on success, 1 on failure
 */
static int apt_cmd_list(int installed_only) {
    /* Ensure apt is initialized and index is loaded */
    if (!apt_is_initialized()) {
        if (apt_init() != 0 || apt_load_index() < 0) {
            fprintf(stderr, "apt list: package system not initialized\n");
            return 1;
        }
    }
    
    int count = apt_get_package_count(0);
    if (count == 0) {
        printf("No packages found.\n");
        return 0;
    }
    
    /* Print header */
    printf("%-20s %-10s %-8s %s\n", "Package", "Version", "Status", "Description");
    printf("%-20s %-10s %-8s %s\n", "-------", "-------", "------", "-----------");
    
    /* List packages */
    Package *packages = apt_list_packages(installed_only);
    int displayed = 0;
    
    for (int i = 0; i < count; i++) {
        Package *pkg = &packages[i];
        
        /* Skip if filtering for installed only */
        if (installed_only && !pkg->installed) {
            continue;
        }
        
        /* Print package info */
        printf("%-20s %-10s %-8s %s\n",
               pkg->name,
               pkg->version,
               pkg->installed ? "[inst]" : "",
               pkg->description);
        displayed++;
    }
    
    printf("\n%d package(s) listed.\n", displayed);
    
    return 0;
}

/**
 * apt_cmd_search - Search for packages
 * 
 * Searches package names and descriptions for a keyword.
 * 
 * @param keyword: Search term
 * 
 * @return 0 on success, 1 on failure
 */
static int apt_cmd_search(const char *keyword) {
    if (keyword == NULL || strlen(keyword) == 0) {
        fprintf(stderr, "apt search: missing search term\n");
        fprintf(stderr, "Usage: apt search <keyword>\n");
        return 1;
    }
    
    /* Ensure apt is initialized and index is loaded */
    if (!apt_is_initialized()) {
        if (apt_init() != 0 || apt_load_index() < 0) {
            fprintf(stderr, "apt search: package system not initialized\n");
            return 1;
        }
    }
    
    /* Search for matching packages */
    Package *results[APT_MAX_PACKAGES];
    int count = apt_search_packages(keyword, results, APT_MAX_PACKAGES);
    
    if (count == 0) {
        printf("No packages found matching '%s'.\n", keyword);
        return 0;
    }
    
    printf("Packages matching '%s':\n\n", keyword);
    printf("%-20s %-10s %-8s %s\n", "Package", "Version", "Status", "Description");
    printf("%-20s %-10s %-8s %s\n", "-------", "-------", "------", "-----------");
    
    for (int i = 0; i < count; i++) {
        Package *pkg = results[i];
        printf("%-20s %-10s %-8s %s\n",
               pkg->name,
               pkg->version,
               pkg->installed ? "[inst]" : "",
               pkg->description);
    }
    
    printf("\n%d package(s) found.\n", count);
    
    return 0;
}

/**
 * apt_cmd_show - Show package details
 * 
 * Displays detailed information about a specific package.
 * 
 * @param name: Package name
 * 
 * @return 0 on success, 1 on failure
 */
static int apt_cmd_show(const char *name) {
    if (name == NULL || strlen(name) == 0) {
        fprintf(stderr, "apt show: missing package name\n");
        fprintf(stderr, "Usage: apt show <package>\n");
        return 1;
    }
    
    /* Ensure apt is initialized and index is loaded */
    if (!apt_is_initialized()) {
        if (apt_init() != 0 || apt_load_index() < 0) {
            fprintf(stderr, "apt show: package system not initialized\n");
            return 1;
        }
    }
    
    /* Find the package */
    Package *pkg = apt_find_package(name);
    if (pkg == NULL) {
        fprintf(stderr, "apt show: package '%s' not found\n", name);
        return 1;
    }
    
    /* Display package details */
    printf("Package: %s\n", pkg->name);
    printf("Version: %s\n", pkg->version);
    printf("Status: %s\n", pkg->installed ? "installed" : "not installed");
    
    /* If installed, read and display installation date from METADATA */
    if (pkg->installed) {
        char metadata_path[APT_PATH_LEN];
        snprintf(metadata_path, sizeof(metadata_path), "%s/%s/METADATA",
                 g_apt_config.packages_dir, name);
        
        FILE *fp = fopen(metadata_path, "r");
        if (fp != NULL) {
            char line[256];
            char install_date[64] = "unknown";
            
            /* Parse METADATA file for InstallDate field */
            while (fgets(line, sizeof(line), fp) != NULL) {
                if (strncmp(line, "InstallDate:", 12) == 0) {
                    /* Extract date string after "InstallDate: " */
                    char *date_str = line + 12;
                    /* Trim leading whitespace */
                    while (*date_str == ' ') date_str++;
                    /* Trim trailing newline */
                    char *newline = strchr(date_str, '\n');
                    if (newline) *newline = '\0';
                    strncpy(install_date, date_str, sizeof(install_date) - 1);
                    install_date[sizeof(install_date) - 1] = '\0';
                    break;
                }
            }
            fclose(fp);
            
            printf("Installed: %s\n", install_date);
        }
    }
    
    printf("Filename: %s\n", pkg->filename);
    printf("Dependencies: %s\n", strlen(pkg->dependencies) > 0 ? pkg->dependencies : "none");
    printf("Description: %s\n", pkg->description);
    
    return 0;
}

/**
 * apt_cmd_install - Install a package with optional dependency auto-install
 * 
 * Handles the 'apt install' subcommand by delegating to the
 * full installation implementation in install.c.
 * 
 * Supports the --auto-install flag to automatically install missing dependencies.
 * 
 * @param argc: Number of arguments
 * @param argv: Argument array (argv[2] is package name, argv[3] may be --auto-install)
 * 
 * @return 0 on success, 1 on failure
 */
static int apt_cmd_install(int argc, char **argv) {
    const char *name = NULL;
    int auto_install = 0;
    
    /* Parse arguments - look for package name and --auto-install flag */
    for (int i = 2; i < argc; i++) {
        if (argv[i] == NULL) break;
        
        if (strcmp(argv[i], "--auto-install") == 0) {
            auto_install = 1;
        } else if (name == NULL) {
            name = argv[i];
        }
    }
    
    if (name == NULL || strlen(name) == 0) {
        fprintf(stderr, "apt install: missing package name\n");
        fprintf(stderr, "Usage: apt install <package> [--auto-install]\n");
        return 1;
    }
    
    /* Ensure apt is initialized and index is loaded */
    if (!apt_is_initialized()) {
        if (apt_init() != 0 || apt_load_index() < 0) {
            fprintf(stderr, "apt install: package system not initialized\n");
            fprintf(stderr, "Run 'apt init' first.\n");
            return 1;
        }
    }
    
    /* If --auto-install flag is present, resolve and install dependencies first */
    if (auto_install) {
        printf("Auto-installing dependencies for %s...\n", name);
        /* install_self=0 means only install dependencies, not the package itself */
        if (apt_install_dependencies(name, 0) != 0) {
            fprintf(stderr, "apt install: failed to install dependencies\n");
            return 1;
        }
    }
    
    /* Call the full installation function from install.c */
    /* This performs dependency checking, extraction, etc. */
    int result = apt_install_package(name);
    
    return (result == 0) ? 0 : 1;
}

/**
 * apt_cmd_remove - Remove a package with optional force flag
 * 
 * Handles the 'apt remove' subcommand by delegating to the
 * full removal implementation in remove.c.
 * 
 * Supports the --force flag to skip dependent checking and force removal.
 * 
 * @param argc: Number of arguments
 * @param argv: Argument array (argv[2] is package name, argv[3] may be --force)
 * 
 * @return 0 on success, 1 on failure
 */
static int apt_cmd_remove(int argc, char **argv) {
    const char *name = NULL;
    int force = 0;
    
    /* Parse arguments - look for package name and --force flag */
    for (int i = 2; i < argc; i++) {
        if (argv[i] == NULL) break;
        
        if (strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (name == NULL) {
            name = argv[i];
        }
    }
    
    if (name == NULL || strlen(name) == 0) {
        fprintf(stderr, "apt remove: missing package name\n");
        fprintf(stderr, "Usage: apt remove <package> [--force]\n");
        return 1;
    }
    
    /* Ensure apt is initialized and index is loaded */
    if (!apt_is_initialized()) {
        if (apt_init() != 0 || apt_load_index() < 0) {
            fprintf(stderr, "apt remove: package system not initialized\n");
            fprintf(stderr, "Run 'apt init' first.\n");
            return 1;
        }
    }
    
    /* Call the full removal function from remove.c */
    /* This performs dependency checking, recursive removal, etc. */
    /* Pass force flag to skip dependent checking if --force is present */
    int result = apt_remove_package_with_force(name, force);
    
    return (result == 0) ? 0 : 1;
}

/**
 * apt_cmd_help - Display help for apt command
 * 
 * Shows usage information and available subcommands.
 */
static void apt_cmd_help(void) {
    printf("Usage: apt <command> [options]\n\n");
    printf("Commands:\n");
    printf("  init            Initialize the package system\n");
    printf("  update          Refresh package index\n");
    printf("  list            List all packages\n");
    printf("  list --installed  List installed packages only\n");
    printf("  search <term>   Search for packages\n");
    printf("  show <package>  Show package details\n");
    printf("  install <pkg>   Install a package\n");
    printf("  remove <pkg>    Remove a package\n");
    printf("  verify <pkg>    Verify package integrity\n");
    printf("  help            Show this help message\n");
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

/**
 * builtin_apt - Main entry point for apt command
 * 
 * Parses the subcommand and dispatches to the appropriate handler.
 * 
 * @param argv: Command arguments (argv[0] = "apt")
 * @param env:  Shell environment (unused)
 * 
 * @return 0 on success, non-zero on failure
 */
int builtin_apt(char **argv, Env *env) {
    (void)env;  /* Unused */
    
    /* Count argc */
    int argc = 0;
    while (argv[argc] != NULL) argc++;
    
    /* Check for subcommand */
    if (argv[1] == NULL) {
        apt_cmd_help();
        return 0;
    }
    
    const char *cmd = argv[1];
    
    /* Dispatch to appropriate subcommand handler */
    if (strcmp(cmd, "init") == 0) {
        return apt_cmd_init();
    }
    else if (strcmp(cmd, "update") == 0) {
        return apt_cmd_update();
    }
    else if (strcmp(cmd, "list") == 0) {
        /* Check for --installed flag */
        int installed_only = 0;
        if (argv[2] != NULL && strcmp(argv[2], "--installed") == 0) {
            installed_only = 1;
        }
        return apt_cmd_list(installed_only);
    }
    else if (strcmp(cmd, "search") == 0) {
        return apt_cmd_search(argv[2]);
    }
    else if (strcmp(cmd, "show") == 0) {
        return apt_cmd_show(argv[2]);
    }
    else if (strcmp(cmd, "install") == 0) {
        return apt_cmd_install(argc, argv);
    }
    else if (strcmp(cmd, "remove") == 0) {
        return apt_cmd_remove(argc, argv);
    }
    else if (strcmp(cmd, "verify") == 0) {
        if (argv[2] == NULL) {
            fprintf(stderr, "apt verify: missing package name\n");
            fprintf(stderr, "Usage: apt verify <package>\n");
            return 1;
        }
        /* Ensure apt is initialized */
        if (!apt_is_initialized()) {
            if (apt_init() != 0 || apt_load_index() < 0) {
                fprintf(stderr, "apt verify: package system not initialized\n");
                return 1;
            }
        }
        return (apt_verify_package(argv[2]) == 0) ? 0 : 1;
    }
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        apt_cmd_help();
        return 0;
    }
    else {
        fprintf(stderr, "apt: unknown command '%s'\n", cmd);
        fprintf(stderr, "Run 'apt help' for usage information.\n");
        return 1;
    }
}
