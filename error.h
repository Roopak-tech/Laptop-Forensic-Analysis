#pragma once

typedef enum _ENUM_WSTRemoteStatus {
	WST_ERR_SUCCESS = 0,
	WST_ERR_UNKNOWN = -1,
	WST_ERR_PATH_DOES_NOT_EXIST = -2,
	WST_ERR_PIPE_CREATE = -3,
	WST_ERR_PIPE_CONNECT = -4,
	WST_ERR_PIPE_WRITE = -5,
	WST_ERR_PIPE_READ = -6,
	WST_ERR_PIPE_DISCONNECT = -7,
	WST_ERR_SCAN_ABORT = -8,
	WST_ERR_INVALID_CMDLINE = -9,
	WST_ERR_SHUTDOWN = -10,
	WST_ERR_NO_PATHS = -11,
	WST_ERR_EINVAL_OCCURRED = -12,
	WST_ERR_NO_FILES = -13,
	WST_ERR_CANNOT_CREATE_FILE = -14
} WSTRemoteStatus;

static const char *WSTRemoteStatusStrings[15] = {
	"Success",
	"Unknown error",
	"Path does not exist",
	"Error creating pipe",
	"Error connecting pipe",
	"Error writing to pipe",
	"Error reading from pipe",
	"Error disconnecting pipe",
	"Acquisition aborted",
	"Invalid command line",
	"Shutting down",
	"No scan paths specified",
	"EINVAL occured (typically an access denied error)",
	"No files found",
	"Could not create required file"
};
