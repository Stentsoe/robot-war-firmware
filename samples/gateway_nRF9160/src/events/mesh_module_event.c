
#include "mesh_module_event.h"

static void profile_mesh_module_event(struct log_event_buf *buf,
                                      const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(mesh_module_event,
                      ENCODE(),
                      ENCODE(),
                      profile_mesh_module_event);

static char *type_to_str(enum mesh_module_event_type type)
{
    switch (type)
    {
    case MESH_EVT_READY:
        return "MESH_EVT_READY";
    case MESH_EVT_ROBOT_ADDED:
        return "MESH_EVT_ROBOT_ADDED";
    case MESH_EVT_OP_STATUS:
        return "MESH_EVT_OP_STATUS";
    case MESH_EVT_MOVEMENT_REPORTED:
        return "MESH_EVT_MOVEMENT_REPORTED";
    default:
        return "UNKNOWN";
    }
}

static void log_mesh_module_evt(const struct app_event_header *evt)
{

    struct mesh_module_event *mesh_module_evt = cast_mesh_module_event(evt);

    APP_EVENT_MANAGER_LOG(evt, "Type: %s", type_to_str(mesh_module_evt->type));
}

APP_EVENT_TYPE_DEFINE(mesh_module_event,
                      log_mesh_module_evt,
                      &mesh_module_event_info,
                      APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
