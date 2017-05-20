;
; NESNet
; Send http requests from your NES games! 
; Licensed under the MIT license.
;

.feature c_comments
.import popa,popax,pusha, pushax
.import _nesnet_buffer

; Some constant you probably don't want to play with here. They aren't particularly clear in what they do w/o looking at code.
; If you know what you're doing, have at!
.define INCOMING_BYTE_DELAY 50 ; How long do we wait between incoming bytes?
.define OUTGOING_BIT_DELAY 100 ; How much delay do we put between each bit we send out in order to tell the fake "controller" 1/0?
.define NESNET_RESPONSE_WAIT_TIME 255 ; How long do we wait for a response from the controller before giving up?

.define HTTP_GET 		'G'
.define HTTP_PUT 		'E'
.define HTTP_POST		'A'
.define HTTP_DELETE		'C'

.define NET_STATE_IDLE					0
.define NET_STATE_GET_INIT 				1
.define NET_STATE_GET_HANDSHAKE 		2
.define NET_STATE_GET_SENDING_METHOD	3
.define NET_STATE_GET_SENDING_URL	 	4
.define NET_STATE_GET_WAITING			5
.define NET_STATE_GET_RES_CODE			6
.define NET_STATE_GET_RES_LENGTH		7
.define NET_STATE_GET_RECEIVING_BYTES	8
.define NET_STATE_GET_DONE				9


; Note: Since ca65 is kind of a pain, all exports are at the bottom. 
; If you really want to see what you can do though, the details should all be in nesnet.h :)

; TODO: macros cannot be placed in scopes - find a way to make these not risk clashing with someone else's stuff
; Maybe as simple as HttpLib_phy and HttpLib_ply, etc. Would be nice if there were a cleaner way though.
.macro phy
	sta NET_TEMP
	tya
	pha
	lda NET_TEMP
.endmacro
.macro ply
	sta NET_TEMP
	pla
	tay
	lda NET_TEMP
.endmacro
.macro phx
	sta NET_TEMP
	txa
	pha
	lda NET_TEMP
.endmacro
.macro plx
	sta NET_TEMP
	pla
	tax
	lda NET_TEMP
.endmacro

.macro trigger_latch
	lda #1
	sta $4016
	lda #0
	sta $4016
.endmacro

.macro http_get_style_request METHOD
	pha
	lda #1
	sta NET_REQUEST_IN_PROGRESS
	lda #NET_STATE_GET_INIT
	sta NET_CURRENT_STATE
	pla
	sta MAX_LENGTH
	stx MAX_LENGTH+1

	jsr popax
	sta RESPONSE
	stx RESPONSE+1

	jsr popax
	clc
	adc #7 ; Add 7 to skip http://, since http is the only supported protocol for now.
	sta URL
	txa
	adc #0
	sta URL+1
	rts

.endmacro

.macro http_post_style_request METHOD
	pha
	lda #1
	sta NET_REQUEST_IN_PROGRESS
	pla

	sta MAX_LENGTH
	stx MAX_LENGTH+1

	jsr popax
	sta RESPONSE
	stx RESPONSE+1

	jsr popax
	sta HTTP_DATA_LENGTH
	stx HTTP_DATA_LENGTH+1

	jsr popax
	sta HTTP_DATA
	stx HTTP_DATA+1

	jsr popax
	clc
	adc #7 ; Add 7 to skip http://, since http is the only supported protocol for now.
	sta URL
	txa
	adc #0
	sta URL+1
	rts
