#pragma once
#include <stdint.h>

enum peripheral_msg_type
{
    HELLO=0x00,
    SET_MOVEMENT_CONFIG=0x01,
    SET_LIGHT_CONFIG=0x02,
    ROBOT_ADDED=0x03,
    CONFIG_ACK=0x04,
    MOVEMENT_REPORTED=0x05,
    CLEAR_TO_MOVE=0x06,
};

struct mesh_uart_config_ack_data
{
    int32_t seq_num;
}__packed;

struct mesh_uart_robot_added_data
{
    uint64_t addr;
}__packed;

struct mesh_uart_movement_reported_data
{
    uint64_t addr;
    int32_t x;
    int32_t y;
    int32_t yaw;
}__packed;

struct mesh_uart_msg_header
{
    enum peripheral_msg_type type;
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

struct mesh_uart_config_ack_msg
{
    struct mesh_uart_msg_header header;
    struct mesh_uart_config_ack_data data;
}__packed;

struct mesh_uart_movement_reported_msg
{
    struct mesh_uart_msg_header header;
    struct mesh_uart_movement_reported_data data;
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
    struct mesh_uart_config_ack_msg config_ack;
    struct mesh_uart_movement_reported_msg movement_reported;
    struct mesh_uart_clear_to_move_msg clear_to_move;
};