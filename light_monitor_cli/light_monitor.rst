.. _bt_mesh_chat_client_model:

Light Monitor client
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

The Chat Client model is a vendor model, and therefore in the application, when defining the node composition data, it needs to be declared in the third argument in the :c:macro:`BT_MESH_ELEM` macro:

.. code-block:: c

    static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
					BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_MONITOR(&monitor))),
   };

.. _bt_mesh_chat_client_model_messages:

Messages
========

The Light Monitor Client Model defines the following Messages: 

 Set Light Test Start
      Used to Start a test.
      Set Light Test Start message has a payload of 6 Bytes

 Get Status
   Used to retrieve the current value of the sensor on a light monitor server 
   Get status does not have a payload

 Get Test result
   Used to retrieve the result of the most recent test, is called if no test result has been received from that node when a test is finished on the client
   Get test result does not have a payload
   
 Set Light Test Start Single
   Used to start the test on a single server, is called when get test ack fails
   Set Light Test Start Single has a payload of 6 Bytes

 Get Test Ack
   Used to check if a server has started the test, is called when no ack is received after set light test start
   get test ack has no payload

 Get Result Log
   Used to retrieve the log of stored results from a server
   get result log has no payload
   
 Calibrate Node
   Used to calibrate the threshold value for a test failure on a single server
   calibrate node has no payload


Configuration
*************
|config|

Configuration options
=====================

The following configuration parameters are associated with the Light Monitor Client model:

.. _CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH:

CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH - Message length configuration
   Maximum length of the message to be sent over the mesh network.

.. _bt_mesh_chat_client_model_states:

States
******

The Light Monitor Client model contains no States

Extended models
***************

None.

Persistent storage
******************

N/a