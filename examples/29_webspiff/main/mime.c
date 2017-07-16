#include "mongoose.h"

#define MIME_ENTRY(_ext, _type) \
  { _ext, sizeof(_ext) - 1, _type }
static const struct {
  const char *extension;
  size_t ext_len;
  const char *mime_type;
} mg_static_builtin_mime_types[] = {
    MIME_ENTRY("html", "text/html"),
    MIME_ENTRY("html", "text/html"),
    MIME_ENTRY("htm", "text/html"),
    MIME_ENTRY("shtm", "text/html"),
    MIME_ENTRY("shtml", "text/html"),
    MIME_ENTRY("css", "text/css"),
    MIME_ENTRY("js", "application/x-javascript"),
    MIME_ENTRY("ico", "image/x-icon"),
    MIME_ENTRY("gif", "image/gif"),
    MIME_ENTRY("jpg", "image/jpeg"),
    MIME_ENTRY("jpeg", "image/jpeg"),
    MIME_ENTRY("png", "image/png"),
    MIME_ENTRY("svg", "image/svg+xml"),
    MIME_ENTRY("txt", "text/plain"),
    MIME_ENTRY("torrent", "application/x-bittorrent"),
    MIME_ENTRY("wav", "audio/x-wav"),
    MIME_ENTRY("mp3", "audio/x-mp3"),
    MIME_ENTRY("mid", "audio/mid"),
    MIME_ENTRY("m3u", "audio/x-mpegurl"),
    MIME_ENTRY("ogg", "application/ogg"),
    MIME_ENTRY("ram", "audio/x-pn-realaudio"),
    MIME_ENTRY("xml", "text/xml"),
    MIME_ENTRY("ttf", "application/x-font-ttf"),
    MIME_ENTRY("json", "application/json"),
    MIME_ENTRY("xslt", "application/xml"),
    MIME_ENTRY("xsl", "application/xml"),
    MIME_ENTRY("ra", "audio/x-pn-realaudio"),
    MIME_ENTRY("doc", "application/msword"),
    MIME_ENTRY("exe", "application/octet-stream"),
    MIME_ENTRY("zip", "application/x-zip-compressed"),
    MIME_ENTRY("xls", "application/excel"),
    MIME_ENTRY("tgz", "application/x-tar-gz"),
    MIME_ENTRY("tar", "application/x-tar"),
    MIME_ENTRY("gz", "application/x-gunzip"),
    MIME_ENTRY("arj", "application/x-arj-compressed"),
    MIME_ENTRY("rar", "application/x-rar-compressed"),
    MIME_ENTRY("rtf", "application/rtf"),
    MIME_ENTRY("pdf", "application/pdf"),
    MIME_ENTRY("swf", "application/x-shockwave-flash"),
    MIME_ENTRY("mpg", "video/mpeg"),
    MIME_ENTRY("webm", "video/webm"),
    MIME_ENTRY("mpeg", "video/mpeg"),
    MIME_ENTRY("mov", "video/quicktime"),
    MIME_ENTRY("mp4", "video/mp4"),
    MIME_ENTRY("m4v", "video/x-m4v"),
    MIME_ENTRY("asf", "video/x-ms-asf"),
    MIME_ENTRY("avi", "video/x-msvideo"),
    MIME_ENTRY("bmp", "image/bmp"),
    {NULL, 0, NULL}};

struct mg_str mg_get_mime_type(const char *path) {
  const char *ext, *overrides;
  size_t i, path_len;
  struct mg_str r, k, v;

  path_len = strlen(path);

  for (i = 0; mg_static_builtin_mime_types[i].extension != NULL; i++) {
    ext = path + (path_len - mg_static_builtin_mime_types[i].ext_len);
    if (path_len > mg_static_builtin_mime_types[i].ext_len && ext[-1] == '.' &&
        mg_casecmp(ext, mg_static_builtin_mime_types[i].extension) == 0) {
      r.p = mg_static_builtin_mime_types[i].mime_type;
      r.len = strlen(r.p);
      return r;
    }
  }

  r.len = strlen(r.p);
  return r;
}
