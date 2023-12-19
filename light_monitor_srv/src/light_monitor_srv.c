/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "light_monitor_srv.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include <bluetooth/mesh/sensor_srv.h>

uint8_t err;

extern int handle_get_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;

	if (monitor->handlers->get) {
		monitor->handlers->get(monitor, ctx);
	}
	return 0;
}

static int handle_light_test_start(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	uint16_t test_duration;
	uint32_t time_stamp;
	test_duration = net_buf_simple_pull_le16(buf);
	time_stamp = net_buf_simple_pull_le32(buf);
	struct bt_mesh_light_monitor *monitor = model->user_data;

	if (monitor->handlers->test) {
		monitor->handlers->test(monitor, ctx, test_duration, time_stamp);
	}
	return 0;
}

static int handle_log_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	if (monitor->handlers->get_log) {
		monitor->handlers->get_log(monitor, ctx);
	}
	return 0;
}

static int handle_ack_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	if (monitor->handlers->get_ack) {
		monitor->handlers->get_ack(monitor, ctx);
	}
	return 0;
}

static int handle_result_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	if (monitor->handlers->get_result) {
		monitor->handlers->get_result(monitor, ctx);
	}
	return 0;
}

static int handle_calibrate(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	printk("calibrating\n");
	struct bt_mesh_light_monitor *monitor = model->user_data;
	if (monitor->setup_handlers->calibrate) {
		monitor->setup_handlers->calibrate(monitor, ctx);
	}
	return 0;
}

const struct bt_mesh_model_op _bt_mesh_light_monitor_op[] = {
	{ GET_STATUS_OPCODE, GET_STATUS_LEN, handle_get_status },
	{ TEST_START_OPCODE, TEST_START_LEN, handle_light_test_start },
	{ GET_LOG_OPCODE, GET_LOG_LEN, handle_log_get },
	{ GET_ACK_OPCODE, GET_ACK_LEN, handle_ack_get },
	{ GET_RESULT_OPCODE, GET_RESULT_LEN, handle_result_get },

	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_light_monitor_setup_op[] = {
	{ CALIBRATE_OPCODE, CALIBRATE_LEN, handle_calibrate },
	
	BT_MESH_MODEL_OP_END,
};

uint16_t msg;
bool result_msg;
uint32_t time_stamp;
extern int send_sensor_update(struct bt_mesh_light_monitor *monitor, uint16_t update_value)
{
	msg = update_value;
	struct net_buf_simple *buf = monitor->model->pub->msg;

	bt_mesh_model_msg_init(buf, UPDATE_STATUS_OPCODE);
	net_buf_simple_add_le16(buf, msg);

	return bt_mesh_model_publish(monitor->model);
}

extern int send_test_result(struct bt_mesh_light_monitor *monitor, bool result)
{
	result_msg = result;
	struct net_buf_simple *buf = monitor->model->pub->msg;

	bt_mesh_model_msg_init(buf, TEST_RESULT_OPCODE);
	net_buf_simple_add_mem(buf, &result_msg, TEST_RESULT_LEN);
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		err = bt_mesh_model_data_store(monitor->model, true, NULL, &monitor->res_sto,
					       sizeof(monitor->res_sto));
	}

	return bt_mesh_model_publish(monitor->model);
}

extern int send_logged_result(struct bt_mesh_light_monitor *monitor, uint32_t time_stamp,
			      bool result)
{
	result_msg = result;
	time_stamp = time_stamp;
	struct net_buf_simple *buf = monitor->model->pub->msg;
	printk("Sendt logged result is %u and time is %d \n", result_msg, time_stamp);

	bt_mesh_model_msg_init(buf, RESULT_LOGG_OPCODE);
	net_buf_simple_add_mem(buf, &result_msg, 1);
	net_buf_simple_add_le32(buf, time_stamp);

	return bt_mesh_model_publish(monitor->model);
}

extern int get_test_start(struct bt_mesh_light_monitor *monitor)
{
	struct net_buf_simple *buf = monitor->model->pub->msg;

	bt_mesh_model_msg_init(buf, GET_START_OPCODE);

	return bt_mesh_model_publish(monitor->model);
}

extern int send_test_ack(struct bt_mesh_light_monitor *monitor)
{
	struct net_buf_simple *buf = monitor->model->pub->msg;

	bt_mesh_model_msg_init(buf, TEST_ACK_OPCODE);

	return bt_mesh_model_publish(monitor->model);
}

extern int send_calibrated_ok(struct bt_mesh_light_monitor *monitor)
{
	struct net_buf_simple *buf = monitor->model->pub->msg;

	bt_mesh_model_msg_init(buf, CALIBRATE_OK_OPCODE);

	return bt_mesh_model_publish(monitor->model);
}

static int bt_mesh_light_monitor_update_handler(struct bt_mesh_model *model)
{
	return 0;
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_light_monitor_srv_settings_set(struct bt_mesh_model *model, const char *name,
						  size_t len_rd, settings_read_cb read_cb,
						  void *cb_arg)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;

	if (name) {
		return -ENOENT;
	}
	ssize_t bytes = read_cb(cb_arg, &monitor->res_sto, sizeof(monitor->res_sto));

	if (bytes < 0) {
		return bytes;
	}

	if (bytes != 0 && bytes != sizeof(monitor->res_sto)) {
		return -EINVAL;
	}

	return 0;
}
#endif

static int bt_mesh_light_monitor_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_monitor *monitor = model->user_data;
	monitor->model = model;
	monitor->setup_pub.msg = &monitor->setup_pub_buf;

	net_buf_simple_init_with_data(&monitor->setup_pub_buf, monitor->buf,
				      sizeof(monitor->setup_pub_buf));

	net_buf_simple_init_with_data(&monitor->pub_msg, monitor->buf, sizeof(monitor->buf));
	monitor->pub.msg = &monitor->pub_msg;
	monitor->pub.update = bt_mesh_light_monitor_update_handler;

	return 0;
}

static int bt_mesh_light_monitor_setup_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_monitor *monitor_setup = model->user_data;

	return bt_mesh_model_extend(model, monitor_setup->model);
}

const struct bt_mesh_model_cb _bt_mesh_light_monitor_setup_cb = {
	.init = bt_mesh_light_monitor_setup_init,
};

const struct bt_mesh_model_cb _bt_mesh_light_monitor_cb = {
	.init = bt_mesh_light_monitor_init,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = bt_mesh_light_monitor_srv_settings_set,
#endif
}; /*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
