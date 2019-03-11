#include "promptCryptFileUtils.h"
#include "utils/file_io/cryptFileUtils.h"
#include "utils/promptUtils.h"

oidc_error_t _promptAndCryptAndWriteToAnyFile(
    const char* text, const char* filepath, const char* oidc_filename,
    const char* hint, const char* suggestedPassword, const char* pw_cmd) {
  if (text == NULL || hint == NULL ||
      (filepath == NULL && oidc_filename == NULL)) {
    oidc_setArgNullFuncError(__func__);
    return oidc_errno;
  }
  oidc_error_t (*encryptAndWriteFnc)(const char*, const char*, const char*) =
      encryptAndWriteToFile;
  if (oidc_filename != NULL) {
    encryptAndWriteFnc = encryptAndWriteToOidcFile;
  }
  char* encryptionPassword =
      getEncryptionPasswordFor(hint, suggestedPassword, pw_cmd);
  if (encryptionPassword == NULL) {
    return oidc_errno;
  }
  oidc_error_t ret =
      encryptAndWriteFnc(text, encryptionPassword, oidc_filename ?: filepath);
  secFree(encryptionPassword);
  return ret;
}

oidc_error_t promptEncryptAndWriteToFile(const char* text, const char* filepath,
                                         const char* hint,
                                         const char* suggestedPassword,
                                         const char* pw_cmd) {
  if (text == NULL || filepath == NULL || hint == NULL) {
    oidc_setArgNullFuncError(__func__);
    return oidc_errno;
  }
  return _promptAndCryptAndWriteToAnyFile(text, filepath, NULL, hint,
                                          suggestedPassword, pw_cmd);
}

oidc_error_t promptEncryptAndWriteToOidcFile(const char* text,
                                             const char* filename,
                                             const char* hint,
                                             const char* suggestedPassword,
                                             const char* pw_cmd) {
  if (text == NULL || filename == NULL || hint == NULL) {
    oidc_setArgNullFuncError(__func__);
    return oidc_errno;
  }
  return _promptAndCryptAndWriteToAnyFile(text, NULL, filename, hint,
                                          suggestedPassword, pw_cmd);
}

struct resultWithEncryptionPassword getDecryptedFileAndPasswordFor(
    const char* filepath, const char* pw_cmd) {
  if (filepath == NULL) {
    oidc_setArgNullFuncError(__func__);
    return RESULT_WITH_PASSWORD_NULL;
  }
  return _getDecryptedTextAndPasswordWithPromptFor(filepath, filepath,
                                                   decryptFile, 0, pw_cmd);
}

struct resultWithEncryptionPassword getDecryptedOidcFileAndPasswordFor(
    const char* filename, const char* pw_cmd) {
  if (filename == NULL) {
    oidc_setArgNullFuncError(__func__);
    return RESULT_WITH_PASSWORD_NULL;
  }
  return _getDecryptedTextAndPasswordWithPromptFor(filename, filename,
                                                   decryptOidcFile, 1, pw_cmd);
}

char* getDecryptedFileFor(const char* filepath, const char* pw_cmd) {
  if (filepath == NULL) {
    oidc_setArgNullFuncError(__func__);
    return NULL;
  }
  struct resultWithEncryptionPassword res =
      getDecryptedFileAndPasswordFor(filepath, pw_cmd);
  secFree(res.password);
  return res.result;
}

char* getDecryptedOidcFileFor(const char* filename, const char* pw_cmd) {
  if (filename == NULL) {
    oidc_setArgNullFuncError(__func__);
    return NULL;
  }
  struct resultWithEncryptionPassword res =
      getDecryptedOidcFileAndPasswordFor(filename, pw_cmd);
  secFree(res.password);
  return res.result;
}
