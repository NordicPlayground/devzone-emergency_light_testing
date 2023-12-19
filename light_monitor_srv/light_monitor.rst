.. _bt_mesh_chat_client_model:

Light Monitor Server Model
#################

.. contents::
   :local:
   :depth: 2

The Light Monitor Client model is a vendor model that alllows the communcation with Light Monitor Sensor models and Calibration Server Models, by starting tests, retriving logs, updating the calibration and receiving updates from the sensors. 

Overview
********

In this section, you can find more detailed information about the following aspects of the Chat Client:

* `Composition data structure`_
* `Messages`_

.. _bt_mesh_chat_client_model_composition:

Composition data structure
==========================

The Light Monitor Server model is a vendor model, and therefore in the application, when defining the node composition data, it needs to be declared in the third argument in the :c:macro:`BT_MESH_ELEM` macro:

.. code-block:: c

    static struct bt_mesh_elem elements[] = {
        BT_MESH_ELEM(1,
            BT_MESH_MODEL_LIST(
                BT_MESH_MODEL_CFG_SRV(&cfg_srv),
                BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
            BT_MESH_MODEL_LIST(
                BT_MESH_MODEL_LIGHT_MONITOR(&monitor)),
    };

.. _bt_mesh_chat_client_model_messages:

Messages
========

The Light Monitor Server model defines the following messages:

sensor update
   Used to report the current sensor value

test result
   Used to report the result of a test

logged result
   Used to send a single logged result

test start
   Used to request a test start from the Client
   When the server receives a test ack request and there is no test running this is used. Needed since the test requires the timestamp of when it starts
   Has no payload


test ack
   Used to acknowledge that a test is running

calibrated ok
   Used to acknowledge that the sensor has been calibrated successfully




Presence
   Used to report the current model presence.
   When the model periodic publication is configured, the Chat Client model will publish its current presence, regardless of whether it has been changed or not.
   Presence message has a defined length of 1 byte.

Presence Get
   Used to retrieve the current model presence.
   Upon receiving the Presence Get message, the Chat Client model will send the Presence message with the current model presence stored in the response.
   The message doesn't have any payload.

Message
   Used to send a non-private text message.
   The payload consists of the text string terminated by ``\0``.
   The length of the text string can be configured at the compile-time using :ref:`CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH <CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH>` option.

Private Message
   Used to send a private text message.
   When the model receives this message, it replies with the Message Reply.
   The payload consists of the text string terminated by ``\0``.
   The length of the text string can be configured at the compile-time using :ref:`CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH <CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH>` option.

Message Reply
   Used to reply on the received Private Message to confirm the reception.
   The message doesn't have any payload.

Configuration
*************
|config|

Configuration options
=====================

The following configuration parameters are associated with the Chat Client model:

.. _CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH:

CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH - Message length configuration
   Maximum length of the message to be sent over the mesh network.

.. _bt_mesh_chat_client_model_states:

States
******

The Chat Client model contains 

Extended models
***************

None.

Persistent storage
******************

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Chat Client stores its presence state.
