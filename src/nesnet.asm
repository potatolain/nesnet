;
; NESNet
; Send http requests from your NES games! 
; Licensed under the MIT license.
;

.feature c_comments
.import popa,popax,pusha, pushax
.import _nesnet_buffer

; Some constants you probably don't want to play with here. They aren't particularly clear in what they do w/o looking at code.
; If you know what you're doing, have at!
.define INCOMING_BYTE_DELAY 50 ; How long do we wait between incoming bytes?
.define OUTGOING_BIT_DELAY 100 ; How much delay do we put between each bit we send out in order to tell the fake "controller" 1/0?
.define NESNET_RESPONSE_WAIT_TIME 255 ; How long do we wait for a response from the controller before giving up?

; Character prefix for each request type. 
.define HTTP_GET 		'G'
.define HTTP_PUT 		'E'
.define HTTP_POST		'A'
.define HTTP_DELETE		'C'

.define NET_STATE_IDLE						0

.define NET_STATE_GET_INIT 					1
.define NET_STATE_GET_HANDSHAKE 			2
.define NET_STATE_GET_SENDING_METHOD		3
.define NET_STATE_GET_SENDING_URL	 		4
.define NET_STATE_GET_WAITING				5
.define NET_STATE_GET_RES_CODE				6
.define NET_STATE_GET_RES_LENGTH			7
.define NET_STATE_GET_RECEIVING_BYTES		8
.define NET_STATE_GET_DONE					9

.define NET_STATE_DELETE_INIT 				51
.define NET_STATE_DELETE_HANDSHAKE 			52
.define NET_STATE_DELETE_SENDING_METHOD		53
.define NET_STATE_DELETE_SENDING_URL	 	54
.define NET_STATE_DELETE_WAITING			55
.define NET_STATE_DELETE_RES_CODE			56
.define NET_STATE_DELETE_RES_LENGTH			57
.define NET_STATE_DELETE_RECEIVING_BYTES	58
.define NET_STATE_DELETE_DONE				59

.define NET_STATE_POST_INIT 				101
.define NET_STATE_POST_HANDSHAKE 			102
.define NET_STATE_POST_SENDING_METHOD		103
.define NET_STATE_POST_SENDING_URL	 		104
.define NET_STATE_POST_SENDING_DATA_LEN		105
.define NET_STATE_POST_SENDING_DATA			106
.define NET_STATE_POST_WAITING				107
.define NET_STATE_POST_RES_CODE				108
.define NET_STATE_POST_RES_LENGTH			109
.define NET_STATE_POST_RECEIVING_BYTES		110
.define NET_STATE_POST_DONE					111

.define NET_STATE_PUT_INIT 					151
.define NET_STATE_PUT_HANDSHAKE 			152
.define NET_STATE_PUT_SENDING_METHOD		153
.define NET_STATE_PUT_SENDING_URL	 		154
.define NET_STATE_PUT_SENDING_DATA_LEN		155
.define NET_STATE_PUT_SENDING_DATA			156
.define NET_STATE_PUT_WAITING				157
.define NET_STATE_PUT_RES_CODE				158
.define NET_STATE_PUT_RES_LENGTH			159
.define NET_STATE_PUT_RECEIVING_BYTES		160
.define NET_STATE_PUT_DONE					161


; Note: Since ca65 is kind of a pain, all exports are at the bottom. 
; If you really want to see what you can do though, the details should all be in nesnet.h :)

.macro nesnet_phx
	sta NET_TEMP
	txa
	pha
	lda NET_TEMP
.endmacro
.macro nesnet_plx
	sta NET_TEMP
	pla
	tax
	lda NET_TEMP
.endmacro

.macro nesnet_trigger_latch
	lda #1
	sta $4016
	lda #0
	sta $4016
.endmacro

