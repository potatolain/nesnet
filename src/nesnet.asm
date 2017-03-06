.feature c_comments
.import popa,popax,pusha, pushax
.import _nesnet_buffer

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

.scope HttpLib ; Use a separate scope to encapsulate some variables we use.

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
		.asciiz "/test"

	get: 
	
		sta RESPONSE
		stx RESPONSE+1

		jsr popax
		sta URL
		stx URL+1

		; TODO: Make a real handshake so we stop triggering the stupid powerpak
		.repeat 8
			trigger_latch
		.endrepeat
		
		; Dumb noop loop to waste some time before polling the pad yet again.
		; TODO: This is probably overzealous
		ldy #0
		ldx #0
		@loop_wait:
			nop
			iny
			cpy #0
			bne @loop_wait
			inx
			cpx #120
			bne @loop_wait

	; ========== START EXTREMELY TIME SENSITIVE CODE ==========

		; If you change anything in here, be ready to adjust the photon firmware to match.
		; Also realize any changes will have a serious
		; impact on the speed of up/down, so BE CAREFUL. You have been warned.

		; Numbers on the left are timings in clock cycles, where applicable. 1 clock cycle ~= 559ns on ntsc

		; Okay, time to send some data over.
		ldy #0
		@string_loop:
			ldx #0
			@byte_loop:

/* 5-6 */		lda (URL), y
/* 2   */		cmp #0
/* 2   */		beq @break_loop
/* 4-5 */		and byte_to_bit_lookup, x ; Get the bit we're looking for from a simple lookup table

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
/* 3 */				jmp @after_data ; Keeping things in sync
				@after_data:
				phx
				ldx #0
				@loop:
					nop
					inx
					cpx #150
					bne @loop
				plx
				inx
				cpx #8
				bne @byte_loop
			iny
			cpy #254
			bne @string_loop ; TODO: Allow for more than 255 chars
		@break_loop:

		; Finally, send a 0 byte
		.repeat 8, J ; TODO: This could be a loop?
			.scope .ident(.concat("derrup", .string(J))) ; Hack to shut up the local @things
				trigger_latch
					ldx #0
					@loop_0:
						nop
						inx
						cpx #150
						bne @loop_0
			.endscope
		.endrepeat

	; ========== END TIME SENSITIVE SECTION ==========


		ldx #0
		ldy #0
		@loop_zero:
			jsr get_pad_values_no_retry ; Wait until we start seeing real bytes flow in
			cmp #0
			bne @escape_zero
			inx
			cpx #0
			bne @loop_zero
			iny
			cpy #200
			bne @loop_zero
			jmp @get_complete_failure ; If we get to this point, it's never responding...

		@escape_zero:


		; Ignore the first char all 3 times it shows up.
		jsr get_pad_values_no_retry
		jsr get_pad_values_no_retry ; Done ignoring...
		
		jsr get_pad_values
		; Read status code - temporarily put it in LEN until we've read everything.
		sta URL
		jsr get_pad_values
		sta URL+1
		dec URL+1 ; HACK: In the driver, we specifically increment this byte so it is never zero. (Which causes weirdness on this driver.) 
		ldy #0

		@loop: 
			jsr get_pad_values
			cmp #0
			beq @after_data
			sta (RESPONSE), y
			iny
			cpy #0
			bne @loop
			inc RESPONSE+1
			jmp @loop

		@after_data:
		lda #0
		sta (RESPONSE), y ; null terminate the string...

		lda URL
		ldx URL+1
		
		rts

	@get_complete_failure:
		lda #0
		sta (RESPONSE), y
		lda #<(599)
		ldx #>(599)
		rts

test_connection:
	lda #<(test_url)
	ldx #>(test_url)
	jsr pushax
	lda #<(_nesnet_buffer)
	ldx #>(_nesnet_buffer)
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

.endscope
get_pad_values: 
	ldx #0

@padPollPort:

	; Forcibly space out the latch requests a little to reduce the likelihood the photon will get a latch out of order
	.repeat 12
		nop 
	.endrepeat 
	lda #1
	sta CTRL_PORT1
	lda #0
	sta CTRL_PORT1
	lda #8
	sta <NET_TEMP

@padPollLoop:

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
	.repeat 12
		nop 
	.endrepeat 


@padPollPort:

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
	dec <NET_TEMP
	bne @padPollLoop


	lda NET_BUFFER
@done:
	rts
.export _http_get = HttpLib::get
.export _nesnet_check_connected = HttpLib::test_connection