/*

	jsr do_handshake
	
	jsr NESNET_WAIT_NMI

	lda #METHOD
	jsr send_byte_to_nes
	jsr NESNET_WAIT_NMI

	; Okay, time to send some data over.
	ldy #0
	@string_loop:
		lda (URL), y
		cmp #0
		beq @break_loop

		jsr send_byte_to_nes
		iny
		tya
		and #%00000001
		cmp #0
		bne @skip_waiting
			jsr NESNET_WAIT_NMI
		@skip_waiting:
		cpy #254
		bne @string_loop ; TODO: Allow for more than 255 chars. Also include nmi wait logic when this happens.
	@break_loop:

	; Finally, send a 0 byte
	lda #0
	jsr send_byte_to_nes

	jsr NESNET_WAIT_NMI
	
	; Okay... next, we need to tell it about some of our data
	lda HTTP_DATA_LENGTH
	jsr send_byte_to_nes

	jsr NESNET_WAIT_NMI
	lda HTTP_DATA_LENGTH+1
	jsr send_byte_to_nes
	; Here we go!
	ldy #0
	@data_loop:
		lda (HTTP_DATA), y
		jsr send_byte_to_nes
		jsr NESNET_WAIT_NMI

		iny
		cpy #0
		bne @no_loopy
			inc HTTP_DATA+1
		@no_loopy:
		dec HTTP_DATA_LENGTH
		lda HTTP_DATA_LENGTH
		cmp #255
		bne @not_dec
			dec HTTP_DATA_LENGTH+1
			lda HTTP_DATA_LENGTH+1
			cmp #255
			beq @after_data
		@not_dec:
		jmp @data_loop
	@after_data:

	jsr get_nes_response
	
	pha
	lda #0
	sta NET_REQUEST_IN_PROGRESS
	pla

*/
	rts
.endmacro

.scope HttpLib ; Use a separate scope to encapsulate some labels/etc we use.

	byte_to_bit_lookup: 
		.byte %00000001
		.byte %00000010
		.byte %00000100
		.byte %00001000
		.byte %00010000
		.byte %00100000
		.byte %01000000
		.byte %10000000

	test_url: 
		.asciiz "http:///test"

	get: 
		http_get_style_request HTTP_GET

	delete:
		http_get_style_request HTTP_DELETE

	post: 
		http_post_style_request HTTP_POST

	put:
		http_post_style_request HTTP_PUT	

	request_complete:
		; Need to invert the first bit
		lda NET_REQUEST_IN_PROGRESS
		eor #%00000001 ; Flip it good
		rts

	response_code:
		lda NET_RESPONSE_CODE
		ldx NET_RESPONSE_CODE+1
		rts


	test_connection:
		; FIXME: Need a better way to determine this. How do we do this with the new setup?
		; Maybe something to trigger warmup, then another one that checks warmup?
		lda #1
		rts
		lda #<(test_url)
		ldx #>(test_url)
		jsr pushax
		lda #<(_nesnet_buffer)
		ldx #>(_nesnet_buffer)
		jsr pushax

		; Desired length is length of buffer... 4 whole bytes.
		lda #4
		ldx #0

		jsr get
		
		lda _nesnet_buffer
		cmp #'T'
		bne @bad_end
		lda _nesnet_buffer+1
		cmp #'E'
		bne @bad_end
		lda _nesnet_buffer+2
		cmp #'S'
		bne @bad_end
		lda _nesnet_buffer+3
		cmp #'T'
		bne @bad_end
		@happy_end: 
			lda #1
			ldx #0
			rts
		@bad_end:
			lda #0
			ldx #0
			rts

	send_byte_to_nes: 
			sta _nesnet_buffer+19
			ldx #0
			@byte_loop: ; TODO: loop this 3 times, like we do in the other direction?

				lda _nesnet_buffer+19
				and byte_to_bit_lookup, x ; Get the bit we're looking for from a simple lookup table

/* 2   */		cmp #0
/* 2   */		beq @zero 			; Timing note: adds 1-2 if it jumps to zero.
/* 12  */			trigger_latch ; Putting these right next to eachother makes it easier to determine what is/isn't a match.
/* 12  */			trigger_latch
/* 3   */			jmp @after_data
				@zero: 
/* 1-2 */			; From branch
/* 12 */			trigger_latch
/* 2 */				nop
/* 2 */				nop
/* 2 */				nop
/* 2 */				nop
/* 2 */				nop
/* 2 */				nop
/* 3 */				jmp @after_data ; Keeping things in sync cycles-wise.
				@after_data:
				phx
				ldx #0
				@loop:
					nop
					inx
					cpx #OUTGOING_BIT_DELAY
					bne @loop
				plx
				inx
				cpx #8
				bne @byte_loop
				; Okay, really brief... let's copy the latest latch into the p1 buffer
				@padPollPort:
					ldx NET_P1_BUFFER_POS

					lda #8
					sta <NET_TEMP

				@padPollLoop:

					lda CTRL_PORT1
					lsr a
					ror NET_P1_BUFFER, x
					dec <NET_TEMP
					bne @padPollLoop
					
					inx
					cpx #3
					bne @no_rot
						ldx #0
					@no_rot:
					stx NET_P1_BUFFER_POS

				
			rts

