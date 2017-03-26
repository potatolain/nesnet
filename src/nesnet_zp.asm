; Zeropage symbols for nesnet library. Pop this into your ZEROPAGE segment where you like.

URL:	 			.res 2
RESPONSE:	 		.res 2
RESPONSE_LENGTH:	.res 2
MAX_LENGTH:			.res 2 ; TODO: This variable could be consolidated away with proper comparisons at the right time.
NET_TEMP:			.res 1
NET_BUFFER:			.res 3