.macro http_get_style_request METHOD
	pha
	lda #1
	sta NET_REQUEST_IN_PROGRESS
	lda #METHOD
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
	lda #METHOD
	sta NET_CURRENT_STATE
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
		http_get_style_request NET_STATE_GET_INIT

	delete:
		http_get_style_request NET_STATE_DELETE_INIT

	post: 
		http_post_style_request NET_STATE_POST_INIT

	put:
		http_post_style_request NET_STATE_PUT_INIT

	request_complete:
		; Need to invert the first bit
		lda NET_REQUEST_IN_PROGRESS
		eor #%00000001 ; Flip it good
		rts

	response_code:
		lda NET_RESPONSE_CODE
		ldx NET_RESPONSE_CODE+1
		rts


	; Test if your connection is ready. This actually hides a lot from you. Just call it until it's happy.
	test_connection:
		lda _nesnet_buffer+18
		cmp #1
		beq @happy_end

		lda NET_REQUEST_IN_PROGRESS
		cmp #1
		bne @not_in_progress
			jsr do_cycle
			jmp @bad_end
		@not_in_progress:

		; Request not in progress? If it looks like we have a valid response, test...
		lda _nesnet_buffer
		cmp #'T'
		beq @test_request

		; Else, kick off a test.
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
		jmp @bad_end
		
		@test_request:
		lda _nesnet_buffer+1
		cmp #'E'
		bne @bad_end
		lda _nesnet_buffer+2
		cmp #'S'
		bne @bad_end
		lda _nesnet_buffer+3
		cmp #'T'
		bne @bad_end
		@happier_end:
			lda #1
			sta _nesnet_buffer+18
			; Fall through to happy end
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
			@byte_loop:

				lda _nesnet_buffer+19
				and byte_to_bit_lookup, x ; Get the bit we're looking for from a simple lookup table

/* 2   */		cmp #0
/* 2   */		beq @zero 			; Timing note: adds 1-2 if it jumps to zero.
/* 12  */			nesnet_trigger_latch ; Putting these right next to eachother makes it easier to determine what is/isn't a match.
/* 12  */			nesnet_trigger_latch
/* 3   */			jmp @after_data
				@zero: 
