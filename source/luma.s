.global lumaFlushDataCacheRange
.global lumaInvalidateInstructionCacheRange

.section .text

.type lumaFlushDataCacheRange, %function
lumaFlushDataCacheRange:
	svc 0x92
	bx lr

.type lumaInvalidateInstructionCacheRange, %function
lumaInvalidateInstructionCacheRange:
	svc 0x94
	bx lr
