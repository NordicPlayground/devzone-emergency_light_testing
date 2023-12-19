/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_monitor
 * @{
 * @brief API for the Bluetooth Mesh Chat Client model.
 */

#ifndef BT_MESH_LIGHT_MONITOR_CLI_H__
#define BT_MESH_LIGHT_MONITOR_CLI_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/sensor_cli.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID 0x0059
#define BT_MESH_LIGHT_MONITOR_VENDOR_MODEL_ID 0x000C

#define BT_MESH_LIGHT_MONITOR_VENDOR_SETUP_MODEL_ID 0x000D

#define GET_STATUS_OPCODE BT_MESH_MODEL_OP_3(0x01, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define TEST_ACK_OPCODE BT_MESH_MODEL_OP_3(0x02, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define TEST_RESULT_OPCODE BT_MESH_MODEL_OP_3(0x03, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define UPDATE_STATUS_OPCODE BT_MESH_MODEL_OP_3(0x04, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define TEST_START_OPCODE BT_MESH_MODEL_OP_3(0x05, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define RESULT_LOG_OPCODE BT_MESH_MODEL_OP_3(0x06, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_LOG_OPCODE BT_MESH_MODEL_OP_3(0x07, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_ACK_OPCODE BT_MESH_MODEL_OP_3(0x08, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_START_OPCODE BT_MESH_MODEL_OP_3(0x09, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_RESULT_OPCODE BT_MESH_MODEL_OP_3(0x0A, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_TEST_START_OPCODE BT_MESH_MODEL_OP_3(0x0B, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)

#define CALIBRATE_OPCODE BT_MESH_MODEL_OP_3(0x0C, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define CALIBRATE_OK_OPCODE BT_MESH_MODEL_OP_3(0x0D, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)

#define BT_MESH_LIGHT_MONITOR_MSG_MINLEN_MESSAGE 1
#define BT_MESH_LIGHT_MONITOR_MSG_MAXLEN_MESSAGE                                                   \
	(CONFIG_BT_MESH_LIGHT_MONITOR_MESSAGE_LENGTH + 1) /* + \0 */
#define BT_MESH_LIGHT_MONITOR_MSG_LEN_MESSAGE_REPLY 0
#define BT_MESH_LIGHT_MONITOR_MSG_LEN_PRESENCE 1
#define BT_MESH_LIGHT_MONITOR_MSG_LEN_PRESENCE_GET 0

#define GET_STATUS_LEN 0
#define TEST_ACK_LEN 0
#define TEST_RESULT_LEN 1
#define STATUS_UPDATE_LEN 2
#define TEST_START_LEN 1
#define RESULT_LOG_LEN 5
#define GET_LOG_LEN 1
#define GET_START_LEN 0
#define GET_RESULT_LEN 0
#define CALIBRATE_LEN 0

#define SLEEP_TIME_MS 1000
#define RECEIVE_BUFF_SIZE 2000
#define RECEIVE_TIMEOUT 100

/*test*/
/* Forward declaration of the Bluetooth Mesh Light Monitor Client model context. */
struct bt_mesh_light_monitor;

#define BT_MESH_MODEL_LIGHT_MONITOR(_monitor)                                                      \
	BT_MESH_MODEL_VND_CB(BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID,                              \
			     BT_MESH_LIGHT_MONITOR_VENDOR_MODEL_ID, _bt_mesh_light_monitor_op,     \
			     &(_monitor)->pub,                                                     \
			     BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_monitor, _monitor),      \
			     &_bt_mesh_light_monitor_cb)										   


/** Bluetooth Mesh Light Monitor handlers. */
struct bt_light_monitor_handlers {
	/** @brief Called after the monitor has been provisioned, or after all
     * mesh data has been loaded from persistent storage.
     *
     * @param[in] monitor Light Monitor instance that has been started.
     */
	void (*const start)(struct bt_mesh_light_monitor *monitor);

	/** @brief Handler for an update message.
     *
     * @param[in] monitor Light Monitor instance that received the update message.
     * @param[in] ctx Context of the incoming message.
     * @param[in] msg Received update message.nRF Connect SDK
     */
	void (*const update)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
			     uint16_t msg);

	/** @brief Handler for a result.
     *
     * @param[in] monitor Light Monitor instance that received the result.
     * @param[in] ctx Context of the incoming message.
     * @param[in] result Pointer to the received result.
     */
	void (*const result)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
			     bool *result);

	/** @brief Handler for a result log entry.
     *
     * @param[in] monitor Light Monitor instance that received the result log.
     * @param[in] ctx Context of the incoming message.
     * @param[in] result The result of given test.
     * @param[in] time_stamp The timestamp of when the test was started.
     */
	int (*const result_log)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
				 bool result, uint32_t time_stamp);

	/** @brief Handler for a test acknowledgement message.
     *
     * @param[in] monitor Light Monitor instance that received the test acknowledgement message.
     * @param[in] ctx Context of the incoming message.
     */
	int (*const test_ack)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a get_start message.
     *
     * @param[in] monitor Light Monitor instance that received the get start message.
     * @param[in] ctx Context of the incoming message.
     */
	int (*const get_start)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);


	/** @brief Handler for a calibrate_ok message.
     *
     * @param[in] monitor Light Monitor instance that received the get start message.
     * @param[in] ctx Context of the incoming message.
     */
	void (*const calibrate_ok)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);
};

struct bt_mesh_light_monitor {
	/** Access model pointer. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publication message. */
	struct net_buf_simple pub_msg;

	struct bt_mesh_sensor_cli sensor_cli;
	/** Publication message buffer. */
	uint16_t buf[BT_MESH_MODEL_BUF_LEN(UPDATE_STATUS_OPCODE,
					   BT_MESH_LIGHT_MONITOR_MSG_MAXLEN_MESSAGE)];
	/** Handler function structure. */
	const struct bt_light_monitor_handlers *handlers;
};

struct bt_mesh_light_monitor_setup {
	/** Access model pointer. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publication message. */
	struct net_buf_simple pub_msg;
	/** Publication message buffer. */
	uint16_t buf[BT_MESH_MODEL_BUF_LEN(UPDATE_STATUS_OPCODE,
					   BT_MESH_LIGHT_MONITOR_MSG_MAXLEN_MESSAGE)];
	/** Handler function structure. */
	const struct bt_light_monitor_setup_handlers *handlers;
};

int set_light_test_start(struct bt_mesh_light_monitor *monitor, uint16_t test_duration,
			 uint32_t time_stamp);
int get_status(struct bt_mesh_light_monitor *monitor);
int get_test_result(struct bt_mesh_light_monitor *monitor, uint16_t addr);
int set_light_test_start_single(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
				uint16_t test_duration, uint32_t timestamp);
int get_test_ack(struct bt_mesh_light_monitor *monitor, uint16_t addr);
int get_result_log(struct bt_mesh_light_monitor *monitor, uint16_t addr);
int calibrate_node(struct bt_mesh_light_monitor *monitor, uint16_t addr);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_monitor_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_monitor_cb;
extern const struct bt_mesh_model_op _bt_mesh_light_monitor_setup_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_monitor_setup_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_MONITOR_H__ */

/** @} */
