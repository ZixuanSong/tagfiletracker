#pragma once

//database interface specific error messages
#define DB_STATEMENT_PREPARE_MSG	"Error preparing database statement"
#define DB_STATEMENT_STEP_MSG		"Error stepping database statement"
#define DB_OPEN_MSG					"Error openning database"
#define DB_DEFAULT_TABLE_MSG		"Error creating default table"
#define DB_EXEC_MSG					"Error executing multi statement query"

//daemon specific error messages
#define DAEMON_DB_MSG				"Database Error, data might be in an unstable state"
#define DAEMON_INIT_DB_MSG			"Error initializing database"

//QFile specific error messages
#define QFILE_OPEN_MSG				"Error opening file"

//ffmpeg specific error message
#define FFMPEG_ALLOC_MSG			"Error allocating ffmpeg object"
#define FFMPEG_OPEN_INPUT_MSG		"Error opening input file"
#define FFMPEG_FIND_CODEC_MSG		"Error finding codec"
#define FFMPEG_COPY_PARAM_MSG		"Error copying codec parameters"
#define FFMPEG_OPEN_CODEC_MSG		"Error openning codec"
#define FFMPEG_READ_FRAME_MSG		"Error reading frame from input context"
#define FFMPEG_DECODE_PACKET_MSG	"Error decoding packet"
#define FFMPEG_RECV_FRAME_MSG		"Error receiving frame from codec"
#define FFMPEG_GET_SWS_CONTEXT_MSG	"Error generating sws context"
#define FFMPEG_SWS_SCALE_MSG		"Error converting with sws_scale"

//filename map specific error
#define FILENAME_MAP_INVALID_BLOCK_MSG		"Map block is invalid"

//misc error
#define QREGEX_INVALID_MSG			"Invalid regular expression"

namespace Error {
	enum ErrorCode {
		NONE = 0,

		//database interface specific errors
		DB_OPEN,
		DB_STATEMENT_PREPARE,
		DB_STATEMENT_STEP,
		DB_DEFAULT_TABLE,
		DB_EXEC,

		//daemon specific errors
		DAEMON_DB,
		DAEMON_INIT_DB,

		//QFile specific errors
		QFILE_OPEN,
		QFILE_READ,
		QFILE_WRITE,

		//QJson related errors
		QJSON_PARSE,

		//ffmpeg spefic errors
		FFMPEG_ALLOC,
		FFMPEG_OPEN_INPUT,
		FFMPEG_FIND_CODEC,
		FFMPEG_COPY_PARAM,
		FFMPEG_OPEN_CODEC,
		FFMPEG_READ_FRAME,
		FFMPEG_DECODE_PACKET,
		FFMPEG_RECV_FRAME,
		FFMPEG_GET_SWS_CONTEXT,
		FFMPEG_SWS_SCALE,

		//filename map specific error
		FILENAME_MAP_INVALID_BLOCK,
		
		//misc error
		QREGEX_INVALID
	};
}