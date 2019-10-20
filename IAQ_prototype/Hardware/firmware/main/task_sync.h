#ifndef _TASK_SYNC_H_
#define _TASK_SYNC_H_

typedef enum {
    acquisition_mode = 0,
    setup_mode,
} acquisition_mode_t;

/*Flags set by the caller, to inform the callee task that they should set their respective bit to notify the caller task*/
/*This is useful in situations where the calling task may or may not want to get the success of the callee task's signal operation*/
/*This is for non-top level tasks*/
#define NOTIFY_CALLER_WEBSERVER_STATUS_TASK_DONE ( 1UL << 0UL )
#define NOTIFY_CALLER_WEBSERVER_THERMAL_VIEWER_TASK_DONE ( 1UL << 1UL )
#define NOTIFY_CALLER_WEBSERVER_TAG_INFO_TASK_DONE ( 1UL << 2UL )

/*This is set by a caller to notify the task via an event group that it should complete its actions and end*/
#define NOTIFY_WEBSERVER_STATUS_TASK_CLOSE ( 1UL << 3UL )
#define NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_CLOSE ( 1UL << 4UL )
#define NOTIFY_WEBSERVER_TAG_INFO_TASK_CLOSE ( 1UL << 5UL )
#define NOTIFY_WEBSERVER_CLOSE ( 1UL << 6UL )

/*This is set by a callee to notify the caller task via an event group that it has completed its actions and ended*/
#define NOTIFY_BT_CALLBACK_DONE ( 1UL << 7UL )
#define NOTIFY_WEBSERVER_STATUS_TASK_DONE ( 1UL << 8UL )
#define NOTIFY_WEBSERVER_THERMAL_VIEWER_TASK_DONE ( 1UL << 9UL )
#define NOTIFY_WEBSERVER_TAG_INFO_TASK_DONE ( 1UL << 10UL )
#define NOTIFY_WEBSERVER_DONE ( 1UL << 11UL )

/*Notify the bluetooth task and any other tasks of importance that the acquisition window has closed*/
#define NOTIFY_ACQUISITION_WINDOW_DONE ( 1UL << 12UL )
#define CURRENT_MODE_ACQUISITION ( 1UL << 13UL )

#endif