/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "light_monitor_cli.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include <bluetooth/mesh/sensor_cli.h>

static int handle_test_ack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;

	if (monitor->handlers->test_ack) {
		monitor->handlers->test_ack(monitor, ctx);
	}
	return 0;
}

static int handle_test_result(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	bool *result;

	result = net_buf_simple_pull_mem(buf, buf->len);
	if (monitor->handlers->result) {
		monitor->handlers->result(monitor, ctx, result);
	}
	return 0;
}

static int handle_test_log(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	bool *result;
	uint32_t time_stamp;
	time_stamp = net_buf_simple_remove_le32(buf);
	result = net_buf_simple_pull_mem(buf, 1);
	if (monitor->handlers->result_log) {
		monitor->handlers->result_log(monitor, ctx, *result, time_stamp);
	}
	return 0;
}

static int handle_message_status_update(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	uint16_t msg;

	msg = net_buf_simple_pull_le16(buf);
	if (monitor->handlers->update) {
		monitor->handlers->update(monitor, ctx, msg);
	}
	return 0;
}

static int handle_test_start_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	if (monitor->handlers->get_start) {
		monitor->handlers->get_start(monitor, ctx);
	}

	return 0;
}

static int handle_calibrate_ok(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	if (monitor->handlers->calibrate_ok) {
		monitor->handlers->calibrate_ok(monitor, ctx);
	}
	return 0;
}

const struct bt_mesh_model_op _bt_mesh_light_monitor_op[] = {
	{ TEST_ACK_OPCODE, TEST_ACK_LEN, handle_test_ack },
	{ TEST_RESULT_OPCODE, TEST_RESULT_LEN, handle_test_result },
	{ UPDATE_STATUS_OPCODE, STATUS_UPDATE_LEN, handle_message_status_update },
	{ RESULT_LOG_OPCODE, RESULT_LOG_LEN, handle_test_log },
	{ GET_START_OPCODE, GET_START_LEN, handle_test_start_get },
	{ CALIBRATE_OK_OPCODE, CALIBRATE_LEN, handle_calibrate_ok },
	BT_MESH_MODEL_OP_END,
};
uint16_t msg;
bool result_msg;

int set_light_test_start(struct bt_mesh_light_monitor *monitor, uint16_t test_duration,
			 uint32_t timestamp)
{
	struct net_buf_simple *buf = monitor->model->pub->msg;

	bt_mesh_model_msg_init(buf, TEST_START_OPCODE);
	net_buf_simple_add_le16(buf, test_duration);
	net_buf_simple_add_le32(buf, timestamp);

	return bt_mesh_model_publish(monitor->model);
}

int get_test_result(struct bt_mesh_light_monitor *monitor, uint16_t addr)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = monitor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = false,
	};
	BT_MESH_MODEL_BUF_DEFINE(buf, GET_RESULT_OPCODE,
				 BT_MESH_LIGHT_MONITOR_MSG_LEN_MESSAGE_REPLY);
	bt_mesh_model_msg_init(&buf, GET_RESULT_OPCODE);

	return bt_mesh_model_send(monitor->model, &ctx, &buf, NULL, NULL);
}

int set_light_test_start_single(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
				uint16_t test_duration, uint32_t timestamp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, TEST_START_OPCODE, 6);

	bt_mesh_model_msg_init(&buf, TEST_START_OPCODE);
	net_buf_simple_add_le16(&buf, test_duration);
	net_buf_simple_add_le32(&buf, timestamp);

	(void)bt_mesh_model_send(monitor->model, ctx, &buf, NULL, NULL);

	return 0;
}

int get_test_ack(struct bt_mesh_light_monitor *monitor, uint16_t addr)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = monitor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = false,
	};
	BT_MESH_MODEL_BUF_DEFINE(buf, GET_ACK_OPCODE, BT_MESH_LIGHT_MONITOR_MSG_LEN_MESSAGE_REPLY);
	bt_mesh_model_msg_init(&buf, GET_ACK_OPCODE);
	return bt_mesh_model_send(monitor->model, &ctx, &buf, NULL, NULL);
}

int calibrate_node(struct bt_mesh_light_monitor *monitor, uint16_t addr)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = monitor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = false,
	};
	BT_MESH_MODEL_BUF_DEFINE(buf, CALIBRATE_OPCODE, CALIBRATE_LEN);
	bt_mesh_model_msg_init(&buf, CALIBRATE_OPCODE);
	return bt_mesh_model_send(monitor->model, &ctx, &buf, NULL, NULL);
}

int get_status(struct bt_mesh_light_monitor *monitor)
{
	struct net_buf_simple *buf = monitor->model->pub->msg;

	bt_mesh_model_msg_init(buf, GET_STATUS_OPCODE);

	return bt_mesh_model_publish(monitor->model);
}

int get_result_log(struct bt_mesh_light_monitor *monitor, uint16_t addr)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr, .app_idx = monitor->model->keys[0], .send_ttl = BT_MESH_TTL_DEFAULT,
		/*.send_rel = false, */

	};
	BT_MESH_MODEL_BUF_DEFINE(buf, GET_LOG_OPCODE, 2);
	bt_mesh_model_msg_init(&buf, GET_LOG_OPCODE);

	return bt_mesh_model_send(monitor->model, &ctx, &buf, NULL, NULL);
}

static int bt_mesh_light_monitor_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_light_monitor_cli *monitor = model->user_data;

	return 0;
}

static int bt_mesh_light_monitor_init(struct bt_mesh_model *model)
{
	int err;
	struct bt_mesh_light_monitor *monitor = model->user_data;
	monitor->model = model;
	struct bt_mesh_sensor_cli sensor_cli;

	err = bt_mesh_model_extend(model, sensor_cli.model);

	net_buf_simple_init_with_data(&monitor->pub_msg, monitor->buf, sizeof(monitor->buf));
	monitor->pub.msg = &monitor->pub_msg;
	monitor->pub.update = bt_mesh_light_monitor_update_handler;

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_monitor_cb = { .init = bt_mesh_light_monitor_init }; /*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
