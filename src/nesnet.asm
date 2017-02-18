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

_do_net_stuff: 
	sta PTR2
	stx PTR2+1

	; Seed dummy bytes with data.. eventually we'll want this to be a string.
	lda #75
	sta DUMMY_BYTE
	lda #62
	sta DUMMY_BYTE+1
	lda #5
	sta DUMMY_BYTE+2

	; Kinda junky "are you there" handshake - ideal is more complex
	.repeat 8
		lda #1
		sta $4016
		lda #0
		sta $4016
	.endrepeat
	
	; Dumb noop loop to waste some time before polling the pad yet again.
	; TODO: This is probably overzealous
	ldy #0
	@loop_wait:
		.repeat 32
			nop
		.endrepeat
		iny
		cpy #0
		bne @loop_wait

	; Okay, time to send some data over.
	

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