; FIXME: Baleeted
	get_nes_response:
		ldx #0
		ldy #0
		@loop_zero:
			jsr get_pad_values_no_retry ; Wait until we start seeing real bytes flow in
			cmp #0
			bne @escape_zero
			inx
			jsr NESNET_WAIT_NMI ; Play music n stuff while we wait...
			cpx #NESNET_RESPONSE_WAIT_TIME
			bne @loop_zero
			jmp @get_complete_failure ; If we get to this point, it's never responding...

		@escape_zero:


		; Ignore the first char all 3 times it shows up.
		jsr get_pad_values_no_retry
		jsr get_pad_values_no_retry ; Done ignoring...

		; Ignore the next two chars too 
		; TODO: Determine why this helps.
		jsr get_pad_values
		jsr get_pad_values
		

		jsr get_pad_values
		; Read status code for later user.
		sta NET_RESPONSE_CODE
		jsr get_pad_values
		sta NET_RESPONSE_CODE+1
		dec NET_RESPONSE_CODE+1

		jsr get_pad_values
		sta RESPONSE_LENGTH
		jsr get_pad_values
		sta RESPONSE_LENGTH+1
		dec RESPONSE_LENGTH+1
		jsr NESNET_WAIT_NMI

		; TODO: Can we do something smart with RESPONSE_LENGTH and MAX_LENGTH to save 2 bytes in zp?
		lda RESPONSE_LENGTH+1
		cmp MAX_LENGTH+1
		bcc @use_response_length
		bne @dont_use_response_length
			; Okay, they're equal... compare lo byte
			lda RESPONSE_LENGTH
			cmp MAX_LENGTH
			bcc @use_response_length
			; Else, use max length; fallthru.
		@dont_use_response_length:
			lda MAX_LENGTH+1
			sta RESPONSE_LENGTH+1
			lda MAX_LENGTH
			sta RESPONSE_LENGTH
		@use_response_length:

		jsr NESNET_WAIT_NMI

		ldy #0

		@loop: 
			jsr get_pad_values
			sta (RESPONSE), y
			dec RESPONSE_LENGTH
			lda RESPONSE_LENGTH
			and #%00000001
			cmp #0
			bne @no_sleep
			@sleep:
				jsr NESNET_WAIT_NMI
			@no_sleep:
			lda RESPONSE_LENGTH
			cmp #255
			bne @no_zeroing_response
				; Okay, we went over 255 bytes. 
				
				; Did we go over the full length?
				dec RESPONSE_LENGTH+1
				lda RESPONSE_LENGTH+1
				cmp #255
				beq @after_data ; Else just kinda carry on...
			@no_zeroing_response:
			iny
			cpy #0
			bne @loop
			inc RESPONSE+1
			jmp @loop

		@after_data:
		lda #0
		sta (RESPONSE), y ; null terminate the string...

		rts
	; Part of the above method... if all goes wrong, tell us.
	@get_complete_failure:
		lda #0
		sta (RESPONSE), y
		lda #<(599)
		sta NET_RESPONSE_CODE
		lda #>(599)
		sta NET_RESPONSE_CODE+1
		rts

	do_handshake:
		inc NET_RESPONSE_CODE
		lda NET_RESPONSE_CODE
		cmp #5 ; 1-4 are just noops while we wait 
		bcc @done
		beq @do_latch
		cmp #11 ; more noops.
		bcc @done

		; Okay you made it.. next step.
		inc NET_CURRENT_STATE
		lda #0
		sta NET_REQUEST_BYTE_NUM
		sta NET_REQUEST_BYTE_NUM+1

		@done:
			rts

		@do_latch:
		; TODO: Make a real handshake so we stop triggering the stupid powerpak
			.repeat 8
				trigger_latch
			.endrepeat
			rts

	

	do_send_url:
		ldy NET_REQUEST_BYTE_NUM
	
		lda (URL), y
		jsr send_byte_to_nes
		lda (URL), y ; Since send_byte is destructive.
		cmp #0
		beq @end_of_url ; This ensures we send the null terminator too. (Photon firmware expects this)
		cpy #254
		beq @end_of_url ; TODO: Allow for more than 255 chars.

		iny
		sty NET_REQUEST_BYTE_NUM
		rts

		@end_of_url:
			lda #0
			sta NET_REQUEST_BYTE_NUM
			sta NET_REQUEST_BYTE_NUM+1 ; This is probably a noop, but it doesn't hurt. Might make for less bugs if we allow longer urls.
			; Also set response code to 0 to use as a temporary counter for the next step
			sta NET_RESPONSE_CODE
			sta NET_RESPONSE_CODE+1
			inc NET_CURRENT_STATE
			rts

	do_get_waiting:
		jsr get_pad_values_no_retry ; Wait until we start seeing real bytes flow in
		cmp #0
		bne @escape_zero
		inc NET_RESPONSE_CODE
		lda NET_RESPONSE_CODE
		cmp #NESNET_RESPONSE_WAIT_TIME
		beq @complete_failure ; If you get here, we've waited too many nmi cycles without a response. Fail and call it done.
		rts ; You didn't get a response and you didn't get a byte. Just move on.

		@escape_zero: ; Okay, we got a non-zero byte. Carry on...

			; If you see this, 
			; Ignore the first char all 3 times it shows up.
			jsr get_pad_values_no_retry
			jsr get_pad_values_no_retry ; Done ignoring...

			; Ignore the next two chars too 
			; TODO: Determine why this helps.
			jsr get_pad_values
			jsr get_pad_values

			; And move onto next step.
			inc NET_CURRENT_STATE
			rts
		@complete_failure:
			lda #0
			sta (RESPONSE), y
			lda #<(599)
			sta NET_RESPONSE_CODE
			lda #>(599)
			sta NET_RESPONSE_CODE+1
			rts

	do_get_res_code:

		jsr get_pad_values
		; Read status code for later user.
		sta NET_RESPONSE_CODE
		jsr get_pad_values
		sta NET_RESPONSE_CODE+1
		dec NET_RESPONSE_CODE+1
		inc NET_CURRENT_STATE
		rts

	do_get_res_len:
		jsr get_pad_values
		sta RESPONSE_LENGTH
		jsr get_pad_values
		sta RESPONSE_LENGTH+1
		dec RESPONSE_LENGTH+1
		jsr NESNET_WAIT_NMI

		; TODO: Can we do something smart with RESPONSE_LENGTH and MAX_LENGTH to save 2 bytes in zp?
		lda RESPONSE_LENGTH+1
		cmp MAX_LENGTH+1
		bcc @use_response_length
		bne @dont_use_response_length
			; Okay, they're equal... compare lo byte
			lda RESPONSE_LENGTH
			cmp MAX_LENGTH
			bcc @use_response_length
			; Else, use max length; fallthru.
		@dont_use_response_length:
			lda MAX_LENGTH+1
			sta RESPONSE_LENGTH+1
			lda MAX_LENGTH
			sta RESPONSE_LENGTH
		@use_response_length:
		inc NET_CURRENT_STATE

		rts

	get_response:
		ldy #0
		jsr get_pad_values
		sta (RESPONSE), y
		dec RESPONSE_LENGTH
		lda RESPONSE_LENGTH
		cmp #255
		bne @no_zeroing_response
			; Okay, we went over 255 bytes. 
			
			; Did we go over the full length?
			dec RESPONSE_LENGTH+1
			lda RESPONSE_LENGTH+1
			cmp #255
			beq @after_data ; Else just kinda carry on...
		@no_zeroing_response:
		inc RESPONSE
		lda RESPONSE
		cmp #0
		bne @done
			inc RESPONSE+1
		jmp @done

		@after_data:
		inc NET_CURRENT_STATE
		iny
		lda #0
		sta (RESPONSE), y ; null terminate the string...
		@done:
			rts

	pad_poll:
		lda NET_REQUEST_IN_PROGRESS
		cmp #1
		beq @connected
			; If you're not connected, we need to do a full request
			jsr get_pad_values
		@connected:
		; If a request is in progress, use the last known result

			lda <NET_P1_BUFFER
			cmp <NET_P1_BUFFER+1
			beq @done
			cmp <NET_P1_BUFFER+2
			beq @done
			lda <NET_P1_BUFFER+1
		@done:

		rts

	do_cycle: 
		lda NET_CURRENT_STATE
		cmp #NET_STATE_IDLE
		bne @not_idle
			rts ; Idle, you say? I'm on it, coach!
		@not_idle:
		cmp #NET_STATE_GET_INIT ; Starting a get request? Okay.. 
		bne @not_get_init
			; Starting to do a get request... track the progress of that using NET_RESPONSE_CODE, since that won't be set yet.
			lda #0
			sta NET_RESPONSE_CODE
			inc NET_CURRENT_STATE
			rts
		@not_get_init:
		cmp #NET_STATE_GET_HANDSHAKE
		bne @not_get_handshake
			jsr do_handshake
			rts
		@not_get_handshake:
		cmp #NET_STATE_GET_SENDING_METHOD
		bne @not_sending_method
			lda #HTTP_GET
			jsr send_byte_to_nes
			inc NET_CURRENT_STATE
			rts
		@not_sending_method:
		cmp #NET_STATE_GET_SENDING_URL
		bne @not_get_url
			jsr do_send_url
			rts
		@not_get_url:
		cmp #NET_STATE_GET_WAITING
		bne @not_waiting
			jsr do_get_waiting
			rts
		@not_waiting:
		cmp #NET_STATE_GET_RES_CODE
		bne @not_res_code
			jsr do_get_res_code
			rts
		@not_res_code:
		cmp #NET_STATE_GET_RES_LENGTH
		bne @not_res_length
			jsr do_get_res_len
			rts
		@not_res_length:
		cmp #NET_STATE_GET_RECEIVING_BYTES
		bne @not_bytes
			jsr get_response
			rts
		@not_bytes:
		; If we don't have a known one (done requests intentionally left out) let's assume the request is done.
		lda #0
		sta NET_REQUEST_IN_PROGRESS
		rts

