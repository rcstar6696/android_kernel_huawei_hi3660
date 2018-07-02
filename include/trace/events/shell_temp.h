#undef TRACE_SYSTEM
#define TRACE_SYSTEM shell_temp

#if !defined(_SHELL_TEMP_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SHELL_TEMP_H

#include <linux/tracepoint.h>

TRACE_EVENT(calc_shell_temp,	/* [false alarm]: native interface */
	TP_PROTO(int i, int j,
			int coef, int temp,
			long sum),
	TP_ARGS(i, j, coef, temp, sum),

	TP_STRUCT__entry(
		__field(int, i)
		__field(int, j)
		__field(int, coef)
		__field(int, temp)
		__field(long, sum)
	),

	TP_fast_assign(
		__entry->i = i;
		__entry->j = j;
		__entry->coef = coef;
		__entry->temp = temp;
		__entry->sum = sum;
	),

	TP_printk("sensor=%d time=%d coef=%d temp=%d sum=%ld",
				__entry->i, __entry->j, __entry->coef,
				__entry->temp, __entry->sum)
);

TRACE_EVENT(shell_temp,		/* [false alarm]: native interface */
	TP_PROTO(int temp),
	TP_ARGS(temp),

	TP_STRUCT__entry(
		__field(int, temp)
	),

	TP_fast_assign(
		__entry->temp = temp;
	),

	TP_printk("shell temp=%d", __entry->temp)
);
#endif

/* This part must be outside protection */
#include <trace/define_trace.h>
