/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/drivers/uart.h>

#include "light_monitor_cli.h"
#include "model_handler.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
/*#include <power/reboot.h>*/

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/logging/log.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define THRESHOLD_VALUE 3500
#define DIGITAL_PIN 29

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
static const struct device *gpio_dev;
static struct bt_mesh_light_monitor monitor;
bool nodes_list_reading_active = false;
struct NodesList active_nodes;
bool ack_list[1024] = { 0 };
bool res_list[1024] = { 0 };
bool test_running = false;
int err;

static const struct shell *monitor_shell;
/******************************************************************************/
/*************************** Health server setup ******************************/
/******************************************************************************/
/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
		BIT(0) | BIT(1),
		BIT(1) | BIT(2),
		BIT(2) | BIT(3),
		BIT(3) | BIT(0),
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);
/******************************************************************************/
/**************************** peripheral setup ********************************/
/******************************************************************************/
/*The uart is configured and logic for the test is handled here*/
uint32_t this_test_timestamp;
uint16_t this_test_duration;
uint16_t ack_idx;

static void result_checker();
static void ack_checker();
struct k_timer ack_timer;
struct k_work ack_work;
K_WORK_DEFINE(ack_work, ack_checker);
struct k_timer result_timer;
struct k_work result_work;
K_WORK_DEFINE(result_work, result_checker);

static void set_array_to_false(bool array[])
{
	for (int x = 0; x < active_nodes.len; x++) {
		array[active_nodes.nodes[x]] = false;
	}
}

static void ack_work_handler(struct k_timer *timer)
{
	k_work_submit(&ack_work);
}

static struct k_work_delayable sched_ack_work;

static void sched_ack_work_cb(struct k_work *work)
{
	ack_checker();
}

static void ack_checker()
{
	if (active_nodes.nodes[ack_idx] == 0) {
		shell_print(monitor_shell, "Empty nodes list \n");
	} else if (ack_list[active_nodes.nodes[ack_idx]] == false) {
		get_test_ack(&monitor, active_nodes.nodes[ack_idx]);
		shell_print(monitor_shell, "Get test ack for node %d\n", active_nodes.nodes[ack_idx]);
	}

	if (ack_idx >= active_nodes.len) {
		ack_idx = 0;
		set_array_to_false(ack_list);
	} else {
		ack_idx++;
		k_work_reschedule(&sched_ack_work, K_MSEC(500));
	}
}

static void result_work_handler(struct k_timer *timer)
{
	k_work_submit(&result_work);
}

static struct k_work_delayable sched_res_work;

static void sched_res_work_cb(struct k_work *work)
{
	result_checker();
}

static void result_checker()
{
	if (active_nodes.nodes[ack_idx] == 0) {
		shell_print(monitor_shell, "Empty nodes list\n");
	} else if (res_list[active_nodes.nodes[ack_idx]] == false) {
		get_test_result(&monitor, active_nodes.nodes[ack_idx]);
		shell_print(monitor_shell, "Get test result for node %d\n", active_nodes.nodes[ack_idx]);
	}
	if (ack_idx >= active_nodes.len) {
		ack_idx = 0;
		test_running = false;
		set_array_to_false(res_list);
	} else {
		ack_idx++;
		k_work_reschedule(&sched_res_work, K_MSEC(1000));
	}
}

static int test_start(uint16_t duration, uint32_t time)
{
	ack_idx = 0;
	test_running = true;
	set_light_test_start(&monitor, duration, time);
	k_timer_init(&ack_timer, ack_work_handler, NULL);
	k_timer_start(&ack_timer, K_SECONDS(2), K_NO_WAIT);
	k_timer_init(&result_timer, result_work_handler, NULL);
	k_timer_start(&result_timer, K_SECONDS(duration + 5), K_NO_WAIT);
	this_test_timestamp = time;
	this_test_duration = duration;
	return 0;
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}
	if (pressed == BIT(0)) {
		shell_print(monitor_shell, "button 1 was pressed\n");
	}
	if (pressed == BIT(1)) {
		shell_print(monitor_shell, "button 2 was pressed\n");
		err = get_status(&monitor);
	}
	if (pressed == BIT(2)) {
		gpio_pin_set(gpio_dev, DIGITAL_PIN, 0);
		shell_print(monitor_shell, "button 3 was pressed\n");
	}
	if (pressed == BIT(3)) {
		gpio_pin_set(gpio_dev, DIGITAL_PIN, 1);
		shell_print(monitor_shell, "button 4 was pressed\n");
	}

	if (err < 0) {
		shell_print(monitor_shell, "err is %d \n", err);
	}
}

/******************************************************************************/
/*************************** monitor model setup ******************************/
/******************************************************************************/
/*All handling of monitor model cb  are done here*/

static void handle_start(struct bt_mesh_light_monitor *monitor)
{
	shell_print(monitor_shell, "Started \n");
}

static void handle_status_update(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
				 uint16_t msg)
{
	shell_print(monitor_shell, "status %d %d", ctx->addr, msg);
}

static void handle_result(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
			  bool *result)
{
	shell_print(monitor_shell, "result %d %s", ctx->addr,  *result ? "passed" : "failed");
	res_list[ctx->addr] = true;

}

static int handle_test_ack(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx)
{
	shell_print(monitor_shell, "acking %d waiting", ctx->addr);

	ack_list[ctx->addr] = true;
	return 0;
}

static int handle_get_start(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx)
{
	set_light_test_start_single(monitor, ctx, this_test_duration, this_test_timestamp);
	return 0;
}

