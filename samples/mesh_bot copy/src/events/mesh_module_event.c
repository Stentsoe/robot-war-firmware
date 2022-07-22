
#include "mesh_module_event.h"

static char *type_to_str(mesh_module_event_type type)
{
    switch (type)
    {
        case MESH_EVT_PROVISIONED: {
            return "MESH_EVT_PROVISIONED";
        }
        case MESH_EVT_DISCONNECTED: {
            return "MESH_EVT_DISCONNECTED";
        }
        case MESH_EVT_MOVEMENT_RECEIVED: {
            return "MESH_EVT_MOVEMENT_RECEIVED";
        }
        case MESH_EVT_CLEAR_TO_MOVE_RECEIVED: {
            return "MESH_EVT_CLEAR_TO_MOVE_RECEIVED";
        }
    default:
        return "UNKNOWN";
    }
}

static void log_mesh_event(const struct app_event_header *header)
{
    struct mesh_module_event *evt = cast_mesh_module_event(header);
    char *type_str = type_to_str(evt->type);

    APP_EVENT_MANAGER_LOG(header, "Type: %s", type_str);
}

APP_EVENT_TYPE_DEFINE(
    mesh_module_event,
    log_mesh_event,
    NULL,
    APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_LOG_MESH_MODULE_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));