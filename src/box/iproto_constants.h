#ifndef TARANTOOL_IPROTO_CONSTANTS_H_INCLUDED
#define TARANTOOL_IPROTO_CONSTANTS_H_INCLUDED
/*
 * Copyright 2010-2016, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <stdbool.h>
#include <stdint.h>
#include <trivia/util.h>

#include <msgpuck.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum {
	/** Maximal iproto package body length (2GiB) */
	IPROTO_BODY_LEN_MAX = 2147483648UL,
	/* Maximal length of text handshake (greeting) */
	IPROTO_GREETING_SIZE = 128,
	/** marker + len + prev crc32 + cur crc32 + (padding) */
	XLOG_FIXHEADER_SIZE = 19
};

enum iproto_key {
	IPROTO_REQUEST_TYPE = 0x00,
	IPROTO_SYNC = 0x01,
	/* Replication keys (header) */
	IPROTO_REPLICA_ID = 0x02,
	IPROTO_LSN = 0x03,
	IPROTO_TIMESTAMP = 0x04,
	IPROTO_SCHEMA_ID = 0x05,
	/* Leave a gap for other keys in the header. */
	IPROTO_SPACE_ID = 0x10,
	IPROTO_INDEX_ID = 0x11,
	IPROTO_LIMIT = 0x12,
	IPROTO_OFFSET = 0x13,
	IPROTO_ITERATOR = 0x14,
	IPROTO_INDEX_BASE = 0x15,
	/* Leave a gap between integer values and other keys */
	IPROTO_KEY = 0x20,
	IPROTO_TUPLE = 0x21,
	IPROTO_FUNCTION_NAME = 0x22,
	IPROTO_USER_NAME = 0x23,
	/* Replication keys (body) */
	IPROTO_INSTANCE_UUID = 0x24,
	IPROTO_CLUSTER_UUID = 0x25,
	IPROTO_VCLOCK = 0x26,
	IPROTO_EXPR = 0x27, /* EVAL */
	IPROTO_OPS = 0x28, /* UPSERT but not UPDATE ops, because of legacy */
	/* Leave a gap between request keys and response keys */
	IPROTO_DATA = 0x30,
	IPROTO_ERROR = 0x31,
	IPROTO_KEY_MAX
};

#define bit(c) (1ULL<<IPROTO_##c)

#define IPROTO_HEAD_BMAP (bit(REQUEST_TYPE) | bit(SYNC) | bit(REPLICA_ID) |\
			  bit(LSN) | bit(SCHEMA_ID))
#define IPROTO_BODY_BMAP (bit(SPACE_ID) | bit(INDEX_ID) | bit(LIMIT) |\
			  bit(OFFSET) | bit(ITERATOR) | bit(INDEX_BASE) |\
			  bit(KEY) | bit(TUPLE) | bit(FUNCTION_NAME) | \
			  bit(USER_NAME) | bit(EXPR) | bit(OPS))

static inline bool
xrow_header_has_key(const char *pos, const char *end)
{
	unsigned char key = pos < end ? *pos : (unsigned char) IPROTO_KEY_MAX;
	return key < IPROTO_KEY_MAX && IPROTO_HEAD_BMAP & (1ULL<<key);
}

static inline bool
iproto_body_has_key(const char *pos, const char *end)
{
	unsigned char key = pos < end ? *pos : (unsigned char) IPROTO_KEY_MAX;
	return key < IPROTO_KEY_MAX && IPROTO_BODY_BMAP & (1ULL<<key);
}

#undef bit

static inline uint64_t
iproto_key_bit(unsigned char key)
{
	return 1ULL << key;
}

extern const unsigned char iproto_key_type[IPROTO_KEY_MAX];

/**
 * IPROTO command codes
 */
enum iproto_type {
	/** Acknowledgement that request or command is successful */
	IPROTO_OK = 0,

	/** SELECT request */
	IPROTO_SELECT = 1,
	/** INSERT request */
	IPROTO_INSERT = 2,
	/** REPLACE request */
	IPROTO_REPLACE = 3,
	/** UPDATE request */
	IPROTO_UPDATE = 4,
	/** DELETE request */
	IPROTO_DELETE = 5,
	/** CALL request - wraps result into [tuple, tuple, ...] format */
	IPROTO_CALL_16 = 6,
	/** AUTH request */
	IPROTO_AUTH = 7,
	/** EVAL request */
	IPROTO_EVAL = 8,
	/** UPSERT request */
	IPROTO_UPSERT = 9,
	/** CALL request - returns arbitrary MessagePack */
	IPROTO_CALL = 10,
	/** The maximum typecode used for box.stat() */
	IPROTO_TYPE_STAT_MAX = IPROTO_CALL + 1,

	/** PING request */
	IPROTO_PING = 64,
	/** Replication JOIN command */
	IPROTO_JOIN = 65,
	/** Replication SUBSCRIBE command */
	IPROTO_SUBSCRIBE = 66,

	/** General information about Vinyl's runs stored in .index file */
	VY_INDEX_RUN_INFO = 100,
	/** Information about Vinyl's page stored in .index file */
	VY_INDEX_PAGE_INFO = 101,
	/** Offsets for Vinyl's pages stored in .run file */
	VY_RUN_PAGE_INDEX = 102,

	/**
	 * Error codes = (IPROTO_TYPE_ERROR | ER_XXX from errcode.h)
	 */
	IPROTO_TYPE_ERROR = 1 << 15
};

/** IPROTO type name by code */
extern const char *iproto_type_strs[];

/**
 * Returns IPROTO type name by @a type code.
 * @param type IPROTO type.
 */
