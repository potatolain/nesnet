.feature c_comments

; Note: Since ca65 is kind of a pain, all exports are at the bottom. 
; If you really want to see what you can do though, the details should all be in nesnet.h :)

; TODO: macros cannot be placed in scopes - find a way to make these not risk clashing with someone else's stuff
; Maybe as simple as HttpLib_phy and HttpLib_ply, etc. Would be nice if there were a cleaner way though.
.macro phy
	sta MACRO_TEMP
	tya
	pha
	lda MACRO_TEMP
.endmacro
.macro ply
	sta MACRO_TEMP
	pla
	tay
	lda MACRO_TEMP
.endmacro
.macro phx
	sta MACRO_TEMP
	txa
	pha
	lda MACRO_TEMP
.endmacro
.macro plx
	sta MACRO_TEMP
	pla
	tax
	lda MACRO_TEMP
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
			lda #1
			jsr get_pad_values ; use the c function to get the pad state. However, we want the exact state, in PAD_STATE
			lda <(PAD_STATE+1)
			cmp #0
			beq @loop_zero


		; a is already PAD_STATE, but we want to ignore the first char, (used to adjust timing) so do nothing with it.
		lda #1
		jsr get_pad_values
		; Read status code - temporarily put it in LEN until we've read everything.
		lda <(PAD_STATE+1)
		sta LEN
		lda #1
		jsr get_pad_values
		lda <(PAD_STATE+1)
		sta LEN+1
		ldy #0

		@loop: 
			phy
			lda #1
			jsr get_pad_values
			ply
			lda <(PAD_STATE+1)
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

		lda LEN
		ldx LEN+1
		
		rts
.endscope

get_pad_values: 

	tay
	ldx #0

@padPollPort:

	lda #1
	sta CTRL_PORT1
	lda #0
	sta CTRL_PORT1
	lda #8
	sta <TEMP

@padPollLoop:

	lda CTRL_PORT1,y
	lsr a
	ror <PAD_BUF,x
	dec <TEMP
	bne @padPollLoop


	; TODO: Do we want to do triple posts of chars, then read them 3 times? Should see how consistent we can get it without, as this will slow things down a lot.
	; Alternatively, maybe we let the user choose?
	;inx
	;cpx #3
	;bne @padPollPort

	lda <PAD_BUF
	;cmp <PAD_BUF+1
	;beq @done
	;cmp <PAD_BUF+2
	;beq @done
	;lda <PAD_BUF+1

@done:

	sta <PAD_STATE,y

	rts


.export _http_get = HttpLib::get
