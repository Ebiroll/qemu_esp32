add-symbol-file rom.elf 0x40000000

#################################################################################
# .gdbinit-FreeRTOS-helpers
#
# Created on: 29.03.2013
#     Author: Artem Pisarenko
# Modified: 30.12.2016
#     Author: Olof Astrand
#
# Helper script providing support for FreeRTOS-aware debugging
# Supported architectures: esp32
# Features:
# - showing tasks (handle and name);
#  To be implemented
# - switching context to specified task from almost any context (excluding system exceptions);
# - restore current running context.
#################################################################################
# Command "freertos_show_threads"
# Shows tasks table: handle(xTaskHandle) and name
define freertos_show_threads
	set $thread_list_size = 0
	set $thread_list_size = uxCurrentNumberOfTasks
	if ($thread_list_size == 0)
		echo FreeRTOS missing or scheduler isn't started\n
	else
		set $current_thread = pxCurrentTCB
		set $tasks_found = 0
		set $idx = 0

		set $task_list = pxReadyTasksLists
		set $task_list_size = sizeof(pxReadyTasksLists)/sizeof(pxReadyTasksLists[0])
		while ($idx < $task_list_size)
			_freertos_show_thread_item $task_list[$idx]
			set $idx = $idx + 1
		end

		_freertos_show_thread_item xDelayedTaskList1
		_freertos_show_thread_item xDelayedTaskList2
		_freertos_show_thread_item xPendingReadyList
		
		printf "--suspended--\n"
		#set $VAL_dbgFreeRTOSConfig_suspend = dbgFreeRTOSConfig_suspend_value
		#if ($VAL_dbgFreeRTOSConfig_suspend != 0)
		_freertos_show_thread_item xSuspendedTaskList
		#end

		printf "--terminating--\n"
		#set $VAL_dbgFreeRTOSConfig_delete = dbgFreeRTOSConfig_delete_value
		#if ($VAL_dbgFreeRTOSConfig_delete != 0)
		_freertos_show_thread_item xTasksWaitingTermination
		#end
	end
end



#######################
# Internal functions
define _freertos_show_thread_item
	set $list_thread_count = $arg0.uxNumberOfItems
	set $prev_list_elem_ptr = -1
	set $list_elem_ptr = $arg0.xListEnd.pxPrevious
	while (($list_thread_count > 0) && ($list_elem_ptr != 0) && ($list_elem_ptr != $prev_list_elem_ptr) && ($tasks_found < $thread_list_size))
		set $threadid = $list_elem_ptr->pvOwner
		set $thread_name_str = (*((tskTCB *)$threadid)).pcTaskName
		set $core_id = (*((tskTCB *)$threadid)).xCoreID
		set $stack = (*((tskTCB *)$threadid)).pxStack
		set $stack_top = (*((tskTCB *)$threadid)).pxTopOfStack
		set $tasks_found = $tasks_found + 1
		set $list_thread_count = $list_thread_count - 1
		set $prev_list_elem_ptr = $list_elem_ptr
		set $list_elem_ptr = $prev_list_elem_ptr->pxPrevious
		if ($threadid == $current_thread)
			printf "0x%x\t%s\t0x%x\t0x%x\t0x%x\t%d<---RUNNING\n", $threadid, $thread_name_str, $core_id, $stack, $stack_top ,$stack_top-$stack 
		else
			printf "0x%x\t%s\t0x%x\t0x%x\t0x%x\t%d\n", $threadid, $thread_name_str, $core_id, $stack, $stack_top , $stack_top-$stack 
		end
		if (*(unsigned int*)((*((tskTCB *)$threadid)).pxTopOfStack+4)>0x4000 & (*(unsigned int*)((*((tskTCB *)$threadid)).pxTopOfStack+4)<0x40C00000))
		        x/3i *(unsigned int*) ((*((tskTCB *)$threadid)).pxTopOfStack+4)
		else
		       p/x *(unsigned int*) ((*((tskTCB *)$threadid)).pxTopOfStack+4)
	end
end
