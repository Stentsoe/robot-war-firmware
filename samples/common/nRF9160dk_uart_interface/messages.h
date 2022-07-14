#pragma once
#include <stdint.h>

enum mesh_uart_msg_type
{
    HELLO=0x00, // Handshake message.
    SET_MOVEMENT_CONFIG=0x01, // Set robot movement configuration.
    SET_LIGHT_CONFIG=0x02, // Set robot light configuration.
    ROBOT_ADDED=0x03, // Robot added to the network.
    STATUS=0x04, // Status of previous command.
    MOVEMENT_REPORTED=0x05, // Movement report from robot.
    CLEAR_TO_MOVE=0x06, // Robots ready to move.
};

struct mesh_uart_status_data
{
    int16_t status;
}__packed;

struct mesh_uart_robot_added_data
{
    uint8_t mac_address[6]; // 48 bit MAC address.
    uint16_t mesh_address; // 16 bit address on the mesh network.
}__packed;

struct mesh_uart_movement_reported_data
{
    uint16_t addr; // Mesh network address
    int32_t x;
    int32_t y;
    int32_t yaw;
}__packed;

struct mesh_uart_set_movement_config_data
{
    uint16_t addr;
    uint32_t time;
    int32_t angle;
}__packed;

struct mesh_uart_msg_header
{
    enum mesh_uart_msg_type type;
}__packed;

struct mesh_uart_hello_msg
{
    struct mesh_uart_msg_header header;
    uint16_t echo;
}__packed;

struct mesh_uart_robot_added_msg
{
    struct mesh_uart_msg_header header;
    struct mesh_uart_robot_added_data data;
}__packed;

struct mesh_uart_status_msg
{
    struct mesh_uart_msg_header header;
    struct mesh_uart_status_data data;
}__packed;

struct mesh_uart_movement_reported_msg
{
    struct mesh_uart_msg_header header;
    struct mesh_uart_movement_reported_data data;
}__packed;

struct mesh_uart_set_movement_config_msg
{
    struct mesh_uart_msg_header header;
    struct mesh_uart_set_movement_config_data data;
}__packed;

struct mesh_uart_clear_to_move_msg
{
    struct mesh_uart_msg_header header;
}__packed;

union mesh_uart_msg
{
    struct mesh_uart_msg_header header;
    struct mesh_uart_hello_msg hello;
    struct mesh_uart_robot_added_msg robot_added;
    struct mesh_uart_status_msg status;
    struct mesh_uart_movement_reported_msg movement_reported;
    struct mesh_uart_set_movement_config_msg set_movement_config;
    struct mesh_uart_clear_to_move_msg clear_to_move;
};