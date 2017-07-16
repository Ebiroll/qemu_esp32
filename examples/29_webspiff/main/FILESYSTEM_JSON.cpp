#include <stdlib.h>
#include <JSON.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <esp_log.h>

static char tag[] = "FILESYSTEM_JSON";

JsonObject FILESYSTEM_GET_JSON(std::string path) {
	ESP_LOGD(tag, "FILESYSTEM_GET_JSON: path is %s", path.c_str());
	ESP_LOGD(tag, "Get file %s", path.c_str());
	struct stat statBuf;
	int rc = stat(path.c_str(), &statBuf);

	JsonObject obj = JSON::createObject();
	if (rc == -1) {
		ESP_LOGE(tag, "Failed to stat file, errno=%s", strerror(errno));
		obj.setInt("errno", errno);
		return obj;
	}
	// Do one thing if the file is a regular file.
	if (S_ISREG(statBuf.st_mode)) {

	}
	// Do a different thing if the file is a directory.
	else if (S_ISDIR(statBuf.st_mode)) {
		ESP_LOGD(tag, "Processing directory: %s", path.c_str());

	}
	return obj;
} // FILESYSTEM_GET_JSON
