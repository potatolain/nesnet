; Zeropage symbols for nesnet library. Pop this into your ZEROPAGE segment where you like.

; TODO: Names here are super generic. Prefix with HTTP?
; TODO: Can we eliminate any of these? Or at least move out of zp? This is kinda a lot to ask...
URL:	 					.res 2
HTTP_DATA:					.res 2
HTTP_DATA_LENGTH:			.res 2
RESPONSE:	 				.res 2
RESPONSE_LENGTH:			.res 2
MAX_LENGTH:					.res 2 ; TODO: This variable could be consolidated away with proper comparisons at the right time.
NET_TEMP:					.res 1
NET_REQUEST_IN_PROGRESS:	.res 1
NET_BUFFER:					.res 3
NET_P1_BUFFER:				.res 3
NET_P1_BUFFER_POS:			.res 1
NET_RESPONSE_CODE: 			.res 2
NET_CURRENT_STATE:			.res 1
NET_REQUEST_BYTE_NUM:		.res 2