.endscope
get_pad_values: 
	ldx #0

@padPollPort:

	; Forcibly space out the latch requests a little to reduce the likelihood the photon will get a latch out of order
	tya
	pha
	ldy #0
	@loop_delay:
		nop
		iny
		cpy #INCOMING_BYTE_DELAY
		bne @loop_delay
	pla
	tay

	lda #1
	sta CTRL_PORT1
	lda #0
	sta CTRL_PORT1
	lda #8
	sta <NET_TEMP

@padPollLoop:

	lda CTRL_PORT1
	lsr a
	ror <NET_P1_BUFFER, x

	lda CTRL_PORT2
	lsr a
	ror <NET_BUFFER, x
	dec <NET_TEMP
	bne @padPollLoop


	inx
	cpx #3
	bne @padPollPort

	lda <NET_BUFFER
	cmp <NET_BUFFER+1
	beq @done
	cmp <NET_BUFFER+2
	beq @done
	lda <NET_BUFFER+1

@done:

	rts

get_pad_values_no_retry: 
	; Forcibly space out the latch requests a little to reduce the likelihood the photon will get a latch out of order
	; TODO: Is this still needed after the async refactor?
	txa
	pha
	tya
	pha

	ldy #0
	@loop_delay:
		nop
		iny
		cpy #INCOMING_BYTE_DELAY
		bne @loop_delay


@padPollPort:
	ldx NET_P1_BUFFER_POS

	lda #1
	sta CTRL_PORT1
	lda #0
	sta CTRL_PORT1
	lda #8
	sta <NET_TEMP

@padPollLoop:

	lda CTRL_PORT2
	lsr a
	ror <NET_BUFFER
	lda CTRL_PORT1
	lsr a
	ror NET_P1_BUFFER, x
	dec <NET_TEMP
	bne @padPollLoop
	
	inx
	cpx #3
	bne @no_rot
		ldx #0
	@no_rot:
	stx NET_P1_BUFFER_POS


	pla
	tay
	pla
	tax
	lda NET_BUFFER
	rts


.export _http_get = HttpLib::get
.export _http_post = HttpLib::post
.export _http_put = HttpLib::put
.export _http_delete = HttpLib::delete
.export _nesnet_check_connected = HttpLib::test_connection
.export _nesnet_pad_poll = HttpLib::pad_poll
.export _http_request_complete = HttpLib::request_complete
.export _http_response_code = HttpLib::response_code
.export _nesnet_do_cycle = HttpLib::do_cycle