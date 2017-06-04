; This file defines default configuration for NESNet. 
; You can copy it to your project folder and customize, or you can just link to the default configuration in the library.

; The number of requests bytes to get in one nmi. Default is 2. 
; Increase it to get faster responses, drop it if you need to get back some CPU time.

; Up the number of bytes per call for nesnet, so that we get responses faster. We don't do much during nmi here anyway.
.define NESNET_BYTES_PER_CALL 5