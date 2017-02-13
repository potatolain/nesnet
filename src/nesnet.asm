.export _do_net_stuff

.macro store thing, place
	lda #thing
	sta place
.endmacro

_do_net_stuff: 
	sta PTR
	stx PTR+1
	ldy #0

	lda #'C'
	sta (PTR), y
	iny

	lda #'A'
	sta (PTR), y
	iny

	lda #'T'
	sta (PTR), y
	iny
	sta (PTR), y
	iny

	lda #'S'
	sta (PTR), y
	iny

	lda #0
	sta (PTR), y
	rts