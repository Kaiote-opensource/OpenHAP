#ifndef _TASK_SYNC_H_
#define _TASK_SYNC_H_

/*This is set by a caller to notify the task via an event group that it should complete its actions and end*/
#define NOTIFY_WEBSERVER_STATUS_TASK_CLOSE ( 1UL << 0UL )
#define NOTIFY_WEBSERVER_PARTICULATE_MATTER_TASK_CLOSE ( 1UL << 1UL )
#define NOTIFY_WEBSERVER_GPIO_INTERRUPT_TASK_CLOSE ( 1UL << 2UL )
#define NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_CLOSE ( 1UL << 3UL )
#define NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE ( 1UL << 4UL )
#define NOTIFY_BT_CALLBACK_DONE ( 1UL << 5UL )
#endif