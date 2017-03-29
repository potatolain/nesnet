; NESNet needs a method to call to wait for vblanks for precise timing. 
; This should be a vblank_wait type function. Default value is for neslib.asm by Shiru.
NESNET_WAIT_NMI =		_ppu_wait_nmi