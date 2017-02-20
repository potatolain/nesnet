.feature c_comments

.export _do_net_stuff

.macro store thing, place
	lda #thing
	sta place
.endmacro

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

.macro trigger_latch
	lda #1
	sta $4016
	lda #0
	sta $4016
.endmacro

_do_net_stuff: 
	sta PTR2
	stx PTR2+1

	; Seed dummy bytes with data.. eventually we'll want this to be a string.
	lda #'C'
	sta DUMMY_BYTES
	lda #'A'
	sta DUMMY_BYTES+1
	lda #'T'
	sta DUMMY_BYTES+2


	; Kinda junky "are you there" handshake - ideal is more complex
	.repeat 8
		trigger_latch
	.endrepeat
	
	; Dumb noop loop to waste some time before polling the pad yet again.
	; TODO: This is probably overzealous
	ldy #0
	@loop_wait:
		.repeat 120
			nop
		.endrepeat
		iny
		cpy #0
		bne @loop_wait

; ========== START EXTREMELY TIME SENSITIVE CODE ==========

	; If you change anything in here, be very sure all code paths take roughly the same amount of time, and 
	; the photon firmware is adjusted if it has to take any longer. Also realize any changes will have a serious
	; impact on the speed of up/down, so BE CAREFUL. You have been warned.

	; Okay, time to send some data over.
	trigger_latch ; consider this "priming" the string...
	.define TESTSTRING "What kind of poke?"
	.repeat .strlen(TESTSTRING), J
	.repeat 8, I ; FIXME: lazy assed crap one byte implementation
		trigger_latch

		.scope .ident(.concat("derrup", .string(I), .string(J))) ; Hack to shut up the local @things
			lda #(.strat(TESTSTRING, J) >> I)
			and #1
			cmp #0
			beq @zero
				trigger_latch
				jmp @after_data
			@zero: 
				nop
				nop
				nop
				nop
				nop
				nop
				jmp @after_data ; Keeping things in sync
			@after_data:
			/*.repeat 40
				nop
			.endrepeat*/
			ldx #0
			@loop:
				nop
				inx
				cpx #150
				bne @loop
		.endscope

	.endrepeat
	.endrepeat

	; Finally, send a 0 byte
	.repeat 8, J
		.scope .ident(.concat("derrup", .string(J))) ; Hack to shut up the local @things
			trigger_latch
				ldx #0
				@loop:
					nop
					inx
					cpx #150
					bne @loop
		.endscope
	.endrepeat

; ========== END TIME SENSITIVE SECTION ==========



	@loop_zero:
		lda #1
		jsr _pad_poll ; use the c function to get the pad state. However, we want the exact state, in PAD_STATE
		lda <(PAD_STATE+1)
		cmp #0
		beq @loop_zero


	; a is already PAD_STATE, so grab it.
	ldy #0
	sta (PTR2), y
	iny

	@loop: 
		phy
		lda #1
		jsr _pad_poll
		ply
		lda <(PAD_STATE+1)
		cmp #0
		beq @after_data
		sta (PTR2), y
		iny
		jmp @loop

	@after_data:
	lda #0
	sta (PTR2), y ; null terminate the string...

	
	rts