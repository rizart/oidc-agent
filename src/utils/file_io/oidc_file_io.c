#include "oidc_file_io.h"

#include "defines/settings.h"
#include "file_io.h"
#include "list/list.h"

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

char* possibleLocations[] = {"~/.config/oidc-agent/", "~/.oidc-agent/"};

/** @fn char* readOidcFile(const char* filename)
 * @brief reads a file located in the oidc dir and returns a pointer to the
 * content
 * @param filename the filename of the file
 * @return a pointer to the file content. Has to be freed after usage.
 */
char* readOidcFile(const char* filename) {
  char* path = concatToOidcDir(filename);
  char* c    = readFile(path);
  secFree(path);
  return c;
}

/** @fn void writeOidcFile(const char* filename, const char* text)
 * @brief writes text to a file located in the oidc directory
 * @note \p text has to be nullterminated and must not contain nullbytes.
 * @param filename the file to be written
 * @param text the nullterminated text to be written
 * @return OIDC_OK on success, OID_EFILE if an error occured. The system sets
 * errno.
 */
oidc_error_t writeOidcFile(const char* filename, const char* text) {
  char*        path = concatToOidcDir(filename);
  oidc_error_t er   = writeFile(path, text);
  secFree(path);
  return er;
}

/** @fn int oidcFileDoesExist(const char* filename)
 * @brief checks if a file exists in the oidc dir
 * @param filename the file to be checked
 * @return 1 if the file does exist, 0 if not
 */
int oidcFileDoesExist(const char* filename) {
  char* path = concatToOidcDir(filename);
  int   b    = fileDoesExist(path);
  secFree(path);
  return b;
}

/** @fn char* getOidcDir()
 * @brief get the oidc directory path
 * @return a pointer to the oidc directory path. Has to be freed after usage. If
 * no oidc dir is found, NULL is returned
 */
char* getOidcDir() {
  char*        home = getenv("HOME");
  unsigned int i;
  for (i = 0; i < sizeof(possibleLocations) / sizeof(*possibleLocations); i++) {
    char* path = oidc_strcat(home, possibleLocations[i] + 1);
    syslog(LOG_AUTHPRIV | LOG_DEBUG, "Checking if dir '%s' exists.", path);
    if (dirExists(path) > 0) {
      return path;
    }
    secFree(path);
  }
  return NULL;
}

oidc_error_t createOidcDir() {
  char* home       = getenv("HOME");
  char* configPath = oidc_strcat(home, "/.config");
  char* oidcdir    = NULL;
  if (dirExists(configPath) > 0) {
    oidcdir = oidc_strcat(home, possibleLocations[0] + 1);
  } else {
    oidcdir = oidc_strcat(home, possibleLocations[1] + 1);
  }
  secFree(configPath);
  oidc_error_t ret = createDir(oidcdir);
  char*        issuerconfig_path =
      oidc_sprintf("%s/%s", oidcdir, ISSUER_CONFIG_FILENAME);
  secFree(oidcdir);
  int fd = open(issuerconfig_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  close(fd);
  secFree(issuerconfig_path);
  return ret;
}

/** @fn int removeOidcFile(const char* filename)
 * @brief removes a file located in the oidc dir
 * @param filename the filename of the file to be removed
 * @return On success, 0 is returned.  On error, -1 is returned, and errno is
 * set appropriately.
 */
int removeOidcFile(const char* filename) {
  char* path = concatToOidcDir(filename);
  int   r    = removeFile(path);
  secFree(path);
  return r;
}

char* concatToOidcDir(const char* filename) {
  char* oidc_dir = getOidcDir();
  char* path     = oidc_strcat(oidc_dir, filename);
  secFree(oidc_dir);
  return path;
}

list_t* getFileListForDirIf(const char* dirname,
                            int(match(const char*, const char*)),
                            const char* arg) {
  DIR*           dir;
  struct dirent* ent;
  if ((dir = opendir(dirname)) != NULL) {
    list_t* list = list_new();
    list->free   = (void (*)(void*)) & _secFree;
    list->match  = (int (*)(void*, void*)) & strequal;
    while ((ent = readdir(dir)) != NULL) {
      if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
#ifdef _DIRENT_HAVE_DTYPE
        if (ent->d_type == DT_REG) {
#endif
          if (match(ent->d_name, arg)) {
            list_rpush(list, list_node_new(oidc_strcopy(ent->d_name)));
          }
#ifdef _DIRENT_HAVE_DTYPE
        }
#endif
      }
    }
    closedir(dir);
    return list;
  } else {
    oidc_seterror(strerror(errno));
    oidc_errno = OIDC_EERROR;
    return NULL;
  }
}