/* 1-2 */			; From branch
/* 12 */			nesnet_trigger_latch
/* 2 */				nop
/* 2 */				nop
/* 2 */				nop
/* 2 */				nop
/* 2 */				nop
/* 2 */				nop
/* 3 */				jmp @after_data ; Keeping things in sync cycles-wise.
				@after_data:
				nesnet_phx
				ldx #0
				@loop:
					nop
					inx
					cpx #OUTGOING_BIT_DELAY
					bne @loop
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

				nesnet_plx
				inx
				cpx #8
				bne @byte_loop


				
			rts

	do_handshake:
		inc NET_RESPONSE_CODE
		lda NET_RESPONSE_CODE
		cmp #1 ; 1-4 are just noops while we wait 
		beq @do_latch
		cmp #3 ; more noops.
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
				nesnet_trigger_latch
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

	do_send_data_len:
		; Okay... next, we need to tell it about some of our data
		lda HTTP_DATA_LENGTH
		jsr send_byte_to_nes

		lda HTTP_DATA_LENGTH+1
		jsr send_byte_to_nes
		inc NET_CURRENT_STATE
		ldy #0
		sty NET_REQUEST_BYTE_NUM
		sty NET_REQUEST_BYTE_NUM+1

		rts
	do_send_data:
		ldy NET_REQUEST_BYTE_NUM

		lda (HTTP_DATA), y
		jsr send_byte_to_nes

		iny
		sty NET_REQUEST_BYTE_NUM
		cpy #0
		bne @no_loopy
			inc HTTP_DATA+1
			inc NET_REQUEST_BYTE_NUM+1
		@no_loopy:
		dec HTTP_DATA_LENGTH
		lda HTTP_DATA_LENGTH
		cmp #255
		bne @not_dec
			dec HTTP_DATA_LENGTH+1
			lda HTTP_DATA_LENGTH+1
			cmp #255
			beq @everything_sent
		@not_dec:
		rts
		
		@everything_sent:
			inc NET_CURRENT_STATE
			lda #0
			sta NET_REQUEST_BYTE_NUM
			sta NET_REQUEST_BYTE_NUM+1 ; This is probably a noop, but it doesn't hurt. Might make for less bugs if we allow longer urls.
			; Also set response code to 0 to use as a temporary counter for the next step
			sta NET_RESPONSE_CODE
			sta NET_RESPONSE_CODE+1

			rts

	do_get_waiting:
		.repeat 3
			jsr get_pad_values_no_retry ; Wait until we start seeing real bytes flow in
			cmp #0
			bne @escape_zero
		.endrepeat
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

			lda NET_P1_BUFFER
			cmp NET_P1_BUFFER+1
			beq @done
			cmp NET_P1_BUFFER+2
			beq @done
			lda NET_P1_BUFFER+1
		@done:

		rts

	do_cycle: 
		lda NET_CURRENT_STATE
		cmp #NET_STATE_IDLE
		beq @idle

		cmp #NET_STATE_GET_INIT ; Starting a get request? Okay.. 
		beq @init
		cmp #NET_STATE_DELETE_INIT
		beq @init

		cmp #NET_STATE_GET_HANDSHAKE
		beq @handshake
		cmp #NET_STATE_DELETE_HANDSHAKE
		beq @handshake

		cmp #NET_STATE_GET_SENDING_METHOD
		beq @method
		cmp #NET_STATE_DELETE_SENDING_METHOD
		beq @method

		cmp #NET_STATE_GET_SENDING_URL
		beq @url
		cmp #NET_STATE_DELETE_SENDING_URL
		beq @url

		cmp #NET_STATE_GET_WAITING
		beq @wait
		cmp #NET_STATE_DELETE_WAITING
		beq @wait

		cmp #NET_STATE_GET_RES_CODE
		beq @res_code
		cmp #NET_STATE_DELETE_RES_CODE
		beq @res_code

		cmp #NET_STATE_GET_RES_LENGTH
		beq @res_len
		cmp #NET_STATE_DELETE_RES_LENGTH
		beq @res_len

		cmp #NET_STATE_GET_RECEIVING_BYTES
		beq @receive
		cmp #NET_STATE_DELETE_RECEIVING_BYTES
		beq @receive

		; Put the other half of the methods on the other side of these methods, so we don't have to do any
		; funky logic to jump to them. (Range of -128-127 for beq)
		jmp @after_methods
	@idle: 
		rts
	@init: 
		lda #0
		sta NET_RESPONSE_CODE
		inc NET_CURRENT_STATE
		rts
	@handshake: 
		jsr do_handshake
		rts
	@method: 
		lda NET_CURRENT_STATE
		cmp #NET_STATE_GET_SENDING_METHOD
		bne @not_get
			ldx #HTTP_GET
			jmp @doit
		@not_get:
		cmp #NET_STATE_DELETE_SENDING_METHOD
		bne @not_delete
			ldx #HTTP_DELETE
			jmp @doit
		@not_delete:
		cmp #NET_STATE_POST_SENDING_METHOD
		bne @not_post	
			ldx #HTTP_POST
			jmp @doit
		@not_post:
		cmp #NET_STATE_PUT_SENDING_METHOD
		bne @not_put
			ldx #HTTP_PUT
			;jmp #doit
		@not_put:
		@doit:
		txa
		jsr send_byte_to_nes
		inc NET_CURRENT_STATE
		rts
	@url:
		jsr do_send_url
		rts
	@data_len:
		jsr do_send_data_len
		rts
	@data:
		jsr do_send_data
		rts
	@wait:
		jsr do_get_waiting
		rts
	@res_code:
		jsr do_get_res_code
		rts
	@res_len:
		jsr do_get_res_len
		rts
	@receive:
		jsr do_receive
		rts
	@after_methods:

		cmp #NET_STATE_PUT_INIT ; Starting a get request? Okay.. 
		beq @init
		cmp #NET_STATE_POST_INIT
		beq @init

		cmp #NET_STATE_PUT_HANDSHAKE
		beq @handshake
		cmp #NET_STATE_POST_HANDSHAKE
		beq @handshake

		cmp #NET_STATE_PUT_SENDING_METHOD
		beq @method
		cmp #NET_STATE_POST_SENDING_METHOD
		beq @method

		cmp #NET_STATE_PUT_SENDING_URL
		beq @url
		cmp #NET_STATE_POST_SENDING_URL
		beq @url

		cmp #NET_STATE_PUT_SENDING_DATA_LEN
		beq @data_len
		cmp #NET_STATE_POST_SENDING_DATA_LEN
		beq @data_len

		cmp #NET_STATE_PUT_SENDING_DATA
		beq @data
		cmp #NET_STATE_POST_SENDING_DATA
		beq @data

		cmp #NET_STATE_PUT_WAITING
		beq @wait
		cmp #NET_STATE_POST_WAITING
		beq @wait

		cmp #NET_STATE_PUT_RES_CODE
		beq @res_code
		cmp #NET_STATE_POST_RES_CODE
		beq @res_code

		cmp #NET_STATE_PUT_RES_LENGTH
		beq @res_len
		cmp #NET_STATE_POST_RES_LENGTH
		beq @res_len

		cmp #NET_STATE_PUT_RECEIVING_BYTES
		beq @receive
		cmp #NET_STATE_POST_RECEIVING_BYTES
		beq @receive

		; If you didn't hit anything, assume the request must be complete.
		lda #0
		sta NET_REQUEST_IN_PROGRESS

		rts

	do_receive:
		lda NET_CURRENT_STATE
		pha
		jsr get_response
		pla 
		pha
		cmp NET_CURRENT_STATE
		bne @doner
		.if NESNET_BYTES_PER_CALL > 0
			.repeat (NESNET_BYTES_PER_CALL-1)
				.repeat 20
					nop
				.endrepeat
				jsr get_response
				pla
				pha
				cmp NET_CURRENT_STATE
				bne @doner
			.endrepeat
		.endif
		@doner:
		pla
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