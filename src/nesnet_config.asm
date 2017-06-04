; This file defines default configuration for NESNet. 
; You can copy it to your project folder and customize, or you can just link to the default configuration in the library.

; The number of requests bytes to get in one nmi. Default is 2. 
; Increase it to get faster responses, drop it if you need to get back some CPU time.
.define NESNET_BYTES_PER_CALL 2