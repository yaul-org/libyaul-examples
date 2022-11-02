for char in range(256):
  print("        { VDP2_SCRN_PND_CONFIG_3(0, VDP2_VRAM_ADDR(0, 0x%05X), VDP2_CRAM_MODE_0_OFFSET(0, 0, 0)) }," % ((char << 5) << 2))