static inline const char *
iproto_type_name(uint32_t type)
{
	if (type >= IPROTO_TYPE_STAT_MAX)
		return NULL;
	return iproto_type_strs[type];
}

/**
 * Returns IPROTO key name by @a key code.
 * @param key IPROTO key.
 */
static inline const char *
iproto_key_name(enum iproto_key key)
{
	extern const char *iproto_key_strs[];
	if (key >= IPROTO_KEY_MAX)
		return NULL;
	return iproto_key_strs[key];
}

/**
 * Returns a map of mandatory members of IPROTO DML request.
 * @param type iproto type.
 */
static inline uint64_t
request_key_map(uint32_t type)
{
	/** Advanced requests don't have a defined key map. */
	assert(type <= IPROTO_CALL);
	extern const uint64_t iproto_body_key_map[];
	return iproto_body_key_map[type];
}

/**
 * A read only request, CALL is included since it
 * may be read-only, and there are separate checks
 * for all database requests issues from CALL.
 */
static inline bool
iproto_type_is_select(uint32_t type)
{
	return type <= IPROTO_SELECT || type == IPROTO_CALL || type == IPROTO_EVAL;
}

/** A common request with a mandatory and simple body (key, tuple, ops)  */
static inline bool
iproto_type_is_request(uint32_t type)
{
	return type > IPROTO_OK && type <= IPROTO_UPSERT;
}

/**
 * The request is "synchronous": no other requests
 * on this connection should be taken before this one
 * ends.
 */
static inline bool
iproto_type_is_sync(uint32_t type)
{
	return type == IPROTO_JOIN || type == IPROTO_SUBSCRIBE;
}

/** A data manipulation request. */
static inline bool
iproto_type_is_dml(uint32_t type)
{
	return (type >= IPROTO_SELECT && type <= IPROTO_DELETE) ||
		type == IPROTO_UPSERT;
}

/** This is an error. */
static inline bool
iproto_type_is_error(uint32_t type)
{
	return (type & IPROTO_TYPE_ERROR) != 0;
}

/** The snapshot row metadata repeats the structure of REPLACE request. */
struct PACKED request_replace_body {
	uint8_t m_body;
	uint8_t k_space_id;
	uint8_t m_space_id;
	uint32_t v_space_id;
	uint8_t k_tuple;
};

/**
 * Xrow keys for Vinyl's run information.
 * @sa struct vy_run_info.
 */
enum vy_run_info_key {
	/** Minimal LSN over all statements in a run. */
	VY_RUN_INFO_MIN_LSN = 1,
	/** Maximal LSN over all statements in a run. */
	VY_RUN_INFO_MAX_LSN = 2,
	/** Number of pages in a run. */
	VY_RUN_INFO_PAGE_COUNT = 3,
	/** Bloom filter for keys. */
	VY_RUN_INFO_BLOOM = 4,
	/** The last key in this enum + 1 */
	VY_RUN_INFO_KEY_MAX = VY_RUN_INFO_BLOOM + 1
};

/**
 * Return vy_run_info key name by @a key code.
 * @param key key
 */
static inline const char *
vy_run_info_key_name(enum vy_run_info_key key)
{
	if (key < VY_RUN_INFO_MIN_LSN || key >= VY_RUN_INFO_KEY_MAX)
		return NULL;
	extern const char *vy_run_info_key_strs[];
	return vy_run_info_key_strs[key];
}

/**
 * Xrow keys for Vinyl's page information.
 * @sa struct vy_run_info.
 */
enum vy_page_info_key {
	VY_PAGE_INFO_OFFSET = 1,
	/** Size of page data on the disk. */
	VY_PAGE_INFO_SIZE = 2,
	/** Size of page data in memory, i.e. unpacked. */
	VY_PAGE_INFO_UNPACKED_SIZE = 3,
	/* The number of rows in a page */
	VY_PAGE_INFO_ROW_COUNT = 4,
	/* Minimal LSN of all keys in a page. */
	VY_PAGE_INFO_MIN_KEY = 5,
	/* Page index offset in a page */
	VY_PAGE_INFO_PAGE_INDEX_OFFSET = 6,
	/** The last key in this enum + 1 */
	VY_PAGE_INFO_KEY_MAX = VY_PAGE_INFO_PAGE_INDEX_OFFSET + 1
};

/**
 * Return vy_page_info key name by @a key code.
 * @param key key
 */
static inline const char *
vy_page_info_key_name(enum vy_page_info_key key)
{
	if (key < VY_PAGE_INFO_OFFSET || key >= VY_PAGE_INFO_KEY_MAX)
		return NULL;
	extern const char *vy_page_info_key_strs[];
	return vy_page_info_key_strs[key];
}

/**
 * Keys for Vinyl's page index.
 * @sa struct vy_page.
 */
enum vy_page_index_key {
	/** An array of row offsets in a page data (a page index). */
	VY_PAGE_INDEX_INDEX = 1,
	/** The last key in this enum + 1 */
	VY_PAGE_INDEX_KEY_MAX = VY_PAGE_INDEX_INDEX + 1
};

/**
 * Return vy_page_info key name by @a key code.
 * @param key key
 */
static inline const char *
vy_page_index_key_name(enum vy_page_index_key key)
{
	if (key < VY_PAGE_INDEX_INDEX || key >= VY_PAGE_INDEX_KEY_MAX)
		return NULL;
	extern const char *vy_page_index_key_strs[];
	return vy_page_index_key_strs[key];
}

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* TARANTOOL_IPROTO_CONSTANTS_H_INCLUDED */