static int handle_result_log(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
			      bool result, uint32_t time_stamp)
{
	shell_print(monitor_shell, "logged %d %u %d \n", result, time_stamp, ctx->addr);

	return 0;
}

static void handle_series_entry(struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_sensor_type *sensor, uint8_t index,
				uint8_t count, const struct bt_mesh_sensor_series_entry *entry)
{
	shell_print(monitor_shell, "status %d", ctx->addr);
}

static void handle_sensor_data(struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_sensor_type *sensor,
			       const struct sensor_value *value)
{
	uint16_t msg;
	msg = value->val1;
	shell_print(monitor_shell, "status %d %d", ctx->addr, msg);
}

static void handle_calibrate_ok(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx)
{
	shell_print(monitor_shell, "calibrate is ok for node %d \n", ctx->addr);
}

static const struct bt_light_monitor_handlers monitor_handlers = {
	.start = handle_start,
	.update = handle_status_update,
	.result = handle_result,
	.test_ack = handle_test_ack,
	.result_log = handle_result_log,
	.get_start = handle_get_start,
	.calibrate_ok = handle_calibrate_ok

};

static const struct bt_mesh_sensor_cli_handlers sensor_handlers = {
	.series_entry = handle_series_entry,
	.data = handle_sensor_data,
};

static struct bt_mesh_light_monitor monitor = {
	.handlers = &monitor_handlers,
	.sensor_cli.cb = &sensor_handlers,
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
					BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_MONITOR(&monitor))),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static int cmd_test_start(const struct shell *shell, size_t argc, char *argv[])
{

		uint32_t msg_value;
		uint16_t msg_duration;

		msg_duration = strtol(argv[1], NULL, 0);
		msg_value = strtol(argv[2], NULL, 0);

		if (test_running == false) {
			test_start(msg_duration, msg_value);
		} else {
			shell_print(monitor_shell, "Test is already running \n");
		}

	return 0;
}

static int cmd_get_status(const struct shell *shell, size_t argc, char *argv[])
{
	get_status(&monitor);

	return 0;
}

static int cmd_get_result_log(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t msg_value;

	msg_value = strtol(argv[1], NULL, 0);
	get_result_log(&monitor, msg_value);

	return 0;
}

static int cmd_get_test_ack(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t msg_value;


	msg_value = strtol(argv[1], NULL, 0);
	get_test_ack(&monitor, msg_value);

	return 0;
}

static int cmd_nodes_list(const struct shell *shell, size_t argc, char *argv[])
{

	for (int i = 0; i < active_nodes.len; i++) {
		uint16_t nodeValue = active_nodes.nodes[i];
		shell_print(monitor_shell, "nodeok %d\n", nodeValue);
	}

	return 0;
}

static int cmd_portok(const struct shell *shell, size_t argc, char *argv[])
{
	shell_print(monitor_shell,"PORTOK\n");

	return 0;
}

static int cmd_reset_nodes(const struct shell *shell, size_t argc, char *argv[])
{
	active_nodes.nodes[0] = 0;
	active_nodes.len = 0;

	return 0;
}

static int cmd_add_first_node(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t msg_value;

	msg_value = strtol(argv[1], NULL, 0);
	active_nodes.nodes[active_nodes.len] = msg_value;

	return 0;
}

static int cmd_add_node(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t msg_value;

	msg_value = strtol(argv[1], NULL, 0);
	active_nodes.len++;
	active_nodes.nodes[active_nodes.len] = msg_value;
	return 0;
}


static int cmd_calibrate_node(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t msg_value;

	msg_value = strtol(argv[1], NULL, 0);
	calibrate_node(&monitor, msg_value);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(monitor_cmds,
	SHELL_CMD_ARG(start, NULL, "Start test", cmd_test_start, 3, 0),
	SHELL_CMD_ARG(status, NULL, "Get status", cmd_get_status, 0, 0),
	SHELL_CMD_ARG(log, NULL, "get log", cmd_get_result_log, 2, 0),
	SHELL_CMD_ARG(ack, NULL, "get ack from selected node. Input is node addr", cmd_get_test_ack, 2, 0),
	SHELL_CMD_ARG(nodeslist, NULL, "Get a list of the nodes registered on the card", cmd_nodes_list, 0, 0),
	SHELL_CMD_ARG(portok, NULL, "Getting the right port helper", cmd_portok, 0, 0),
	SHELL_CMD_ARG(reset_nodes, NULL, "Resets the list of nodes back to 0", cmd_reset_nodes, 0, 0),
	SHELL_CMD_ARG(add_first_node, NULL, "Add the first node to a blank list", cmd_add_first_node, 2, 0),
	SHELL_CMD_ARG(add_node, NULL, "Add a node to a not empty list", cmd_add_node, 2, 0),
	SHELL_CMD_ARG(calibrate, NULL, "Calibrate the sensor on the node", cmd_calibrate_node, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_monitor(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(monitor, &monitor_cmds, "Bluetooth Mesh Chat Client commands",
		       cmd_monitor, 1, 1);

const struct bt_mesh_comp *model_handler_init(void)
{
	//k_work_init_delayable(&send_msg_work, send_msg_work_cb);
	k_work_init_delayable(&attention_blink_work, attention_blink);
	k_work_init_delayable(&sched_ack_work, sched_ack_work_cb);
	k_work_init_delayable(&sched_res_work, sched_res_work_cb);

	monitor_shell = shell_backend_uart_get_ptr();
	shell_print(monitor_shell, ">>> Shell test <<<");
	/* uart_init(); */
	static struct button_handler button_handler = {
		.cb = button_handler_cb,
	};
	dk_button_handler_add(&button_handler);
	return &comp;
}