int alwaysOne(const char* a __attribute__((unused)),
              const char* b __attribute__((unused))) {
  return 1;
}

list_t* getFileListForDir(const char* dirname) {
  return getFileListForDirIf(dirname, &alwaysOne, NULL);
}

int isClientConfigFile(const char* filename,
                       const char* a __attribute__((unused))) {
  const char* const suffix = ".clientconfig";
  if (strEnds(filename, suffix)) {
    return 1;
  }
  char* pos = NULL;
  if ((pos = strstr(filename, suffix))) {
    pos += strlen(suffix);
    while (*pos != '\0') {
      if (!isdigit(*pos)) {
        return 0;
      }
      pos++;
    }
    return 1;
  }
  return 0;
}

int isAccountConfigFile(const char* filename,
                        const char* a __attribute__((unused))) {
  if (isClientConfigFile(filename, a)) {
    return 0;
  }
  if (strEnds(filename, ".config")) {
    return 0;
  }
  return 1;
}

list_t* getAccountConfigFileList() {
  char*   oidc_dir = getOidcDir();
  list_t* list     = getFileListForDirIf(oidc_dir, &isAccountConfigFile, NULL);
  secFree(oidc_dir);
  return list;
}

list_t* getClientConfigFileList() {
  char*        oidc_dir = getOidcDir();
  list_t*      list = getFileListForDirIf(oidc_dir, &isClientConfigFile, NULL);
  list_node_t* node;
  list_iterator_t* it = list_iterator_new(list, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    char* old = node->val;
    node->val = oidc_strcat(oidc_dir, old);
    secFree(old);
  }
  secFree(oidc_dir);
  return list;
}

int compareFilesByName(const char* filename1, const char* filename2) {
  return strcmp(filename1, filename2);
}

int compareOidcFilesByDateModified(const char* filename1,
                                   const char* filename2) {
  struct stat* stat1 = secAlloc(sizeof(struct stat));
  struct stat* stat2 = secAlloc(sizeof(struct stat));
  char*        path1 = concatToOidcDir(filename1);
  char*        path2 = concatToOidcDir(filename2);
  stat(path1, stat1);
  stat(path2, stat2);
  int ret = 0;
  if (stat1->st_mtime < stat2->st_mtime) {
    ret = -1;
  }
  if (stat1->st_mtime > stat2->st_mtime) {
    ret = 1;
  }
  secFree(stat1);
  secFree(stat2);
  return ret;
}
int compareOidcFilesByDateAccessed(const char* filename1,
                                   const char* filename2) {
  struct stat* stat1 = secAlloc(sizeof(struct stat));
  struct stat* stat2 = secAlloc(sizeof(struct stat));
  char*        path1 = concatToOidcDir(filename1);
  char*        path2 = concatToOidcDir(filename2);
  stat(path1, stat1);
  stat(path2, stat2);
  int ret = 0;
  if (stat1->st_atime < stat2->st_atime) {
    ret = -1;
  }
  if (stat1->st_atime > stat2->st_atime) {
    ret = 1;
  }
  secFree(stat1);
  secFree(stat2);
  return ret;
}
