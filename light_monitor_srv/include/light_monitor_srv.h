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

#ifndef BT_MESH_LIGHT_MONITOR_srv_H__
#define BT_MESH_LIGHT_MONITOR_srv_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/sensor_srv.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID 0x0059
#define BT_MESH_LIGHT_MONITOR_VENDOR_MODEL_ID 0x000B

#define BT_MESH_LIGHT_MONITOR_SETUP_VENDOR_MODEL_ID 0x000C

#define GET_STATUS_OPCODE BT_MESH_MODEL_OP_3(0x01, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define TEST_ACK_OPCODE BT_MESH_MODEL_OP_3(0x02, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define TEST_RESULT_OPCODE BT_MESH_MODEL_OP_3(0x03, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define UPDATE_STATUS_OPCODE BT_MESH_MODEL_OP_3(0x04, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define TEST_START_OPCODE BT_MESH_MODEL_OP_3(0x05, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define RESULT_LOGG_OPCODE BT_MESH_MODEL_OP_3(0x06, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_LOG_OPCODE BT_MESH_MODEL_OP_3(0x07, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_ACK_OPCODE BT_MESH_MODEL_OP_3(0x08, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_START_OPCODE BT_MESH_MODEL_OP_3(0x09, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_RESULT_OPCODE BT_MESH_MODEL_OP_3(0x0A, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define GET_TEST_START_OPCODE BT_MESH_MODEL_OP_3(0x0B, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)

#define CALIBRATE_OPCODE BT_MESH_MODEL_OP_3(0x0C, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)
#define CALIBRATE_OK_OPCODE BT_MESH_MODEL_OP_3(0x0D, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)

/** Non-private message opcode. */
#define BT_MESH_LIGHT_MONITOR_OP_MESSAGE                                                           \
	BT_MESH_MODEL_OP_3(0x0A, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)

/** Message reply opcode. */
#define BT_MESH_LIGHT_MONITOR_OP_MESSAGE_REPLY                                                     \
	BT_MESH_MODEL_OP_3(0x0C, BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID)

#define GET_STATUS_LEN 0
#define TEST_ACK_LEN 0
#define TEST_RESULT_LEN 1
#define STATUS_UPDATE_LEN 2
#define TEST_START_LEN 1
#define GET_LOG_LEN 0
#define RESULT_LOGG_LEN 5
#define GET_ACK_LEN 0
#define GET_START_LEN 0
#define GET_RESULT_LEN 0

#define CALIBRATE_LEN 0

#define BT_MESH_LIGHT_MONITOR_MSG_MINLEN_MESSAGE 1
#define BT_MESH_LIGHT_MONITOR_MSG_MAXLEN_MESSAGE                                                   \
	(CONFIG_BT_MESH_LIGHT_MONITOR_MESSAGE_LENGTH + 1) /* + \0 */
#define BT_MESH_LIGHT_MONITOR_MSG_LEN_MESSAGE_REPLY 0
#define BT_MESH_LIGHT_MONITOR_MSG_LEN_PRESENCE 1
#define BT_MESH_LIGHT_MONITOR_MSG_LEN_PRESENCE_GET 0

#define SLEEP_TIME_MS 1000
#define RECEIVE_BUFF_SIZE 10
#define RECEIVE_TIMEOUT 100

/* Forward declaration of the Bluetooth Mesh light Monitor Server model context. */
struct bt_mesh_light_monitor;

#define BT_MESH_MODEL_LIGHT_MONITOR(_monitor)                                                      \
	BT_MESH_MODEL_VND_CB(BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID,                              \
			     BT_MESH_LIGHT_MONITOR_VENDOR_MODEL_ID, _bt_mesh_light_monitor_op,     \
			     &(_monitor)->pub,                                                     \
			     BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_monitor, _monitor),      \
			     &_bt_mesh_light_monitor_cb),                                          \
		BT_MESH_MODEL_VND_CB(BT_MESH_LIGHT_MONITOR_VENDOR_COMPANY_ID,                      \
				     BT_MESH_LIGHT_MONITOR_SETUP_VENDOR_MODEL_ID,                  \
				     _bt_mesh_light_monitor_setup_op, &(_monitor)->setup_pub,      \
				     BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_monitor,         \
							     _monitor),                            \
				     &_bt_mesh_light_monitor_setup_cb)

