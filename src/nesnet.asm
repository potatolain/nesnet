.feature c_comments

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



		@loop_zero:
			jsr get_pad_values ; use the c function to get the pad state. However, we want the exact state, in PAD_STATE
			cmp #0
			beq @loop_zero


		; a is already PAD_STATE, but we want to ignore the first char, (used to adjust timing) so do nothing with it.
		jsr get_pad_values
		; Read status code - temporarily put it in LEN until we've read everything.
		sta URL
		jsr get_pad_values
		sta URL+1
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
.endscope

get_pad_values: 

@padPollPort:

	lda #1
	sta CTRL_PORT1
	lda #0
	sta CTRL_PORT1
	lda #8
	sta <NET_TEMP+1

@padPollLoop:

	lda CTRL_PORT2
	lsr a
	ror <NET_TEMP
	dec <NET_TEMP+1
	bne @padPollLoop


	; TODO: Do we want to do triple posts of chars, then read them 3 times? Should see how consistent we can get it without, as this will slow things down a lot.
	; Alternatively, maybe we let the user choose?
	; TODO2: Need more vars for buffer... trying to avoid using nesnet stuff directly. May also want separate defines for stuff like CTRL_PORTx
	;inx
	;cpx #3
	;bne @padPollPort

	lda <NET_TEMP
	;cmp <NET_TEMP+1
	;beq @done
	;cmp <NET_TEMP+2
	;beq @done
	;lda <NET_TEMP+1

@done:

	rts


.export _http_get = HttpLib::get
