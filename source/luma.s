.global lumaFlushDataCacheRange
.global lumaInvalidateInstructionCacheRange

.section .text

.type lumaFlushDataCacheRange, %function
lumaFlushDataCacheRange:
	svc 0x91
	bx lr

.type lumaInvalidateInstructionCacheRange, %function
lumaInvalidateInstructionCacheRange:
	svc 0x93
	bx lr