/** Bluetooth Mesh Light Monitor handlers. */
struct bt_light_monitor_handlers {
	/** @brief Called after the monitor has been started.
     *
     * @param[in] monitor Light Monitor instance that has been started.
     */
	void (*const start)(struct bt_mesh_light_monitor *monitor);

	/** @brief Handler for an update message.
     *
     * @param[in] monitor Light Monitor instance that received the update message.
     * @param[in] ctx Context of the incoming message.
     * @param[in] msg Pointer to a received update message.
     */
	void (*const update)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
			     const uint16_t *msg);

	/** @brief Handler for starting a test
     *
     * @param[in] monitor Light Monitor instance that received the test message.
     * @param[in] ctx Context of the incoming message.
     * @param[in] duration The duration for which the test will run in seconds.
     * @param[in] time_stamp The timestamp of when the test start message was sent.
     */
	void (*const test)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
			   uint16_t duration, uint32_t time_stamp);

	/** @brief Handler for a result.
     *
     * @param[in] monitor Light Monitor instance that received the result.
     * @param[in] ctx Context of the incoming message.
     * @param[in] result Pointer to the received result.
     */
	void (*const result)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
			     bool *result);

	/** @brief Handler for a get message.
     *
     * @param[in] monitor Light Monitor instance that received the get message.
     * @param[in] ctx Context of the incoming message.
     */
	int (*const get)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a get_log message.
     *
     * @param[in] monitor Light Monitor instance that received the get_log message.
     * @param[in] ctx Context of the incoming message.
     */
	int (*const get_log)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a test_ack message.
     *
     * @param[in] monitor Light Monitor instance that received the test_ack message.
     * @param[in] ctx Context of the incoming message.
     */
	int (*const test_ack)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a get_ack message.
     *
     * @param[in] monitor Light Monitor instance that received the get_ack message.
     * @param[in] ctx Context of the incoming message.
     */
	int (*const get_ack)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a get_result message.
     *
     * @param[in] monitor Light Monitor instance that received the get_result message.
     * @param[in] ctx Context of the incoming message.
     */
	int (*const get_result)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);

};

struct bt_light_monitor_setup_handlers {

	/** @brief Handler for a calibrate message.
     *
     * @param[in] monitor Light Monitor instance that received the calibrate message.
     * @param[in] ctx Context of the incoming message.
     */
	void (*const calibrate)(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx);
};

struct test_result {
	bool result;
	uint32_t time_stamp;
};

struct results_store {
	struct test_result results[8];
	uint8_t last_result_idx;
};

struct bt_mesh_light_monitor {
	/** Generic sensor Server instance. */
	struct bt_mesh_sensor_srv sensor_srv;
	/**light sensor instance*/
	struct bt_mesh_sensor light_sensor;
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

	const struct bt_light_monitor_handlers *handlers;

	const struct bt_light_monitor_setup_handlers *setup_handlers;
	struct results_store res_sto;

	struct bt_mesh_model_pub setup_pub;
	/* Publication buffer */
	struct net_buf_simple setup_pub_buf;
	/* Publication data */
};

extern int send_sensor_update(struct bt_mesh_light_monitor *monitor, uint16_t sample_value);
extern int set_light_test_start(struct bt_mesh_light_monitor *monitor, uint16_t test_duration,
				uint32_t time_stamp);
extern int send_test_ack(struct bt_mesh_light_monitor *monitor);
extern int send_test_result(struct bt_mesh_light_monitor *monitor, bool result);
extern int handle_get_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf);
extern int get_status(struct bt_mesh_light_monitor *monitor);
extern int send_logged_result(struct bt_mesh_light_monitor *monitor, uint32_t time_stamp,
			      bool result);
extern int get_test_start(struct bt_mesh_light_monitor *monitor);
extern int send_calibrated_ok(struct bt_mesh_light_monitor *monitor);

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
